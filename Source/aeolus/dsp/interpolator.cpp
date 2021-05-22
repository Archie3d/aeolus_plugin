
#include "aeolus/dsp/interpolator.h"

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

Interpolator::Interpolator(float ratio)
    : _accL{0.0f}
    , _accR{0.0f}
    , _accIndex{0}
    , _accFrac{0.0f}
    , _ratio{ratio}
{

}

void Interpolator::reset()
{
    ::memset (_accL, 0, sizeof (float) * 8);
    ::memset (_accR, 0, sizeof (float) * 8);
    _accIndex = 0;
    _accFrac = 0.0f;
}

bool Interpolator::canRead() const noexcept
{
    return _accFrac < 1.0f;
}

bool Interpolator::read(float& l, float& r) noexcept
{
    if (_accFrac >= 1.0f)
        return false;

    l = math::lagr(&_accL[_accIndex], _accFrac);
    r = math::lagr(&_accR[_accIndex], _accFrac);

    _accFrac += _ratio;

    return true;
}

bool Interpolator::canWrite() const noexcept
{
    return _accFrac >= 1.0f;
}

bool Interpolator::write(float l, float r) noexcept
{
    if (_accFrac < 1.0f)
        return false;

    _accL[_accIndex] = _accL[_accIndex + 4] = l;
    _accR[_accIndex] = _accR[_accIndex + 4] = r;

    _accIndex = (_accIndex + 1) % 4;
    _accFrac -= 1.0f;

    return true;
}

} // namespace dsp

AEOLUS_NAMESPACE_END
