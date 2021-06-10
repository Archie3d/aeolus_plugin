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

#include "spatial.h"

AEOLUS_NAMESPACE_BEGIN

using namespace juce;

namespace dsp {

SpatialSource::SpatialSource()
    : _sampleRate{SAMPLE_RATE}
    , _sourcePosition{0.0f, 0.0f}
    , _listenerPosition{0.0f, 0.0f}
    , _listenerOrientation{0.0f}
    , _listenerLeftRightDistance{0.3f}
    , _delayLine{}
    , _filterSpec{}
    , _filterState{}
{
    recalculate();
}

void SpatialSource::reset()
{
    _delayLine.reset();

    BiquadFilter::resetState(_filterSpec[0], _filterState[0]);
    BiquadFilter::resetState(_filterSpec[1], _filterState[1]);
}

void SpatialSource::tick(float x, float& l, float& r)
{
    _delayLine.write(x);

    l = BiquadFilter::tick(_filterSpec[0], _filterState[0], _delayLine.read(_leftDelay) * _leftAttenuation);
    r = BiquadFilter::tick(_filterSpec[1], _filterState[1], _delayLine.read(_rightDelay) * _rightAttenuation);
}

void SpatialSource::process(float* in, float* outL, float* outR, int numFrames)
{
    for (int i = 0; i < numFrames; ++i) {
        tick(in[i], outL[i], outR[i]);
    }
}

static float distanceToCutOffFrequency(float d)
{
    return 22.0e3f * expf(-0.09f * d);
}

void SpatialSource::recalculate()
{
    constexpr float speedOfSound = 330.0f; // [m/s]

    Position left{-0.5f * _listenerLeftRightDistance, 0.0f};
    Position right{0.5f * _listenerLeftRightDistance, 0.0f};
    left.rotate(_listenerOrientation);
    right.rotate(_listenerOrientation);

    Position sourceRelativeToListener{_sourcePosition.x - _listenerPosition.x, _sourcePosition.y - _listenerPosition.y};
    float leftAngle = left.angleTo(sourceRelativeToListener);
    float rightAngle = right.angleTo(sourceRelativeToListener);

    left.x += _listenerPosition.x;
    left.y += _listenerPosition.y;
    right.x += _listenerPosition.x;
    right.y += _listenerPosition.y;

    const float leftDistance = _sourcePosition.distanceTo(left);
    const float rightDistance = _sourcePosition.distanceTo(right);
    const float maxDistance = std::max(leftDistance, rightDistance);
    const float maxT = maxDistance / speedOfSound;
    size_t delayLengthInSamples = (size_t)(_sampleRate * maxT + 0.5f);

    _delayLine.resize(delayLengthInSamples);

    _leftDelay = leftDistance * _sampleRate / speedOfSound;
    _rightDelay = rightDistance * _sampleRate / speedOfSound;

    constexpr float att = 0.7f; // [0..1]

    // Angular attenuation
    _leftAttenuation = 0.5f * att * (cosf(leftAngle) + 1.0f) + 1.0f - att;
    _rightAttenuation = 0.5f * att * (cosf(rightAngle) + 1.0f) + 1.0f - att;

    _filterSpec[0].type = BiquadFilter::LowPass;
    _filterSpec[0].dbGain = 0.0f;
    _filterSpec[0].q = 0.7071f;
    _filterSpec[0].sampleRate = SAMPLE_RATE;

    _filterSpec[1] = _filterSpec[0];

    _filterSpec[0].freq = distanceToCutOffFrequency(leftDistance);
    _filterSpec[1].freq = distanceToCutOffFrequency(rightDistance);

    BiquadFilter::updateSpec(_filterSpec[0]);
    BiquadFilter::updateSpec(_filterSpec[1]);
}

} // namespace dsp

AEOLUS_NAMESPACE_END