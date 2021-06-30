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

#include "aeolus/globals.h"

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

/**
 * @brief ADSR-style envelope with exponential slopes.
 */
class Envelope
{
public:

    enum State
    {
        Off = 0,
        Attack,
        Decay,
        Sustain,
        Release,
        NumStates
    };

    constexpr static float AttackTargetRatio = 0.3f;
    constexpr static float DecayReleaseTargetRatio = 0.0001f;

    struct Trigger
    {
        float attack      = 0.0f;
        float decay       = 0.0f;
        float sustain     = 1.0f; 
        float release     = 1.0f;
    };

    Envelope();

    State state() const noexcept { return currentState; }

    void trigger(const Trigger& trigger, float sampleRate = SAMPLE_RATE_F);
    void release();
    void release(float t, float sampleRate = SAMPLE_RATE_F);

    float next();

    float level() const noexcept { return currentLevel; }

private:

    static float calculate(float rate, float targetRatio);

    State currentState;
    float currentLevel;

    float attackRate;
    float attackCoef;
    float attackBase;

    float decayRate;
    float decayCoef;
    float decayBase;

    float releaseRate;
    float releaseCoef;
    float releaseBase;

    float sustainLevel;
};

} // namespace dsp

AEOLUS_NAMESPACE_END
