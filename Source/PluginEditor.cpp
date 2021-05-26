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

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================
AeolusAudioProcessorEditor::AeolusAudioProcessorEditor (AeolusAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , _audioProcessor(p)
    , _stopButtons()
    , _midiKeyboard(p.getEngine().getMidiKeyboardState(), MidiKeyboardComponent::horizontalKeyboard)
    , _versionLabel{{}, JucePlugin_VersionString}
    , _cpuLoadLabel{{}, "CPU Load:"}
    , _cpuLoadValueLabel{}
    , _voiceCountLabel{{}, "Voices:"}
    , _voiceCountValueLabel{}
{
    getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, Colour(0x1F, 0x1F, 0x1F));

    setSize (740, 480);
    setResizeLimits(740, 480, 2048, 1920);

    addAndMakeVisible(_versionLabel);
    _versionLabel.setFont (Font(Font::getDefaultMonospacedFontName(), 10, Font::plain));
    _versionLabel.setJustificationType (Justification::right);

    addAndMakeVisible(_cpuLoadLabel);
    _cpuLoadValueLabel.setFont (Font(Font::getDefaultMonospacedFontName(), 12, Font::plain));
    _cpuLoadValueLabel.setJustificationType (Justification::right);
    _cpuLoadValueLabel.setColour(Label::textColourId, Colours::lightyellow);
    addAndMakeVisible(_cpuLoadValueLabel);

    addAndMakeVisible(_voiceCountLabel);
    _voiceCountValueLabel.setFont (Font(Font::getDefaultMonospacedFontName(), 12, Font::plain));
    _voiceCountValueLabel.setJustificationType (Justification::right);
    _voiceCountValueLabel.setColour(Label::textColourId, Colours::lightyellow);
    addAndMakeVisible(_voiceCountValueLabel);

    populateStops();

    _midiKeyboard.setScrollButtonsVisible(false);
    _midiKeyboard.setAvailableRange(24, 108);
    addAndMakeVisible(_midiKeyboard);

    startTimerHz(10);
}

AeolusAudioProcessorEditor::~AeolusAudioProcessorEditor() = default;

//==============================================================================
void AeolusAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour(Colour(0x33, 0x33, 0x33));
    g.fillRect(0, 0, getWidth(), 30);
}

void AeolusAudioProcessorEditor::resized()
{
    constexpr int margin = 5;

    _versionLabel.setBounds(getWidth() - 60, margin, 60 - margin, 20);

    _cpuLoadLabel.setBounds(margin, margin, 70, 20);
    _cpuLoadValueLabel.setBounds(_cpuLoadLabel.getRight() + margin, margin, 30, 20);
    _voiceCountLabel.setBounds(150, margin, 50, 20);
    _voiceCountValueLabel.setBounds(_voiceCountLabel.getRight() + margin, margin, 24, 20);

    constexpr int W = 120;
    constexpr int H = 30;
    constexpr int S = 5;
    constexpr int T = margin * 2 + 20;

    const int grid = getWidth() / (W + S);

    int i = 0;

    for (auto* button : _stopButtons) {
        int x = (i % grid) * (W + S);
        int y = (i / grid) * (H + S) + T;

        button->setBounds(x, y, W, H);

        ++i;
    }

    int keyboardWidth = jmin((int)_midiKeyboard.getTotalKeyboardWidth(), getWidth());

    _midiKeyboard.setBounds((getWidth() - keyboardWidth) / 2, getHeight() - 70, keyboardWidth, 70);
}

void AeolusAudioProcessorEditor::timerCallback()
{
    refresh();
}

void AeolusAudioProcessorEditor::populateStops()
{
    auto& division = _audioProcessor.getEngine().getDivision();

    for (int i = 0; i < division.getStopsCount(); ++i) {
        auto& stop = division[i];
        auto stopName = stop.rankwave->getStopName();



        auto button = std::make_unique<StopButton>(stopName);
        button->setToggleState(stop.enabled, juce::dontSendNotification);
        button->setClickingTogglesState(true);

        auto* buttonPtr = button.get();
        //buttonPtr->setColour(StopButton::textColourId, Colours::lightgrey);
        //buttonPtr->setColour(StopButton::tickColourId, Colours::yellow);

        buttonPtr->setClickingTogglesState(true);

        button->onClick = [this, i, buttonPtr]() {
            auto& division = _audioProcessor.getEngine().getDivision();
            division[i].enabled = buttonPtr->getToggleState();
        };

        addAndMakeVisible(button.get());
        _stopButtons.add(button.release());
    }

    resized();
}

void AeolusAudioProcessorEditor::refresh()
{
    updateMeters();
    updateMidiKeyboardRange();
}

void AeolusAudioProcessorEditor::updateMeters()
{
    auto strLoad = String (int (_audioProcessor.getProcessLoad() * 100.0f)) + "%";
    auto strVoices = String (_audioProcessor.getActiveVoiceCount());

    _cpuLoadValueLabel.setText (strLoad, dontSendNotification);
    _voiceCountValueLabel.setText (strVoices, dontSendNotification);
}

void AeolusAudioProcessorEditor::updateMidiKeyboardRange()
{
    auto range = _audioProcessor.getEngine().getMidiKeyboardRange();

    if (range.getStart() < 0 || range.getEnd() < 0)
        _midiKeyboard.setPlayableRange(0, 0);
    else
        _midiKeyboard.setPlayableRange(range.getStart(), range.getEnd());
}
