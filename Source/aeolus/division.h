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
#include "aeolus/rankwave.h"
#include "aeolus/stop.h"
#include "aeolus/voice.h"
#include "aeolus/audioparam.h"
#include "aeolus/levelmeter.h"
#include "aeolus/dsp/filter.h"

#include <atomic>
#include <vector>
#include <bitset>

AEOLUS_NAMESPACE_BEGIN

class Engine;

/**
 * @brief Single keyboard division.
 *
 * A division may have multiple stops available, which can be enabled or
 * disabled individually.
 */
class Division
{
public:

    enum Params
    {
        GAIN = 0,

        NUM_PARAMS
    };

    /// Link with another division.
    struct Link
    {
        Division* division;
        bool enabled = false;
    };

    /// Volume level
    struct Level
    {
        LevelMeter left;
        LevelMeter right;
    };

    //--------------------------------------------------------------------------

    Division(Engine& engine, const juce::String& name = juce::String());

    /**
     * @brief Load the division configuration from a JSON object.
     *
     * This will configure the division from an organ configuration data.
     */
    void initFromVar(const juce::var& v);

    juce::var getPersistentState() const;
    void setPersistentState(const juce::var& v);

    Engine& getEngine() noexcept { return _engine; }

    juce::String getName() const { return _name; }
    juce::String getMnemonic() const { return _mnemonic; }

    /**
     * Populate linked divisions from the division names.
     * This method must be called by the engine when all the divisions
     * have been loaded and initialized.
     */
    void populateLinkedDivisions();

    int getLinksCount() const noexcept;
    void enableLink(int i, bool ena);
    bool isLinkEnabled(int i);
    Link& getLinkByIndex(int i);
    void cancelAllLinks();


    void clear();
    Stop& addRankwave(Rankwave* ptr, bool ena = false, const juce::String& name = juce::String());
    Stop& addRankwaves(const std::vector<Rankwave*> rw, bool ena = false, const juce::String& name = juce::String());

    juce::AudioParameterFloat* getParamGain() noexcept { return _paramGain; }
    void setParamGain(juce::AudioParameterFloat* param) noexcept { _paramGain = param; }

    AudioParameterPool& parameters() noexcept { return _params; }

    Level& volumeLevel() noexcept { return _volumeLevel; }

    int getStopsCount() const noexcept;
    void enableStop(int i, bool ena);
    bool isStopEnabled(int i) const;
    Stop& getStopByIndex(int i);
    void disableAllStops();

    void getAvailableRange(int& minNote, int& maxNote) const noexcept;

    int getMIDIChannel() const noexcept { return _midiChannel; }
    bool isForMIDIChannel(int channel) const noexcept;
    void setMIDIChannel(int channel) noexcept { _midiChannel = channel; }

    bool hasSwell() const noexcept { return _hasSwell; }
    void setHasSwell(bool v) noexcept { _hasSwell = v; }
    bool hasTremulant() const noexcept { return _hasTremulant; }
    void setHasTremulant(bool v) noexcept { _hasTremulant = v; }
    bool isTremulantEnabled() const noexcept { return _tremulantEnabled; }
    void setTremulantEnabled(bool ena) noexcept;

    float getTremulantLevel(bool update = true);

    //------------------------------------------------------

    // All the following methods must be called on the audio thread.

    void noteOn(int note, int midiChannel);
    void noteOff(int note, int midiChannel);
    void allNotesOff();

    void handleControlMessage(const juce::MidiMessage& msg);

    bool process(juce::AudioBuffer<float>& targetBuffer, juce::AudioBuffer<float>& voiceBuffer);
    void modulate(juce::AudioBuffer<float>& targetBuffer, const juce::AudioBuffer<float>& tremulantBuffer);

    void releaseVoicesOfDisabledStops();
    void triggerVoicesOfEnabledStops();

    List<Voice>& getActiveVoices() noexcept { return _activeVoices; }

    /**
     * Tells the division has been alreayd triggered by a linked division,
     * so that it should not be receiving the same note on/off event.
     */
    bool hasBeenTriggered() const noexcept { return _triggerFlag; }

    /**
     * Clears trigger flag.
     * This must be called on all divisions before processing the
     * next note on/off event.
     */
    void clearTriggerFlag() noexcept { _triggerFlag = false; }

private:

    bool triggerVoicesForStop(int stopIndex, int note);

    Engine& _engine;

    juce::String _name;     ///< The division name.
    juce::String _mnemonic; ///< Short mnemonic name.

    /// List of linked divisions names.
    juce::StringArray _linkedDivisionNames;
    std::vector<Link> _linkedDivisions;

    bool _hasSwell;         ///< Whetehr this division has a swell control.
    bool _hasTremulant;     ///< Whether this division has a remulant control.

    std::atomic<int> _midiChannel;          ///< Division MIDI channel.
    std::atomic<bool> _tremulantEnabled;    ///< Whether tremulant is enabled.

    float _tremulantLevel;
    float _tremulantMaxLevel;
    std::atomic<float> _tremulantTargetLevel;

    /// Stored gain parameter for easy access from the devision control UI component
    juce::AudioParameterFloat* _paramGain;
    AudioParameterPool _params;

    /// Swell low-pass filter.
    dsp::BiquadFilter::Spec _swellFilterSpec;
    dsp::BiquadFilter::State _swellFilterStateL;
    dsp::BiquadFilter::State _swellFilterStateR;

    /// Delay lines used for tremulant frequency modulation.
    dsp::DelayLine _tremulantDelayL;
    dsp::DelayLine _tremulantDelayR;

    std::vector<Stop> _stops;   ///< All the stops this division has.

    List<Voice> _activeVoices;  ///< Active voices on this division.

    std::bitset<TOTAL_NOTES> _keysState; ///< MIDI keys state 1 = on, 0 = off.

    /// Tells whether this division has been triggered.
    /// This is used to avoid a division to be triggered multiple
    /// times by the same not on/off even, which is the case
    /// for linked divisions.
    bool _triggerFlag;

    Level _volumeLevel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Division)
};

AEOLUS_NAMESPACE_END
