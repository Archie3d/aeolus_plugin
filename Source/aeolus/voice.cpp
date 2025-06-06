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

using namespace juce;

AEOLUS_NAMESPACE_BEGIN

Voice::Voice(Engine& engine)
    : _engine(engine)
    , _state{}
    , _stopIndex{-1}
    , _buffer{0}
    , _delayLine{SAMPLE_RATE}
    , _delay{0}
    , _chiff{}
    , _panPosition{0.0f}
    , _spatialSource{}
{
}

void Voice::trigger(const Pipewave::State& state)
{
    jassert(_state.isIdle());
    _state = state;

    // Chiff
    const auto freq = _state.pipewave->getPipeFrequency();
    const auto dt = 1.0f / freq;

    // Delay pipe harmonic signal so that chiff noise builds up first
    _delay = (int) jmin((float)_delayLine.size(), 0.5f * dt * SAMPLE_RATE_F);

    _chiff.setAttack(5.0f * dt);
    _chiff.setDecay(100.0f * dt);
    _chiff.setSustain(0.01f);
    _chiff.setRelease(100.0f * dt);

    // Frequency-dependant chiff attenuation
    float att = 1.0f - expf(-freq / 3000.0f);
    _chiff.setGain(jmin(1.0f, 0.02f * _state.chiffGain * att));
    _chiff.setFrequency(freq);
    _chiff.trigger();

    // Spatialisation
    int note = _state.pipewave->getNote();
    float k = note % 2 != 0 ? 1.0f : -1.0f;

    // Wider spread for low-pitched pipes
    const auto& model = _state.pipewave->getModel();
    const float width = 0.15f * model.getFd() / model.getFn();

    float x = width * k * (float)abs(note - 65);

    // Assuming notes range [36..96]
    float n = k * float(abs(note - 65)); // ~[-30..30]
    _panPosition = jlimit(0.0f, 1.0f, (n + 30.0f) / 60.0f);

    _spatialSource.setSampleRate(SAMPLE_RATE_F);
    _spatialSource.setSourcePosition(x, 5.0f);
    _spatialSource.recalculate();
    _postReleaseCounter = _spatialSource.getPostFxSamplesCount() + 2 * _delay + (int)TREMULANT_DELAY_LENGTH;
}

void Voice::release()
{
    // This voice is pending to be reclaimed.
    if (_state.env == Pipewave::Over)
        return;

    _state.release();

    _chiff.release();
}

void Voice::reset()
{
    _state.reset();
    _stopIndex = -1;

    memset(_buffer, 0, sizeof(float) * SUB_FRAME_LENGTH);

    _delayLine.reset();
    _chiff.reset();
    _spatialSource.reset();
}

void Voice::process(float* outL, float* outR)
{
    memset(_buffer, 0, sizeof(float) * SUB_FRAME_LENGTH);

    const auto gain = _state.gain;

    if (_state.env == Pipewave::Over) {
        _postReleaseCounter -= std::min((int)_postReleaseCounter, SUB_FRAME_LENGTH);

        for (int i = 0; i < SUB_FRAME_LENGTH; ++i) {
            _delayLine.write(0.0f);
            _buffer[i] = _delayLine.readNearest(_delay) * gain;
        }

    } else {
        auto* pipe = _state.pipewave;
        pipe->play(_state, _buffer);

        for (int i = 0; i < SUB_FRAME_LENGTH; ++i) {
            _delayLine.write(_buffer[i]);
            _buffer[i] = _delayLine.readNearest(_delay) * gain;
        }
    }

    _chiff.process(_buffer, SUB_FRAME_LENGTH);

    // Spatial modellig is only applied on stereo voice output
    if (outL != outR) {
        _spatialSource.process(_buffer, outL, outR, SUB_FRAME_LENGTH);
    } else {
        memcpy(outL, _buffer, sizeof(float) * SUB_FRAME_LENGTH);
    }
}

bool Voice::isOver() const noexcept
{
    return (_state.env == Pipewave::Over) && _postReleaseCounter == 0;
}

bool Voice::isActive() const noexcept
{
    return _state.env == Pipewave::Attack;
}

bool Voice::isForNote(int note) const noexcept
{
    if (_state.pipewave != nullptr)
        return _state.pipewave->getNote() == note;

    return false;
}

int Voice::getNote() const
{
    if (_state.pipewave != nullptr)
        return _state.pipewave->getNote();

    return -1;
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
