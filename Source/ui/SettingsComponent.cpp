// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Arthur Benilov <arthur.benilov@gmail.com>
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

#include "ui/SettingsComponent.h"

using namespace juce;

namespace ui {

SettingsComponent::SettingsComponent()
    : _settingsLabel {{}, "Global settings"}
    , _uiScalingFactorLabel {{}, "UI scaling factor"}
    , _uiScalingFactorSlider{Slider::IncDecButtons, Slider::TextBoxLeft}
    , _defaultButton{"Default"}
    , _okButton{"OK"}
    , _cancelButton{"Cancel"}
{
    auto* g = aeolus::EngineGlobal::getInstance();

    addAndMakeVisible(_settingsLabel);
    _settingsLabel.setJustificationType(Justification::centred);
    _settingsLabel.setColour(Label::textColourId, Colours::lightyellow);
    auto font = _settingsLabel.getFont();
    font.setHeight(font.getHeight() * 1.2f);
    _settingsLabel.setFont(font);

    addAndMakeVisible(_uiScalingFactorLabel);
    addAndMakeVisible(_uiScalingFactorSlider);
    _uiScalingFactorSlider.setTextBoxStyle(Slider::TextBoxLeft, false, 70, 20);
    _uiScalingFactorSlider.setTextValueSuffix("%");
    _uiScalingFactorSlider.setRange(aeolus::UI_SCALING_MIN, aeolus::UI_SCALING_MAX, aeolus::UI_SCALING_SETP);
    _uiScalingFactorSlider.setValue(g->getUIScalingFactor(), juce::dontSendNotification);


    addAndMakeVisible(_defaultButton);
    _defaultButton.onClick = [this] {
        _uiScalingFactorSlider.setValue(aeolus::UI_SCALING_DEFAULT);
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

float SettingsComponent::getUIScalingFactor() const
{
    return (float)_uiScalingFactorSlider.getValue();
}

void SettingsComponent::resized()
{
    constexpr int margin = 6;

    auto bounds = getLocalBounds();
    bounds.reduce(margin, margin);

    _settingsLabel.setBounds(bounds.removeFromTop(20));

    bounds.removeFromTop(3 * margin);

    auto row = bounds.removeFromTop(20);
    _uiScalingFactorLabel.setBounds(row.removeFromLeft(120));
    _uiScalingFactorSlider.setBounds(row.removeFromLeft(110));


    row = bounds.removeFromBottom(20);
    _defaultButton.setBounds(row.removeFromLeft(60));
    _cancelButton.setBounds(row.removeFromRight(60));
    row.removeFromRight(margin);
    _okButton.setBounds(row.removeFromRight(60));
}

} // namespace ui
