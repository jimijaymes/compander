#pragma once
#include <JuceHeader.h>
#include <array>
class CompanderProcessor final:public juce::AudioProcessor{
public:
 CompanderProcessor();
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override{return true;}
 const juce::String getName() const override{return "Compander 0.1";}
 bool acceptsMidi() const override{return false;}
 bool producesMidi() const override{return false;}
 bool isMidiEffect() const override{return false;}
 double getTailLengthSeconds() const override{return 0;}
 int getNumPrograms() override{return 1;}
 int getCurrentProgram() override{return 0;}
 void setCurrentProgram(int) override{}
 const juce::String getProgramName(int) override{return {};}
 void changeProgramName(int,const juce::String&) override{}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 juce::AudioProcessorValueTreeState parameters;
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 struct Band{float split=0,envEncode=0,envDecode=0;};
 struct Channel{std::array<Band,4> bands;float highCut=0,preEmphasisLow=0,deEmphasisLow=0,liftLow=0,liftEnvEncode=0,liftEnvDecode=0,simpleHighLow=0;};
 std::array<Channel,2> channels{};
 double sampleRate=44100.0;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompanderProcessor)
};
