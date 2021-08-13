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

#include "aeolus/engine.h"
#include "ui/CustomLookAndFeel.h"

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================
AeolusAudioProcessorEditor::AeolusAudioProcessorEditor (AeolusAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , _audioProcessor(p)
    , _divisionsViewport{}
    , _divisionsComponent{}
    , _divisionViews{}
    , _midiKeyboard(p.getEngine().getMidiKeyboardState(), MidiKeyboardComponent::horizontalKeyboard)
    , _overlay{}
    , _sequencerView(p.getEngine().getSequencer())
    , _versionLabel{{}, JucePlugin_VersionString}
    , _cpuLoadLabel{{}, "CPU Load:"}
    , _cpuLoadValueLabel{}
    , _voiceCountLabel{{}, "Voices:"}
    , _voiceCountValueLabel{}
    , _reverbLabel{{}, "Reverb:"}
    , _reverbComboBox{}
    , _reverbSlider{*p.getParametersContainer().reverbWet, juce::Slider::LinearHorizontal}
    , _volumeLabel{{}, "Volume:"}
    , _volumeSlider{*p.getParametersContainer().volume, juce::Slider::LinearHorizontal}
    , _volumeLevelL{p.getEngine().getVolumeLevel().left, ui::LevelIndicator::Orientation::Horizontal}
    , _volumeLevelR{p.getEngine().getVolumeLevel().right, ui::LevelIndicator::Orientation::Horizontal}
    , _panicButton{"PANIC"}
    , _cancelButton{"Cancel"}
    , _midiControlChannelLabel{{}, {"Conrol channel"}}
    , _midiControlChannelComboBox{}
{
    setLookAndFeel(&ui::CustomLookAndFeel::getInstance());

    setSize (1260, 600);
    setResizeLimits(1024, 600, 4096, 4096);

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

    addAndMakeVisible(_reverbLabel);

    addAndMakeVisible(_reverbComboBox);

    auto* g = aeolus::EngineGlobal::getInstance();

    int id = 0;
    for (const auto& ir : g->getIRs()) {
        _reverbComboBox.addItem(ir.name, ++id);
    }

    _reverbComboBox.setSelectedId(_audioProcessor.getEngine().getReverbIR() + 1);

    _reverbComboBox.onChange = [this]() {
        const auto irNum = _reverbComboBox.getSelectedId() - 1;
        auto& engine = _audioProcessor.getEngine();

        if (engine.getReverbIR() != irNum)
            engine.postReverbIR(irNum);
    };

    addAndMakeVisible(_reverbSlider);

    _volumeLevelL.setSkew(0.5f);
    addAndMakeVisible(_volumeLevelL);
    _volumeLevelR.setSkew(0.5f);
    addAndMakeVisible(_volumeLevelR);

    addAndMakeVisible(_volumeLabel);
    addAndMakeVisible(_volumeSlider);
    _volumeSlider.setSkewFactor(0.5f);
    _volumeSlider.setLookAndFeel(&ui::CustomLookAndFeel::getInstance());

    _panicButton.setColour(TextButton::textColourOffId, Colour(0xFF, 0xFF, 0xFF));
    _panicButton.setColour(TextButton::buttonColourId, Colour(0xCC, 0x33, 0x00));
    addAndMakeVisible(_panicButton);
    _panicButton.onClick = [this] {
        _audioProcessor.panic();
    };

    _cancelButton.onClick = [this]() {
        for (auto* divisionView : _divisionViews) {
            divisionView->cancelAllStops();
            divisionView->cancelAllLinks();
            divisionView->cancelTremulant();
        }
    };

    addAndMakeVisible(_cancelButton);

    addAndMakeVisible(_divisionsViewport);
    _divisionsViewport.setViewedComponent(&_divisionsComponent, false /* don't delete */);
    _divisionsViewport.setScrollBarsShown(true, false);

    populateDivisions();

    _midiKeyboard.setScrollButtonsVisible(false);
    _midiKeyboard.setAvailableRange(24, 108);
    addAndMakeVisible(_midiKeyboard);

    _midiControlChannelLabel.setColour(Label::textColourId, Colour(0x99, 0x99, 0x99));
    addAndMakeVisible(_midiControlChannelLabel);

    _midiControlChannelComboBox.addItem("All", 1);
    for (int i = 1; i <= 16; ++i) {
        _midiControlChannelComboBox.addItem(String(i), i + 1);
    }

    _midiControlChannelComboBox.setSelectedId(1 + _audioProcessor.getEngine().getMIDIControlChannel(), juce::dontSendNotification);
    _midiControlChannelComboBox.onChange = [this]() {
        _audioProcessor.getEngine().setMIDIControlChannel(_midiControlChannelComboBox.getSelectedId() - 1);
    };

    addAndMakeVisible(_midiControlChannelComboBox);

    // Overlay and sequencer must go on the very top

    addChildComponent(_overlay);

    _overlay.onClick = [this]() {
        _sequencerView.cancelProgramMode();
    };

    _sequencerView.addListener(this);

    addAndMakeVisible(_sequencerView);

    resized();

    startTimerHz(10);
}

AeolusAudioProcessorEditor::~AeolusAudioProcessorEditor()
{
    _sequencerView.removeListener(this);
};

//==============================================================================
void AeolusAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour(Colour(0x36, 0x35, 0x33));
    g.fillRect(0, 0, getWidth(), 30);
}

void AeolusAudioProcessorEditor::resized()
{
    _overlay.setBounds(getLocalBounds());

    constexpr int margin = 5;

    _versionLabel.setBounds(getWidth() - 60, margin, 60 - margin, 20);

    _cpuLoadLabel.setBounds(margin, margin, 70, 20);
    _cpuLoadValueLabel.setBounds(_cpuLoadLabel.getRight() + margin, margin, 36, 20);
    _voiceCountLabel.setBounds(150, margin, 56, 20);
    _voiceCountValueLabel.setBounds(_voiceCountLabel.getRight() + margin, margin, 30, 20);

    _reverbLabel.setBounds(_voiceCountValueLabel.getRight() + 40, margin, 60, 20);
    _reverbComboBox.setBounds(_reverbLabel.getRight() + margin, margin, 220, 20);
    _reverbSlider.setBounds(_reverbComboBox.getRight() + margin, margin, 100, 20);

    _volumeLabel.setBounds(_reverbSlider.getRight() + 40, margin, 60, 20);
    _volumeSlider.setBounds(_volumeLabel.getRight() + margin, margin, 100, 20);

    _volumeLevelL.setBounds(_volumeSlider.getX() + 5, _volumeSlider.getY() + 2, _volumeSlider.getWidth() - 10, 2);
    _volumeLevelR.setBounds(_volumeSlider.getX() + 5, _volumeSlider.getY() + _volumeSlider.getHeight() - 4, _volumeSlider.getWidth() - 10, 2);

    _panicButton.setBounds(_volumeSlider.getRight() + 40, margin, 50, 20);

    constexpr int W = 120;
    constexpr int H = 30;
    constexpr int S = 5;
    constexpr int T = margin * 2 + 20;
    constexpr int sequencerHeight = 26;
    constexpr int keyboardHeight = 70;

    int y = 0;;

    for (auto* divisionView : _divisionViews) {
        const auto h = divisionView->getEstimatedHeightForWidth(getWidth());
        divisionView->setBounds(0, y, getWidth(), h);
        y += h;
    }

    _divisionsComponent.setBounds(0, 0, getWidth(), y);
    _divisionsViewport.setBounds(0, T, getWidth(), getHeight() - T - keyboardHeight - sequencerHeight);

    int keyboardWidth = jmin((int)_midiKeyboard.getTotalKeyboardWidth(), getWidth());
    _midiKeyboard.setBounds((getWidth() - keyboardWidth) / 2, getHeight() - keyboardHeight, keyboardWidth, keyboardHeight);

    _sequencerView.setBounds(_midiKeyboard.getX(), _midiKeyboard.getY() - sequencerHeight, _midiKeyboard.getWidth(), sequencerHeight);

    _cancelButton.setColour(TextButton::buttonColourId, Colour(0x66, 0x66, 0x33));
    _cancelButton.setBounds((_midiKeyboard.getX() - 60)/2, getHeight() - 60, 60, 35);

    int x = _midiKeyboard.getRight() + (getWidth() - _midiKeyboard.getRight() - 100) / 2;
    _midiControlChannelLabel.setBounds(x, _midiKeyboard.getY(), 100, 20);
    _midiControlChannelComboBox.setBounds(x, _midiControlChannelLabel.getBottom() + 5, 100, 20);
}

void AeolusAudioProcessorEditor::timerCallback()
{
    refresh();
}

void AeolusAudioProcessorEditor::onSequencerEnterProgramMode()
{
    _overlay.setVisible(true);
}

void AeolusAudioProcessorEditor::onSequencerLeaveProgramMode()
{
    _overlay.setVisible(false);
}

void AeolusAudioProcessorEditor::populateDivisions()
{
    for (int i = 0; i < _audioProcessor.getEngine().getDivisionCount(); ++i) {
        auto* div = _audioProcessor.getEngine().getDivisionByIndex(i);

        auto view = std::make_unique<ui::DivisionView>(div);
        _divisionsComponent.addAndMakeVisible(view.get());
        _divisionViews.add(view.release());
    }
}

void AeolusAudioProcessorEditor::refresh()
{
    updateMeters();
    updateDivisionViews();
    updateSequencerView();
    updateMidiKeyboardRange();
    updateMidiControlChannel();
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

void AeolusAudioProcessorEditor::updateDivisionViews()
{
    for (auto* dv : _divisionViews)
        dv->update();
}

void AeolusAudioProcessorEditor::updateSequencerView()
{
    _sequencerView.update();
}

void AeolusAudioProcessorEditor::updateMidiControlChannel()
{
    const int ch = _audioProcessor.getEngine().getMIDIControlChannel();
    _midiControlChannelComboBox.setSelectedId(1 + ch, juce::dontSendNotification);
}