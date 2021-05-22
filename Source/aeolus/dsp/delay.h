#pragma once

#include "aeolus/globals.h"

#include <vector>

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

/**
* @brief Delay line with samples linear interpolation.
*/
class DelayLine
{
public:

    DelayLine(size_t size = 1024);
    void resize(size_t size);
    void reset();
    void write(float x);
    float read(float delay) const;

    size_t size() const { return _buffer.size(); }

private:
    std::vector<float> _buffer;
    size_t _writeIndex;
};


} // namespace dsp

AEOLUS_NAMESPACE_END
