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
#include "aeolus/dsp/delay.h"
#include "aeolus/dsp/filter.h"

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

/**
 * @brief Sound source spatial modeller.
 *
 * This class take mono audio source and models a stereo output
 * based on the source and listener relative positions.
 * All positioning is performed in 2D space. Positions are specified in meters.
 */
class SpatialSource
{
public:

    struct Position
    {
        float x;
        float y;

        void rotate(float a)
        {
            const float c = cosf(a);
            const float s = sinf(a);
            const float x2 = c * x - s * y;
            const float y2 = s * x + s * y;
            x = x2;
            y = y2;
        }

        float distanceTo(const Position& other) const noexcept
        {
            return sqrt((other.x - x) * (other.x - x) + (other.y - y) * (other.y - y));
        }

        float angleTo(const Position& other) const noexcept
        {
            return atan2f(other.y, other.x) - atan2f(y, x);
        }
    };

    SpatialSource();

    void reset();

    void tick(float x, float& l, float& r);

    void process(float* in, float* outL, float* outR, int numFrames);

    void setSampleRate(float sr) { _sampleRate = sr; }
    void setSourcePosition(float x, float y) noexcept { _sourcePosition = {x, y}; }
    void setListenerPosition(float x, float y) noexcept { _listenerPosition = {x, y}; }

    void recalculate();

    size_t getPostFxSamplesCount() const { return _delayLine.size(); }

private:

    float _sampleRate;

    Position _sourcePosition;
    Position _listenerPosition;
    float _listenerOrientation;
    float _listenerLeftRightDistance;

    DelayLine _delayLine;
    int _leftDelay;
    int _rightDelay;
    float _leftAttenuation;
    float _rightAttenuation;

    // Attenuation filters
    BiquadFilter::Spec _filterSpec[2];
    BiquadFilter::State _filterState[2];
};

} // namespace dsp

AEOLUS_NAMESPACE_END