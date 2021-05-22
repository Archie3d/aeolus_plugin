
#pragma once

#include "aeolus/globals.h"
#include "aeolus/ringbuffer.h"
#include "aeolus/voice.h"
#include "aeolus/addsynth.h"
#include "aeolus/rankwave.h"
#include "aeolus/division.h"
#include "aeolus/dsp/interpolator.h"

AEOLUS_NAMESPACE_BEGIN

class EngineGlobal : public juce::DeletedAtShutdown
{
public:

    int getStopsCount() const noexcept { return _rankwaves.size(); }
    Rankwave* getStop(int i) { return _rankwaves[i]; }

    void updateStops(float sampleRate);

    JUCE_DECLARE_SINGLETON (EngineGlobal, false)

private:
    EngineGlobal();
    ~EngineGlobal() override  { clearSingletonInstance(); }

    void loadRankwaves();

    juce::OwnedArray<Rankwave> _rankwaves;
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

    // For now we have only one division with all the stops.
    Division& getDivision() { return _division; }

    juce::var getPersistentState() const;
    void setPersistentState(const juce::var& state);

    void postNoteEvent(bool onOff, int note);

private:

    void populateDivisions();

    void processSubFrame();

    void processPendingNoteEvents();

    float _sampleRate;

    RingBuffer<NoteEvent, 1024> _pendingNoteEvents;

    VoicePool _voicePool;       ///< Voices.
    List<Voice> _activeVoices;  ///< Active voices.

    // For now we have only one division
    Division _division;

    juce::AudioBuffer<float> _subFrameBuffer;
    juce::AudioBuffer<float> _voiceFrameBuffer;

    int _remainedSamples;

    dsp::Interpolator _interpolator;

    juce::MidiKeyboardState _midiKeybaordState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};

AEOLUS_NAMESPACE_END
