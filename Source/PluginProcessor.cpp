#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {
float coeff(float seconds,double sr){return std::exp(-1.0f/juce::jmax(1.0f,seconds*(float)sr));}
float pole(float hz,double sr){return std::exp(-2.0f*juce::MathConstants<float>::pi*hz/(float)sr);}
float db(float d){return juce::Decibels::decibelsToGain(d);}
float gainCurve(float envelope,float strength,bool expansion){
 // Complementary 2:1 compressor and 1:2 expander at 100% strength,
 // referenced to -18 dBFS. Protect low-level signals from extreme gain.
 const float level=juce::Decibels::gainToDecibels(juce::jmax(1e-5f,envelope),-100.0f);
 // Bound gain changes to realistic working ranges. In particular,
 // very quiet bands must never receive tens of dB of instantaneous boost.
 const float difference=juce::jlimit(-24.0f,18.0f,level+18.0f);
 // A 2:1 encoder needs a 1:2 decoder: expander slope is +1,
 // not +0.5. This was a major source of tracking error.
 const float target=(expansion?1.0f:-0.5f)*strength*difference;
 return db(juce::jlimit(-12.0f,12.0f,target));
}
}
juce::AudioProcessorValueTreeState::ParameterLayout CompanderProcessor::layout(){
 using F=juce::AudioParameterFloat;using B=juce::AudioParameterBool;using C=juce::AudioParameterChoice;
 juce::AudioProcessorValueTreeState::ParameterLayout p;
 p.add(std::make_unique<F>(juce::ParameterID{"input",1},"Input (dB)",juce::NormalisableRange<float>(-24,24,.1f),0));
 p.add(std::make_unique<F>(juce::ParameterID{"output",1},"Output (dB)",juce::NormalisableRange<float>(-24,24,.1f),0));
 p.add(std::make_unique<F>(juce::ParameterID{"amount",1},"Companding Amount (%)",juce::NormalisableRange<float>(0,200,.1f),50));
 p.add(std::make_unique<F>(juce::ParameterID{"emphasis",1},"HF Emphasis (%)",juce::NormalisableRange<float>(0,200,.1f),50));
 p.add(std::make_unique<F>(juce::ParameterID{"sheen",1},"Sheen (%)",juce::NormalisableRange<float>(0,100,.1f),0));
 p.add(std::make_unique<F>(juce::ParameterID{"split",1},"Frequency Split (Hz)",juce::NormalisableRange<float>(250,5000,1,.4f),1300));
 p.add(std::make_unique<C>(juce::ParameterID{"mode",1},"Compander Model",juce::StringArray{"DMM Frequency-Sensitive","dbx-Style Broadband","Low-Level Lift / Restore","DMM + Low-Level Lift"},0));
 p.add(std::make_unique<B>(juce::ParameterID{"compress",1},"Compressor On",true));
 p.add(std::make_unique<B>(juce::ParameterID{"expand",1},"Expander On",true));
 p.add(std::make_unique<B>(juce::ParameterID{"bandwidth",1},"BBD Bandwidth",false));
 p.add(std::make_unique<B>(juce::ParameterID{"liftHighOnly",1},"Lift High Frequencies Only (5 kHz)",false));
 p.add(std::make_unique<F>(juce::ParameterID{"expansionReduction",1},"Reduce Expansion (%)",juce::NormalisableRange<float>(0,100,.1f),0));
 return p;
}
CompanderProcessor::CompanderProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),parameters(*this,nullptr,"STATE",layout()){}
void CompanderProcessor::prepareToPlay(double sr,int){sampleRate=sr;channels={};setLatencySamples(0);}
void CompanderProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&){
 juce::ScopedNoDenormals noDenormals;
 const int count=juce::jmin(2,buffer.getNumChannels());
 const float input=db(*parameters.getRawParameterValue("input"));
 const float output=db(*parameters.getRawParameterValue("output"));
 const float amount=*parameters.getRawParameterValue("amount")*.01f;
 const float emphasis=*parameters.getRawParameterValue("emphasis")*.01f;
 const float sheen=juce::jlimit(0.f,1.f,*parameters.getRawParameterValue("sheen")*.01f);
 const float split=*parameters.getRawParameterValue("split");
 const int mode=juce::jlimit(0,3,(int)*parameters.getRawParameterValue("mode"));
 const bool dmm=mode==0||mode==3;
 const bool lowLevelMode=mode==2;
 const bool combinedMode=mode==3;
 const bool compressor=*parameters.getRawParameterValue("compress")>.5f;
 const bool expander=*parameters.getRawParameterValue("expand")>.5f;
 const bool bandwidth=*parameters.getRawParameterValue("bandwidth")>.5f;
 const bool liftHighOnly=(lowLevelMode||combinedMode)&&*parameters.getRawParameterValue("liftHighOnly")>.5f;
 const float expansionScale=1.f-juce::jlimit(0.f,1.f,*parameters.getRawParameterValue("expansionReduction")*.01f);
 const float liftSplitPole=pole(5000.f,sampleRate);
 const float a1=pole(split*.48f,sampleRate),a2=pole(split*2.4f,sampleRate);
 // Analog-delay-style treble pre-emphasis before the compressor, with
 // mathematically complementary de-emphasis after the expander.
 // Works in both DMM and broadband/dbx-style modes.
 const float emphasisPole=pole(split,sampleRate);
 const float emphasisBoost=juce::jlimit(0.f,2.f,emphasis)*1.0f;
 // Sheen reduces playback de-emphasis only, leaving record-side HF
 // pre-emphasis and its compression unchanged.
 const float deEmphasisBoost=emphasisBoost*(1.f-sheen);
 const float cutoff=pole(6800,sampleRate);
 // Detector must not follow individual waveform cycles; very short
 // time constants produce audible gain modulation on bass and midrange.
 const float attack=coeff(dmm?.012f:.010f,sampleRate);
 const float release=coeff(dmm?.180f:.160f,sampleRate);
 // HF emphasis adjusts the amount of frequency-selective companding,
 // but should not multiply it into several times the nominal ratio.
 const float weights[3]={dmm?.40f:1.0f,dmm?.65f:0.0f,dmm?.70f:0.0f};
 // The third mode uses an upward compressor on quiet signals during
 // encoding and its complementary downward expander during decoding.
 // 100% = 2:1 below -24 dBFS, followed by 1:2 below that threshold.
 const float lowThreshold=-24.f;
 const float lowStrength=juce::jlimit(0.f,1.f,amount);
 for(int c=0;c<count;++c){
  auto& state=channels[(size_t)c];
  auto* samples=buffer.getWritePointer(c);
  for(int i=0;i<buffer.getNumSamples();++i){
   float x=samples[i]*input;
   // Complementary crossover: dry low + processed high reconstructs
   // the input exactly when processing is at unity gain.
   float dryLow=0.f;
   if(liftHighOnly&&!combinedMode){
    state.liftLow=liftSplitPole*state.liftLow+(1.f-liftSplitPole)*x;
    dryLow=state.liftLow;
    x-=dryLow;
   }
   // Separate encode/decode crossover memories: avoid sharing filter history
   // across the compressor and expander sides.
   auto stage=[&](float value,bool encode){
    if((encode&&!compressor)||(!encode&&!expander))return value;
    auto& s=state.bands;
    const int base=encode?0:2;
    float low1=s[(size_t)base].split=a1*s[(size_t)base].split+(1-a1)*value;
    float low2=s[(size_t)(base+1)].split=a2*s[(size_t)(base+1)].split+(1-a2)*value;
    if(lowLevelMode){
     auto& env=encode?s[0].envEncode:s[0].envDecode;
     const float v=std::abs(value);
     const float k=v>env?attack:release;
     env=k*env+(1.f-k)*v;
     const float level=juce::Decibels::gainToDecibels(juce::jmax(1e-6f,env),-120.f);
     const float below=juce::jmin(0.f,level-lowThreshold);
     // Encoder slope: 1 - 0.5*strength. Decoder inverse slope:
     // 1/(1 - 0.5*strength). The decoder therefore attenuates the
     // quiet signals that the encoder raised.
     const float slope=1.f-.5f*lowStrength;
     const float delta=encode?(-.5f*lowStrength*below):
       (expansionScale*(1.f/slope-1.f)*below);
     return value*db(juce::jlimit(-18.f,18.f,delta));
    }
    if(!dmm){
     auto& env=encode?s[0].envEncode:s[0].envDecode;
     float v=std::abs(value),k=v>env?attack:release;
     env=k*env+(1-k)*v;
     return value*gainCurve(env,juce::jmin(1.0f,amount),!encode);
    }
    const float parts[3]={low1,low2-low1,value-low2};
    float result=0;
    for(int band=0;band<3;++band){
     auto& env=encode?s[(size_t)band].envEncode:s[(size_t)band].envDecode;
     float level=std::abs(parts[band]);
     float k=level>env?attack:release;
     env=k*env+(1-k)*level;
     float strength=juce::jmin(1.0f,amount*weights[band]);
     result+=parts[band]*gainCurve(env,strength,!encode);
    }
    return result;
   };
   // Pre-emphasis is applied on the record/encode side. Use a
   // single-pole shelving boost, rather than permanent HF gain.
   if(compressor && emphasisBoost>0.f){
    state.preEmphasisLow=emphasisPole*state.preEmphasisLow+(1.f-emphasisPole)*x;
    x+=emphasisBoost*(x-state.preEmphasisLow);
   }
   // Hybrid: DMM three-band companding across the whole signal, plus
   // an independent low-level lift/restore pair. Its optional 5 kHz+
   // crossover affects only the lift branch, not the DMM processing.
   auto liftStage=[&](float value,bool encode){
    if((encode&&!compressor)||(!encode&&!expander))return value;
    auto& env=encode?state.liftEnvEncode:state.liftEnvDecode;
    const float v=std::abs(value);
    const float k=v>env?attack:release;
    env=k*env+(1.f-k)*v;
    const float level=juce::Decibels::gainToDecibels(juce::jmax(1e-6f,env),-120.f);
    const float below=juce::jmin(0.f,level-lowThreshold);
    const float slope=1.f-.5f*lowStrength;
    const float delta=encode?(-.5f*lowStrength*below):
      (expansionScale*(1.f/slope-1.f)*below);
    return value*db(juce::jlimit(-18.f,18.f,delta));
   };
   if(combinedMode){
    // Encode in series, then decode in reverse order.
    x=stage(x,true);
    float hybridLow=0.f;
    if(liftHighOnly){
     state.liftLow=liftSplitPole*state.liftLow+(1.f-liftSplitPole)*x;
     hybridLow=state.liftLow;
     x-=hybridLow;
    }
    x=liftStage(x,true);
    x=liftStage(x,false);
    if(liftHighOnly)x+=hybridLow;
    x=stage(x,false);
   }else{
    x=stage(x,true);
    x=stage(x,false);
   }
   // Exact inverse of y = x + k*(x - LP(x)), including filter state.
   // This returns the HF boost to unity when both sides are enabled.
   if(expander && emphasisBoost>0.f){
    const float previousLow=state.deEmphasisLow;
    const float original=(x+deEmphasisBoost*emphasisPole*previousLow)/
                         (1.f+deEmphasisBoost*emphasisPole);
    state.deEmphasisLow=emphasisPole*previousLow+(1.f-emphasisPole)*original;
    x=original;
   }
   if(liftHighOnly)x+=dryLow;
   if(bandwidth){state.highCut=cutoff*state.highCut+(1-cutoff)*x;x=state.highCut;}
   samples[i]=x*output;
  }
 }
 for(int c=count;c<buffer.getNumChannels();++c)buffer.clear(c,0,buffer.getNumSamples());
}
juce::AudioProcessorEditor* CompanderProcessor::createEditor(){return new CompanderEditor(*this);}
void CompanderProcessor::getStateInformation(juce::MemoryBlock& b){auto xml=parameters.copyState().createXml();copyXmlToBinary(*xml,b);}
void CompanderProcessor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size))if(xml->hasTagName(parameters.state.getType()))parameters.replaceState(juce::ValueTree::fromXml(*xml));}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new CompanderProcessor();}
