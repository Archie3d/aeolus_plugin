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
#include "aeolus/audioparam.h"
#include "aeolus/levelmeter.h"
#include "aeolus/dsp/convolver.h"
#include "aeolus/dsp/interpolator.h"

#include <vector>

AEOLUS_NAMESPACE_BEGIN

/**
 * @brief A global shared instance of the organ engine.
 * 
 * This class in a singleton which is shared among all the plugin instances.
 */
class EngineGlobal : public juce::DeletedAtShutdown
{
public:

    /**
     * Impulse response descriptor for IRs embedded as binary resources.
     */
    struct IR
    {
        juce::String name;
        const char* data;
        size_t size;

        float gain;
        bool zeroDelay;

        juce::AudioBuffer<float> waveform;
    };

    int getStopsCount() const noexcept { return _rankwaves.size(); }
    Rankwave* getStop(int i) { return _rankwaves[i]; }

    juce::StringArray getAllStopNames() const;
    Rankwave* getStopByName(const juce::String& name);

    const std::vector<IR>& getIRs() const noexcept { return _irs; }
    int getLongestIRLength() const noexcept { return _longestIRLength; }

    void updateStops(float sampleRate);

    JUCE_DECLARE_SINGLETON (EngineGlobal, false)

private:
    EngineGlobal();
    ~EngineGlobal() override  { clearSingletonInstance(); }

    void loadRankwaves();
    void loadIRs();

    juce::OwnedArray<Rankwave> _rankwaves;
    juce::HashMap<juce::String, Rankwave*> _rankwavesByName;

    std::vector<IR> _irs;
    int _longestIRLength;   ///< Longest IR length in samples
};

//==============================================================================

/**
 * @brief Organ engine.
 * This class defines the top-level organ engine that performs MIDI events processing
 * and audio generation.
 */
class Engine
{
public:

    //--------------------------------------------------------------------------

    struct NoteEvent
    {
        bool on;
        int note;
        int midiChannel;
    };

    struct IRSwithEvent
    {
        int num;
    };

    struct Level
    {
        LevelMeter left;
        LevelMeter right;
    };

    enum {
        VOLUME = 0,

        NUM_PARAMS
    };

    //--------------------------------------------------------------------------

    Engine();

    /**
     * This method returns external processing sample rate as mandated
     * by the plugin host. Internally the organ engine performs processing
     * with a fixed SAMPLE_RATE.
     */
    float getSampleRate() const noexcept { return _sampleRate; }

    /**
     * Returns the number of active (playing) voices.
     */
    int getVoiceCount() const noexcept { return _voicePool.getNumberOfActiveVoices(); }

    /**
     * Called by the host pefore starting requesting the audio blocks.
     */
    void prepareToPlay(float sampleRate, int frameSize);

    /**
     * Set the reverb IR bu its number.
     * @note This can be called upon initialisation or on the audio thread.
     */
    void setReverbIR(int num);

    /**
     * Set the reverb IR by its number asynchronously.
     * This to be called on the main (UI) thread.
     */
    void postReverbIR(int num);

    /**
     * Returns currently set reverb IR number.
     */
    int getReverbIR() const noexcept { return _selectedIR; }

    /**
     * Set reverb wet output level (linear).
     * @note This must be called on the audio thread.
     */
    void setReverbWet(float v);

    /**
     * Set global output volume level (linear).
     * @note This must be called on the audio thread.
     */
    void setVolume(float v);

    /**
     * Returns volume levels.
     */
    Level& getVolumeLevel() noexcept { return _volumeLevel; }

    /**
     * Generate audio.
     */
    void process(float* outL, float* outR, int numFrames, bool isNonRealtime = false);

    /**
     * Handle note-on events.
     */
    void noteOn(int note, int midiChannel);

    /**
     * Handle note-off events.
     */
    void noteOff(int note, int midiChannel);

    /**
     * Release all the active voices immediately.
     */
    void allNotesOff();

    juce::MidiKeyboardState& getMidiKeyboardState() noexcept { return _midiKeyboardState; }

    juce::Range<int> getMidiKeyboardRange() const;

    VoicePool& getVoicePool() noexcept { return _voicePool; }

    int getDivisionCount() const noexcept { return _divisions.size(); }
    Division* getDivisionByIndex(int i) { return _divisions[i]; }
    Division* getDivisionByName(const juce::String& name);

    juce::var getPersistentState() const;
    void setPersistentState(const juce::var& state);

    void postNoteEvent(bool onOff, int note, int midiChannel);

private:

    void populateDivisions();
    void loadDivisionsFromConfig(juce::InputStream& stream);

    void clearDivisionsTriggerFlag();

    bool processSubFrame();

    void processPendingNoteEvents();
    void processPendingIRSwitchEvents();

    void generateTremulant();
    void modulateDivision(Division* division);
    void applyVolume(float* outL, float* outR, int numFrames);

    float _sampleRate;

    RingBuffer<NoteEvent, 1024> _pendingNoteEvents;

    VoicePool _voicePool;           ///< All the voices.

    AudioParameterPool _params;     ///< Internal parameters.

    /// List of all divisions
    juce::OwnedArray<Division> _divisions;

    juce::AudioBuffer<float> _subFrameBuffer;
    juce::AudioBuffer<float> _divisionFrameBuffer;
    juce::AudioBuffer<float> _voiceFrameBuffer;

    int _remainedSamples;

    juce::AudioBuffer<float> _tremulantBuffer;
    float _tremulantPhase;

    dsp::Convolver _convolver;
    std::atomic<int> _selectedIR;
    RingBuffer<IRSwithEvent, 1024> _irSwitchEvents;
    int _reverbTailCounter;

    dsp::Interpolator _interpolator;

    juce::MidiKeyboardState _midiKeyboardState;

    Level _volumeLevel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};

AEOLUS_NAMESPACE_END
