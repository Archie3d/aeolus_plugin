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

#include "ui/StopButton.h"
#include "ui/CustomLookAndFeel.h";

using namespace juce;

namespace ui {

StopButton::StopButton(aeolus::Division& division, int stopIndex)
    : Button{division.getStopByIndex(stopIndex).name}
    , _division{division}
    , _stopIndex{stopIndex}
    , _stop{division.getStopByIndex(stopIndex)}
    , _margin{5}
{
    setClickingTogglesState(true);

    setToggleState(_stop.enabled, juce::dontSendNotification);

    this->onClick = [this]() {
        _division.enableStop(_stopIndex, getToggleState());
    };
}

void StopButton::update()
{
    setToggleState(_division.isStopEnabled(_stopIndex), juce::dontSendNotification);
}

void StopButton::paintButton (Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds();
    bounds.reduce(_margin, _margin);

    g.setColour(Colours::black);
    g.fillEllipse(bounds.toFloat());

    auto color = getToggleState() ? Colour(0xFF, 0xF0, 0x00) : Colour(0x90, 0x90, 0x90);

    int offset = 2;

    if (shouldDrawButtonAsDown)
        offset += 2;

    if (shouldDrawButtonAsHighlighted) {
        color = color.brighter();
    }

    g.setColour(color);
    g.fillEllipse(bounds.getX() + offset, bounds.getY() + offset,
                  bounds.getWidth() - 8, bounds.getHeight() - 8);

    Colour textColour = _stop.type == aeolus::Division::Stop::Reed
                                    ? (getToggleState() ? Colour(240, 40, 0) : Colour(128, 20, 0))
                                    : Colour(0, 0, 0);

    g.setColour(textColour);

    auto font = CustomLookAndFeel::getStopButtonFont();
    font.setHeight(13);
    g.setFont(font);

    g.drawMultiLineText(getName(), bounds.getX() + offset,
                        bounds.getY() + bounds.getHeight()/2 + offset - 4,
                        bounds.getWidth() - 10,
                        Justification::centred);
}



} // namespace ui
