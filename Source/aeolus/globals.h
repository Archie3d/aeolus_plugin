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

#include <JuceHeader.h>

#define AEOLUS_NAMESPACE_BEGIN namespace aeolus {
#define AEOLUS_NAMESPACE_END }
#define AEOLUS_USING_NAMESPACE using namespace aeolus;

AEOLUS_NAMESPACE_BEGIN

/// Processing sample rate. It is low enough
/// since there are not many harmonics to be generated
/// and thus we can get away without using an interpolation filter
/// when upsampling only.
constexpr static float SAMPLE_RATE = 44100.0f;

constexpr static float SAMPLE_RATE_R = 1.0f / SAMPLE_RATE;

/// Number of notes used in parameters look-up table.
constexpr static int N_NOTES = 11;

/// Gap between the N_NOTES notes within the look-up tables.
constexpr static int NOTES_GAP = 6;

/// Number of harmonics used.
constexpr static int N_HARM = 64;

/// Lowest possible note.
constexpr static int NOTE_MIN = 36;

/// Highest possible note.
constexpr static int NOTE_MAX = 96;

/// Length of a processing frame (in samples).
constexpr static int SUB_FRAME_LENGTH = 64;

/// Tremulant modulation frequency.
constexpr static float TREMULANT_FREQUENCY = 6.283184f;
constexpr static float TREMULANT_PHASE_INCREMENT = juce::MathConstants<float>::twoPi * TREMULANT_FREQUENCY / SAMPLE_RATE;
constexpr static float TREMULANT_LEVEL = 0.2f;

//==============================================================================

/// Values used by previous version of the synth.
namespace deprecated {

    constexpr static int N_HARM = 48;

    constexpr static int NOTE_MAX = 46;
} // namespace deprecated

//==============================================================================

namespace math {

float exp2ap(float x);

/// Linear interpolation
template <typename T>
T lerp (T a, T b, T frac) { return a + (b - a) * frac; }

/// Lagrange polynomial interpolation
template <typename T>
T lagr (T x_1, T x0, T x1, T x2, T frac)
{
    const T c1 = x1 - (1.0f / 3.0f) * x_1 - 0.5f * x0 - (1.0f / 6.0f) * x2;
    const T c2 = 0.5f * (x_1 + x1) - x0;
    const T c3 = (1.0f / 6.0f) * (x2 - x_1) + 0.5f * (x0 - x1);
    return ((c3 * frac + c2) * frac + c1) * frac + x0;
}

template <typename T>
T lagr (T* x, T frac)
{
    const T c1 = x[2] - (1.0f / 3.0f) * x[0] - 0.5f * x[1] - (1.0f / 6.0f) * x[3];
    const T c2 = 0.5f * (x[0] + x[2]) - x[1];
    const T c3 = (1.0f / 6.0f) * (x[3] - x[0]) + 0.5f * (x[1] - x[2]);
    return ((c3 * frac + c2) * frac + c1) * frac + x[1];
}

} // namespace math

AEOLUS_NAMESPACE_END
