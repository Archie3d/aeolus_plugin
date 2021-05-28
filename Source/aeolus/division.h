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
#include "aeolus/voice.h"

#include <atomic>
#include <vector>

AEOLUS_NAMESPACE_BEGIN

class Engine;

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

    Division(Engine& engine, const juce::String& name = juce::String());

    /**
     * @brief Load the division configuration from a JSON object.
     * 
     * This will configure the division from an organ configuration data.
     */
    void initFromVar(const juce::var& v);

    juce::var getPersistentState() const;
    void setPersistentState(const juce::var& v);

    juce::String getName() const { return _name; }

    void clear();
    void addRankwave(Rankwave* ptr, bool ena = false, const juce::String& name = juce::String());

    int getStopsCount() const noexcept { return (int)_rankwaves.size(); }
    void enableStop(int i, bool ena) { _rankwaves[i].enabled = ena; }
    bool isStopEnabled(int i) const { return _rankwaves[i].enabled; }
    Stop& getStopByIndex(int i) { return _rankwaves[i]; }

    void getAvailableRange(int& minNote, int& maxNote) const noexcept;

    int getMIDIChannel() const noexcept { return _midiChannel; }
    bool isForMIDIChannel(int channel) const noexcept;
    void setMIDIChannel(int channel) noexcept { _midiChannel = channel; }

    bool hasSwell() const noexcept { return _hasSwell; }
    void setHasSwell(bool v) noexcept { _hasSwell = v; }
    bool hasTremulant() const noexcept { return _hasTremulant; }
    void setHasTremulant(bool v) noexcept { _hasTremulant = v; }
    bool isTremulantEnabled() const noexcept { return _tremulantEnabled; }
    void setTremulantEnabled(bool ena) noexcept;

    float getTremulantLevel(bool update = true);

    void noteOn(int note, int midiChannel);
    void noteOff(int note, int midiChannel);

    List<Voice>& getActiveVoices() noexcept { return _activeVoices; }

private:

    Engine& _engine;

    juce::String _name;     ///< The division name.
    bool _hasSwell;         ///< Whetehr this division has a swell control.
    bool _hasTremulant;     ///< Whether this division has a remulant control.

    std::atomic<int> _midiChannel;          ///< Division MIDI channel.
    std::atomic<bool> _tremulantEnabled;    ///< Whether tremulant is enabled.

    float _tremulantLevel;
    float _tremulantMaxLevel;
    std::atomic<float> _tremulantTargetLevel;

    std::vector<Stop> _rankwaves;    ///< All the stops this division has.

    List<Voice> _activeVoices;  ///< Active voices on this division.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Division)
};

AEOLUS_NAMESPACE_END
