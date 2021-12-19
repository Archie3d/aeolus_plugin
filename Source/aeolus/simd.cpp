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


#include <cassert>
#include "aeolus/simd.h"

#ifdef SIMD
#   undef SIMD
#endif

#if defined (__APPLE__)
#   include <x86intrin.h>
#   define SIMD
#elif defined (_MSC_VER)
#   include <pmmintrin.h>
#   include <immintrin.h>
#   include <intrin.h>
#   define SIMD
#endif

AEOLUS_NAMESPACE_BEGIN

// perform cpuid function
static void cpuid_func(uint32_t* regs, unsigned funcId)
{
#if defined _WIN32
    __cpuid((int*) regs, (int) funcId);
#else
    asm volatile
        ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
            : "a" (funcId), "c" (0));
    // ECX is set to zero for CPUID function 4
#endif
}

CPUTraits CPUTraits::get()
{
    CPUTraits cpu;

    enum
    {
        eax = 0,
        ebx = 1,
        ecx = 2,
        edx = 3
    };

    uint32_t regs[4];

    cpuid_func (regs, 1);
    cpu.sse3     = (regs[ecx] & (1 <<  0)) != 0;
    cpu.ssse3    = (regs[ecx] & (1 <<  9)) != 0;
    cpu.fma      = (regs[ecx] & (1 << 12)) != 0;
    cpu.sse41    = (regs[ecx] & (1 << 19)) != 0;
    cpu.sse42    = (regs[ecx] & (1 << 20)) != 0;
    cpu.sse      = (regs[ecx] & (1 << 25)) != 0;
    cpu.sse2     = (regs[ecx] & (1 << 26)) != 0;
    cpu.avx      = (regs[ecx] & (1 << 28)) != 0;

    cpuid_func (regs, 7);
    cpu.avx2     = (regs[ebx] & (1 <<  5)) != 0;
    cpu.avx512f  = (regs[ebx] & (1 << 16)) != 0;
    cpu.avx512pf = (regs[ebx] & (1 << 26)) != 0;
    cpu.avx512er = (regs[ebx] & (1 << 27)) != 0;
    cpu.avx512cd = (regs[ebx] & (1 << 28)) != 0;

    return cpu;
};

//------------------------------------------------------------------------------

static inline bool is_aligned(const void *pointer, size_t byte_count)
{
    return (uintptr_t)pointer % byte_count == 0;
}

//------------------------------------------------------------------------------

namespace no_simd {
    void add(float* out, const float* in, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
            out[i] += in[i];
    }

    void mul_const_add(float* out, const float* in, const float k, size_t size)
    {

        for (size_t i = 0; i < size; ++i)
            out[i] += in[i] * k;
    }

    void add_mul_const(float* out, const float* in, const float k, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
            out[i] = k * (out[i] + in[i]);
    }

    void mul_const(float* out, const float k, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
            out[i] *= k;
    }

    float mul_reduce(const float* x, const float* y, size_t size)
    {
        float sum = 0.0f;

        for (size_t i = 0; i < size; ++i)
            sum += x[i] * y[i];

        return sum;
    }

    void complex_mul(float* res, const float* a, const float* b, size_t size)
    {
        for (size_t i = 0; i < size; i += 2) {
            res[i] = a[i] * b[i] - a[i + 1]* b[i + 1];
            res[i + 1] = a[i] * b[i + 1] + a[i + 1] * b[i];
        }
    }

    void complex_mul_conj(float* res, const float* a, const float* b, size_t size)
    {
        for (size_t i = 0; i < size; i += 2) {
            const float x = a[i] * b[i] - a[i + 1] * b[i + 1];
            const float y = -a[i] * b[i + 1] - a[i + 1] * b[i];
            res[i] = x;
            res[i + 1] = y;
        }
    }

    void fft_step(float* data, const float* w, size_t n)
    {
        for (unsigned i = 0; i < n; i += 2) {
            const float tempr = data[i + n] * w[i] - data[i + n + 1] * w[i + 1];
            const float tempi = data[i + n] * w[i + 1] + data[i + n + 1] * w[i];
            data[i + n] = data[i] - tempr;
            data[i + n + 1] = data[i + 1] - tempi;
            data[i] += tempr;
            data[i + 1] += tempi;
        }
    }

} // namespace no_simd

//------------------------------------------------------------------------------

#ifdef SIMD

namespace sse {
    void add (float* out, const float* in, size_t size)
    {
        assert((size & 0x3) == 0);

        for (size_t i = 0; i < size; i += 4) {
            __m128 x = _mm_load_ps ((float*)&out[i]);
            __m128 y = _mm_load_ps ((const float*)&in[i]);
            __m128 s = _mm_add_ps (x, y);
            _mm_store_ps ((float*)&out[i], s);
        }
    }

    void mul_const_add(float* out, const float* in, const float k, size_t size)
    {
        assert((size & 0x3) == 0);
        __m128 kv = _mm_set1_ps (k);

        for (size_t i = 0; i < size; i += 4) {
            __m128 x = _mm_load_ps ((float*)&out[i]);
            __m128 y = _mm_load_ps ((const float*)&in[i]);
            __m128 z = _mm_mul_ps (y, kv);
            __m128 s = _mm_add_ps (x, z);
            _mm_store_ps ((float*)&out[i], s);
        }
    }

    void add_mul_const(float* out, const float* in, const float k, size_t size)
    {
        assert((size & 0x3) == 0);
        __m128 kv = _mm_set1_ps (k);

        for (size_t i = 0; i < size; i += 4) {
            __m128 x = _mm_load_ps ((float*)&out[i]);
            __m128 y = _mm_load_ps ((const float*)&in[i]);
            x = _mm_add_ps (x, y);
            x = _mm_mul_ps (x, kv);
            _mm_store_ps ((float*)&out[i], x);
        }
    }

    void mul_const(float* out, const float k, size_t size)
    {
        assert((size & 0x3) == 0);
        __m128 kv = _mm_set1_ps (k);

        for (size_t i = 0; i < size; i += 4) {
            __m128 a = _mm_load_ps ((float*)&out[i]);
            __m128 b = _mm_mul_ps (a, kv);
            _mm_store_ps ((float*)&out[i], b);
        }
    }

    float mul_reduce(const float* x, const float* y, size_t size)
    {
        assert((size & 0x3) == 0);
        __m128 sum = _mm_set1_ps (0.0f);

        for (size_t i = 0; i < size; i += 4) {
            __m128 vx = _mm_load_ps ((const float*)&x[i]);
            __m128 vy = _mm_load_ps ((const float*)&y[i]);
            __m128 vs = _mm_mul_ps (vx, vy);
            sum = _mm_add_ps (sum, vs);
        }

        sum = _mm_hadd_ps (sum, sum);
        sum = _mm_hadd_ps (sum, sum);

        alignas (16) float vec[4];

        _mm_store_ps (vec, sum);
        return vec[0];
    }

    float mul_reduce_unaligned(const float* x, const float* y, size_t size)
    {
        float sum = 0.0f;
        __m128 vsum = _mm_set1_ps (0.0f);

        size_t i = 0;
        size_t n = size - (size % 4);

        if (n > 0) {
            for (i = 0; i < n; i += 4) {
                __m128 vx = _mm_loadu_ps ((const float*)&x[i]);
                __m128 vy = _mm_loadu_ps ((const float*)&y[i]);
                __m128 vs = _mm_mul_ps (vx, vy);
                vsum = _mm_add_ps (vsum, vs);
            }

            vsum = _mm_hadd_ps (vsum, vsum);
            vsum = _mm_hadd_ps (vsum, vsum);

            alignas (16) float vec[4];
            _mm_store_ps (vec, vsum);

            sum = vec[0];
        }

        for (; i < size; ++i)
            sum += x[i] * y[i];

        return sum;
    }

    void complex_mul(float* res, const float* a, const float* b, size_t size)
    {
        assert ((size & 0x3) == 0);
        __m128 factors = _mm_set_ps (1.0f, -1.0f, 1.0f, -1.0f);

        for (size_t i = 0; i < size; i += 4) {
            __m128 a01 = _mm_load_ps ((const float*)&a[i]); // [a0.r, a0.i, a1.r, a1.i]
            __m128 b01 = _mm_load_ps ((const float*)&b[i]); // [b0.r, b0.i, b1.r, b1.i]

            __m128 b01r = _mm_shuffle_ps (b01, b01, 0xA0); // [b0.r, b0.r, b1.r, b1.r] 
            __m128 b01i = _mm_shuffle_ps (b01, b01, 0xF5); // [b0.i, b0.i, b1.i, b1.i]

            __m128 a01ir = _mm_shuffle_ps (a01, a01, 0xB1); // [a0.i, a0.r, a1.i, a1.r]

            __m128 r0 = _mm_mul_ps (a01, b01r);      // [a0.r*b0.r, a0.i*b0.r, a1.r*b1.r, a1.i*b1.r]
            __m128 r3 = _mm_mul_ps (a01ir, b01i);    // [a0.i*b0.i, a0.r*b0.i, a1.i*b1.i, a1.r*b1.i]
            r3 = _mm_mul_ps (r3, factors);           // [-a0.i*b0.i, a0.r*b0.i, -a1.i*b1.i, a1.r*b1.i]
            r3 = _mm_add_ps (r0, r3);                // [a0.r*b0.r - a0.i*b0.i, a0.i*b0.r + a0.r*b0.i,
                                                     //  a1.r*b1.r - a1.i*b1.i, a1.i*b1.r + a1.r*b1.i]
            _mm_store_ps ((float*)&res[i], r3);
        }
    }

    void complex_mul_conj(float* res, const float* a, const float* b, size_t size)
    {
        assert ((size & 0x3) == 0);
        __m128 factors = _mm_set_ps (1.0f, -1.0f, 1.0f, -1.0f);
        __m128 conj = _mm_set_ps (-1.0f, 1.0f, -1.0f, 1.0f);

        for (size_t i = 0; i < size; i += 4) {
            __m128 a01 = _mm_load_ps ((const float*)&a[i]); // [a0.r, a0.i, a1.r, a1.i]
            __m128 b01 = _mm_load_ps ((const float*)&b[i]); // [b0.r, b0.i, b1.r, b1.i]

            __m128 b01r = _mm_shuffle_ps (b01, b01, 0xA0); // [b0.r, b0.r, b1.r, b1.r]
            __m128 b01i = _mm_shuffle_ps (b01, b01, 0xF5); // [b0.i, b0.i, b1.i, b1.i]

            __m128 a01ir = _mm_shuffle_ps (a01, a01, 0xB1); // [a0.i, a0.r, a1.i, a1.r]

            __m128 r0 = _mm_mul_ps (a01, b01r);      // [a0.r*b0.r, a0.i*b0.r, a1.r*b1.r, a1.i*b1.r]
            __m128 r3 = _mm_mul_ps (a01ir, b01i);    // [a0.i*b0.i, a0.r*b0.i, a1.i*b1.i, a1.r*b1.i]
            r3 = _mm_mul_ps (r3, factors);           // [-a0.i*b0.i, a0.r*b0.i, -a1.i*b1.i, a1.r*b1.i]
            r3 = _mm_add_ps (r0, r3);                // [a0.r*b0.r - a0.i*b0.i, a0.i*b0.r + a0.r*b0.i,
                                                     //  a1.r*b1.r - a1.i*b1.i, a1.i*b1.r + a1.r*b1.i]
            r3 = _mm_mul_ps (r3, conj);

            _mm_store_ps ((float*)&res[i], r3);
        }
    }

    void fft_step(float* data, const float* w, size_t n)
    {
        assert ((n & 0x3) == 0);
        assert (is_aligned (data, 16));
        assert (is_aligned (w, 16));

        float* dataN = &data[n];

        __m128 factors = _mm_set_ps (1.0f, -1.0f, 1.0f, -1.0f);

        for (size_t i = 0; i < n; i += 4) {
            // Complex multiplication
            __m128 a01 = _mm_load_ps ((const float*)&dataN[i]);
            __m128 b01 = _mm_load_ps ((const float*)&w[i]);

            __m128 b01r = _mm_shuffle_ps (b01, b01, 0xA0);
            __m128 b01i = _mm_shuffle_ps (b01, b01, 0xF5);

            __m128 a01ir = _mm_shuffle_ps (a01, a01, 0xB1);

            __m128 r0 = _mm_mul_ps (a01, b01r);
            __m128 r = _mm_mul_ps (a01ir, b01i);
            r = _mm_mul_ps (r, factors);
            r = _mm_add_ps (r0, r);


            r0 = _mm_load_ps ((const float*)&data[i]);
            __m128 r1 = _mm_sub_ps (r0, r);
            _mm_store_ps ((float*)&dataN[i], r1);

            __m128 r2 = _mm_add_ps (r0, r);
            _mm_store_ps ((float*)&data[i], r2);
        }
    }

    namespace fma {
#if 0
        void mul_const_add(float* out, const float* in, const float k, size_t size)
        {
            assert ((size & 0x3) == 0);
            __m128 kv = _mm_set1_ps (k);

            for (size_t i = 0; i < size; i += 4) {
                __m128 x = _mm_load_ps ((float*)&out[i]);
                __m128 y = _mm_load_ps ((const float*)&in[i]);
                __m128 s = _mm_fmadd_ps (y, kv, x);
                _mm_store_ps ((float*)&out[i], s);
            }
        }

        float mul_reduce(const float* x, const float* y, size_t size)
        {
            assert ((size & 0x3) == 0);
            __m128 sum = _mm_set1_ps (0.0f);

            for (size_t i = 0; i < size; i += 4) {
                __m128 vx = _mm_load_ps ((const float*)&x[i]);
                __m128 vy = _mm_load_ps ((const float*)&y[i]);
                sum = _mm_fmadd_ps (vx, vy, sum);
            }

            sum = _mm_hadd_ps (sum, sum);
            sum = _mm_hadd_ps (sum, sum);

            alignas (16) float vec[4];

            _mm_store_ps (vec, sum);
            return vec[0];
        }

        float mul_reduce_unaligned(const float* x, const float* y, size_t size)
        {
            float sum = 0.0f;
            __m128 vsum = _mm_set1_ps (0.0f);

            size_t i = 0;
            size_t n = size - (size % 4);

            if (n > 0) {
                for (i = 0; i < n; i += 4) {
                    __m128 vx = _mm_loadu_ps ((const float*)&x[i]);
                    __m128 vy = _mm_loadu_ps ((const float*)&y[i]);
                    vsum = _mm_fmadd_ps (vx, vy, vsum);
                }

                vsum = _mm_hadd_ps (vsum, vsum);
                vsum = _mm_hadd_ps (vsum, vsum);

                alignas (16) float vec[4];
                _mm_store_ps (vec, vsum);

                sum = vec[0];
            }

            for (; i < size; ++i)
                sum += x[i] * y[i];

            return sum;
        }

        void complex_mul(float* res, const float* a, const float* b, size_t size)
        {
            assert ((size & 0x3) == 0);
            __m128 factors = _mm_set_ps (1.0f, -1.0f, 1.0f, -1.0f);

            for (size_t i = 0; i < size; i += 4) {
                __m128 a01 = _mm_load_ps ((const float*)&a[i]); // [a0.r, a0.i, a1.r, a1.i]
                __m128 b01 = _mm_load_ps ((const float*)&b[i]); // [b0.r, b0.i, b1.r, b1.i]

                __m128 b01r = _mm_shuffle_ps (b01, b01, 0xA0); // [b0.r, b0.r, b1.r, b1.r] 
                __m128 b01i = _mm_shuffle_ps (b01, b01, 0xF5); // [b0.i, b0.i, b1.i, b1.i]

                __m128 a01ir = _mm_shuffle_ps (a01, a01, 0xB1); // [a0.i, a0.r, a1.i, a1.r]

                __m128 r0 = _mm_mul_ps (a01, b01r);      // [a0.r*b0.r, a0.i*b0.r, a1.r*b1.r, a1.i*b1.r]
                __m128 r3 = _mm_mul_ps (a01ir, b01i);    // [a0.i*b0.i, a0.r*b0.i, a1.i*b1.i, a1.r*b1.i]
                r3 = _mm_fmadd_ps (r3, factors, r0);     // [-a0.i*b0.i, a0.r*b0.i, -a1.i*b1.i, a1.r*b1.i]
                                                         // [a0.r*b0.r - a0.i*b0.i, a0.i*b0.r + a0.r*b0.i,
                                                         //  a1.r*b1.r - a1.i*b1.i, a1.i*b1.r + a1.r*b1.i]
                _mm_store_ps ((float*)&res[i], r3);
            }
        }

        void complex_mul_conj(float* res, const float* a, const float* b, size_t size)
        {
            assert ((size & 0x3) == 0);
            __m128 factors = _mm_set_ps (1.0f, -1.0f, 1.0f, -1.0f);
            __m128 conj = _mm_set_ps (-1.0f, 1.0f, -1.0f, 1.0f);

            for (size_t i = 0; i < size; i += 4) {
                __m128 a01 = _mm_load_ps ((const float*)&a[i]); // [a0.r, a0.i, a1.r, a1.i]
                __m128 b01 = _mm_load_ps ((const float*)&b[i]); // [b0.r, b0.i, b1.r, b1.i]

                __m128 b01r = _mm_shuffle_ps (b01, b01, 0xA0); // [b0.r, b0.r, b1.r, b1.r]
                __m128 b01i = _mm_shuffle_ps (b01, b01, 0xF5); // [b0.i, b0.i, b1.i, b1.i]

                __m128 a01ir = _mm_shuffle_ps (a01, a01, 0xB1); // [a0.i, a0.r, a1.i, a1.r]

                __m128 r0 = _mm_mul_ps (a01, b01r);      // [a0.r*b0.r, a0.i*b0.r, a1.r*b1.r, a1.i*b1.r]
                __m128 r3 = _mm_mul_ps (a01ir, b01i);    // [a0.i*b0.i, a0.r*b0.i, a1.i*b1.i, a1.r*b1.i]
                
                r3 = _mm_fmadd_ps (r3, factors, r0);     // [-a0.i*b0.i, a0.r*b0.i, -a1.i*b1.i, a1.r*b1.i]
                                                         // [a0.r*b0.r - a0.i*b0.i, a0.i*b0.r + a0.r*b0.i,
                                                         //  a1.r*b1.r - a1.i*b1.i, a1.i*b1.r + a1.r*b1.i]
                r3 = _mm_mul_ps (r3, conj);

                _mm_store_ps ((float*)&res[i], r3);
            }
        }

        void fft_step(float* data, const float* w, size_t n)
        {
            assert ((n & 0x3) == 0);
            assert (is_aligned (data, 16));
            assert (is_aligned (w, 16));

            float* dataN = &data[n];

            __m128 factors = _mm_set_ps (1.0f, -1.0f, 1.0f, -1.0f);

            for (size_t i = 0; i < n; i += 4) {
                // Complex multiplication
                __m128 a01 = _mm_load_ps ((const float*)&dataN[i]);
                __m128 b01 = _mm_load_ps ((const float*)&w[i]);

                __m128 b01r = _mm_shuffle_ps (b01, b01, 0xA0);
                __m128 b01i = _mm_shuffle_ps (b01, b01, 0xF5);

                __m128 a01ir = _mm_shuffle_ps (a01, a01, 0xB1);

                __m128 r0 = _mm_mul_ps (a01, b01r);
                __m128 r = _mm_mul_ps (a01ir, b01i);
                r = _mm_fmadd_ps (r, factors, r0);

                r0 = _mm_load_ps ((const float*)&data[i]);
                __m128 r1 = _mm_sub_ps (r0, r);
                _mm_store_ps ((float*)&dataN[i], r1);

                __m128 r2 = _mm_add_ps (r0, r);
                _mm_store_ps ((float*)&data[i], r2);
            }
        }
#endif
    } // namespace fma

} // namespace sse

#if 0
namespace avx {

    void add(float* out, const float* in, size_t size)
    {
        assert ((size & 0x7) == 0);
        assert (is_aligned (out, 32));
        assert (is_aligned (in, 32));


        for (size_t i = 0; i < size; i += 8) {
            __m256 x = _mm256_load_ps ((float*)&out[i]);
            __m256 y = _mm256_load_ps ((const float*)&in[i]);
            __m256 s = _mm256_add_ps (x, y);
            _mm256_store_ps ((float*)&out[i], s);
        }

        _mm256_zeroupper();
    }

    void mul_const_add(float* out, const float* in, const float k, size_t size)
    {
        assert ((size & 0x7) == 0);
        assert (is_aligned (out, 32));
        assert (is_aligned (in, 32));

        __m256 kv = _mm256_set1_ps (k);

        for (size_t i = 0; i < size; i += 8) {
            __m256 x = _mm256_load_ps ((float*)&out[i]);
            __m256 y = _mm256_load_ps ((const float*)&in[i]);
            __m256 z = _mm256_mul_ps (y, kv);
            __m256 s = _mm256_add_ps (x, z);
            _mm256_store_ps ((float*)&out[i], s);
        }

        _mm256_zeroupper();
    }

    void add_mul_const(float* out, const float* in, const float k, size_t size)
    {
        assert ((size & 0x7) == 0);
        assert (is_aligned (out, 32));
        assert (is_aligned (in, 32));


        __m256 kv = _mm256_set1_ps (k);

        for (size_t i = 0; i < size; i += 8) {
            __m256 x = _mm256_load_ps ((float*)&out[i]);
            __m256 y = _mm256_load_ps ((const float*)&in[i]);
            x = _mm256_add_ps (x, y);
            x = _mm256_mul_ps (x, kv);
            _mm256_store_ps ((float*)&out[i], x);
        }

        _mm256_zeroupper();
    }

    void mul_const(float* out, const float k, size_t size)
    {
        assert ((size & 0x7) == 0);
        assert (is_aligned (out, 32));

        __m256 kv = _mm256_set1_ps (k);

        for (size_t i = 0; i < size; i += 8) {
            __m256 a = _mm256_load_ps ((float*)&out[i]);
            __m256 b = _mm256_mul_ps (a, kv);
            _mm256_store_ps ((float*)&out[i], b);
        }

        _mm256_zeroupper();
    }

    float mul_reduce(const float* x, const float* y, size_t size)
    {
        assert ((size & 0x7) == 0);
        assert (is_aligned (x, 32));
        assert (is_aligned (y, 32));

        __m256 sum = _mm256_set1_ps (0.0f);

        for (size_t i = 0; i < size; i += 8) {
            __m256 vx = _mm256_load_ps ((const float*)&x[i]);
            __m256 vy = _mm256_load_ps ((const float*)&y[i]);
            __m256 vs = _mm256_mul_ps (vx, vy);
            sum = _mm256_add_ps (sum, vs);
        }

        sum = _mm256_hadd_ps (sum, sum);
        sum = _mm256_hadd_ps (sum, sum);
        sum = _mm256_hadd_ps (sum, sum);

        alignas (32) float vec[8];
        _mm256_store_ps (vec, sum);

        _mm256_zeroupper();

        return vec[0];
    }

    float mul_reduce_unaligned(const float* x, const float* y, size_t size)
    {
        assert ((size & 0x07) == 0);

        float sum = 0.0f;
        __m256 vsum = _mm256_set1_ps (0.0f);

        size_t i = 0;
        size_t n = size - (size % 8);

        if (n > 0) {
            for (i = 0; i < n; i += 8) {
                __m256 vx = _mm256_loadu_ps ((const float*)&x[i]);
                __m256 vy = _mm256_loadu_ps ((const float*)&y[i]);
                __m256 vs = _mm256_mul_ps (vx, vy);
                vsum = _mm256_add_ps (vsum, vs);
            }

            vsum = _mm256_hadd_ps (vsum, vsum);
            vsum = _mm256_hadd_ps (vsum, vsum);
            vsum = _mm256_hadd_ps (vsum, vsum);

            alignas (32) float vec[8];
            _mm256_store_ps (vec, vsum);

            sum = vec[0];
        }

        for (; i < size; ++i)
            sum += x[i] * y[i];

        _mm256_zeroupper();

        return sum;
    }

    void complex_mul(float* res, const float* a, const float* b, size_t size)
    {
        if (size < 8) {
            sse::complex_mul (res, a, b, size);
            return;
        }

        assert ((size & 0x07) == 0);
        assert (is_aligned (res, 32));
        assert (is_aligned (a, 32));
        assert (is_aligned (b, 32));


        __m256 factors = _mm256_set_ps (1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f);

        for (size_t i = 0; i < size; i += 8) {
            __m256 a01 = _mm256_load_ps ((const float*)&a[i]); // [a0.r, a0.i, a1.r, a1.i]
            __m256 b01 = _mm256_load_ps ((const float*)&b[i]); // [b0.r, b0.i, b1.r, b1.i]

            __m256 b01r = _mm256_shuffle_ps (b01, b01, 0xA0); // [b0.r, b0.r, b1.r, b1.r] 
            __m256 b01i = _mm256_shuffle_ps (b01, b01, 0xF5); // [b0.i, b0.i, b1.i, b1.i]

            __m256 a01ir = _mm256_shuffle_ps (a01, a01, 0xB1); // [a0.i, a0.r, a1.i, a1.r]

            __m256 r0 = _mm256_mul_ps (a01, b01r);      // [a0.r*b0.r, a0.i*b0.r, a1.r*b1.r, a1.i*b1.r]
            __m256 r3 = _mm256_mul_ps (a01ir, b01i);    // [a0.i*b0.i, a0.r*b0.i, a1.i*b1.i, a1.r*b1.i]
            r3 = _mm256_mul_ps (r3, factors);           // [-a0.i*b0.i, a0.r*b0.i, -a1.i*b1.i, a1.r*b1.i]
            r3 = _mm256_add_ps (r0, r3);                // [a0.r*b0.r - a0.i*b0.i, a0.i*b0.r + a0.r*b0.i,
                                                     //  a1.r*b1.r - a1.i*b1.i, a1.i*b1.r + a1.r*b1.i]
            _mm256_store_ps ((float*)&res[i], r3);
        }

        _mm256_zeroupper();
    }

    void complex_mul_conj(float* res, const float* a, const float* b, size_t size)
    {
        if (size < 8) {
            sse::complex_mul_conj (res, a, b, size);
            return;
        }

        assert ((size & 0x7) == 0);
        assert (is_aligned (res, 32));
        assert (is_aligned (a, 32));
        assert (is_aligned (b, 32));

        __m256 factors = _mm256_set_ps (1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f);
        __m256 conj = _mm256_set_ps (-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

        for (size_t i = 0; i < size; i += 8) {
            __m256 a01 = _mm256_load_ps ((const float*)&a[i]); // [a0.r, a0.i, a1.r, a1.i]
            __m256 b01 = _mm256_load_ps ((const float*)&b[i]); // [b0.r, b0.i, b1.r, b1.i]

            __m256 b01r = _mm256_shuffle_ps (b01, b01, 0xA0); // [b0.r, b0.r, b1.r, b1.r]
            __m256 b01i = _mm256_shuffle_ps (b01, b01, 0xF5); // [b0.i, b0.i, b1.i, b1.i]

            __m256 a01ir = _mm256_shuffle_ps (a01, a01, 0xB1); // [a0.i, a0.r, a1.i, a1.r]

            __m256 r0 = _mm256_mul_ps (a01, b01r);      // [a0.r*b0.r, a0.i*b0.r, a1.r*b1.r, a1.i*b1.r]
            __m256 r3 = _mm256_mul_ps (a01ir, b01i);    // [a0.i*b0.i, a0.r*b0.i, a1.i*b1.i, a1.r*b1.i]

            r3 = _mm256_mul_ps (r3, factors);           // [-a0.i*b0.i, a0.r*b0.i, -a1.i*b1.i, a1.r*b1.i]
            r3 = _mm256_add_ps (r0, r3);                // [a0.r*b0.r - a0.i*b0.i, a0.i*b0.r + a0.r*b0.i,
                                                        //  a1.r*b1.r - a1.i*b1.i, a1.i*b1.r + a1.r*b1.i]
            r3 = _mm256_mul_ps (r3, conj);

            _mm256_store_ps ((float*)&res[i], r3);
        }

        _mm256_zeroupper();
    }

    void fft_step(float* data, const float* w, size_t n)
    {
        assert ((n & 0x7) == 0);
        assert (is_aligned (data, 32));
        assert (is_aligned (w, 32));

        float* dataN = &data[n];

        __m256 factors = _mm256_set_ps (1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f);

        for (size_t i = 0; i < n; i += 8) {
            // Complex multiplication
            __m256 a01 = _mm256_load_ps ((const float*)&dataN[i]);
            __m256 b01 = _mm256_load_ps ((const float*)&w[i]);

            __m256 b01r = _mm256_shuffle_ps (b01, b01, 0xA0);
            __m256 b01i = _mm256_shuffle_ps (b01, b01, 0xF5);

            __m256 a01ir = _mm256_shuffle_ps (a01, a01, 0xB1);

            __m256 r0 = _mm256_mul_ps (a01, b01r);
            __m256 r = _mm256_mul_ps (a01ir, b01i);
            r = _mm256_mul_ps (r, factors);
            r = _mm256_add_ps (r0, r);


            r0 = _mm256_load_ps ((const float*)&data[i]);
            __m256 r1 = _mm256_sub_ps (r0, r);
            _mm256_store_ps ((float*)&dataN[i], r1);

            __m256 r2 = _mm256_add_ps (r0, r);
            _mm256_store_ps ((float*)&data[i], r2);
        }

        _mm256_zeroupper();
    }

    namespace fma {

        void mul_const_add(float* out, const float* in, const float k, size_t size)
        {
            assert ((size & 0x7) == 0);
            assert (is_aligned (out, 32));
            assert (is_aligned (in, 32));

            __m256 kv = _mm256_set1_ps (k);

            for (size_t i = 0; i < size; i += 8) {
                __m256 x = _mm256_load_ps ((float*)&out[i]);
                __m256 y = _mm256_load_ps ((const float*)&in[i]);
                __m256 s = _mm256_fmadd_ps (y, kv, x);
                _mm256_store_ps ((float*)&out[i], s);
            }

            _mm256_zeroupper();
        }


        float mul_reduce(const float* x, const float* y, size_t size)
        {
            assert ((size & 0x7) == 0);
            assert (is_aligned (x, 32));
            assert (is_aligned (y, 32));

            __m256 sum = _mm256_set1_ps (0.0f);

            for (size_t i = 0; i < size; i += 8) {
                __m256 vx = _mm256_load_ps ((const float*)&x[i]);
                __m256 vy = _mm256_load_ps ((const float*)&y[i]);
                sum = _mm256_fmadd_ps (vx, vy, sum);
            }

            sum = _mm256_hadd_ps (sum, sum);
            sum = _mm256_hadd_ps (sum, sum);

            alignas (32) float vec[8];
            _mm256_store_ps (vec, sum);

            _mm256_zeroupper();

            return vec[0] + vec[4];
        }

        float mul_reduce_unaligned(const float* x, const float* y, size_t size)
        {
            assert ((size & 0x07) == 0);

            float sum = 0.0f;
            __m256 vsum = _mm256_set1_ps (0.0f);

            size_t i = 0;
            size_t n = size - (size % 8);

            if (n > 0) {
                for (i = 0; i < n; i += 8) {
                    __m256 vx = _mm256_loadu_ps ((const float*)&x[i]);
                    __m256 vy = _mm256_loadu_ps ((const float*)&y[i]);
                    vsum = _mm256_fmadd_ps (vx, vy, vsum);
                }

                vsum = _mm256_hadd_ps (vsum, vsum);
                vsum = _mm256_hadd_ps (vsum, vsum);

                alignas (32) float vec[8];
                _mm256_store_ps (vec, vsum);

                sum = vec[0] + vec[4];
            }

            for (; i < size; ++i)
                sum += x[i] * y[i];

            _mm256_zeroupper();

            return sum;
        }

        void complex_mul(float* res, const float* a, const float* b, size_t size)
        {
            if (size < 8) {
                sse::complex_mul (res, a, b, size);
                return;
            }

            assert ((size & 0x07) == 0);
            assert (is_aligned (res, 32));
            assert (is_aligned (a, 32));
            assert (is_aligned (b, 32));


            __m256 factors = _mm256_set_ps (1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f);

            for (size_t i = 0; i < size; i += 8) {
                __m256 a01 = _mm256_load_ps ((const float*)&a[i]); // [a0.r, a0.i, a1.r, a1.i]
                __m256 b01 = _mm256_load_ps ((const float*)&b[i]); // [b0.r, b0.i, b1.r, b1.i]

                __m256 b01r = _mm256_shuffle_ps (b01, b01, 0xA0); // [b0.r, b0.r, b1.r, b1.r] 
                __m256 b01i = _mm256_shuffle_ps (b01, b01, 0xF5); // [b0.i, b0.i, b1.i, b1.i]

                __m256 a01ir = _mm256_shuffle_ps (a01, a01, 0xB1); // [a0.i, a0.r, a1.i, a1.r]

                __m256 r0 = _mm256_mul_ps (a01, b01r);      // [a0.r*b0.r, a0.i*b0.r, a1.r*b1.r, a1.i*b1.r]
                __m256 r3 = _mm256_mul_ps (a01ir, b01i);    // [a0.i*b0.i, a0.r*b0.i, a1.i*b1.i, a1.r*b1.i]
                r3 = _mm256_fmadd_ps (r3, factors, r0);     // [-a0.i*b0.i, a0.r*b0.i, -a1.i*b1.i, a1.r*b1.i]
                                                            // [a0.r*b0.r - a0.i*b0.i, a0.i*b0.r + a0.r*b0.i,
                                                            //  a1.r*b1.r - a1.i*b1.i, a1.i*b1.r + a1.r*b1.i]
                _mm256_store_ps ((float*)&res[i], r3);
            }

            _mm256_zeroupper();
        }


        void complex_mul_conj(float* res, const float* a, const float* b, size_t size)
        {
            if (size < 8) {
                sse::complex_mul_conj (res, a, b, size);
                return;
            }

            assert ((size & 0x7) == 0);
            assert (is_aligned (res, 32));
            assert (is_aligned (a, 32));
            assert (is_aligned (b, 32));


            __m256 factors = _mm256_set_ps (1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f);
            __m256 conj = _mm256_set_ps (-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

            for (size_t i = 0; i < size; i += 8) {
                __m256 a01 = _mm256_load_ps ((const float*)&a[i]); // [a0.r, a0.i, a1.r, a1.i]
                __m256 b01 = _mm256_load_ps ((const float*)&b[i]); // [b0.r, b0.i, b1.r, b1.i]

                __m256 b01r = _mm256_shuffle_ps (b01, b01, 0xA0); // [b0.r, b0.r, b1.r, b1.r]
                __m256 b01i = _mm256_shuffle_ps (b01, b01, 0xF5); // [b0.i, b0.i, b1.i, b1.i]

                __m256 a01ir = _mm256_shuffle_ps (a01, a01, 0xB1); // [a0.i, a0.r, a1.i, a1.r]

                __m256 r0 = _mm256_mul_ps (a01, b01r);      // [a0.r*b0.r, a0.i*b0.r, a1.r*b1.r, a1.i*b1.r]
                __m256 r3 = _mm256_mul_ps (a01ir, b01i);    // [a0.i*b0.i, a0.r*b0.i, a1.i*b1.i, a1.r*b1.i]

                r3 = _mm256_fmadd_ps (r3, factors, r0);     // [-a0.i*b0.i, a0.r*b0.i, -a1.i*b1.i, a1.r*b1.i]
                                                            // [a0.r*b0.r - a0.i*b0.i, a0.i*b0.r + a0.r*b0.i,
                                                            //  a1.r*b1.r - a1.i*b1.i, a1.i*b1.r + a1.r*b1.i]
                r3 = _mm256_mul_ps (r3, conj);

                _mm256_store_ps ((float*)&res[i], r3);
            }

            _mm256_zeroupper();
        }

        void fft_step(float* data, const float* w, size_t n)
        {
            assert ((n & 0x7) == 0);
            assert (is_aligned (data, 16));
            assert (is_aligned (w, 16));

            float* dataN = &data[n];

            __m256 factors = _mm256_set_ps (1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f);

            for (size_t i = 0; i < n; i += 8) {
                // Complex multiplication
                __m256 a01 = _mm256_load_ps ((const float*)&dataN[i]);
                __m256 b01 = _mm256_load_ps ((const float*)&w[i]);

                __m256 b01r = _mm256_shuffle_ps (b01, b01, 0xA0);
                __m256 b01i = _mm256_shuffle_ps (b01, b01, 0xF5);

                __m256 a01ir = _mm256_shuffle_ps (a01, a01, 0xB1);

                __m256 r0 = _mm256_mul_ps (a01, b01r);
                __m256 r = _mm256_mul_ps (a01ir, b01i);
                r = _mm256_fmadd_ps (r, factors, r0);


                r0 = _mm256_load_ps ((const float*)&data[i]);
                __m256 r1 = _mm256_sub_ps (r0, r);
                _mm256_store_ps ((float*)&dataN[i], r1);

                __m256 r2 = _mm256_add_ps (r0, r);
                _mm256_store_ps ((float*)&data[i], r2);
            }

            _mm256_zeroupper();
        }

    } // namespace fma

} // namespace avx
#endif

#endif // SIMD

//------------------------------------------------------------------------------

void  (*simd::add)(float*, const float*, size_t)                            = &no_simd::add;
void  (*simd::mul_const_add)(float*, const float*, const float, size_t)     = &no_simd::mul_const_add;
void  (*simd::add_mul_const)(float*, const float*, const float, size_t)     = &no_simd::add_mul_const;
void  (*simd::mul_const)(float*, const float, size_t)                       = &no_simd::mul_const;
float (*simd::mul_reduce)(const float*, const float*, size_t)               = &no_simd::mul_reduce;
float (*simd::mul_reduce_unaligned)(const float*, const float*, size_t)     = &no_simd::mul_reduce;
void  (*simd::complex_mul)(float*, const float*, const float*, size_t)      = &no_simd::complex_mul;
void  (*simd::complex_mul_conj)(float*, const float*, const float*, size_t) = &no_simd::complex_mul_conj;
void  (*simd::fft_step)(float*, const float*, size_t)                       = &no_simd::fft_step;

#ifdef SIMD

static const bool simd_map = []() -> bool {

    const auto cpu = CPUTraits::get();

    if (cpu.sse) {
        simd::add                  = &sse::add;
        simd::mul_const_add        = &sse::mul_const_add;
        simd::add_mul_const        = &sse::add_mul_const;
        simd::mul_const            = &sse::mul_const;
        simd::mul_reduce           = &sse::mul_reduce;
        simd::mul_reduce_unaligned = &sse::mul_reduce_unaligned;
        simd::complex_mul          = &sse::complex_mul;
        simd::complex_mul_conj     = &sse::complex_mul_conj;
        simd::fft_step             = &sse::fft_step;

#if 0
        if (cpu.fma) {
            simd::mul_const_add        = &sse::fma::mul_const_add;
            simd::mul_reduce           = &sse::fma::mul_reduce;
            simd::mul_reduce_unaligned = &sse::fma::mul_reduce_unaligned;
            simd::complex_mul          = &sse::fma::complex_mul;
            simd::complex_mul_conj     = &sse::fma::complex_mul_conj;
            simd::fft_step             = &sse::fma::fft_step;
        }
#endif
    }

#if 0
    if (cpu.avx) {
        simd::add                  = &avx::add;
        simd::mul_const_add        = &avx::mul_const_add;
        simd::add_mul_const        = &avx::add_mul_const;
        simd::mul_const            = &avx::mul_const;
        simd::mul_reduce           = &avx::mul_reduce;
        simd::mul_reduce_unaligned = &avx::mul_reduce_unaligned;
        simd::complex_mul          = &avx::complex_mul;
        simd::complex_mul_conj     = &avx::complex_mul_conj;
        simd::fft_step             = &avx::fft_step;

        if (cpu.fma) {
            simd::mul_const_add        = &avx::fma::mul_const_add;
            simd::mul_reduce           = &avx::fma::mul_reduce;
            simd::mul_reduce_unaligned = &avx::fma::mul_reduce_unaligned;
            simd::complex_mul          = &avx::fma::complex_mul;
            simd::complex_mul_conj     = &avx::fma::complex_mul_conj;
            simd::fft_step             = &avx::fma::fft_step;
        }
    }
#endif

    return true;
}();

#endif

AEOLUS_NAMESPACE_END
