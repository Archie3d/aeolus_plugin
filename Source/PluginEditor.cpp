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
    , _divisionViews{}
    , _midiKeyboard(p.getEngine().getMidiKeyboardState(), MidiKeyboardComponent::horizontalKeyboard)
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
{
    getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, Colour(0x1F, 0x1F, 0x1F));

    setSize (1180, 600);
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

    addAndMakeVisible(_reverbLabel);

    addAndMakeVisible(_reverbComboBox);
    _reverbComboBox.setColour(ComboBox::backgroundColourId, Colour(0x33, 0x33, 0x33));    
    _reverbComboBox.setColour(ComboBox::arrowColourId, Colour(0x66, 0x66, 0x66));

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

    _reverbSlider.setLookAndFeel(&ui::CustomLookAndFeel::getInstance());
    addAndMakeVisible(_reverbSlider);

    _volumeLevelL.setSkew(0.5f);
    addAndMakeVisible(_volumeLevelL);
    _volumeLevelR.setSkew(0.5f);
    addAndMakeVisible(_volumeLevelR);

    addAndMakeVisible(_volumeLabel);
    addAndMakeVisible(_volumeSlider);
    _volumeSlider.setSkewFactor(0.5f);
    _volumeSlider.setLookAndFeel(&ui::CustomLookAndFeel::getInstance());

    populateDivisions();

    _midiKeyboard.setScrollButtonsVisible(false);
    _midiKeyboard.setAvailableRange(24, 108);
    addAndMakeVisible(_midiKeyboard);

    resized();

    startTimerHz(10);
}

AeolusAudioProcessorEditor::~AeolusAudioProcessorEditor() = default;

//==============================================================================
void AeolusAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour(Colour(0x36, 0x35, 0x33));
    g.fillRect(0, 0, getWidth(), 30);
}

void AeolusAudioProcessorEditor::resized()
{
    constexpr int margin = 5;

    _versionLabel.setBounds(getWidth() - 60, margin, 60 - margin, 20);

    _cpuLoadLabel.setBounds(margin, margin, 70, 20);
    _cpuLoadValueLabel.setBounds(_cpuLoadLabel.getRight() + margin, margin, 36, 20);
    _voiceCountLabel.setBounds(150, margin, 50, 20);
    _voiceCountValueLabel.setBounds(_voiceCountLabel.getRight() + margin, margin, 30, 20);

    _reverbLabel.setBounds(_voiceCountValueLabel.getRight() + 60, margin, 60, 20);
    _reverbComboBox.setBounds(_reverbLabel.getRight() + margin, margin, 220, 20);
    _reverbSlider.setBounds(_reverbComboBox.getRight() + margin, margin, 100, 20);

    _volumeLabel.setBounds(_reverbSlider.getRight() + 60, margin, 60, 20);
    _volumeSlider.setBounds(_volumeLabel.getRight() + margin, margin, 100, 20);

    _volumeLevelL.setBounds(_volumeSlider.getX() + 5, _volumeSlider.getY() + 2, _volumeSlider.getWidth() - 10, 2);
    _volumeLevelR.setBounds(_volumeSlider.getX() + 5, _volumeSlider.getY() + _volumeSlider.getHeight() - 4, _volumeSlider.getWidth() - 10, 2);

    constexpr int W = 120;
    constexpr int H = 30;
    constexpr int S = 5;
    constexpr int T = margin * 2 + 20;

    int y = T;

    for (auto* divisionView : _divisionViews) {
        const auto h = divisionView->getEstimatedHeightForWidth(getWidth());
        divisionView->setBounds(0, y, getWidth(), h);
        y += h;
    }

    int keyboardWidth = jmin((int)_midiKeyboard.getTotalKeyboardWidth(), getWidth());

    _midiKeyboard.setBounds((getWidth() - keyboardWidth) / 2, getHeight() - 70, keyboardWidth, 70);
}

void AeolusAudioProcessorEditor::timerCallback()
{
    refresh();
}

void AeolusAudioProcessorEditor::populateDivisions()
{
    for (int i = 0; i < _audioProcessor.getEngine().getDivisionCount(); ++i) {
        auto* div = _audioProcessor.getEngine().getDivisionByIndex(i);

        auto view = std::make_unique<ui::DivisionView>(div);
        addAndMakeVisible(view.get());
        _divisionViews.add(view.release());
    }
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
