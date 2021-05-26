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
#include "aeolus/rankwave.h"

#include <vector>

AEOLUS_NAMESPACE_BEGIN

/**
 * @brief Single keyboard division.
 * 
 * A division may have multiple stops available, which can be enabled or
 * disabled individually.
 */
class Division
{
public:

    /// Single stop descriptor.
    struct Stop
    {
        Rankwave* rankwave = nullptr;   ///< Corresponding pipe model.
        bool enabled = false;           ///< Stop enablement flag.
        juce::String name = "";         ///< Stop display name.
    };

    Division(const juce::String& name = juce::String());

    /**
     * @brief Load the division configuration from a JSON object.
     */
    void fromVar(const juce::var& v);

    void clear();
    void addRankwave(Rankwave* ptr, bool ena = false, const juce::String& name = juce::String());

    int getStopsCount() const noexcept { return (int)_rankwaves.size(); }
    void enableStop(int i, bool ena) { _rankwaves[i].enabled = ena; }
    bool isStopEnabled(int i) const { return _rankwaves[i].enabled; }
    const Stop& operator[](int i) const { return _rankwaves[i]; }
    Stop& operator[](int i) { return _rankwaves[i]; }
    Stop& getStopByIndex(int i) { return _rankwaves[i]; }

    void getAvailableRange(int& minNote, int& maxNote) const noexcept;

    bool isForMIDIChannel(int channel) const noexcept;

private:

    juce::String _name;     ///< The division name.
    bool _hasSwell;         ///< Whetehr this division has a swell control.
    bool _hasTremulant;     ///< Whether this division has a remulant control.

    int _midiChannel;       ///< Division MIDI channel.

    std::vector<Stop> _rankwaves;    ///< All the stops this division has.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Division)
};

AEOLUS_NAMESPACE_END
