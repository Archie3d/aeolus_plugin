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

/// Multibus output option (must be set in the project configuration)
#ifndef AEOLUS_MULTIBUS_OUTPUT
#   define AEOLUS_MULTIBUS_OUTPUT 0
#endif

AEOLUS_NAMESPACE_BEGIN

#if AEOLUS_MULTIBUS_OUTPUT
    constexpr static int N_OUTPUT_CHANNELS = 8;
    constexpr static int N_VOICE_CHANNELS = 1;
#else
    constexpr static int N_OUTPUT_CHANNELS = 2;
    constexpr static int N_VOICE_CHANNELS = 2;
#endif

/// Processing sample rate. It is low enough
/// since there are not many harmonics to be generated
/// and thus we can get away without using an interpolation filter
/// when upsampling only.
constexpr static int SAMPLE_RATE = 44100;
constexpr static float SAMPLE_RATE_F = (float) SAMPLE_RATE;
constexpr static float SAMPLE_RATE_R = 1.0f / SAMPLE_RATE_F;

/// Global volume gain.
constexpr static float VOLUME_GAIN = 4.0f;

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

/// Total number of MIDI notes.
constexpr static int TOTAL_NOTES = 128;

/// Maxumim number of pipes in combined stops (like mixtures)
constexpr static int MAX_RANK = 5;

/// Length of a processing frame (in samples).
constexpr static int SUB_FRAME_LENGTH = 64;

/// Tremulant modulation frequency.
constexpr static float TREMULANT_FREQUENCY = 6.283184f;
constexpr static float TREMULANT_PHASE_INCREMENT = juce::MathConstants<float>::twoPi * TREMULANT_FREQUENCY / SAMPLE_RATE;

/// Tremulant OSC wavetable amplitude.
constexpr static float TREMULANT_LEVEL = 1.0f;

/// Number of steps in the sequencer.
constexpr static int SEQUENCER_N_STEPS = 16;

/// mid-A tuning frequency.
constexpr static float TUNING_FREQUENCY_MIN = 350.0f;
constexpr static float TUNING_FREQUENCY_MAX = 550.0f;
constexpr static float TUNING_FREQUENCY_STEP = 1.0f;
constexpr static float TUNING_FREQUENCY_DEFAULT = 440.0f;

//==============================================================================

// MIDI controls
enum {
    CC_VOLUME = 7,
    CC_REVERB = 91
};

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
T lagr (T x_1, T x0, T x1, T x2, T frac) noexcept
{
    const T c1 = x1 - (1.0f / 3.0f) * x_1 - 0.5f * x0 - (1.0f / 6.0f) * x2;
    const T c2 = 0.5f * (x_1 + x1) - x0;
    const T c3 = (1.0f / 6.0f) * (x2 - x_1) + 0.5f * (x0 - x1);
    return ((c3 * frac + c2) * frac + c1) * frac + x0;
}

template <typename T>
T lagr (const T* const x, T frac) noexcept
{
    const T c1 = x[2] - (1.0f / 3.0f) * x[0] - 0.5f * x[1] - (1.0f / 6.0f) * x[3];
    const T c2 = 0.5f * (x[0] + x[2]) - x[1];
    const T c3 = (1.0f / 6.0f) * (x[3] - x[0]) + 0.5f * (x[1] - x[2]);
    return ((c3 * frac + c2) * frac + c1) * frac + x[1];
}

//----------------------------------------------------------

template<unsigned M, unsigned N, unsigned B, unsigned A>
struct SinCosSeries
{
    constexpr static double value =
        1.0 - (A * juce::MathConstants<double>::pi / B) * ( A * juce::MathConstants<double>::pi / B) / M / (M + 1)
        * SinCosSeries<M + 2, N, B, A>::value;
};

template<unsigned N, unsigned B, unsigned A>
struct SinCosSeries<N, N, B, A> {
    constexpr static double value = 1.0;
};

template<unsigned B, unsigned A, typename T = double>
struct Sin;

template<unsigned B, unsigned A>
struct Sin<B, A, float>
{
    constexpr static float value = (A * juce::MathConstants<float>::pi / B) * float (SinCosSeries<2, 24, B, A>::value);
};

template<unsigned B, unsigned A>
struct Sin<B, A, double> {
    constexpr static double value = (A * juce::MathConstants<double>::pi / B) * SinCosSeries<2, 34, B, A>::value;
};

template<unsigned B, unsigned A, typename T = double>
struct Cos;

template<unsigned B, unsigned A>
struct Cos<B, A, float>
{
    constexpr static float value = float (SinCosSeries<1, 23, B, A>::value);
};

template<unsigned B, unsigned A>
struct Cos<B, A, double>
{
    constexpr static double value = SinCosSeries<1, 33, B, A>::value;
};

//----------------------------------------------------------

template <typename T>
constexpr bool isPowerOfTwo(T v)
{
    return (v & (v - 1)) == 0;
}


} // namespace math

AEOLUS_NAMESPACE_END
