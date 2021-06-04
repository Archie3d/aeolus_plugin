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
#include "aeolus/dsp/filter.h"
#include "aeolus/dsp/adsrenv.h"

#include <array>

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

class Chiff
{
public:

    struct Harmonic
    {
        BiquadFilter::Spec spec;
        BiquadFilter::State state;
        Envelope envelope;
        float gain;

        Harmonic();
        void reset();
        void setFrequency(float f);
        void setGain(float g);
        void trigger(const Envelope::Trigger& env);
        float tick(float x);
    };

    Chiff();

    void setAttack(float v);
    void setDecay(float v);
    void setSustain(float v);
    void setRelease(float v);
    void setGain(float v);
    void setFrequency(float v);

    void reset();

    void trigger();
    void release();
    bool isActive() const noexcept;

    void process(float* out, int numFrames);

private:

    constexpr static size_t NUM_HARMONICS = 4;
    constexpr static float harmonicGain = 0.85f;
    constexpr static float harmonicAttack = 0.75f;
    constexpr static float harmonicDecay = 0.6f;
    constexpr static float harmonicSustain = 0.98f;
    constexpr static float harmonicRelease = 0.6f;

    std::array<Harmonic, NUM_HARMONICS> _harmonics;

    Envelope::Trigger _envelopeTrigger;
};

} // namespace dsp

AEOLUS_NAMESPACE_END
