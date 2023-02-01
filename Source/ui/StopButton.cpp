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
#include "ui/CustomLookAndFeel.h"

using namespace juce;

namespace ui {

namespace details {
    const static juce::Colour onColour { 0xFF, 0xF0, 0x00 };
    const static juce::Colour offColour{ 0x66, 0x66, 0x66 };

    const static juce::Colour textFluteOnColour { 0x00, 0x00, 0x00 };
    const static juce::Colour textFluteOffColour{ 0x11, 0x11, 0x11 };
    const static juce::Colour textReedOnColour  { 0xFF, 0x33, 0x00 };
    const static juce::Colour textReedOffColour { 0x7F, 0x00, 0x00 };
}

StopButton::StopButton(aeolus::Division& division, int stopIndex)
    : Button{division.getStopByIndex(stopIndex).getName()}
    , _division{division}
    , _stopIndex{stopIndex}
    , _stop{division.getStopByIndex(stopIndex)}
    , _margin{4}
{
    setClickingTogglesState(true);

    setToggleState(_stop.isEnabled(), juce::dontSendNotification);
    updateColourFromState();

    this->onClick = [this]() {
        startColourAnimation();
        _division.enableStop(_stopIndex, getToggleState());
    };
}

void StopButton::update()
{
    const auto targetState{ _division.isStopEnabled(_stopIndex) };

    if (getToggleState() != targetState) {
        setToggleState(_division.isStopEnabled(_stopIndex), juce::dontSendNotification);
        startColourAnimation();
    }
}

void StopButton::paintButton (Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds();
    bounds.reduce(_margin, _margin);

    g.setColour(Colours::black);
    g.fillEllipse(bounds.toFloat());

    int offset = 2;

    if (shouldDrawButtonAsDown)
        offset += 2;

    auto paintColour{ shouldDrawButtonAsHighlighted ? colour.brighter() : colour };

    g.setColour(paintColour);
    g.fillEllipse(float(bounds.getX() + offset), float(bounds.getY() + offset),
                  float(bounds.getWidth() - 8), float(bounds.getHeight() - 8));

    const bool on{ getToggleState() };

    const Colour textColour = _stop.getType() == aeolus::Stop::Type::Reed
                                        ? (on ? details::textReedOnColour : details::textReedOffColour)
                                        : (on ? details::textFluteOnColour : details::textFluteOffColour);

    g.setColour(textColour);

    auto font{ CustomLookAndFeel::getStopButtonFont() };
    font.setHeight(14);
    g.setFont(font);

    g.drawMultiLineText(getName(), bounds.getX() + offset,
                        bounds.getY() + bounds.getHeight()/2 + offset - 8,
                        bounds.getWidth() - 10,
                        Justification::centred);
}

void StopButton::timerCallback()
{
    colour = colour.interpolatedWith(targetColour, 0.25f);

    if (colour.getRed() == targetColour.getRed()
      || colour.getGreen() == targetColour.getGreen()
      || colour.getBlue() == targetColour.getBlue())
    {
        colour = targetColour;
        stopTimer();
    }

    repaint();
}

void StopButton::updateColourFromState()
{
    targetColour = colour = getToggleState() ? details::onColour : details::offColour;
}

void StopButton::startColourAnimation()
{
    targetColour = getToggleState() ? details::onColour : details::offColour;
    startTimerHz(20);
}

} // namespace ui
