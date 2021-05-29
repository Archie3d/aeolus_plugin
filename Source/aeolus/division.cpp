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
    , _tremulantMaxLevel{1.0f}
    , _tremulantTargetLevel{0.0f}
    , _paramGain{nullptr}
    , _params{Division::NUM_PARAMS}
    , _rankwaves{}
{
}

void Division::initFromVar(const var& v)
{
    if (const auto* obj = v.getDynamicObject()) {
        _name = obj->getProperty("name");
        _hasSwell = obj->getProperty("swell");
        _hasTremulant = obj->getProperty("tremulant");

        _tremulantMaxLevel = 0.0f;

        if (_hasTremulant)
            _tremulantMaxLevel = obj->getProperty("tremulant_level");

        auto* g = EngineGlobal::getInstance();

        if (const auto* arr = obj->getProperty("stops").getArray()) {
            for (int i = 0; i < arr->size(); ++i) {
                if (const auto* stopObj = arr->getUnchecked(i).getDynamicObject()) {
                    const String stopName = stopObj->getProperty("name");
                    const String pipeName = stopObj->getProperty("pipe");

                    if (auto* rankwavePtr = g->getStopByName(pipeName)) {
                        addRankwave(rankwavePtr, false, stopName);
                    } else {
                        DBG("Stop pipe " + pipeName + " cannot be found.");
                    }
                }
            }
        }
    }
}

var Division::getPersistentState() const
{
    // Per division, but we have only one so far
    auto* divisionObj = new DynamicObject();

    divisionObj->setProperty("midi_channel", getMIDIChannel());
    divisionObj->setProperty("tremulant_enabled", isTremulantEnabled());

    Array<var> stops;

    for (const auto& stop : _rankwaves) {
        auto* stopObj = new DynamicObject();
        stopObj->setProperty("name", stop.rankwave->getStopName());
        stopObj->setProperty("enabled", stop.enabled);

        stops.add(var{stopObj});
    }

    divisionObj->setProperty("stops", stops);

    return var{divisionObj};
}

void Division::setPersistentState(const juce::var& v)
{
    if (const auto* divisionObj = v.getDynamicObject()) {

        setMIDIChannel(divisionObj->getProperty("midi_channel"));
        setTremulantEnabled(divisionObj->getProperty("tremulant_enabled"));

        if (const auto* stops = divisionObj->getProperty("stops").getArray()) {
            for (int i = 0; i < stops->size(); ++i) {
                if (const auto* stopObj = stops->getReference(i).getDynamicObject()) {
                    const String stopName = stopObj->getProperty("name");
                    const bool enabled = stopObj->getProperty("enabled");

                    // This is not optimal but meh...
                    for (auto& stop : _rankwaves) {
                        if (stop.rankwave->getStopName() == stopName) {
                            stop.enabled = enabled;
                            break;
                        }
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
    _tremulantTargetLevel = _tremulantEnabled ? _tremulantMaxLevel : 0.0f;
}

float Division::getTremulantLevel(bool update)
{
    const auto level = _tremulantLevel;

    if (update)
        _tremulantLevel += 0.5f * (_tremulantTargetLevel - _tremulantLevel);

    return level;
}

void Division::noteOn(int note, int midiChannel)
{
    if (!isForMIDIChannel(midiChannel))
        return;

    for (auto& rw : _rankwaves) {
        if (rw.enabled && rw.rankwave->isForNote(note)) {
            auto state = rw.rankwave->trigger(note);

            if (auto* voice = _engine.getVoicePool().trigger(state))
                _activeVoices.append(voice);
        }
    }
}

void Division::noteOff(int note, int midiChannel)
{
    if (!isForMIDIChannel(midiChannel))
        return;

    auto* voice = _activeVoices.first();

    while (voice != nullptr) {
        if (voice->isForNote(note))
            voice->release();

        voice = voice->next();
    }
}

AEOLUS_NAMESPACE_END
