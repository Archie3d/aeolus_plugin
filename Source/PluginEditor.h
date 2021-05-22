/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomMidiKeyboard.h"

//==============================================================================
/**
*/
class AeolusAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                    public juce::Timer
{
public:
    AeolusAudioProcessorEditor (AeolusAudioProcessor&);
    ~AeolusAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // juce::Timer
    void timerCallback() override;

private:

    void populateStops();
    void refresh();

    void updateMeters();
    void updateMidiKeyboardRange();

    AeolusAudioProcessor& _audioProcessor;
    juce::OwnedArray<juce::ToggleButton> _stopButtons;
    CustomMidiKeyboard _midiKeyboard;

    juce::Label _cpuLoadLabel;
    juce::Label _cpuLoadValueLabel;
    juce::Label _voiceCountLabel;
    juce::Label _voiceCountValueLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AeolusAudioProcessorEditor)
};
