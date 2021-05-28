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
#include "aeolus/ringbuffer.h"
#include "aeolus/voice.h"
#include "aeolus/addsynth.h"
#include "aeolus/rankwave.h"
#include "aeolus/division.h"
#include "aeolus/dsp/interpolator.h"

AEOLUS_NAMESPACE_BEGIN

/**
 * @brief A global shared instance of the organ engine.
 * 
 * This class in a singleton which is shared among all the plugin instances.
 */
class EngineGlobal : public juce::DeletedAtShutdown
{
public:

    int getStopsCount() const noexcept { return _rankwaves.size(); }
    Rankwave* getStop(int i) { return _rankwaves[i]; }

    juce::StringArray getAllStopNames() const;
    Rankwave* getStopByName(const juce::String& name);

    void updateStops(float sampleRate);

    JUCE_DECLARE_SINGLETON (EngineGlobal, false)

private:
    EngineGlobal();
    ~EngineGlobal() override  { clearSingletonInstance(); }

    void loadRankwaves();

    juce::OwnedArray<Rankwave> _rankwaves;
    juce::HashMap<juce::String, Rankwave*> _rankwavesByName;
};

//==============================================================================

class Engine
{
public:

    struct NoteEvent
    {
        bool on;
        int note;
    };

    Engine();

    float getSampleRate() const noexcept { return _sampleRate; }
    int getVoiceCount() const noexcept { return _voicePool.getNumberOfActiveVoices(); }

    void prepareToPlay(float sampleRate);

    void process(float* outL, float* outR, int numFrames);

    void noteOn(int note);
    void noteOff(int note);

    juce::MidiKeyboardState& getMidiKeyboardState() noexcept { return _midiKeybaordState; }

    juce::Range<int> getMidiKeyboardRange() const;

    VoicePool& getVoicePool() noexcept { return _voicePool; }

    int getDivisionCount() const noexcept { return _divisions.size(); }
    Division* getDivisionByIndex(int i) { return _divisions[i]; }

    juce::var getPersistentState() const;
    void setPersistentState(const juce::var& state);

    void postNoteEvent(bool onOff, int note);

private:

    void populateDivisions();

    void processSubFrame();

    void processPendingNoteEvents();

    void generateTremulant();
    void modulateDivision(Division* division);

    float _sampleRate;

    RingBuffer<NoteEvent, 1024> _pendingNoteEvents;

    VoicePool _voicePool;       ///< Voices.

    /// List of all divisions
    juce::OwnedArray<Division> _divisions;

    juce::AudioBuffer<float> _subFrameBuffer;
    juce::AudioBuffer<float> _divisionFrameBuffer;
    juce::AudioBuffer<float> _voiceFrameBuffer;

    int _remainedSamples;

    juce::AudioBuffer<float> _tremulantBuffer;
    float _tremulantPhase;

    dsp::Interpolator _interpolator;

    juce::MidiKeyboardState _midiKeybaordState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};

AEOLUS_NAMESPACE_END
