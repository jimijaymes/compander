#pragma once
#include "PluginProcessor.h"
class CompanderEditor final:public juce::AudioProcessorEditor{
public:
 explicit CompanderEditor(CompanderProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 struct Knob{juce::Slider slider;juce::Label label;std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;};
 CompanderProcessor& processor;
 std::array<Knob,8> knobs;
 Knob simpleHighKnob;
 Knob attackKnob,releaseKnob,simpleSpeedKnob;
 void updateModeVisibility();
 juce::ComboBox mode;
 juce::Label modeLabel;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
 juce::ToggleButton compressor,expander,bandwidth,liftHighOnly,matchDeEmphasis;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> compressorAttachment,expanderAttachment,bandwidthAttachment,liftHighOnlyAttachment,matchDeEmphasisAttachment;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompanderEditor)
};
