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
#include "ui/CustomMidiKeyboard.h"
#include "ui/LevelIndicator.h"
#include "ui/ParameterSlider.h"
#include "ui/DivisionView.h"
#include "ui/PresetView.h"
#include "ui/SequencerView.h"
#include "ui/MidiChannelsComponent.h"
#include "ui/OverlayComponent.h"

//==============================================================================
/**
 * @brief Plugin UI.
 */
class AeolusAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                    public juce::Timer,
                                    public ui::SequencerView::Listener
{
public:
    AeolusAudioProcessorEditor(AeolusAudioProcessor&);
    ~AeolusAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    // juce::Timer
    void timerCallback() override;

    // SequencerView::Listener
    void onSequencerEnterProgramMode() override;
    void onSequencerLeaveProgramMode() override;

private:

    void populateDivisions();
    void refresh();

    void updateMeters();
    void updateMidiKeyboardRange();
    void updateMidiKeyboardKeySwitches();
    void updateDivisionViews();
    void updateSequencerView();

    AeolusAudioProcessor& _audioProcessor;

    juce::Viewport _divisionsViewport;
    juce::Component _divisionsComponent;
    juce::OwnedArray<ui::DivisionView> _divisionViews;

    CustomMidiKeyboard _midiKeyboard;

    ui::OverlayComponent _overlay;

    ui::PresetView _presetView;
    ui::SequencerView _sequencerView;

    juce::Label _versionLabel;
    juce::Label _cpuLoadLabel;
    juce::Label _cpuLoadValueLabel;
    juce::Label _voiceCountLabel;
    juce::Label _voiceCountValueLabel;
    juce::Label _reverbLabel;
    juce::ComboBox _reverbComboBox;
    ui::ParameterSlider _reverbSlider;
    juce::Label _volumeLabel;
    ui::ParameterSlider _volumeSlider;

    ui::LevelIndicator _volumeLevelL;
    ui::LevelIndicator _volumeLevelR;

    juce::DrawableButton _tuningButton;

    /// Kill all active voices button
    juce::TextButton _panicButton;

    /// Organ cancel button
    juce::TextButton _cancelButton;

    /// MIDI control channel selection
    juce::Label _midiControlChannelLabel;
    ui::MidiChannelsComponent _midiControlChannels;

    /// MIDI swell channel selection
    juce::Label _midiSwellChannelLabel;
    ui::MidiChannelsComponent _midiSwellChannels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AeolusAudioProcessorEditor)
};
