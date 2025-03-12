// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Arthur Benilov <arthur.benilov@gmail.com>
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

#include "aeolus/dsp/limiter.h"

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

void Limiter::updateSpec(Limiter::Spec& spec)
{
}

void Limiter::resetState(const Limiter::Spec& spec, Limiter::State& state)
{
    state.gain = 1.0f;
    state.sustain = 0;
}

float Limiter::tick(const Limiter::Spec& spec, Limiter::State& state, float in)
{
    const float level = std::abs(in);

    const float targetGain = (level > spec.threshold) ? spec.threshold / level : 1.0f;

    if (state.gain > targetGain) {
        state.gain = std::max(state.gain + (targetGain - state.gain) * spec.attack, targetGain);
        state.sustain = spec.sustain;
    } else {
        if (state.sustain > 0) {
            state.sustain--;
        } else {
            state.gain = std::min(state.gain + (targetGain - state.gain) * spec.release, targetGain);
        }
    }

    return in * state.gain;
}

void Limiter::process(const Limiter::Spec& spec, Limiter::State& state, const float* in, float* out, size_t size)
{
    float targetGain = 1.0f;
    float gain = state.gain;
    int sustain = state.sustain;

    for (size_t i = 0; i < size; ++i) {
        const float level = std::abs(in[i]);
        targetGain = (level > spec.threshold) ? spec.threshold / level : 1.0f;

        if (gain > targetGain) {
            gain = std::max(gain + (targetGain - gain) * spec.attack, targetGain);
            sustain = spec.sustain;
        } else {
            if (sustain > 0) {
                sustain--;
            } else {
                gain = std::min(gain + (targetGain - gain) * spec.release, targetGain);
            }
        }

        out[i] = in[i] * gain;
    }

    state.gain = gain;
    state.sustain = sustain;
}

} // namespace dsp

AEOLUS_NAMESPACE_END
