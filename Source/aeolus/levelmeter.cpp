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

#include "aeolus/levelmeter.h"

using namespace juce;

AEOLUS_NAMESPACE_BEGIN

LevelMeter::LevelMeter()
    : _peak{0.0f}
    , _rms{0.0f}
{
}

LevelMeter::LevelMeter(const LevelMeter& other)
    : _peak{other._peak.load()}
    , _rms{other._rms.load()}
{
}

LevelMeter& LevelMeter::operator = (const LevelMeter& other)
{
    if (this != &other) {
        _peak = other._peak.load();
        _rms = other._rms.load();
    }

    return *this;
}

void LevelMeter::process(const AudioBuffer<float>& buffer)
{
    float peak{0.0f};
    float rms{0.0f};

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        peak = jmax(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
        rms = jmax(rms, buffer.getRMSLevel(ch, 0, buffer.getNumSamples()));
    }

    _peak = peak;
    _rms = rms;
}

void LevelMeter::process(const AudioBuffer<float>& buffer, int channel)
{
    _peak = buffer.getMagnitude(channel, 0, buffer.getNumSamples());
    _rms = buffer.getRMSLevel(channel, 0, buffer.getNumSamples());
}

void LevelMeter::process(float* const buffer, int size)
{
    float* const layout[1] = {buffer};
    AudioBuffer<float> ab(layout, 1, size);
    _peak = ab.getMagnitude(0, 0, size);
    _rms = ab.getRMSLevel(0, 0, size);
}

AEOLUS_NAMESPACE_END
