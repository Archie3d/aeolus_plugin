#pragma once

#include "aeolus/globals.h"
#include "aeolus/list.h"
#include "aeolus/rankwave.h"

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

private:
    Engine& _engine;
    Pipewave::State _state; ///< Pipe state associated with this voice.

    float _buffer[SUB_FRAME_LENGTH];

    dsp::SpatialSource _spatialSource;
    
    // Counter to accounted for the delayed sound before recycling the voice
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
