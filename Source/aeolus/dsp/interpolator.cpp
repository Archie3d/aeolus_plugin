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
