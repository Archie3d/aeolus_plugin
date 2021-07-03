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

Interpolator::Interpolator(float ratio, size_t nChannels)
    : _acc(nChannels)
    , _accIndex{0}
    , _accFrac{0.0f}
    , _ratio{ratio}
{
    jassert(nChannels > 0);
}

void Interpolator::setNumberOfChannels(size_t n)
{
    jassert(n > 0);
    _acc.resize(n);
    reset();
}

void Interpolator::reset()
{
    for (auto& buf : _acc) {
        ::memset(buf.data(), 0, sizeof(float) * 8);
    }

    _accIndex = 0;
    _accFrac = 0.0f;
}

bool Interpolator::canRead() const noexcept
{
    return _accFrac < 1.0f;
}

bool Interpolator::readAllChannels(float* const x) noexcept
{
    jassert(x != nullptr);

    if (_accFrac >= 1.0f)
        return false;

    for (size_t i = 0; i < _acc.size(); ++i) {
        x[i] = math::lagr(&_acc[i].data()[_accIndex], _accFrac);
    }

    _accFrac += _ratio;

    return true;
}

bool Interpolator::read(float& x, size_t channel, bool increment)
{
    if (_accFrac >= 1.0f)
        return false;

    x = math::lagr(&_acc[channel].data()[_accIndex], _accFrac);

    if (increment) {
        _accFrac += _ratio;
    }

    return true;
}

bool Interpolator::read(float& l, float& r) noexcept
{
    jassert(_acc.size() > 1);

    if (_accFrac >= 1.0f)
        return false;

    l = math::lagr(&_acc[0].data()[_accIndex], _accFrac);
    r = math::lagr(&_acc[1].data()[_accIndex], _accFrac);

    _accFrac += _ratio;

    return true;
}

bool Interpolator::canWrite() const noexcept
{
    return _accFrac >= 1.0f;
}

bool Interpolator::writeAllChannels(const float* const x) noexcept
{
    jassert(x != nullptr);

    if (_accFrac < 1.0f)
        return false;

    for (size_t i = 0; i < _acc.size(); ++i) {
        _acc[i][_accIndex] = _acc[i][_accIndex + 4] = x[i];
    }

    _accIndex = (_accIndex + 1) % 4;
    _accFrac -= 1.0f;

    return true;
}

bool Interpolator::write(float x, size_t channel, bool increment)
{
    jassert(_acc.size() > 1);

    if (_accFrac < 1.0f)
        return false;

    _acc[channel][_accIndex] = _acc[channel][_accIndex + 4] = x;

    if (increment) {
        _accIndex = (_accIndex + 1) % 4;
        _accFrac -= 1.0f;
    }

    return true;
}

bool Interpolator::write(float l, float r) noexcept
{
    jassert(_acc.size() > 1);

    if (_accFrac < 1.0f)
        return false;

    _acc[0][_accIndex] = _acc[0][_accIndex + 4] = l;
    _acc[1][_accIndex] = _acc[1][_accIndex + 4] = r;

    _accIndex = (_accIndex + 1) % 4;
    _accFrac -= 1.0f;

    return true;
}

} // namespace dsp

AEOLUS_NAMESPACE_END
