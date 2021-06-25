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
#include <memory>

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

/**
 * @brief Stereo convolution reverb.
 */
class Convolver final
{
public:

    enum Params
    {
        DRY = 0,
        WET,
        GAIN,   // IR sample gain.

        NUM_PARAMS
    };

    // Default parameters set on creation
    constexpr static float DefaultDry  = 0.0f;
    constexpr static float DefaultWet  = 1.0f;
    constexpr static float DefaultGain = 1.0f;

    /// Single convolution block size (in number of samples).
    constexpr static size_t BlockSize = 4096;

    Convolver();
    ~Convolver();

    void setIR(const juce::AudioBuffer<float>& ir);

    void setDryWet(float dry, float wet, bool force = false);
    bool isAudible() const;

    void prepareToPlay(float sampleRate, size_t nFrames);

    void process(const float *inL, const float *inR, float *outL, float *outR, size_t numFrames);

    void setNonRealtime(bool nonRealtime);

    int length() const noexcept;
    void setLength(int len) noexcept;

    bool zeroDelay() const noexcept;
    void setZeroDelay(bool v) noexcept;

protected:

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace dsp

AEOLUS_NAMESPACE_END
