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
