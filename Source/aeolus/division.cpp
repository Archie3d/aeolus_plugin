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

#include "division.h"
#include "engine.h"

using namespace juce;

AEOLUS_NAMESPACE_BEGIN

Division::Division(Engine& engine, const String& name)
    : _engine{engine}
    , _name{name}
    , _hasSwell{false}
    , _hasTremulant{false}
    , _midiChannel{0}
    , _tremulantEnabled{false}
    , _tremulantLevel{0.0f}
    , _tremulantTargetLevel{0.0f}
    , _rankwaves{}
{
}

void Division::fromVar(const var& v)
{
    if (const auto* obj = v.getDynamicObject()) {
        _name = obj->getProperty("name");
        _hasSwell = obj->getProperty("swell");
        _hasTremulant = obj->getProperty("tremulant");

        auto* g = EngineGlobal::getInstance();

        if (const auto* arr = obj->getProperty("stops").getArray()) {
            for (int i = 0; i < arr->size(); ++i) {
                if (const auto* stopObj = arr->getUnchecked(i).getDynamicObject()) {
                    const String stopName = stopObj->getProperty("name");
                    const String pipeName = stopObj->getProperty("pipe");

                    if (auto* rankwavePtr = g->getStopByName(pipeName)) {
                        addRankwave(rankwavePtr, false);
                    } else {
                        DBG("Stop pipe " + pipeName + " cannot be found.");
                    }
                }
            }
        }
    }
}

void Division::clear()
{
    _rankwaves.clear();
}

void Division::addRankwave(Rankwave* ptr, bool ena, const String& name)
{
    jassert(ptr != nullptr);

    Stop ref { ptr, ena, name };

    if (name.isEmpty())
        ref.name = ptr->getStopName();

    _rankwaves.push_back(ref);
}

void Division::getAvailableRange(int& minNote, int& maxNote) const noexcept
{
    minNote = -1;
    maxNote = -1;

    for (const auto& ref : _rankwaves) {
        if (ref.enabled) {
            if (minNote < 0 || minNote > ref.rankwave->getNoteMin())
                minNote = ref.rankwave->getNoteMin();
            if (maxNote < 0 || maxNote < ref.rankwave->getNoteMax())
                maxNote = ref.rankwave->getNoteMax();
        }
    }
}

bool Division::isForMIDIChannel(int channel) const noexcept
{
    return _midiChannel == 0        // any channel
        || _midiChannel == channel;
}

void Division::setTremulantEnabled(bool ena) noexcept
{
    _tremulantEnabled = _hasTremulant && ena;
    _tremulantTargetLevel = _tremulantEnabled ? 1.0f : 0.0f;
}

float Division::getTremulantLevel(bool update)
{
    const auto level = _tremulantLevel;

    if (update)
        _tremulantLevel += 0.5f * (_tremulantTargetLevel - _tremulantLevel);

    return level;
}

void Division::noteOn(int note)
{
    for (auto& rw : _rankwaves) {
        if (rw.enabled && rw.rankwave->isForNote(note)) {
            auto state = rw.rankwave->trigger(note);

            if (auto* voice = _engine.getVoicePool().trigger(state))
                _activeVoices.append(voice);
        }
    }
}

void Division::noteOff(int note)
{
    auto* voice = _activeVoices.first();

    while (voice != nullptr) {
        if (voice->isForNote(note))
            voice->release();

        voice = voice->next();
    }
}

AEOLUS_NAMESPACE_END
