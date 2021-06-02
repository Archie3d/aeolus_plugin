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

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

class Chiff
{
public:

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
    Envelope::Trigger _envelopeTrigger;
    Envelope _envelope;
    float _gain;

    BiquadFilter::Spec _bpSpec;
    BiquadFilter::State _bpState;

    BiquadFilter::Spec _lpSpec;
    BiquadFilter::State _lpState;

};

} // namespace dsp

AEOLUS_NAMESPACE_END
