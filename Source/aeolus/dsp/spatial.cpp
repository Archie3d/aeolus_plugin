#include "spatial.h"

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

SpatialSource::SpatialSource()
    : _sampleRate{SAMPLE_RATE}
    , _sourcePosition{0.0f, 0.0f}
    , _listenerPosition{0.0f, 0.0f}
    , _listenerOrientation{0.0f}
    , _listenerLeftRightDistance{0.3f}
    , _delayLine{}
{
    recalculate();
}

void SpatialSource::reset()
{
    _delayLine.reset();
}

void SpatialSource::tick(float x, float& l, float& r)
{
    _delayLine.write(x);

    l = _delayLine.read(_leftDelay) * _leftAttenuation;
    r = _delayLine.read(_rightDelay) * _rightAttenuation;
}

void SpatialSource::process(float* in, float* outL, float* outR, int numFrames)
{
    for (int i = 0; i < numFrames; ++i) {
        tick(in[i], outL[i], outR[i]);
    }
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

    float leftDistance = _sourcePosition.distanceTo(left);
    float rightDistance = _sourcePosition.distanceTo(right);
    float maxDistance = std::max(leftDistance, rightDistance);
    float maxT = maxDistance / speedOfSound;
    size_t delayLengthInSamples = (size_t)(_sampleRate * maxT + 0.5f);

    _delayLine.resize(delayLengthInSamples);

    _leftDelay = leftDistance * _sampleRate / speedOfSound;
    _rightDelay = rightDistance * _sampleRate / speedOfSound;

    constexpr float att = 0.7f; // [0..1]

    // Angular attenuation
    _leftAttenuation = 0.5f * att * (cosf(leftAngle) + 1.0f) + 1.0f - att;
    _rightAttenuation = 0.5f * att * (cosf(rightAngle) + 1.0f) + 1.0f - att;
}

} // namespace dsp

AEOLUS_NAMESPACE_END