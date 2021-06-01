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

#include "aeolus/dsp/filter.h"

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

void BiquadFilter::updateSpec (BiquadFilter::Spec& spec)
{
    float A = 0.0f;

    if (spec.type == Type::PeakingEq || spec.type == Type::LowShelf || spec.type == Type::HighShelf)
        A = sqrt(powf(10.0f, spec.dbGain / 40.0f));
    else
        A = sqrtf(powf(10.0f, spec.dbGain / 20.0f));

    float w0 = 2.0f * juce::MathConstants<float>::pi * spec.freq / spec.sampleRate;

    float cos_w0 = cos(w0);
    float sin_w0 = sin(w0);
    float alpha = 0.0f;

    switch (spec.type) {
    case Type::LowPass:
    case Type::HighPass:
    case Type::AllPass:
        alpha = sin_w0 / (2.0f * spec.q);
        break;
    case Type::BandPass:
    case Type::Notch:
    case Type::PeakingEq:
        alpha = sin_w0 * sinh(log(2.0f) / 2.0f * spec.q * w0 / sin_w0);
        break;
    case Type::LowShelf:
    case Type::HighShelf:
        alpha = sin_w0 / 2.0f * sqrt((A + 1.0f / A) * (1.0f / spec.q - 1.0f) + 2.0f);
        break;
    default:
        jassertfalse; // Unsupported filter type
    }

    switch (spec.type) {
    case Type::LowPass:
        spec.b[0] = (1.0f - cos_w0) / 2.0f;
        spec.b[1] = 1.0f - cos_w0;
        spec.b[2] = (1.0f - cos_w0) / 2.0f;
        spec.a[0] = 1.0f + alpha;
        spec.a[1] = -2.0f * cos_w0;
        spec.a[2] = 1.0f - alpha;
        break;
    case Type::HighPass:
        spec.b[0] = (1.0f + cos_w0) / 2.0f;
        spec.b[1] = -(1.0f + cos_w0);
        spec.b[2] = (1.0f + cos_w0) / 2.0f;
        spec.a[0] = 1.0f + alpha;
        spec.a[1] = -2.0f * cos_w0;
        spec.a[2] = 1.0f - alpha;
        break;
    case Type::BandPass:
        // Constant 0 dB peak gain
        spec.b[0] = alpha;
        spec.b[1] = 0.0f;
        spec.b[2] = -alpha;
        spec.a[0] = 1.0f + alpha;
        spec.a[1] = -2.0f * cos_w0;
        spec.a[2] = 1.0f - alpha;
        break;
    case Type::Notch:
        spec.b[0] = 1.0f;
        spec.b[1] = -2.0f * cos_w0;
        spec.b[2] = 1.0f;
        spec.a[0] = 1.0f + alpha;
        spec.a[1] = -2.0f * cos_w0;
        spec.a[2] = 1.0f - alpha;
        break;
    case Type::AllPass:
        spec.b[0] = 1.0f - alpha;
        spec.b[1] = -2.0f * cos_w0;
        spec.b[2] = 1.0f + alpha;
        spec.a[0] = 1.0f + alpha;
        spec.a[1] = -2.0f * cos_w0;
        spec.a[2] = 1.0f - alpha;
        break;
    case Type::PeakingEq:
        spec.b[0] = 1.0f + alpha * A;
        spec.b[1] = -2.0f * cos_w0;
        spec.b[2] = 1.0f - alpha * A;
        spec.a[0] = 1.0f + alpha / A;
        spec.a[1] = -2.0f * cos_w0;
        spec.a[2] = 1.0f - alpha / A;
        break;
    case Type::LowShelf:
        spec.b[0] = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * sqrt (A) * alpha);
        spec.b[1] = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos_w0);
        spec.b[2] = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * sqrt (A) * alpha);
        spec.a[0] = (A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * sqrt (A) * alpha;
        spec.a[1] = -2.0f * ((A - 1.0f) + (A + 1.0f) * cos_w0);
        spec.a[2] = (A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * sqrt (A) * alpha;
        break;
    case Type::HighShelf:
        spec.b[0] = A * ((A + 1.0f) + (A - 1.0f) * cos_w0 + 2.0f * sqrt (A) * alpha);
        spec.b[1] = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cos_w0);
        spec.b[2] = A * ((A + 1.0f) + (A - 1.0f) * cos_w0 - 2.0f * sqrt (A) * alpha);
        spec.a[0] = (A + 1.0f) - (A - 1.0f) * cos_w0 + 2.0f * sqrt (A) * alpha;
        spec.a[1] = 2.0f * ((A - 1.0f) - (A + 1.0f) * cos_w0);
        spec.a[2] = (A + 1.0f) - (A - 1.0f) * cos_w0 - 2.0f * sqrt(A) * alpha;
        break;
    default:
        jassertfalse; // Should never get here
    }

    // Normalize the coefficients.
    spec.a[1] /= spec.a[0];
    spec.a[2] /= spec.a[0];
    spec.b[0] /= spec.a[0];
    spec.b[1] /= spec.a[0];
    spec.b[2] /= spec.a[0];
}

void BiquadFilter::resetState(const BiquadFilter::Spec&, BiquadFilter::State& state)
{
    ::memset(&state, 0, sizeof(state));
}

float BiquadFilter::tick(const Spec& spec, State& state, float in)
{
    const float x = in;
    const float y = spec.b[0] * x + spec.b[1] * state.x[0] + spec.b[2] * state.x[1]
                    - spec.a[1] * state.y[0] - spec.a[2] * state.y[1];

    state.x[1] = state.x[0];
    state.x[0] = x;
    state.y[1] = state.y[0];
    state.y[0] = y;

    return y;
}


void BiquadFilter::process(const Spec& spec, State& state, const float* in, float* out, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        const float x = in[i];
        const float y = spec.b[0] * x + spec.b[1] * state.x[0] + spec.b[2] * state.x[1]
                        - spec.a[1] * state.y[0] - spec.a[2] * state.y[1];

        state.x[1] = state.x[0];
        state.x[0] = x;
        state.y[1] = state.y[0];
        state.y[0] = y;

        out[i] = y;
    }
}


} // namespace dsp

AEOLUS_NAMESPACE_END
