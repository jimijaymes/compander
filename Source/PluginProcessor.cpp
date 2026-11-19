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
 const float difference=juce::jlimit(-36.0f,24.0f,level+18.0f);
 return db((expansion?0.5f:-0.5f)*strength*difference);
}
}
juce::AudioProcessorValueTreeState::ParameterLayout CompanderProcessor::layout(){
 using F=juce::AudioParameterFloat;using B=juce::AudioParameterBool;using C=juce::AudioParameterChoice;
 juce::AudioProcessorValueTreeState::ParameterLayout p;
 p.add(std::make_unique<F>(juce::ParameterID{"input",1},"Input (dB)",juce::NormalisableRange<float>(-24,24,.1f),0));
 p.add(std::make_unique<F>(juce::ParameterID{"output",1},"Output (dB)",juce::NormalisableRange<float>(-24,24,.1f),0));
 p.add(std::make_unique<F>(juce::ParameterID{"amount",1},"Companding Amount (%)",juce::NormalisableRange<float>(0,200,.1f),100));
 p.add(std::make_unique<F>(juce::ParameterID{"emphasis",1},"HF Emphasis (%)",juce::NormalisableRange<float>(0,200,.1f),130));
 p.add(std::make_unique<F>(juce::ParameterID{"split",1},"Frequency Split (Hz)",juce::NormalisableRange<float>(250,5000,1,.4f),1300));
 p.add(std::make_unique<C>(juce::ParameterID{"mode",1},"Compander Model",juce::StringArray{"DMM Frequency-Sensitive","dbx-Style Broadband"},0));
 p.add(std::make_unique<B>(juce::ParameterID{"compress",1},"Compressor On",true));
 p.add(std::make_unique<B>(juce::ParameterID{"expand",1},"Expander On",true));
 p.add(std::make_unique<B>(juce::ParameterID{"bandwidth",1},"BBD Bandwidth",false));
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
 const float split=*parameters.getRawParameterValue("split");
 const bool dmm=*parameters.getRawParameterValue("mode")<.5f;
 const bool compressor=*parameters.getRawParameterValue("compress")>.5f;
 const bool expander=*parameters.getRawParameterValue("expand")>.5f;
 const bool bandwidth=*parameters.getRawParameterValue("bandwidth")>.5f;
 const float a1=pole(split*.48f,sampleRate),a2=pole(split*2.4f,sampleRate);
 const float cutoff=pole(6800,sampleRate);
 const float attack=coeff(dmm?.004f:.002f,sampleRate);
 const float release=coeff(dmm?.11f:.08f,sampleRate);
 const float weights[3]={dmm?.48f:1.0f,dmm?1.0f:0.0f,dmm?(1.0f+emphasis*1.65f):0.0f};
 for(int c=0;c<count;++c){
  auto& state=channels[(size_t)c];
  auto* samples=buffer.getWritePointer(c);
  for(int i=0;i<buffer.getNumSamples();++i){
   float x=samples[i]*input;
   // Separate encode/decode crossover memories: avoid sharing filter history
   // across the compressor and expander sides.
   auto stage=[&](float value,bool encode){
    if((encode&&!compressor)||(!encode&&!expander))return value;
    auto& s=state.bands;
    const int base=encode?0:2;
    float low1=s[(size_t)base].split=a1*s[(size_t)base].split+(1-a1)*value;
    float low2=s[(size_t)(base+1)].split=a2*s[(size_t)(base+1)].split+(1-a2)*value;
    if(!dmm){
     auto& env=encode?s[0].envEncode:s[0].envDecode;
     float v=std::abs(value),k=v>env?attack:release;
     env=k*env+(1-k)*v;
     return value*gainCurve(env,amount,!encode);
    }
    const float parts[3]={low1,low2-low1,value-low2};
    float result=0;
    for(int band=0;band<3;++band){
     auto& env=encode?s[(size_t)band].envEncode:s[(size_t)band].envDecode;
     float level=std::abs(parts[band]);
     float k=level>env?attack:release;
     env=k*env+(1-k)*level;
     float strength=amount*weights[band];
     result+=parts[band]*gainCurve(env,strength,!encode);
    }
    return result;
   };
   x=stage(x,true);
   x=stage(x,false);
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
