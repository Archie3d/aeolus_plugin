// ----------------------------------------------------------------------------
//
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomMidiKeyboard.h"

//==============================================================================
/**
 * @brief Plugin UI.
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

    juce::Label _versionLabel;
    juce::Label _cpuLoadLabel;
    juce::Label _cpuLoadValueLabel;
    juce::Label _voiceCountLabel;
    juce::Label _voiceCountValueLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AeolusAudioProcessorEditor)
};
