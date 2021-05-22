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

#include "CustomMidiKeyboard.h"

using namespace juce;

CustomMidiKeyboard::CustomMidiKeyboard(juce::MidiKeyboardState& state, juce::MidiKeyboardComponent::Orientation orientation)
    : MidiKeyboardComponent(state, orientation)
    , _playableRange(getRangeStart(), getRangeEnd())
{
    //setColour(MidiKeyboardComponent::mouseOverKeyOverlayColourId, Colours::pink);
    //setColour(MidiKeyboardComponent::keyDownOverlayColourId,      Colours::red);
}

void CustomMidiKeyboard::setPlayableRange(int noteMin, int noteMax)
{
    if (_playableRange.getStart() != noteMin || _playableRange.getEnd() != noteMax) {
        _playableRange = {noteMin, noteMax};
        repaint();
    }
}

void CustomMidiKeyboard::drawWhiteNote (int midiNoteNumber,
    Graphics& g, Rectangle<float> area,
    bool isDown, bool isOver,
    Colour lineColour, Colour textColour)
{
    if (!isNoteWithinPlayableRange(midiNoteNumber)) {
        g.setColour(Colours::lightgrey.darker());
        g.fillRect (area);
    }

    juce::MidiKeyboardComponent::drawWhiteNote(midiNoteNumber, g, area, isDown, isOver, lineColour, textColour);
}

void CustomMidiKeyboard::drawBlackNote (int midiNoteNumber,
    Graphics& g, Rectangle<float> area,
    bool isDown, bool isOver,
    Colour noteFillColour)
{
    auto colour = noteFillColour;

    if (!isNoteWithinPlayableRange(midiNoteNumber)) {
        colour = Colours::darkgrey.darker();
    }

    juce::MidiKeyboardComponent::drawBlackNote(midiNoteNumber, g, area, isDown, isOver, colour);
}

bool CustomMidiKeyboard::isNoteWithinPlayableRange(int note) const noexcept
{
    return note >= _playableRange.getStart() && note <= _playableRange.getEnd();
}