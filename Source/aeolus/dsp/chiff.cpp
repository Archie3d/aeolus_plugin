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
    : _envelopeTrigger{}
    , _envelope{}
    , _gain{1.0f}
    , _filterSpec{}
    , _filterState{}
{
    _envelopeTrigger.sustain = 0.0f;

    _filterSpec.type = BiquadFilter::BandPass;
    _filterSpec.sampleRate = SAMPLE_RATE;
    _filterSpec.dbGain = 0.0f;
    _filterSpec.q = 0.7071f;

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

void Chiff::setGain(float v)
{
    _gain = v;
}

void Chiff::setFrequency(float v)
{
    _filterSpec.freq = v;
}

void Chiff::reset()
{
    BiquadFilter::resetState(_filterSpec, _filterState);
}

void Chiff::trigger()
{
    BiquadFilter::updateSpec(_filterSpec);
    BiquadFilter::resetState(_filterSpec, _filterState);

    _envelope.trigger(_envelopeTrigger);
}

void Chiff::release()
{
    if (isActive())
        _envelope.release();
}

bool Chiff::isActive() const noexcept
{
    return _envelope.state() != Envelope::Off;
}

void Chiff::process(float* out, int numFrames)
{
    static Random rnd;

    // TODO: modulate filter Q-factor?

    if (!isActive())
        return;

    for (int i = 0; i < numFrames; ++i) {
        float x = _gain * (2.0f * rnd.nextFloat() - 1.0f);
        x = BiquadFilter::tick(_filterSpec, _filterState, x);
        x *= _envelope.next();

        out[i] += x;
    }

}

} // namespace dsp

AEOLUS_NAMESPACE_END
