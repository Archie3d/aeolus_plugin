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

/**
 * Parameter with smoothed float value.
 */
class AudioParameter
{
public:

    AudioParameter(float value = 0.0f,
                   float min = 0.0f,
                   float max = 1.0f,
                   float smooth = 0.5f);
    
    void setName(const juce::String& n) { _paramName = n; }
    const juce::String& name() const noexcept { return _paramName; }
    void setValue(float v, float s, bool force = false);
    void setValue(float v, bool force = false);
    void setSmoothing(float s) noexcept;
    void setRange(float min, float max);

    AudioParameter& operator = (float v);

    float value() const noexcept { return _currentValue; }
    float target() const noexcept { return _targetValue; }
    float min() const noexcept { return _minValue; }
    float max() const noexcept { return _maxValue; }
    bool isSmoothing() const noexcept { return _smoothing || _currentValue != _targetValue; }

    float nextValue();

    float& targetRef() noexcept { return _targetValue; }

private:

    void updateSmoothing();

    juce::String _paramName;   ///< Optional parameter name.

    float _currentValue;
    float _minValue;
    float _maxValue;
    float _targetValue;
    float _frac;
    bool _smoothing;
};

//----------------------------------------------------------

class AudioParameterPool
{
public:
    AudioParameterPool(size_t size);
    size_t size() const { return _params.size(); }
    AudioParameter& operator[] (int index);

    AudioParameter& findByName(const juce::String& n);

private:
    std::vector<AudioParameter> _params;
    AudioParameter _dummyParameter;
};

AEOLUS_NAMESPACE_END