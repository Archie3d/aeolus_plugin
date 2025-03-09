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

#pragma once

#include "aeolus/globals.h"

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

struct Limiter
{

    struct Spec
    {
        float threshold;
        float attack;
        float release;
    };

    struct State
    {
        float gain;
    };

    static void updateSpec(Spec& spec);
    static void resetState(const Spec& spec, State& state);
    static float tick(const Spec& spec, State& state, float in);
    static void process(const Spec& spec, State& state, const float* in, float* out, size_t size);

};

} // namespace dsp

AEOLUS_NAMESPACE_END
