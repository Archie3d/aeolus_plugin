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

AEOLUS_NAMESPACE_BEGIN

/**
 * This class represents a single stop.
 * A stop can be a combination of pipes arranged in zones.
 * A zone is defined for a continuous range of keys and is
 * composed of one or multiple pipes (e.g. mixtures).
 */
class Stop
{
public:

    // Stop type.
    enum class Type {
        Unknown,
        Principal,
        Flute,
        Reed,
        String
    };

    // Zone - a grouping pipes for a range of keys.
    struct Zone {
        juce::Range<int> keyRange;
        std::vector<Rankwave*> rankwaves;

        bool isForKey(int key) const noexcept { return keyRange.contains(key); }
    };

    //------------------------------------------------------

    Stop();

    void initFromVar(const juce::var& v);

    Type getType() const noexcept { return _type; }
    void setType(Type t) noexcept { _type = t; }

    juce::String getName() const { return _name; }
    void setName(const juce::String& name) { _name = name; }

    float getGain() const noexcept { return _gain; }
    void setGain(float g) noexcept { _gain = g; }

    float getChiffGain() const noexcept { return _chiffGain; }
    void setChiffGain(float g) noexcept { _chiffGain = g; }

    bool isEnabled() const noexcept { return _enabled; }
    void setEnabled(bool shouldBeEnabled) noexcept { _enabled = shouldBeEnabled; }

    const std::vector<Zone>& getZones() const noexcept { return _zones; }

    /**
     * Add a zone that consists of a single rankwave (pipe)
     * that covers its entire range of keys.
     */
    void addZone(Rankwave* ptr);

    /**
     * Add a zone composed of multiple pipes.
     */
    void addZone(const std::vector<Rankwave*> rw);

    /**
     * Returns the range of keys this stop can be triggered by.
     */
    juce::Range<int> getKeyRange() const;

    // Convert stop type from its string name.
    static Type getTypeFromString(const juce::String& n);

private:
    Type _type{Type::Unknown};
    juce::String _name;
    std::vector<Zone> _zones;
    float _gain{1.0f};
    float _chiffGain{0.0f};
    bool _enabled{false};
};

AEOLUS_NAMESPACE_END
