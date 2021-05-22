
#include "aeolus/dsp/delay.h"

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

DelayLine::DelayLine(size_t size)
    : _buffer(size, 0.0f)
    , _writeIndex(0)
{
}

void DelayLine::resize (size_t size)
{
    _buffer.resize(size);
    reset();
}

void DelayLine::reset()
{
    _writeIndex = 0;
    ::memset (_buffer.data(), 0, sizeof (float) * _buffer.size());
}

void DelayLine::write (float x)
{
    if (_writeIndex == 0)
        _writeIndex = _buffer.size() - 1;
    else
        --_writeIndex;

    _buffer[_writeIndex] = x;
}

float DelayLine::read (float delay) const
{
    int index = (int)std::floor (delay);
    float frac = delay - (float)index;

    index = (index + _writeIndex) % (int) _buffer.size();
    const auto a = _buffer[index];
    const auto b = index < _buffer.size() - 1 ? _buffer[index + 1] : _buffer[0];

    return math::lerp(a, b, frac);
}

} // namespace dsp

AEOLUS_NAMESPACE_END
