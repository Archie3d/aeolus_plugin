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

#include "aeolus/globals.h"
#include <memory>

namespace ui {

/**
 * @brief Slider widget linked to an audio processor parameter.
 */
class ParameterSlider : public juce::Slider
{
public:
    explicit ParameterSlider (juce::AudioProcessorParameter &p,
                              juce::Slider::SliderStyle style = juce::Slider::LinearVertical,
                              juce::Slider::TextEntryBoxPosition textBoxPosition = juce::Slider::NoTextBox);
    ~ParameterSlider();

    double getValueFromText (const juce::String &text) override;
    juce::String getTextFromValue (double value) override;

private:

    struct Impl;
    std::unique_ptr<Impl> d;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterSlider)
};

} // namespace ui
