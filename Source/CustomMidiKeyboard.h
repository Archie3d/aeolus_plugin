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

#include <JuceHeader.h>
#include <set>

class CustomMidiKeyboard : public juce::MidiKeyboardComponent
{
public:
    CustomMidiKeyboard(juce::MidiKeyboardState& state, juce::MidiKeyboardComponent::Orientation orientation);

    void setPlayableRange(int noteMin, int noteMax);

    void setKeySwitches(const std::set<int> keySwitches);

protected:

    // juce::MidiKeyboardComponent
    void drawWhiteNote(int midiNoteNumber,
                       juce::Graphics& g, juce::Rectangle<float> area,
                       bool isDown, bool isOver,
                       juce::Colour lineColour, juce::Colour textColour) override;

    void drawBlackNote(int midiNoteNumber,
                       juce::Graphics& g, juce::Rectangle<float> area,
                       bool isDown, bool isOver,
                       juce::Colour noteFillColour) override;

private:

    bool isNoteWithinPlayableRange(int note) const noexcept;

    bool isNoteKeySwitch(int note) const noexcept;

    juce::Range<int> _playableRange;
    std::set<int> _keySwitches;
};
