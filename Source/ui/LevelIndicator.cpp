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

#include "ui/LevelIndicator.h"

using namespace juce;

namespace ui {

const static Colour backgroundColour(20, 20, 20);
const static Colour meterColour(60, 255, 200);
const static Colour peakColour(255, 45, 20);

constexpr float smoothFactor = 0.95f;


LevelIndicator::LevelIndicator(aeolus::LevelMeter& meter, Orientation orientation)
    : Component{}
    , _levelMeter{meter}
    , _orientation{orientation}
    , _skew{1.0f}
    , _peakLevel{0.0f}
{
    startTimerHz(20);
}

void LevelIndicator::paint(Graphics& g)
{
    g.setColour(backgroundColour);
    g.fillRect(0.0f, 0.0f, (float)getWidth(), (float)getHeight());

    switch (_orientation) {
    case Orientation::Horizontal:
        paintHorizontal(g);
        break;
    case Orientation::Vertical:
        paintVertical(g);
        break;
    default:
        break;
    }
}

void LevelIndicator::timerCallback()
{
    const float lvl = _levelMeter.getPeakLevel();

    if (lvl > _peakLevel)
        _peakLevel = lvl;
    else
        _peakLevel = smoothFactor * _peakLevel + (1.0f - smoothFactor) * lvl;

    _peakLevel = _levelMeter.getPeakLevel();

    repaint();
}

void LevelIndicator::paintHorizontal(Graphics& g)
{
    auto lvl = jlimit(0.0f, 1.0f, _peakLevel);
    const float w = getWidth() * powf(lvl, _skew);

    if (w > 0) {
        g.setGradientFill(getColourGradient());
        g.fillRect(0.0f, 0.0f, w, (float)getHeight());
    }
}

void LevelIndicator::paintVertical(Graphics& g)
{
    auto lvl = jlimit(0.0f, 1.0f, _peakLevel);
    const float h = getHeight() * powf(lvl, _skew);

    if (h > 0) {
        g.setGradientFill(getColourGradient());
        g.fillRect(0.0f, getHeight() - h, (float)getWidth(), h);
    }
}

ColourGradient LevelIndicator::getColourGradient()
{
    auto lvl = jlimit(0.0f, 1.0f, _peakLevel);
    bool overload = _peakLevel >= 1.0f;

    const float intensity = 0.5f * lvl + 0.5f;

    auto colour = overload ? peakColour : meterColour;
    colour = colour.withMultipliedBrightness(intensity);

    ColourGradient grad;

    switch (_orientation) {
    case Orientation::Horizontal:
        grad = ColourGradient::horizontal(colour, 0, peakColour, (float)getWidth());
        grad.addColour(0.75f, colour);
        break;
    case Orientation::Vertical:
        grad = ColourGradient::vertical(peakColour, 0, colour, (float)getHeight());
        grad.addColour(0.25f, colour);
        break;
    default:
        break;
    }

    return grad;
}

} // namespace ui
