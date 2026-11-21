#include "PluginEditor.h"
namespace {const juce::Colour cream(0xfff3e8cf),gold(0xffd2ad72),cyan(0xff70c5cc);}
CompanderEditor::CompanderEditor(CompanderProcessor& p):AudioProcessorEditor(p),processor(p){
 const char* ids[]={"input","output","amount","emphasis","split","sheen","expansionReduction","mix"};
 const char* names[]={"INPUT","OUTPUT","COMPANDING AMOUNT","HF EMPHASIS","FREQUENCY SPLIT","SHEEN","REDUCE EXPANSION","MIX"};
 for(size_t i=0;i<knobs.size();++i){
  auto& k=knobs[i];k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,95,21);
  k.slider.setColour(juce::Slider::rotarySliderFillColourId,i==2?cyan:gold);
  k.slider.setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xff404951));
  k.slider.setColour(juce::Slider::thumbColourId,cream);
  k.slider.setColour(juce::Slider::textBoxTextColourId,cream);
  k.slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
  k.slider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
  addAndMakeVisible(k.slider);
  k.label.setText(names[i],juce::dontSendNotification);
  k.label.setJustificationType(juce::Justification::centred);
  k.label.setColour(juce::Label::textColourId,cream);
  addAndMakeVisible(k.label);
  k.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,ids[i],k.slider);
 }
 mode.addItem("DMM / FREQUENCY-SENSITIVE",1);
 mode.addItem("DBX / BROADBAND",2);
 mode.addItem("LOW-LEVEL LIFT / RESTORE",3);
 mode.addItem("DMM + LOW-LEVEL LIFT",4);
 mode.addItem("SIMPLE / DMM + LIFT",5);
 mode.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff303941));
 mode.setColour(juce::ComboBox::textColourId,cream);
 mode.setColour(juce::ComboBox::outlineColourId,gold);
 addAndMakeVisible(mode);
 modeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.parameters,"mode",mode);
 modeLabel.setText("COMPANDER MODEL",juce::dontSendNotification);
 modeLabel.setColour(juce::Label::textColourId,cream);
 modeLabel.setJustificationType(juce::Justification::centred);
 addAndMakeVisible(modeLabel);
 compressor.setButtonText("COMPRESSOR ON");
 expander.setButtonText("EXPANDER ON");
 bandwidth.setButtonText("BBD BANDWIDTH");
 liftHighOnly.setButtonText("LIFT: 5 kHz+ ONLY");
 matchDeEmphasis.setButtonText("MATCH DE-EMPHASIS");
 for(auto* b:{&compressor,&expander,&bandwidth,&liftHighOnly,&matchDeEmphasis}){
  b->setColour(juce::ToggleButton::textColourId,cream);
  b->setColour(juce::ToggleButton::tickColourId,cyan);
  addAndMakeVisible(*b);
 }
 compressorAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"compress",compressor);
 expanderAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"expand",expander);
 bandwidthAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"bandwidth",bandwidth);
 liftHighOnlyAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"liftHighOnly",liftHighOnly);
 matchDeEmphasisAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"matchDeEmphasis",matchDeEmphasis);
 simpleHighKnob.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
 simpleHighKnob.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,95,21);
 simpleHighKnob.slider.setColour(juce::Slider::rotarySliderFillColourId,cyan);
 simpleHighKnob.slider.setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xff404951));
 simpleHighKnob.slider.setColour(juce::Slider::thumbColourId,cream);
 simpleHighKnob.slider.setColour(juce::Slider::textBoxTextColourId,cream);
 simpleHighKnob.slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
 simpleHighKnob.slider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
 simpleHighKnob.label.setText("HIGH END",juce::dontSendNotification);
 simpleHighKnob.label.setJustificationType(juce::Justification::centred);
 simpleHighKnob.label.setColour(juce::Label::textColourId,cream);
 addAndMakeVisible(simpleHighKnob.slider);
 addAndMakeVisible(simpleHighKnob.label);
 simpleHighKnob.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,"simpleHigh",simpleHighKnob.slider);
 auto setupTiming=[&](Knob& k,const char* id,const char* name){
  k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,85,21);
  k.slider.setColour(juce::Slider::rotarySliderFillColourId,cyan);
  k.slider.setColour(juce::Slider::rotarySliderOutlineColourId,juce::Colour(0xff404951));
  k.slider.setColour(juce::Slider::thumbColourId,cream);
  k.slider.setColour(juce::Slider::textBoxTextColourId,cream);
  k.slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
  k.slider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
  k.label.setText(name,juce::dontSendNotification);
  k.label.setJustificationType(juce::Justification::centred);
  k.label.setColour(juce::Label::textColourId,cream);
  addAndMakeVisible(k.slider);addAndMakeVisible(k.label);
  k.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters,id,k.slider);
 };
 setupTiming(attackKnob,"attackTime","ATTACK");
 setupTiming(releaseKnob,"releaseTime","RELEASE");
 setupTiming(simpleSpeedKnob,"simpleSpeed","SPEED  FAST - SLOW");
 mode.onChange=[this]{updateModeVisibility();resized();};
 expander.onClick=[this]{updateModeVisibility();};
 updateModeVisibility();
 setSize(1020,575);
}
void CompanderEditor::paint(juce::Graphics& g){
 g.fillAll(juce::Colour(0xff1b2328));
 g.setColour(juce::Colour(0xff2a353c));
 g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(12),12);
 g.setColour(gold);
 g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(12),12,2);
 g.setFont(juce::Font(juce::FontOptions(31,juce::Font::bold)));
 g.setColour(cream);
 g.drawText("COMPANDER  0.1",32,23,600,50,juce::Justification::centredLeft);
 g.setFont(juce::Font(juce::FontOptions(12)));
 g.drawText("ANALOG DELAY / NOISE REDUCTION CHARACTER",34,73,650,20,juce::Justification::centredLeft);
 g.setColour(gold);g.drawLine(32,104,988,104,2);
 g.setColour(cream);g.setFont(juce::Font(juce::FontOptions(11)));
 g.drawText(mode.getSelectedId()==5?"SIMPLE: ONE SPEED CONTROL, CENTRE = ORIGINAL TIMING":"TIMING: CENTRE = ORIGINAL MODE SETTINGS  |  LEFT = FASTER  |  RIGHT = SLOWER",32,529,950,23,juce::Justification::centredLeft);
}
void CompanderEditor::updateModeVisibility(){
 const int selected=mode.getSelectedId();
 const bool simple=selected==5;
 const bool lift=selected==3||selected==4;
 const bool hybrid=selected==4;
 for(size_t i=0;i<knobs.size();++i){
  // Frequency split is unused in Low-Level Lift and fixed in Simple.
  const bool visible=simple?(i==2||i==6||i==7):
    (i!=4||selected!=3);
  knobs[i].slider.setVisible(visible);
  knobs[i].label.setVisible(visible);
 }
 simpleHighKnob.slider.setVisible(simple);
 simpleHighKnob.label.setVisible(simple);
 for(auto* b:{&compressor,&expander,&bandwidth})b->setVisible(!simple);
 liftHighOnly.setVisible(lift);
 // With the expander enabled, de-emphasis is already applied; this
 // switch only changes behaviour when the expander is disabled.
 matchDeEmphasis.setVisible(!simple&&!expander.getToggleState());
 attackKnob.slider.setVisible(!simple);attackKnob.label.setVisible(!simple);
 releaseKnob.slider.setVisible(!simple);releaseKnob.label.setVisible(!simple);
 simpleSpeedKnob.slider.setVisible(simple);simpleSpeedKnob.label.setVisible(simple);
 juce::ignoreUnused(hybrid);
 repaint();
}
void CompanderEditor::resized(){
 for(size_t i=0;i<knobs.size();++i){
  int x=mode.getSelectedId()==5?(i==2?180:(i==6?400:620)):24+(int)i*120;
  knobs[i].slider.setBounds(x,123,125,122);
  knobs[i].label.setBounds(x-5,246,135,23);
 }
 simpleHighKnob.slider.setBounds(825,123,125,122);
 simpleHighKnob.label.setBounds(820,246,135,23);
 attackKnob.slider.setBounds(360,365,105,96);
 attackKnob.label.setBounds(348,461,130,22);
 releaseKnob.slider.setBounds(505,365,105,96);
 releaseKnob.label.setBounds(493,461,130,22);
 simpleSpeedKnob.slider.setBounds(385,365,115,96);
 simpleSpeedKnob.label.setBounds(370,461,145,22);
 modeLabel.setBounds(35,294,235,22);
 mode.setBounds(41,319,246,30);
 compressor.setBounds(322,308,170,42);
 expander.setBounds(502,308,160,42);
 bandwidth.setBounds(678,308,175,42);
 liftHighOnly.setBounds(35,383,245,42);
 matchDeEmphasis.setBounds(685,383,270,42);
}
