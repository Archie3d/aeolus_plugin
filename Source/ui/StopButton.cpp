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

using namespace juce;

namespace ui {

static const Font& getButtonFont()
{
    static Font font(Typeface::createSystemTypefaceFor(BinaryData::WignersFriendRoman1GY8e_ttf,
                                                       BinaryData::WignersFriendRoman1GY8e_ttfSize));
    return font;
}

StopButton::StopButton(const String& name)
    : Button(name)
    , _margin{5}
{
    setClickingTogglesState(true);
}

void StopButton::paintButton (Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds();
    bounds.reduce(_margin, _margin);

    g.setColour(Colours::black);
    g.fillEllipse(bounds.toFloat());

    auto color = getToggleState() ? Colour(0xFF, 0xF0, 0x00) : Colour(0x99, 0x99, 0x99);

    int offset = 2;

    if (shouldDrawButtonAsDown)
        offset += 2;

    if (shouldDrawButtonAsHighlighted) {
        color = color.brighter();
    }

    g.setColour(color);
    g.fillEllipse(bounds.getX() + offset, bounds.getY() + offset,
                  bounds.getWidth() - 8, bounds.getHeight() - 8);

    g.setColour(Colours::black);
    auto font = getButtonFont();
    font.setHeight(14);
    g.setFont(font);

    //g.drawText(getName(), offset, offset, getWidth() - 8, getHeight() - 8, Justification::centred);

    g.drawMultiLineText(getName(), bounds.getX() + offset,
                        bounds.getY() + bounds.getHeight()/2 + offset - 2,
                        bounds.getWidth() - 10,
                        Justification::centred);
}

} // namespace ui
