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

#include "stop.h"

AEOLUS_NAMESPACE_BEGIN

Stop::Stop()
    : _type{Type::Unknown}
    , _name{}
    , _zones()
    , _gain{1.0f}
    , _chiffGain{0.0f}
    , _enabled{false}
{
}

void Stop::addZone(Rankwave* ptr)
{
    jassert(ptr != nullptr);

    Zone zone{};
    zone.keyRange = juce::Range<int>(ptr->getNoteMin(), ptr->getNoteMax() + 1);
    zone.rankwaves.push_back(ptr);

    _zones.push_back(zone);
}

void Stop::addZone(const std::vector<Rankwave*> rw)
{
    if (rw.empty())
        return;

    Zone zone{};
    zone.keyRange = juce::Range<int>(rw[0]->getNoteMin(), rw[0]->getNoteMax() + 1);
    
    for (auto* ptr : rw) {
        juce::Range<int> range(ptr->getNoteMin(), ptr->getNoteMax() + 1);
        zone.keyRange = zone.keyRange.getUnionWith(range);\
        zone.rankwaves.push_back(ptr);
    }

    _zones.push_back(zone);
}

juce::Range<int> Stop::getKeyRange() const
{
    if (_zones.empty())
        return {};

    auto range(_zones[0].keyRange);

    for (const auto& zone : _zones)
        range = range.getUnionWith(zone.keyRange);

    return range;
}

Stop::Type Stop::getTypeFromString(const juce::String& n)
{
    const static std::map<juce::String, Stop::Type> nameToType {
        { "principal", Stop::Type::Principal },
        { "flute",     Stop::Type::Flute },
        { "reed",      Stop::Type::Reed },
        { "string",    Stop::Type::String }
    };

    auto type = Stop::Type::Unknown;

    const auto it = nameToType.find(n.toLowerCase());

    if (it != nameToType.end())
        type = it->second;

    return type;
}

AEOLUS_NAMESPACE_END
