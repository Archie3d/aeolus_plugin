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
#include "aeolus/list.h"
#include "aeolus/rankwave.h"

#include "aeolus/dsp/delay.h"
#include "aeolus/dsp/chiff.h"
#include "aeolus/dsp/spatial.h"

#include <vector>
#include <atomic>

AEOLUS_NAMESPACE_BEGIN

class Engine;

/**
 * @brief Single voice associated with a single pipe.
 */
class Voice : public ListItem<Voice>
{
public:
    Voice() = delete;
    Voice(Engine& engine);

    void trigger(const Pipewave::State& state);
    void release();
    void reset();
    void process(float* outL, float* outR);
    bool isOver() const noexcept;
    bool isForNote(int note) const noexcept;

    void resetAndReturnToPool();

    float getPanPosition() const noexcept { return _panPosition; }

private:
    Engine& _engine;
    Pipewave::State _state; ///< Pipe state associated with this voice.

    float _buffer[SUB_FRAME_LENGTH];

    /// Delay after chiff.
    dsp::DelayLine _delayLine;
    float _delay;

    /// Attack chiff.
    dsp::Chiff _chiff;

    ///  Voice virtual pan position [0..1] used for multibus output
    float _panPosition;

    /// Stereo spatial modeller.
    dsp::SpatialSource _spatialSource;
    
    /// Counter to account for the delayed sound before recycling the voice.
    size_t _postReleaseCounter;
};

//==============================================================================

/**
 * @brief A collection of all the voices.
 */
class VoicePool final
{
public:
    constexpr static int DefaultMaxVoices = 512;

    VoicePool(Engine& engine, int maxVoices = DefaultMaxVoices);

    Voice* trigger(const Pipewave::State& state);
    void resetAndReturnToPool(Voice* voice);

    int getNumberOfActiveVoices() const noexcept { return _voiceCount; }

private:

    Engine& _engine;

    std::vector<Voice> _voices;     ///< All the voices.
    List<Voice> _idleVoices;        ///< Voices available to be triggered.
    std::atomic<int> _voiceCount;   ///< Number of taken voices.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoicePool)
};

AEOLUS_NAMESPACE_END
