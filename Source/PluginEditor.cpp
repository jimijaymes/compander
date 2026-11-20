#include "PluginEditor.h"
namespace {const juce::Colour cream(0xfff3e8cf),gold(0xffd2ad72),cyan(0xff70c5cc);}
CompanderEditor::CompanderEditor(CompanderProcessor& p):AudioProcessorEditor(p),processor(p){
 const char* ids[]={"input","output","amount","emphasis","split","sheen"};
 const char* names[]={"INPUT","OUTPUT","COMPANDING AMOUNT","HF EMPHASIS","FREQUENCY SPLIT","SHEEN"};
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
 for(auto* b:{&compressor,&expander,&bandwidth}){
  b->setColour(juce::ToggleButton::textColourId,cream);
  b->setColour(juce::ToggleButton::tickColourId,cyan);
  addAndMakeVisible(*b);
 }
 compressorAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"compress",compressor);
 expanderAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"expand",expander);
 bandwidthAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.parameters,"bandwidth",bandwidth);
 setSize(900,430);
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
 g.setColour(gold);g.drawLine(32,104,868,104,2);
 g.setColour(cream);g.setFont(juce::Font(juce::FontOptions(11)));
 g.drawText("DMM: FREQUENCY-SENSITIVE  |  DBX: BROADBAND  |  LIFT: QUIET SIGNALS UP / RESTORE DOWN",32,388,830,23,juce::Justification::centredLeft);
}
void CompanderEditor::resized(){
 for(size_t i=0;i<knobs.size();++i){
  int x=28+(int)i*140;
  knobs[i].slider.setBounds(x,123,125,122);
  knobs[i].label.setBounds(x-5,246,135,23);
 }
 modeLabel.setBounds(35,294,235,22);
 mode.setBounds(41,319,246,30);
 compressor.setBounds(322,308,170,42);
 expander.setBounds(502,308,160,42);
 bandwidth.setBounds(678,308,175,42);
}
