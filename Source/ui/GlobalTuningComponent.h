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

#include <functional>
#include "aeolus/globals.h"

namespace ui {

class GlobalTuningComponent : public juce::Component
{
public:
    GlobalTuningComponent();

    float getTuningFrequency() const;
    aeolus::Scale::Type getTuningScaleType() const;

    void resized() override;

    std::function<void()> onOk{};
    std::function<void()> onCancel{};

private:
    juce::Label _globalTuningLabel;
    juce::Label _tuningFrequencyLabel;
    juce::Slider _tuningFrequencySlider;
    juce::Label _scaleLabel;
    juce::ComboBox _scaleComboBox;

    juce::TextButton _defaultButton;
    juce::TextButton _okButton;
    juce::TextButton _cancelButton;
};

} // namespace ui
