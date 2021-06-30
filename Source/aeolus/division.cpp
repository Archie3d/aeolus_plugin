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
    , _mnemonic{name}
    , _linkedDivisionNames{}
    , _linkedDivisions{}
    , _hasSwell{false}
    , _hasTremulant{false}
    , _midiChannel{0}
    , _tremulantEnabled{false}
    , _tremulantLevel{0.0f}
    , _tremulantMaxLevel{1.0f}
    , _tremulantTargetLevel{0.0f}
    , _paramGain{nullptr}
    , _params{Division::NUM_PARAMS}
    , _swellFilterSpec{}
    , _swellFilterStateL{}
    , _swellFilterStateR{}
    , _rankwaves{}
    , _activeVoices{}
    , _triggerFlag{}
    , _volumeLevel{}
{
    _swellFilterSpec.type = dsp::BiquadFilter::LowPass;
    _swellFilterSpec.sampleRate = SAMPLE_RATE_F;
    _swellFilterSpec.dbGain = 0.0f;
    _swellFilterSpec.q = 0.7071f;
    _swellFilterSpec.freq = 0.4f * SAMPLE_RATE_F;

    dsp::BiquadFilter::updateSpec(_swellFilterSpec);
    dsp::BiquadFilter::resetState(_swellFilterSpec, _swellFilterStateL);
    dsp::BiquadFilter::resetState(_swellFilterSpec, _swellFilterStateR);
}

void Division::initFromVar(const var& v)
{
    if (const auto* obj = v.getDynamicObject()) {
        _name = obj->getProperty("name");
        _mnemonic = obj->getProperty("mnemonic");

        if (const auto* link = obj->getProperty("link").getArray()) {
            for (const auto& item : *link)
                _linkedDivisionNames.add(item.toString());
        }

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
                    const String stopType = stopObj->getProperty("type");
                    const float gain = stopObj->hasProperty("gain") ? (float)stopObj->getProperty("gain") : 1.0f;
                    const float chiffGain = stopObj->getProperty("chiff");

                    const auto pipeObj = stopObj->getProperty("pipe");

                    if (const auto* pipes = pipeObj.getArray()) {
                        // Multiple pipes on single stop
                        Rankwave* rankwaves[MAX_RANK] = {nullptr};
                        int r = 0;

                        for (int j = 0; j < pipes->size() && r < MAX_RANK; ++j) {
                            const String pipeName = pipes->getUnchecked(j);

                            if (auto* rankwavePtr = g->getStopByName(pipeName)) {
                                rankwaves[r++] = rankwavePtr;
                            } else {
                                DBG("Stop pipe " + pipeName + " cannot be found.");
                            }
                        }

                        if (r > 0) {
                            auto& s = addRankwaves(rankwaves, r, false, stopName);
                            s.type = Division::stopTypeFromString(stopType);
                            s.gain = gain;
                            s.chiffGain = chiffGain;
                        }

                    } else {
                        // Assume single pipe per stop
                        const String pipeName = stopObj->getProperty("pipe");

                        if (auto* rankwavePtr = g->getStopByName(pipeName)) {
                            auto&s = addRankwave(rankwavePtr, false, stopName);
                            s.type = Division::stopTypeFromString(stopType);
                            s.gain = gain;
                            s.chiffGain = chiffGain;
                        } else {
                            DBG("Stop pipe " + pipeName + " cannot be found.");
                        }
                    }
                }
            }
        }
    }
}

var Division::getPersistentState() const
{
    auto* divisionObj = new DynamicObject();

    divisionObj->setProperty("midi_channel", getMIDIChannel());
    divisionObj->setProperty("tremulant_enabled", isTremulantEnabled());

    {
        Array<var> stops;

        for (const auto& stop : _rankwaves) {
            auto* stopObj = new DynamicObject();
            stopObj->setProperty("name", stop.name);
            stopObj->setProperty("enabled", stop.enabled);

            stops.add(var{stopObj});
        }

        divisionObj->setProperty("stops", stops);
    }

    {
        Array<var> links;

        for (const auto& link : _linkedDivisions) {
            auto* linkObj = new DynamicObject();
            linkObj->setProperty("division", link.division->getName());
            linkObj->setProperty("enabled", link.enabled);

            links.add(var{linkObj});
        }

        divisionObj->setProperty("links", links);
    }

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

                    for (auto& stop : _rankwaves) {
                        if (stop.name == stopName) {
                            stop.enabled = enabled;
                            break;
                        }
                    }
                }
            }
        }

        if (const auto* links = divisionObj->getProperty("links").getArray()) {
            for (int i = 0; i < links->size(); ++i) {
                if (const auto* linkObj = links->getReference(i).getDynamicObject()) {
                    const String divisionName = linkObj->getProperty("division");
                    const bool enabled = linkObj->getProperty("enabled");

                    for (auto& link : _linkedDivisions) {
                        if (link.division->getName() == divisionName)
                            link.enabled = enabled;
                    }
                }
            }
        }
    }
}

void Division::populateLinkedDivisions()
{
    _linkedDivisions.clear();

    for (const auto& name : _linkedDivisionNames) {
        if (auto* division = _engine.getDivisionByName(name)) {
            Link link{division, false};
            _linkedDivisions.push_back(link);
        }
    }
}

int Division::getLinksCount() const noexcept
{
    return (int)_linkedDivisions.size();
}

void Division::enableLink(int i, bool ena)
{
    jassert(isPositiveAndBelow(i, _linkedDivisions.size()));
    _linkedDivisions[i].enabled = ena;
}

bool Division::isLinkEnabled(int i)
{
    jassert(isPositiveAndBelow(i, _linkedDivisions.size()));
    return _linkedDivisions[i].enabled;
}

Division::Link& Division::getLinkByIndex(int i)
{
    jassert(isPositiveAndBelow(i, _linkedDivisions.size()));
    return _linkedDivisions[i];
}

void Division::cancelAllLinks()
{
    for (auto& link : _linkedDivisions)
        link.enabled = false;
}

void Division::clear()
{
    _rankwaves.clear();
}

Division::Stop& Division::addRankwave(Rankwave* ptr, bool ena, const String& name)
{
    jassert(ptr != nullptr);

    Stop ref{};
    ref.rankwave[0] = ptr;
    ref.enabled = ena;
    ref.name = name;

    if (name.isEmpty())
        ref.name = ptr->getStopName();

    _rankwaves.push_back(ref);
    return _rankwaves.back();
}

Division::Stop& Division::addRankwaves(Rankwave** ptr, int size, bool ena, const String& name)
{
    jassert(ptr != nullptr);
    size = jmin(size, MAX_RANK);

    Stop ref{};
    ref.enabled = ena;
    ref.name = name;

    for (int i = 0; i < size; ++i) {
        Rankwave* rwPtr = ptr[i];
        jassert(rwPtr != nullptr);

        ref.rankwave[i] = rwPtr;
    }

    if (name.isEmpty())
        ref.name = ptr[0]->getStopName();

    _rankwaves.push_back(ref);
    return _rankwaves.back();
}

int Division::getStopsCount() const noexcept
{
    return (int)_rankwaves.size();
}

void Division::enableStop(int i, bool ena)
{
    jassert(isPositiveAndBelow(i, _rankwaves.size()));

    _rankwaves[i].enabled = ena;
}

bool Division::isStopEnabled(int i) const
{
    jassert(isPositiveAndBelow(i, _rankwaves.size()));
    return _rankwaves[i].enabled;
}

Division::Stop& Division::getStopByIndex(int i)
{
    jassert(isPositiveAndBelow(i, _rankwaves.size()));
    return _rankwaves[i];
}

void Division::disableAllStops()
{
    for (int i = 0; i < getStopsCount(); ++i)
        enableStop(i, false);
}

void Division::getAvailableRange(int& minNote, int& maxNote) const noexcept
{
    minNote = -1;
    maxNote = -1;

    for (const auto& ref : _rankwaves) {
        if (ref.enabled) {
            for (int i = 0; i < MAX_RANK; ++i) {
                const auto* ptr = ref.rankwave[i];

                if (ptr != nullptr) {
                    if (minNote < 0 || minNote > ptr->getNoteMin())
                        minNote = ptr->getNoteMin();
                    if (maxNote < 0 || maxNote < ptr->getNoteMax())
                        maxNote = ptr->getNoteMax();
                }
            }
        }
    }
}

bool Division::isForMIDIChannel(int channel) const noexcept
{
    return _midiChannel == 0        // accept any channel
        || channel == 0             // broadcast message
        || _midiChannel == channel;
}

void Division::setTremulantEnabled(bool ena) noexcept
{
    if (!_hasTremulant)
        return;

    _tremulantEnabled = ena;
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
    if (hasBeenTriggered())
        return;

    if (!isForMIDIChannel(midiChannel))
        return;

    _triggerFlag = true;

    for (auto& rw : _rankwaves) {
        for (int i = 0; i < MAX_RANK; ++i) {
            if (auto* ptr = rw.rankwave[i]) {
                if (rw.enabled && ptr->isForNote(note)) {
                    auto state = ptr->trigger(note);
                    state.gain = rw.gain;
                    state.chiffGain = rw.chiffGain;

                    if (auto* voice = _engine.getVoicePool().trigger(state))
                        _activeVoices.append(voice);
                }
            }
        }
    }

    // Forward to the linked divisions
    for (auto& link : _linkedDivisions) {
        if (link.enabled) {
            link.division->noteOn(note, 0);
        }
    }
}

void Division::noteOff(int note, int midiChannel)
{
    if (hasBeenTriggered())
        return;

    if (!isForMIDIChannel(midiChannel))
        return;

    _triggerFlag = true;

    auto* voice = _activeVoices.first();

    while (voice != nullptr) {
        if (voice->isForNote(note))
            voice->release();

        voice = voice->next();
    }

    // Forward to the linked divisions
    for (auto& link : _linkedDivisions) {
        if (link.enabled) {
            link.division->noteOff(note, 0);
        }
    }
}

void Division::allNotesOff()
{
    auto* voice = _activeVoices.first();

    while (voice != nullptr) {
        voice->release();
        voice = voice->next();
    }
}

bool Division::process(AudioBuffer<float>& targetBuffer, AudioBuffer<float>& voiceBuffer)
{
    jassert(targetBuffer.getNumSamples() == SUB_FRAME_LENGTH);
    jassert(voiceBuffer.getNumSamples() == SUB_FRAME_LENGTH);

    auto* voice = _activeVoices.first();

    if (voice == nullptr)
        return false;

    while (voice != nullptr) {
        voiceBuffer.clear();
        float* outL = voiceBuffer.getWritePointer(0);
        float* outR = voiceBuffer.getWritePointer(1);

        voice->process(outL, outR);

        targetBuffer.addFrom(0, 0, voiceBuffer, 0, 0, SUB_FRAME_LENGTH);
        targetBuffer.addFrom(1, 0, voiceBuffer, 1, 0, SUB_FRAME_LENGTH);

        if (voice->isOver()) {
            auto* nextVoice = _activeVoices.removeAndReturnNext(voice);
            voice->resetAndReturnToPool();
            voice = nextVoice;
        } else {
            voice = voice->next();
        }
    }

    return true;
}

void Division::modulate(juce::AudioBuffer<float>& targetBuffer, const juce::AudioBuffer<float>& tremulantBuffer)
{
    jassert(targetBuffer.getNumSamples() == SUB_FRAME_LENGTH);
    jassert(tremulantBuffer.getNumSamples() == SUB_FRAME_LENGTH);

    float* outL = targetBuffer.getWritePointer(0);
    float* outR = targetBuffer.getWritePointer(1);

    jassert(outL != nullptr);
    jassert(outR != nullptr);

    const float* gain = tremulantBuffer.getReadPointer(0);
    jassert(gain != nullptr);

    const float lvl = getTremulantLevel(true);

    // Update gain smoothly
    auto& paramGain = _params[Division::GAIN];
    paramGain.setValue(_paramGain->get());

    for (int i = 0; i < SUB_FRAME_LENGTH; ++i) {
        float g = (1.0f + gain[i] * lvl) * paramGain.nextValue();
        outL[i] *= g;
        outR[i] *= g;
    }

    // Apply swell filter
    if (hasSwell()) {
        // Close the filter along with the gain
        const float k = powf(jlimit(0.0f, 1.0f, paramGain.target()), 1.3f);
        _swellFilterSpec.freq = jmap(k, 400.0f, 18000.0f);
        dsp::BiquadFilter::updateSpec(_swellFilterSpec);
        dsp::BiquadFilter::process(_swellFilterSpec, _swellFilterStateL, outL, outL, SUB_FRAME_LENGTH);
        dsp::BiquadFilter::process(_swellFilterSpec, _swellFilterStateR, outR, outR, SUB_FRAME_LENGTH);
    }
}

Division::Stop::Type Division::stopTypeFromString(const String& n)
{
    const static std::map<String, Stop::Type> nameToType {
        { "principal", Stop::Principal },
        { "flute",     Stop::Flute },
        { "reed",      Stop::Reed },
        { "string",    Stop::String }
    };

    auto type = Stop::Unknown;

    const auto it = nameToType.find(n.toLowerCase());

    if (it != nameToType.end())
        type = it->second;

    return type;
}

AEOLUS_NAMESPACE_END
