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

#include "aeolus/engine.h"

using namespace juce;

AEOLUS_NAMESPACE_BEGIN

EngineGlobal::EngineGlobal()
    : _rankwaves{}
{
    loadRankwaves();
}

StringArray EngineGlobal::getAllStopNames() const
{
    StringArray names;

    for (const auto* const rankwave : _rankwaves)
        names.add(rankwave->getStopName());

    return names;
}

Rankwave* EngineGlobal::getStopByName(const String& name)
{
    if (!_rankwavesByName.contains(name))
        return nullptr;

    return _rankwavesByName[name];
}

void EngineGlobal::updateStops(float sampleRate)
{
    for (auto* rw : _rankwaves)
        rw->prepareToPlay(sampleRate);
}

void EngineGlobal::loadRankwaves()
{
    auto& model = *Model::getInstance();

    for (int i = 0; i < model.getStopsCount(); ++i) {
        auto* synth = model[i];
        jassert(synth);

        auto rankwave = std::make_unique<Rankwave>(*synth);
        auto* ptr = rankwave.get();
        _rankwaves.add(rankwave.release());
        _rankwavesByName.set(ptr->getStopName(), ptr);
    }
}

JUCE_IMPLEMENT_SINGLETON(EngineGlobal)

//==============================================================================

Engine::Engine()
    : _sampleRate{SAMPLE_RATE}
    , _voicePool(*this)
    , _activeVoices{}
    , _division{"Default"}
    , _divisions{}
    , _subFrameBuffer{2, SUB_FRAME_LENGTH}
    , _voiceFrameBuffer{2, SUB_FRAME_LENGTH}
    , _remainedSamples{0}
    , _interpolator{1.0f}
    , _midiKeybaordState{}
{
    populateDivisions();
}

void Engine::prepareToPlay(float sampleRate)
{
    _interpolator.setRatio(SAMPLE_RATE / sampleRate); // 44100 / sampleRate
    _interpolator.reset();

    _sampleRate = sampleRate;

    // Make sure the stops wavetable is updated.
    auto* g = EngineGlobal::getInstance();
    g->updateStops(SAMPLE_RATE);
}

void Engine::process(float* outL, float* outR, int numFrames)
{
    jassert(outL != nullptr);
    jassert(outR != nullptr);

    processPendingNoteEvents();

    while (numFrames > 0)
    {
        if (_remainedSamples > 0)
        {
            const int idx = SUB_FRAME_LENGTH - _remainedSamples;
            const float* subL = _subFrameBuffer.getReadPointer(0, idx);
            const float* subR = _subFrameBuffer.getReadPointer(1, idx);

            const int n = jmin(_remainedSamples, numFrames);

            while (_remainedSamples > 0 && _interpolator.canWrite()) {
                _interpolator.write(*subL, *subR);
                --_remainedSamples;
                subL += 1;
                subR += 1;
            }

            while (numFrames > 0 && _interpolator.canRead()) {
                _interpolator.read(*outL, *outR);
                numFrames -= 1;
                outL += 1;
                outR += 1;
            }

            /*
            memcpy (outL, subL, sizeof(float) * n);
            memcpy (outR, subR, sizeof(float) * n);

            _remainedSamples -= n;
            numFrames -= n;

            outL += n;
            outR += n;
            */
        }

        if (_remainedSamples == 0 && numFrames > 0)
        {
            processSubFrame();
            jassert(_remainedSamples > 0);
        }

    }
}

void Engine::noteOn(int note)
{
    for (int i = 0; i < _division.getStopsCount(); ++i) {
        auto& rw = _division[i];

        if (rw.enabled && rw.rankwave->isForNote(note)) {
            auto state = rw.rankwave->trigger(note);

            if (auto* voice = _voicePool.trigger(state)) {
                _activeVoices.append(voice);
            }
        }
    }
}

void Engine::noteOff(int note)
{
    auto* voice = _activeVoices.first();

    while (voice != nullptr) {
        if (voice->isForNote(note))
            voice->release();

        voice = voice->next();
    }
}

Range<int> Engine::getMidiKeyboardRange() const
{
    int minNote;
    int maxNote;

    _division.getAvailableRange(minNote, maxNote);
    return Range<int>(minNote, maxNote);
}

var Engine::getPersistentState() const
{
    auto* obj = new DynamicObject();

    Array<var> divisions;

    {
        // Per division, but we have only one so far
        auto* divisionObj = new DynamicObject();

        Array<var> stops;

        for (int i = 0; i < _division.getStopsCount(); ++i) {
            auto stopRef = _division[i];
            auto* stopObj = new DynamicObject();
            stopObj->setProperty("name", stopRef.rankwave->getStopName());
            stopObj->setProperty("enabled", stopRef.enabled);

            stops.add(var{stopObj});
        }

        divisionObj->setProperty("stops", stops);

        divisions.add(var{divisionObj});
    }


    obj->setProperty("divisions", divisions);

    return var{obj};
}

void Engine::setPersistentState(const var& state)
{
    if (const auto* obj = state.getDynamicObject()) {
        if (const auto* divisions = obj->getProperty("divisions").getArray()) {
            jassert(divisions->size() == 1);

            {
                if (const auto* divisionObj = divisions->getReference(0).getDynamicObject()) {
                    if (const auto* stops = divisionObj->getProperty("stops").getArray()) {
                        for (int i = 0; i < stops->size(); ++i) {
                            if (const auto* stopObj = stops->getReference(i).getDynamicObject()) {
                                const String stopName = stopObj->getProperty("name");
                                const bool enabled = stopObj->getProperty("enabled");

                                // This is not optimal but meh...
                                for (int j = 0; j < _division.getStopsCount(); ++j) {
                                    auto& ref = _division[j];
                                    if (ref.rankwave->getStopName() == stopName) {
                                        ref.enabled = enabled;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void Engine::populateDivisions()
{
    auto& g = *EngineGlobal::getInstance();

    for (int i = 0; i < g.getStopsCount(); ++i) {
        auto* stop = g.getStop(i);

        // Add to all to the same division
        _division.addRankwave(stop, false);
    }
}

void Engine::postNoteEvent(bool onOff, int note)
{
    _pendingNoteEvents.send({onOff, note});
}

void Engine::processSubFrame()
{
    _subFrameBuffer.clear();

    auto* voice = _activeVoices.first();

    while (voice != nullptr) {
        _voiceFrameBuffer.clear();
        float* outL = _voiceFrameBuffer.getWritePointer(0);
        float* outR = _voiceFrameBuffer.getWritePointer(1);

        voice->process(outL, outR);

        _subFrameBuffer.addFrom(0, 0, _voiceFrameBuffer, 0, 0, SUB_FRAME_LENGTH);
        _subFrameBuffer.addFrom(1, 0, _voiceFrameBuffer, 1, 0, SUB_FRAME_LENGTH);

        if (voice->isOver()) {
            auto* nextVoice = _activeVoices.removeAndReturnNext(voice);
            _voicePool.resetAndReturnToPool(voice);
            voice = nextVoice;
        } else {
            voice = voice->next();
        }
    }

    _remainedSamples = SUB_FRAME_LENGTH;
}

void Engine::processPendingNoteEvents()
{
    NoteEvent event;
    while (_pendingNoteEvents.receive(event)) {
        if (event.on)
            noteOn(event.note);
        else
            noteOff(event.note);
    }
}

AEOLUS_NAMESPACE_END
