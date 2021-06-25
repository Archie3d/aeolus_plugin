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

#include "aeolus/dsp/fft.h"

#include <cassert>
#include <cmath>
#include <map>
#include <functional>

using namespace juce;

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

static float hann(int i, int n)
{
    return 0.5f * (1.0f - std::cos (MathConstants<float>::twoPi * i / (n - 1)));
}

static float hamming(int i, int n)
{
    return 0.53836f + 0.46164f * std::cos (MathConstants<float>::twoPi * i / (n - 1));
}

static float blackman(int i, int n)
{
    const auto x = MathConstants<float>::twoPi * i / (n - 1);
    return 0.42659f - 0.49656f * std::cos (x) + 0.076849f * std::cos (2.0f * x);
}

void Fft::direct(Fft::Array& x, Fft::Window win)
{
    applyWindow(x, win);

    // DFT
    unsigned int N = (unsigned int) x.size(), k = N, n;
    float thetaT = MathConstants<float>::pi / N;
    Complex phiT = Complex (std::cos (thetaT), std::sin (thetaT)), T;

    while (k > 1) {
        n = k;
        k >>= 1;
        phiT = phiT * phiT;
        T = 1.0L;

        for (unsigned int l = 0; l < k; l++) {
            for (unsigned int a = l; a < N; a += n) {
                unsigned int b = a + k;
                Complex t = x[a] - x[b];
                x[a] += x[b];
                x[b] = t * T;
            }

            T *= phiT;
        }
    }

    // Decimate
    unsigned int m = (unsigned int) log2 (N);

    for (unsigned int a = 0; a < N; a++) {
        unsigned int b = a;
        // Reverse bits
        b = (((b & 0xaaaaaaaa) >> 1) | ((b & 0x55555555) << 1));
        b = (((b & 0xcccccccc) >> 2) | ((b & 0x33333333) << 2));
        b = (((b & 0xf0f0f0f0) >> 4) | ((b & 0x0f0f0f0f) << 4));
        b = (((b & 0xff00ff00) >> 8) | ((b & 0x00ff00ff) << 8));
        b = ((b >> 16) | (b << 16)) >> (32 - m);

        if (b > a) {
            Complex t = x[a];
            x[a] = x[b];
            x[b] = t;
        }
    }

}

void Fft::inverse(Array &x)
{
    // conjugate the complex numbers
    x = x.apply(std::conj);

    // forward fft
    direct (x);

    // conjugate the complex numbers again
    x = x.apply(std::conj);

    // scale the numbers
    x /= (float) x.size();
}

void Fft::applyWindow(Fft::Array&x, Fft::Window win)
{
    switch (win) {
    case Fft::Window::None:
        break;
    case Fft::Window::Hann:
        for (int i = 0; i < x.size(); ++i)
            x[i] *= hann(i, (int) x.size());
        break;
    case Fft::Window::Hamming:
        for (int i = 0; i < x.size(); ++i)
            x[i] *= hamming(i, (int) x.size());
        break;
    case Fft::Window::Blackman:
        for (int i = 0; i < x.size(); ++i)
            x[i] *= blackman(i, (int) x.size());
        break;
    default:
        break;
    }
}

} // namespace dsp

AEOLUS_NAMESPACE_END
