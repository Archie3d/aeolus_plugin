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

#include "aeolus/stop.h"
#include "aeolus/engine.h"

using namespace juce;

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

std::vector<Rankwave*> getRankwavesFromPipeVar(const var& v)
{
    std::vector<Rankwave*> rankwaves;

    auto addRankwave = [&](const String& name) {
        auto* g = EngineGlobal::getInstance();

        if (auto* rankwave = g->getStopByName(name)) {
            rankwaves.push_back(rankwave);
        } else {
            DBG("Stop pipe " + name + " cannot be found.");
        }
    };


    if (const auto* arr = v.getArray()) {
        for (int i = 0; i < arr->size(); ++i) {
            const String pipeName = arr->getUnchecked(i);
            addRankwave(pipeName);
        }
    } else {
        const String pipeName = v;
        addRankwave(pipeName);
    }

    return rankwaves;
}

void Stop::initFromVar(const var& v)
{


    if (const auto* obj = v.getDynamicObject()) {
        _name = obj->getProperty("name");
        _type = getTypeFromString(obj->getProperty("type"));

        if (obj->hasProperty("gain"))
            _gain = obj->getProperty("gain");

        if (obj->hasProperty("chiff"))
            _chiffGain = obj->getProperty("chiff");

        if (obj->hasProperty("pipe")) {
            const auto pipeObj = obj->getProperty("pipe");

            const auto rankwaves { getRankwavesFromPipeVar(pipeObj) };

            if (!rankwaves.empty())
                addZone(rankwaves);

        } else if (obj->hasProperty("zones")) {

            if (const auto* zonesArr = obj->getProperty("zones").getArray()) {
                for (int i = 0; i < zonesArr->size(); ++i) {
                    if (const auto* zoneObj = zonesArr->getUnchecked(i).getDynamicObject()) {
                        Zone zone{};
                        zone.rankwaves = getRankwavesFromPipeVar(zoneObj->getProperty("pipe"));

                        if (const auto* range = zoneObj->getProperty("range").getArray()) {
                            if (range->size() >= 2)
                                zone.keyRange = Range<int>((int)range->getFirst(), (int)range->getLast() + 1);
                        }

                        if (!zone.rankwaves.empty())
                            _zones.push_back(zone);
                    }

                }
            }
        }
    }
}

void Stop::addZone(Rankwave* ptr)
{
    jassert(ptr != nullptr);

    Zone zone{};
    zone.keyRange = Range<int>(ptr->getNoteMin(), ptr->getNoteMax() + 1);
    zone.rankwaves.push_back(ptr);

    _zones.push_back(zone);
}

void Stop::addZone(const std::vector<Rankwave*> rw)
{
    if (rw.empty())
        return;

    Zone zone{};
    zone.keyRange = Range<int>(rw[0]->getNoteMin(), rw[0]->getNoteMax() + 1);

    for (auto* ptr : rw) {
        Range<int> range(ptr->getNoteMin(), ptr->getNoteMax() + 1);
        zone.keyRange = zone.keyRange.getUnionWith(range);\
        zone.rankwaves.push_back(ptr);
    }

    _zones.push_back(zone);
}

Range<int> Stop::getKeyRange() const
{
    if (_zones.empty())
        return {};

    auto range(_zones[0].keyRange);

    for (const auto& zone : _zones)
        range = range.getUnionWith(zone.keyRange);

    return range;
}

Stop::Type Stop::getTypeFromString(const String& n)
{
    const static std::map<String, Stop::Type> nameToType {
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
