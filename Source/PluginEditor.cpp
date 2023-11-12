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
#include "ui/GlobalTuningComponent.h"

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
    , _tuningButton{"tuningButton", DrawableButton::ImageFitted}
    , _panicButton{"PANIC"}
    , _cancelButton{"Cancel"}
    , _midiControlChannelLabel{{}, {"Control channel"}}
    , _midiControlChannelComboBox{}
    , _midiSwellChannelLabel{{}, {"Swell channel"}}
    , _midiSwellChannelComboBox{}
{
    setLookAndFeel(&ui::CustomLookAndFeel::getInstance());

    loadUI();

    setSize (1420, 640);
    setResizeLimits(1024, 600, 4096, 4096);

    // @todo Rework ...

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

#if !AEOLUS_MULTIBUS_OUTPUT

    // Mutlibus configuration does not have a reverb

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

#endif // !AEOLUS_MULTIBUS_OUTPUT

    _volumeLevelL.setSkew(0.5f);
    addAndMakeVisible(_volumeLevelL);
    _volumeLevelR.setSkew(0.5f);
    addAndMakeVisible(_volumeLevelR);

    addAndMakeVisible(_volumeLabel);
    addAndMakeVisible(_volumeSlider);
    _volumeSlider.setSkewFactor(0.5f);
    _volumeSlider.setLookAndFeel(&ui::CustomLookAndFeel::getInstance());

    addAndMakeVisible(_tuningButton);

    auto loadSVG = [](const char* data, size_t size) -> std::unique_ptr<Drawable> {
        if (auto xml = parseXML(String::fromUTF8(data, (int)size))) {
            return Drawable::createFromSVG(*xml);
        }
        return nullptr;
    };

    auto normalIcon = loadSVG(BinaryData::tuningfork_svg, BinaryData::tuningfork_svgSize);
    auto hoverIcon = loadSVG(BinaryData::tuningforkhover_svg, BinaryData::tuningforkhover_svgSize);
    _tuningButton.setImages(normalIcon.get(), hoverIcon.get());
    _tuningButton.setMouseCursor(MouseCursor::PointingHandCursor);

    _tuningButton.onClick = [this] {
        auto content = std::make_unique<ui::GlobalTuningComponent>();
        content->setSize(240, 144);
        auto* contentPtr = content.get();

        auto& box = CallOutBox::launchAsynchronously(std::move(content), _tuningButton.getBounds(), this);
        contentPtr->onCancel = [&box] { box.dismiss(); };
        contentPtr->onOk = [&box, contentPtr] {
            const float freq = contentPtr->getTuningFrequency();
            const auto scaleType = contentPtr->getTuningScaleType();

            auto* g = aeolus::EngineGlobal::getInstance();
            const bool changed = (g->getTuningFrequency() != freq) || (g->getScale().getType() != scaleType);

            if (changed) {
                g->setTuningFrequency(freq);
                g->setScaleType(scaleType);
                g->rebuildRankwaves();
                g->saveSettings();
            }

            box.dismiss();
        };
    };

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
    _midiKeyboard.setAvailableRange(21, 108);
    addAndMakeVisible(_midiKeyboard);

    _midiControlChannelLabel.setColour(Label::textColourId, Colour(0x99, 0x99, 0x99));
    _midiSwellChannelLabel.setColour(Label::textColourId, Colour(0x99, 0x99, 0x99));

    addAndMakeVisible(_midiControlChannelLabel);
    addAndMakeVisible(_midiSwellChannelLabel);

    _midiControlChannelComboBox.addItem("All", 1);
    _midiSwellChannelComboBox.addItem("All", 1);

    for (int i = 1; i <= 16; ++i) {
        _midiControlChannelComboBox.addItem(String(i), i + 1);
        _midiSwellChannelComboBox.addItem(String(i), i + 1);
    }

    _midiControlChannelComboBox.setSelectedId(1 + _audioProcessor.getEngine().getMIDIControlChannel(), juce::dontSendNotification);
    _midiControlChannelComboBox.onChange = [this]() {
        _audioProcessor.getEngine().setMIDIControlChannel(_midiControlChannelComboBox.getSelectedId() - 1);
    };

    _midiSwellChannelComboBox.setSelectedId(1 + _audioProcessor.getEngine().getMIDISwellChannel(), juce::dontSendNotification);
    _midiSwellChannelComboBox.onChange = [this]() {
        _audioProcessor.getEngine().setMIDISwellChannel(_midiSwellChannelComboBox.getSelectedId() - 1);
    };

    addAndMakeVisible(_midiControlChannelComboBox);
    addAndMakeVisible(_midiSwellChannelComboBox);

    // Overlay and sequencer must go on the very top

    addChildComponent(_overlay);

    _overlay.onClick = [this]() {
        _sequencerView.cancelProgramMode();
    };

    _sequencerView.addListener(this);

    addAndMakeVisible(_sequencerView);

    // @todo We should keep only the view container as the top component
    addAndMakeVisible(viewContainer);

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
    viewContainer.setBounds(getLocalBounds());

    // @todo Rework ...
    _overlay.setBounds(getLocalBounds());

    constexpr int margin = 5;

    _versionLabel.setBounds(getWidth() - 60, getHeight() - 20, 60 - margin, 20);

    _cpuLoadLabel.setBounds(margin, margin, 70, 20);
    _cpuLoadValueLabel.setBounds(_cpuLoadLabel.getRight() + margin, margin, 36, 20);
    _voiceCountLabel.setBounds(150, margin, 56, 20);
    _voiceCountValueLabel.setBounds(_voiceCountLabel.getRight() + margin, margin, 30, 20);

#if !AEOLUS_MULTIBUS_OUTPUT
    _reverbLabel.setBounds(_voiceCountValueLabel.getRight() + 40, margin, 60, 20);
    _reverbComboBox.setBounds(_reverbLabel.getRight() + margin, margin, 220, 20);
    _reverbSlider.setBounds(_reverbComboBox.getRight() + margin, margin, 100, 20);

    _volumeLabel.setBounds(_reverbSlider.getRight() + 40, margin, 60, 20);

#else

    _volumeLabel.setBounds(_voiceCountValueLabel.getRight() + 430, margin, 60, 20);

#endif

    _volumeSlider.setBounds(_volumeLabel.getRight() + margin, margin, 100, 20);

    _volumeLevelL.setBounds(_volumeSlider.getX() + 5, _volumeSlider.getY() + 2, _volumeSlider.getWidth() - 10, 2);
    _volumeLevelR.setBounds(_volumeSlider.getX() + 5, _volumeSlider.getY() + _volumeSlider.getHeight() - 4, _volumeSlider.getWidth() - 10, 2);

    _tuningButton.setBounds(_volumeSlider.getRight() + 40, margin - 2, 24, 24);

    _panicButton.setBounds(getWidth() - 90, margin, 50, 20);

    constexpr int T = margin * 2 + 20;
    constexpr int sequencerHeight = 26;
    constexpr int sequencerPadding = 6;
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

    const float sequencerWidth{ (float)_sequencerView.getOptimalWidth() };
    const float sequencerX{ 0.5f * (getWidth() - sequencerWidth) + ui::SequencerView::buttonWidth };
    _sequencerView.setBounds(sequencerX, _midiKeyboard.getY() - sequencerHeight - sequencerPadding, sequencerWidth, sequencerHeight);

    _cancelButton.setColour(TextButton::buttonColourId, Colour(0x33, 0x33, 0x33));
    _cancelButton.setBounds((_midiKeyboard.getX() - 120)/2, getHeight() - 60, 60, 35);

    int x = _midiKeyboard.getRight() + (getWidth() - _midiKeyboard.getRight() - 200) / 2;

    _midiControlChannelLabel.setBounds(x, _midiKeyboard.getY(), 100, 20);
    _midiControlChannelComboBox.setBounds(_midiControlChannelLabel.getRight() + 5, _midiControlChannelLabel.getY(), 100, 20);

    _midiSwellChannelLabel.setBounds(x, _midiControlChannelLabel.getBottom() + 5, 100, 20);
    _midiSwellChannelComboBox.setBounds(_midiSwellChannelLabel.getRight() + 5, _midiSwellChannelLabel.getY(), 100, 20);
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

void AeolusAudioProcessorEditor::loadUI()
{
    viewContainer.loadFromResource("view.xml", "style.scss");

    auto* view{ viewContainer.getView() };
    jassert(view != nullptr);

    // @todo Here we should be able to access the UI elements like:
    // if (auto el{ view->getElementtById("id") }) { ... }
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
    updateMidiKeyboardKeySwitches();
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

void AeolusAudioProcessorEditor::updateMidiKeyboardKeySwitches()
{
    const auto keySwitches{ _audioProcessor.getEngine().getKeySwitches() };
    _midiKeyboard.setKeySwitches(keySwitches);
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