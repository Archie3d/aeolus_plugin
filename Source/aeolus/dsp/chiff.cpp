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

#include "aeolus/dsp/chiff.h"

AEOLUS_NAMESPACE_BEGIN

using namespace juce;

namespace dsp {

Chiff::Chiff()
    : _noiseEnvelope{}
    , _envelope{}
    , _envelopeTrigger{0.01f, 0.1f, 0.1f, 0.05f}
    , _pipeResonator{SAMPLE_RATE}
    , _pipeDelay{0.0f}
    , _lpSpec{}
    , _lpState{}
    , _gain{1.0f}
{

    _lpSpec.type = BiquadFilter::LowPass;
    _lpSpec.sampleRate = SAMPLE_RATE;
    _lpSpec.dbGain = 0.0f;
    _lpSpec.q = 0.7071f;
}

void Chiff::setAttack(float v)
{
    _envelopeTrigger.attack = v;
}

void Chiff::setRelease(float v)
{
    _envelopeTrigger.release = v;
}

void Chiff::setDecay(float v)
{
    _envelopeTrigger.decay = v;
}

void Chiff::setSustain(float v)
{
    _envelopeTrigger.sustain = v;
}

void Chiff::setGain(float v)
{
    _gain = v;
}

void Chiff::setFrequency(float f)
{
    _pipeDelay = SAMPLE_RATE / f;
    _lpSpec.freq = jmin(0.45f * SAMPLE_RATE, f * 4.0f);
}

void Chiff::reset()
{
    _pipeResonator.reset();
    BiquadFilter::resetState(_lpSpec, _lpState);
}

void Chiff::trigger()
{
    _noiseEnvelope.trigger({0.01f, 0.0f, 1.0f, 0.02f});

    _envelope.trigger(_envelopeTrigger);

    _pipeResonator.reset();
    BiquadFilter::updateSpec(_lpSpec);
    BiquadFilter::resetState(_lpSpec, _lpState);
}

void Chiff::release()
{
    _noiseEnvelope.release();
    _envelope.release();
}

bool Chiff::isActive() const noexcept
{
    // Don't care about the noise envelope here
    return _envelope.state() != Envelope::Off;
}

void Chiff::process(float* out, int numFrames)
{
    static Random rnd;

    if (!isActive())
        return;

    for (int i = 0; i < numFrames; ++i) {
        float x0 = 2.0f * rnd.nextFloat() - 1.0f;
        float x = x0 * _noiseEnvelope.next();
        float y = _pipeResonator.read(_pipeDelay);
        y = BiquadFilter::tick(_lpSpec, _lpState, y);
        y += x;
        _pipeResonator.write(y);

        out[i] += y * _gain * _envelope.next();
    }
}

} // namespace dsp

AEOLUS_NAMESPACE_END
