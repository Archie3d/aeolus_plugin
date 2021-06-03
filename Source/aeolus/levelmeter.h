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

#include <atomic>

AEOLUS_NAMESPACE_BEGIN

class LevelMeter
{
public:

    LevelMeter();

    float getPeakLevel() const noexcept { return _peak; }
    float getRMSLevel() const noexcept { return _rms; }

    void process(const juce::AudioBuffer<float>& buffer, int channel);
    void process(float* const buffer, int size);

private:
    std::atomic<float> _peak;
    std::atomic<float> _rms;
};

AEOLUS_NAMESPACE_END
