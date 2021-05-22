#pragma once

#include "aeolus/globals.h"
#include "aeolus/dsp/delay.h"

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
    float _leftDelay;
    float _rightDelay;
    float _leftAttenuation;
    float _rightAttenuation;
};

} // namespace dsp

AEOLUS_NAMESPACE_END