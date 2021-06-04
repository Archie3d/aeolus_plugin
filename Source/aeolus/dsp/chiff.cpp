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

Chiff::Harmonic::Harmonic()
{
    spec.type = BiquadFilter::BandPass;
    spec.sampleRate = SAMPLE_RATE;
    spec.dbGain = 0.0f;
    spec.q = 0.7071f;
}

void Chiff::Harmonic::reset()
{
    BiquadFilter::resetState(spec, state);
}

void Chiff::Harmonic::setFrequency(float f)
{
    spec.freq = jmin(SAMPLE_RATE * 0.45f, f);
}

void Chiff::Harmonic::setGain(float g)
{
    gain = g;
}

void Chiff::Harmonic::trigger(const Envelope::Trigger& env)
{
    BiquadFilter::updateSpec(spec);
    BiquadFilter::resetState(spec, state);

    envelope.trigger(env);
}

float Chiff::Harmonic::tick(float x)
{
    return gain * envelope.next() * BiquadFilter::tick(spec, state, x);
}

//==============================================================================

Chiff::Chiff()
    : _harmonics{}
    , _envelopeTrigger{0.0f, 0.0f, 0.0f, 0.0f}
{
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
    float g = v;

    for (auto& h : _harmonics) {
        h.setGain(g);
        g *= harmonicGain;
    }
}

void Chiff::setFrequency(float v)
{
    float f = v;

    for (auto& h : _harmonics) {
        h.setFrequency(f);
        f *= 2.0f;
    }
}

void Chiff::reset()
{
    for (auto& h : _harmonics)
        h.reset();
}

void Chiff::trigger()
{
    auto env = _envelopeTrigger;

    for (auto& h : _harmonics) {
        h.trigger(env);
        env.attack *= harmonicAttack;
        env.decay *= harmonicDecay;
        env.sustain *= harmonicSustain;
        env.release *= harmonicRelease;
    }

}

void Chiff::release()
{
    for (auto& h : _harmonics) {
        if (h.envelope.state() != Envelope::Off)
            h.envelope.release();
    }
}

bool Chiff::isActive() const noexcept
{
    for (const auto& h : _harmonics) {
        if (h.envelope.state() != Envelope::Off)
            return true;
    }

    return false;
}

void Chiff::process(float* out, int numFrames)
{
    static Random rnd;

    // TODO: modulate filter Q-factor?

    if (!isActive())
        return;

    for (int i = 0; i < numFrames; ++i) {
        float x0 = 2.0f * rnd.nextFloat() - 1.0f;
        float x = 0.0f;

        for (auto& h : _harmonics) {
            x += h.tick(x0);
        }

        out[i] += x;
    }

}

} // namespace dsp

AEOLUS_NAMESPACE_END
