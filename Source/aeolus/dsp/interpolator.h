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

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

/**
 * @brief Lagrange interpolator.
 * 
 * @note This class does not apply an interpolation filter when downsampling.
 */
class Interpolator
{
public:
    Interpolator(float ratio = 1.0f);

    void setRatio(float r) noexcept { _ratio = r; }
    float getRatio() const noexcept { return _ratio; }

    void reset();

    bool canRead() const noexcept;
    bool read(float& l, float& r) noexcept;

    bool canWrite() const noexcept;
    bool write(float l, float r) noexcept;

private:
    float _accL[8];
    float _accR[8];
    int _accIndex;
    float _accFrac;

    float _ratio;
};

} // namespace dsp

AEOLUS_NAMESPACE_END

