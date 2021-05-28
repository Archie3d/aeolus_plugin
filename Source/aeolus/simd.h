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
#include <stdint.h>

AEOLUS_NAMESPACE_BEGIN

/**
 * CPU features detected with cpuid instruction.
 * @see https://docs.microsoft.com/en-us/cpp/intrinsics/cpuid-cpuidex?view=vs-2019
 */
struct CPUTraits
{
    bool sse3;
    bool ssse3;
    bool fma;
    bool sse41;
    bool sse42;
    bool sse;
    bool sse2;
    bool avx;
    bool avx2;
    bool avx512f;
    bool avx512pf;
    bool avx512er;
    bool avx512cd;

    static CPUTraits get();
};

const CPUTraits CPUID = CPUTraits::get();

/**
 * @brief Vectorized computions.
 *
 * Implementation will be detected in runtime and appropriate function
 * pointer will be assigned. Implementation varies from no SIMD, to SSE,
 * and AVX, this all the pointers must be 32-bytes aligned and sizes
 * must be divisible by 8 (unless explicitly unaligned instruction is used).
 */
struct simd
{
    static void  (*add)(float*, const float*, size_t);
    static void  (*mul_const_add)(float*, const float*, const float, size_t);
    static void  (*add_mul_const)(float*, const float*, const float, size_t);
    static void  (*mul_const)(float*, const float, size_t);
    static float (*mul_reduce)(const float*, const float*, size_t);
    static float (*mul_reduce_unaligned)(const float*, const float*, size_t);
    static void  (*complex_mul)(float*, const float*, const float*, size_t);
    static void  (*complex_mul_conj)(float*, const float*, const float*, size_t);
    static void  (*fft_step)(float*, const float*, size_t);
};

AEOLUS_NAMESPACE_END