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
#include "aeolus/scale.h"
#include "aeolus/voice.h"
#include "aeolus/addsynth.h"
#include "aeolus/rankwave.h"
#include "aeolus/division.h"
#include "aeolus/sequencer.h"
#include "aeolus/audioparam.h"
#include "aeolus/levelmeter.h"
#include "aeolus/dsp/convolver.h"
#include "aeolus/dsp/interpolator.h"

#include "mts/libMTSClient.h"

#include <optional>
#include <vector>

AEOLUS_NAMESPACE_BEGIN

class Engine;

/**
 * @brief A global shared instance of the organ engine.
 *
 * This class in a singleton which is shared among all the plugin instances.
 */
class EngineGlobal : public juce::DeletedAtShutdown,
                     private juce::Timer
{
public:

    class ProcessorProxy
    {
    public:
        virtual ~ProcessorProxy() = default;
        virtual juce::AudioProcessor* getAudioProcessor() = 0;
        virtual Engine& getEngine() = 0;
        virtual void killAllVoices() = 0;
        virtual int getNumberOfActiveVoices() = 0;
    };

    void registerProcessorProxy(ProcessorProxy* proxy);
    void unregisterProcessorProxy(ProcessorProxy* proxy);

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

    void loadSettings();
    void saveSettings();

    int getStopsCount() const noexcept { return _rankwaves.size(); }
    Rankwave* getStop(int i) { return _rankwaves[i]; }

    juce::StringArray getAllStopNames() const;
    Rankwave* getStopByName(const juce::String& name);

    const std::vector<IR>& getIRs() const noexcept { return _irs; }
    int getLongestIRLength() const noexcept { return _longestIRLength; }

    void updateStops(float sampleRate);

    float getTuningFrequency() const noexcept { return _tuningFrequency; }
    void setTuningFrequency(float f) noexcept { _tuningFrequency = f; }

    const Scale& getScale() const noexcept { return _scale; }
    void setScaleType(Scale::Type type) noexcept { _scale.setType(type); }

    bool isConnectedToMTSMaster();
    juce::String getMTSScaleName();
    float getMTSNoteToFrequency(int midiNote, int midiChannel);

    bool isMTSEnabled() const { return _mtsEnabled; }
    void setMTSEnabled(bool shouldBeEnabled) { _mtsEnabled = shouldBeEnabled; }

    void rebuildRankwaves();

    JUCE_DECLARE_SINGLETON (EngineGlobal, false)

private:
    EngineGlobal();
    ~EngineGlobal() override;

    void loadRankwaves();
    void loadIRs();

    /**
     * Refresh MTS tuning table for all MIDI notes.
     * Returns true if there was a change to the tuning.
     */
    bool updateMTSTuningCache();

    // juce::Timer
    void timerCallback() override;

    juce::Array<ProcessorProxy*> _processors;

    juce::OwnedArray<Rankwave> _rankwaves;
    juce::HashMap<juce::String, Rankwave*> _rankwavesByName;

    std::vector<IR> _irs;
    int _longestIRLength;   ///< Longest IR length in samples

    float _sampleRate;
    Scale _scale;
    float _tuningFrequency;

    MTSClient* _mtsClient{};
    bool _mtsEnabled{};
    std::array<float, 128> _mtsTuningCache{};

    juce::ApplicationProperties _globalProperties;
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
     * Returns the reverb tail in seconds.
     */
    float getReverbLengthInSeconds() const;

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
     * Returns currently set MIDI control channel.
     */
    int getMIDIControlChannelsMask() const noexcept { return _midiControlChannelsMask; }

    /**
     * Assign MIDI channel to be used to control the organ stops and sequencer.
     */
    void setMIDIControlChannelsMask(int mask) noexcept { _midiControlChannelsMask = mask; }

    /**
    * Returns currently set MIDI control channel.
    */
    int getMIDISwellChannelsMask() const noexcept { return _midiSwellChannelsMask; }

    /**
    * Assign MIDI channel to be used to control the organ stops and sequencer.
    */
    void setMIDISwellChannelsMask(int mask) noexcept { _midiSwellChannelsMask = mask; }

    /**
     * Generate audio.
     */
    void process(float* outL, float* outR, int numFrames, bool isNonRealtime = false);

    // Multibus version of the processing (does not include the convolver).
    void process(juce::AudioBuffer<float>& out, bool isNonRealtime = false);

    /**
     * Process incoming MIDI messages.
     */
    void processMIDIMessage(const juce::MidiMessage& message);

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

    std::set<int> getKeySwitches() const;

    VoicePool& getVoicePool() noexcept { return _voicePool; }

    int getDivisionCount() const noexcept { return _divisions.size(); }
    Division* getDivisionByIndex(int i) { return _divisions[i]; }
    Division* getDivisionByName(const juce::String& name);

    Sequencer* getSequencer() noexcept { return _sequencer.get(); }

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

    /// Generate tremulant osc waveform for a subframe.
    void generateTremulant();

    /// Apply the gloval volume.
    void applyVolume(juce::AudioBuffer<float>& out);
    void applyVolume(float* outL, float* outR, int numFrames);

    /// Process control MIDI messages: program change (sequencer) and stop buttons CC.
    void processControlMIDIMessage(const juce::MidiMessage& message);

    /// Process stop buttons MIDI controls.
    void processStopControlMessage();

    bool isKeySwitchForward(int key) const;
    bool isKeySwitchBackward(int key) const;

    float _sampleRate;

    RingBuffer<NoteEvent, 1024> _pendingNoteEvents;

    VoicePool _voicePool;           ///< All the voices.

    AudioParameterPool _params;     ///< Internal parameters.

    std::optional<StopControlMode> _stopControlMode{};
    int _stopControlGroup{};
    int _stopControlButton{};

    /// List of all divisions
    juce::OwnedArray<Division> _divisions;

    std::unique_ptr<Sequencer> _sequencer;

    std::vector<int> _sequencerStepBackwardKeySwitches{ SEQUENCER_BACKWARD_MIDI_KEY };
    std::vector<int> _sequencerStepForwardKeySwitches{ SEQUENCER_FORWARD_MIDI_KEY };

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

    std::atomic<int> _midiControlChannelsMask;
    std::atomic<int> _midiSwellChannelsMask;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Engine)
};

AEOLUS_NAMESPACE_END
