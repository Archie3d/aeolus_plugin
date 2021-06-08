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

namespace ui {

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:

    CustomLookAndFeel();

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawLinearSliderThumb(juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawLinearSliderBackground(juce::Graphics& g, int x, int y, int width, int height,
                                    float sliderPos,
                                    float minSliderPos,
                                    float maxSliderPos,
                                    const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    int getSliderThumbRadius(juce::Slider&) override { return 6; }

    static CustomLookAndFeel& getInstance() {
        static CustomLookAndFeel instance;
        return instance;
    }

private:

    void drawRoundThumb (juce::Graphics& g, float x, float y, float diameter, juce::Colour colour, float outlineThickness);

};

} // namespace ui
