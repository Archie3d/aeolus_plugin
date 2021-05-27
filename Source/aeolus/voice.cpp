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

#include "aeolus/voice.h"
#include "aeolus/engine.h"

AEOLUS_NAMESPACE_BEGIN

Voice::Voice(Engine& engine)
    : _engine(engine)
    , _state{}
    , _spatialSource{}
{
}

void Voice::trigger(const Pipewave::State& state)
{
    jassert(_state.isIdle());
    _state = state;

    int note = _state.pipewave->getNote();
    float k = note % 2 != 0 ? 1.0f : -1.0f;

    float x = 0.15f * (float) abs(note - 65) * k + 0.3f;

    _spatialSource.setSampleRate(SAMPLE_RATE);
    _spatialSource.setSourcePosition(x, 5.0f);
    _spatialSource.recalculate();
    _postReleaseCounter = _spatialSource.getPostFxSamplesCount();
}

void Voice::release()
{
    // This voice is pending to be reclaimed.
    if (_state.env == Pipewave::Over)
        return;

    _state.release();
}

void Voice::reset()
{
    _state.reset();
    _spatialSource.reset();
}

void Voice::process(float* outL, float* outR)
{
    memset(_buffer, 0, sizeof(float) * SUB_FRAME_LENGTH);

    if (_state.env == Pipewave::Over) {
        _postReleaseCounter -= std::min((int)_postReleaseCounter, SUB_FRAME_LENGTH);
    } else {
        auto* pipe = _state.pipewave;
        pipe->play(_state, _buffer);
    }

    _spatialSource.process(_buffer, outL, outR, SUB_FRAME_LENGTH);
}

bool Voice::isOver() const noexcept
{
    return (_state.env == Pipewave::Over) && _postReleaseCounter == 0;
}

bool Voice::isForNote(int note) const noexcept
{
    if (_state.pipewave != nullptr)
        return _state.pipewave->getNote() == note;

    return false;
}

void Voice::resetAndReturnToPool()
{
    _engine.getVoicePool().resetAndReturnToPool(this);
}

//==============================================================================

VoicePool::VoicePool(Engine& engine, int maxVoices)
    : _engine{engine}
    , _voices(maxVoices, engine)
    , _idleVoices{}
    , _voiceCount{0}
{
    for (auto& voice : _voices)
        _idleVoices.append(&voice);
}

Voice* VoicePool::trigger(const Pipewave::State& state)
{
    if (auto* voice = _idleVoices.first()) {
        _idleVoices.remove(voice);
        voice->trigger(state);
        ++_voiceCount;

        return voice;
    }

    // No more voices.
    return nullptr;
}

void VoicePool::resetAndReturnToPool(Voice* voice)
{
    jassert(voice != nullptr);

    voice->reset();
    _idleVoices.append(voice);
    --_voiceCount;
}

AEOLUS_NAMESPACE_END
