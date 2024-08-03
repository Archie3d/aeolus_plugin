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

#include "globals.h"
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
    , _midiChannelsMask{ (1 << 16) - 1 } // Select all MIDI channels by default
    , _tremulantEnabled{false}
    , _tremulantLevel{0.0f}
    , _tremulantMaxLevel{TREMULANT_TARGET_LEVEL}
    , _tremulantTargetLevel{0.0f}
    , _paramGain{nullptr}
    , _params{Division::NUM_PARAMS}
    , _swellFilterSpec{}
    , _swellFilterStateL{}
    , _swellFilterStateR{}
    , _tremulantDelayL(TREMULANT_DELAY_LENGTH)
    , _tremulantDelayR(TREMULANT_DELAY_LENGTH)
    , _stops{}
    , _activeVoices{}
    , _keysState{}
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

        if (const auto* arr = obj->getProperty("stops").getArray()) {
            for (int i = 0; i < arr->size(); ++i) {
                Stop stop{};
                stop.initFromVar(arr->getUnchecked(i));

                if (!stop.getZones().empty())
                    _stops.push_back(stop);
            }
        }
    }
}

var Division::getPersistentState() const
{
    auto* divisionObj = new DynamicObject();

    divisionObj->setProperty("midi_channels_mask", getMIDIChannelsMask());
    divisionObj->setProperty("tremulant_enabled", isTremulantEnabled());

    {
        Array<var> stops;

        for (const auto& stop : _stops) {
            auto* stopObj = new DynamicObject();
            stopObj->setProperty("name", stop.getName());
            stopObj->setProperty("enabled", stop.isEnabled());

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

        if (const auto& v = divisionObj->getProperty("midi_channel"); !v.isVoid()) {
            // Handle legacy setting with only one MIDI channel allowed
            const int channel{ (int)v };

            if (channel == 0)
                setMIDIChannelsMask((1 << 16) - 1); // Select all MIDI channels
            else
                setMIDIChannelsMask(1 << (channel - 1));
        } else {
            setMIDIChannelsMask(divisionObj->getProperty("midi_channels_mask"));
        }

        setTremulantEnabled(divisionObj->getProperty("tremulant_enabled"));

        if (const auto* stops = divisionObj->getProperty("stops").getArray()) {
            for (int i = 0; i < stops->size(); ++i) {
                if (const auto* stopObj = stops->getReference(i).getDynamicObject()) {
                    const String stopName = stopObj->getProperty("name");
                    const bool enabled = stopObj->getProperty("enabled");

                    for (auto& stop : _stops) {
                        if (stop.getName()== stopName) {
                            stop.setEnabled(enabled);
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

void Division::clearLinkedDivisions()
{
    _linkedDivisions.clear();
    _linkedFromDivisions.clear();
}

void Division::populateLinkedDivisions()
{
    for (const auto& name : _linkedDivisionNames) {
        if (auto* division = _engine.getDivisionByName(name)) {
            Link link{ division, false };
            _linkedDivisions.push_back(link);
            division->_linkedFromDivisions.push_back(this);
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

    if (_linkedDivisions[i].enabled != ena) {
        _linkedDivisions[i].enabled = ena;
        _engine.getSequencer()->setCurrentStepDirty();
    }
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
    bool changed{ false };

    for (auto& link : _linkedDivisions) {
        if (link.enabled) {
            changed = true;
            link.enabled = false;
        }
    }

    if (changed)
        _engine.getSequencer()->setCurrentStepDirty();
}

void Division::clear()
{
    _stops.clear();
}

Stop& Division::addRankwave(Rankwave* ptr, bool ena, const String& name)
{
    jassert(ptr != nullptr);

    Stop ref{};
    ref.addZone(ptr);
    ref.setEnabled(ena);
    ref.setName(name.isEmpty() ? ptr->getStopName() : name);

    _stops.push_back(ref);
    return _stops.back();
}

Stop& Division::addRankwaves(const std::vector<Rankwave*> rw, bool ena, const String& name)
{
    jassert(!rw.empty());
    Stop ref{};
    ref.addZone(rw);
    ref.setEnabled(ena);
    ref.setName(name.isEmpty() ? rw[0]->getStopName() : name);

    _stops.push_back(ref);
    return _stops.back();
}

int Division::getStopsCount() const noexcept
{
    return (int)_stops.size();
}

void Division::enableStop(int i, bool ena)
{
    jassert(isPositiveAndBelow(i, _stops.size()));

    if (_stops[i].isEnabled() != ena) {
        _stops[i].setEnabled(ena);

        _engine.getSequencer()->setCurrentStepDirty();
    }
}

bool Division::isStopEnabled(int i) const
{
    jassert(isPositiveAndBelow(i, _stops.size()));
    return _stops[i].isEnabled();
}

Stop& Division::getStopByIndex(int i)
{
    jassert(isPositiveAndBelow(i, _stops.size()));
    return _stops[i];
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

    for (const auto& stop : _stops) {
        if (stop.isEnabled()) {
            const auto range{stop.getKeyRange()};

            if (minNote < 0 || minNote > range.getStart())
                minNote = range.getStart();
            if (maxNote < 0 || maxNote < range.getEnd() - 1)
                maxNote = range.getEnd() - 1;
        }
    }
}

bool Division::isForMIDIChannel(int channel) const noexcept
{
    const int mask{ _midiChannelsMask.load() };
    return midi::matchChannelToMask(mask, channel);
}

void Division::setTremulantEnabled(bool ena) noexcept
{
    if (!_hasTremulant)
        return;

    if (_tremulantEnabled != ena) {
        _tremulantEnabled = ena;
        _tremulantTargetLevel = _tremulantEnabled ? _tremulantMaxLevel : 0.0f;

        _engine.getSequencer()->setCurrentStepDirty();
    }
}

float Division::getTremulantLevel(bool update)
{
    const auto level = _tremulantLevel;

    if (update)
        _tremulantLevel += 0.1f * (_tremulantTargetLevel - _tremulantLevel);

    return level;
}

void Division::noteOn(int note, int midiChannel)
{
    if (hasBeenTriggered())
        return;

    if (!isForMIDIChannel(midiChannel))
        return;

    _triggerFlag = true;

    for (int stopIndex = 0; stopIndex < (int)_stops.size(); ++stopIndex)
        triggerVoicesForStop(stopIndex, note);

    if (midiChannel != 0) {
        // Update keys state only when triggered by the assigned MIDI channel
        _keysState.set(note);
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

    if (midiChannel != 0) {
        // Update keys state only when triggered by the assigned MIDI channel.
        _keysState.reset(note);
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
    _keysState.reset();

    auto* voice = _activeVoices.first();

    while (voice != nullptr) {
        voice->release();
        voice = voice->next();
    }
}

void Division::handleControlMessage(const juce::MidiMessage& msg)
{
    const int cc{ msg.getControllerNumber() };

    if (cc != aeolus::CC_MODULATION && cc != aeolus::CC_VOLUME)
        return;

    const int swellCh{ _engine.getMIDISwellChannelsMask() };
    const float value{ float(msg.getControllerValue()) / 127.0f };

    if (msg.getChannel() == 0 || (swellCh & (msg.getChannel() - 1)) != 0) {
        if (_hasSwell && cc == aeolus::CC_VOLUME) {
            *_paramGain = value;
        }
    }

    // Hange manual channel specific controls
    if (!isForMIDIChannel(msg.getChannel()))
        return;

    if (cc == aeolus::CC_MODULATION && hasTremulant())
        setTremulantEnabled(value > 0.5f);
}

bool Division::process(AudioBuffer<float>& targetBuffer, AudioBuffer<float>& voiceBuffer)
{
    jassert(targetBuffer.getNumSamples() == SUB_FRAME_LENGTH);
    jassert(voiceBuffer.getNumSamples() == SUB_FRAME_LENGTH);

    updateAggregatedKeysState();
    releaseVoicesOfDisabledStops();
    triggerVoicesOfEnabledStops();

    auto* voice = _activeVoices.first();

    if (voice == nullptr)
        return false;

    while (voice != nullptr) {
        voiceBuffer.clear();
        float* outL = voiceBuffer.getWritePointer(0);
        float* outR = voiceBuffer.getNumChannels() > 1 ? voiceBuffer.getWritePointer(1) : outL;

        voice->process(outL, outR);

#if AEOLUS_MULTIBUS_OUTPUT
        // Mix voice to the corresponding output channel depending on the pan-position
        int ch = jlimit(0, targetBuffer.getNumChannels() - 1, int(voice->getPanPosition() * targetBuffer.getNumChannels()));
        targetBuffer.addFrom(ch, 0, voiceBuffer, 0, 0, SUB_FRAME_LENGTH);
#else
        targetBuffer.addFrom(0, 0, voiceBuffer, 0, 0, SUB_FRAME_LENGTH);
        targetBuffer.addFrom(1, 0, voiceBuffer, 1, 0, SUB_FRAME_LENGTH);
#endif
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

    const float* gain = tremulantBuffer.getReadPointer(0);
    jassert(gain != nullptr);

    const float lvl = getTremulantLevel(true);

    // Update gain smoothly
    auto& paramGain = _params[Division::GAIN];
    paramGain.setValue(_paramGain->get());

#if AEOLUS_MULTIBUS_OUTPUT

    if (paramGain.isSmoothing()) {

        for (int i = 0; i < SUB_FRAME_LENGTH; ++i) {
            const float g = (1.0f + gain[i] * lvl) * paramGain.nextValue();

            for (int ch = 0; ch < targetBuffer.getNumChannels(); ++ch) {
                targetBuffer.getWritePointer(ch)[i] *= g;
            }
        }

    } else {
        // Gain is stable
        const float pgain = paramGain.target();

        for (int ch = 0; ch < targetBuffer.getNumChannels(); ++ch) {
            float* const out = targetBuffer.getWritePointer(ch);

            for (int i = 0; i < SUB_FRAME_LENGTH; ++i) {
                float g = (1.0f + gain[i] * lvl) * pgain;
                out[i] *= g;
            }
        }
    }

    // No swell filtering for multibus

#else

    float* outL = targetBuffer.getWritePointer(0);
    float* outR = targetBuffer.getWritePointer(1);

    jassert(outL != nullptr);
    jassert(outR != nullptr);

    for (int i = 0; i < SUB_FRAME_LENGTH; ++i) {
        _tremulantDelayL.write(outL[i]);
        _tremulantDelayR.write(outR[i]);

        const float g = (1.0f + gain[i] * lvl) * paramGain.nextValue();

        constexpr float freqModCenter = TREMULANT_DELAY_LENGTH * 0.5f;
        constexpr float freqModAmp = TREMULANT_DELAY_LENGTH * 0.5f * TREMULANT_DELAY_MODULATION_LEVEL;

        const float p = freqModCenter + freqModAmp * (0.5f - gain[i] * lvl);
        outL[i] = _tremulantDelayL.read(p) * g;
        outR[i] = _tremulantDelayR.read(p) * g;
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
#endif
}

void Division::releaseVoicesOfDisabledStops()
{
    auto* voice = _activeVoices.first();

    while (voice != nullptr) {
        bool shouldRelease{ false };

        if (voice->isActive()) {
            const int stopIndex = voice->stopIndex();

            if (isPositiveAndBelow(stopIndex, _stops.size())) {
                const auto& stop = _stops[stopIndex];

                if (!stop.isEnabled())
                    shouldRelease = true;

                if (voice->getNote() >= 0 && !_aggregatedKeysState[voice->getNote()])
                    shouldRelease = true;
            }
        }

        if (shouldRelease)
            voice->release();

        voice = voice->next();
    }
}

void Division::triggerVoicesOfEnabledStops()
{
    if (_aggregatedKeysState.none()) {
        // No keys are pressed
        return;
    }

    std::bitset<TOTAL_NOTES> missingNotes{ _aggregatedKeysState };

    for (int stopIndex = 0; stopIndex < _stops.size(); ++stopIndex) {
        auto& stop = _stops[stopIndex];

        if (!stop.isEnabled())
            continue;

        bool hasVoices = false;

        auto* voice = _activeVoices.first();

        while (voice != nullptr) {
            const int voiceNote{ voice->getNote() };

            if (voice->stopIndex() == stopIndex && voiceNote >= 0 && _aggregatedKeysState[voiceNote]) {
                hasVoices = true;
                missingNotes[voiceNote] = 0;
                break;
            }

            voice = voice->next();
        }

        // Trigger voices for enabled stops
        if (!hasVoices) {
            for (int note = 0; note < missingNotes.size(); ++note) {
                if (missingNotes[note])
                    triggerVoicesForStop(stopIndex, note);
            }
        }
    }
}

void Division::updateAggregatedKeysState()
{
    _aggregatedKeysState = _keysState;

    for (const auto* division : _linkedFromDivisions) {
        for (const auto& link : division->_linkedDivisions) {
            if (link.division == this && link.enabled) {
                _aggregatedKeysState |= division->_aggregatedKeysState;
                break;
            }
        }
    }
}

bool Division::triggerVoicesForStop(int stopIndex, int note)
{
    jassert(isPositiveAndBelow(stopIndex, _stops.size()));

    if (isAlreadyVoiced(stopIndex, note))
        return true;

    const auto& stop = _stops[stopIndex];

    if (!stop.isEnabled())
        return false;

    bool voiceTriggered = false;

    for (const auto& zone : stop.getZones()) {
        if (zone.isForKey(note)) {
            for (auto* rw : zone.rankwaves) {
                auto state = rw->trigger(note);

                // Rankwave may be in the middle of construction, in this case
                // we don't trigger a voice.
                if (state.isTriggered()) {
                    state.gain = stop.getGain();
                    state.chiffGain = stop.getChiffGain();

                    if (auto* voice = _engine.getVoicePool().trigger(state)) {
                        voice->setStopIndex(stopIndex);
                        _activeVoices.append(voice);
                        voiceTriggered = true;
                    }
                }
            }
        }
    }

    return voiceTriggered;
}

bool Division::isAlreadyVoiced(int stopIndex, int note)
{
    auto* voice = _activeVoices.first();

    while (voice != nullptr) {
        if (voice->isActive() && voice->stopIndex() == stopIndex && voice->isForNote(note)) {
            return true;
        }

        voice = voice->next();
    }

    return false;
}

AEOLUS_NAMESPACE_END
