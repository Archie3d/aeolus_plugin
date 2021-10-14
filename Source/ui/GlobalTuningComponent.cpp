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
#include "aeolus/scale.h"

#include "ui/GlobalTuningComponent.h"

using namespace juce;

namespace ui {

GlobalTuningComponent::GlobalTuningComponent()
    : _globalTuningLabel {{}, "Global tuning settings"}
    , _tuningFrequencyLabel {{}, "Mid-A frequency"}
    , _tuningFrequencySlider{Slider::IncDecButtons, Slider::TextBoxLeft}
    , _scaleLabel{{}, "Tuning"}
    , _scaleComboBox{}
    , _defaultButton{"Default"}
    , _okButton{"OK"}
    , _cancelButton{"Cancel"}
{
    auto* g = aeolus::EngineGlobal::getInstance();

    addAndMakeVisible(_globalTuningLabel);
    _globalTuningLabel.setJustificationType(Justification::centred);
    _globalTuningLabel.setColour(Label::textColourId, Colours::lightyellow);
    auto font = _globalTuningLabel.getFont();
    font.setHeight(font.getHeight() * 1.2f);
    _globalTuningLabel.setFont(font);
    
    addAndMakeVisible(_tuningFrequencyLabel);
    addAndMakeVisible(_tuningFrequencySlider);
    _tuningFrequencySlider.setTextBoxStyle(Slider::TextBoxLeft, false, 70, 20);
    _tuningFrequencySlider.setTextValueSuffix("Hz");
    _tuningFrequencySlider.setRange(aeolus::TUNING_FREQUENCY_MIN, aeolus::TUNING_FREQUENCY_MAX, aeolus::TUNING_FREQUENCY_STEP);
    _tuningFrequencySlider.setValue(g->getTuningFrequency(), juce::dontSendNotification);

    addAndMakeVisible(_scaleLabel);
    addAndMakeVisible(_scaleComboBox);
    for (int id = aeolus::Scale::First; id < aeolus::Scale::Total; ++id) {
        _scaleComboBox.addItem(aeolus::Scale::getNameForType(static_cast<aeolus::Scale::Type>(id)), id + 1);
    }
    _scaleComboBox.setSelectedId(g->getScale().getType() + 1);

    addAndMakeVisible(_defaultButton);
    _defaultButton.onClick = [this] {
        _tuningFrequencySlider.setValue(aeolus::TUNING_FREQUENCY_DEFAULT);
        _scaleComboBox.setSelectedId(aeolus::Scale::EqualTemp + 1);
    };

    addAndMakeVisible(_okButton);
    _okButton.onClick = [this] {
        if (onOk) onOk();
    };

    addAndMakeVisible(_cancelButton);
    _cancelButton.onClick = [this] {
        if (onCancel) onCancel();
    };

    _cancelButton.setColour(TextButton::buttonColourId, Colour(0x66, 0x66, 0x33));
    _okButton.setColour(TextButton::buttonColourId, Colour(0x66, 0x66, 0x33));
    _defaultButton.setColour(TextButton::buttonColourId, Colour(0x46, 0x60, 0x16));
}

float GlobalTuningComponent::getTuningFrequency() const
{
    return (float)_tuningFrequencySlider.getValue();
}

aeolus::Scale::Type GlobalTuningComponent::getTuningScaleType() const
{
    return static_cast<aeolus::Scale::Type>(_scaleComboBox.getSelectedId() - 1);
}

void GlobalTuningComponent::resized()
{
    constexpr int margin = 5;

    auto bounds = getLocalBounds();
    bounds.reduce(margin, margin);

    _globalTuningLabel.setBounds(bounds.removeFromTop(20));

    bounds.removeFromTop(3 * margin);
    auto row = bounds.removeFromTop(20);
    _tuningFrequencyLabel.setBounds(row.removeFromLeft(120));
    _tuningFrequencySlider.setBounds(row.removeFromLeft(110));

    bounds.removeFromTop(margin);
    row = bounds.removeFromTop(20);
    _scaleLabel.setBounds(row.removeFromLeft(80));
    _scaleComboBox.setBounds(row.removeFromLeft(150));

    row = bounds.removeFromBottom(20);
    _defaultButton.setBounds(row.removeFromLeft(60));
    _cancelButton.setBounds(row.removeFromRight(60));
    row.removeFromRight(margin);
    _okButton.setBounds(row.removeFromRight(60));
}

} // namespace ui
