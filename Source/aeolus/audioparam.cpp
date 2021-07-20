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

#include <climits>
#include "aeolus/audioparam.h"

using namespace juce;

AEOLUS_NAMESPACE_BEGIN

AudioParameter::AudioParameter(float value,
                               float min,
                               float max,
                               float smooth)
    : _currentValue{value}
    , _minValue{min}
    , _maxValue{max}
    , _targetValue{value}
    , _frac{smooth}
    , _smoothing{false}
{
}

void AudioParameter::setValue(float v, float s, bool force)
{
    _targetValue = jlimit(_minValue, _maxValue, v);

    _frac = jlimit(0.0f, 1.0f, s);

    if (force) {
        _currentValue = _targetValue;
        _smoothing = false;
    } else {
        updateSmoothing();
    }
}

void AudioParameter::setValue(float v, bool force)
{
    _targetValue = jlimit(_minValue, _maxValue, v);

    if (force) {
        _currentValue = _targetValue;
        _smoothing = false;
    } else {
        updateSmoothing();
    }
}

void AudioParameter::setSmoothing(float s) noexcept
{
    _frac = jlimit(0.0f, 1.0f, s);
}

void AudioParameter::setRange(float min, float max)
{
    _minValue = jmin(min, max);
    _maxValue = jmax(min, max);
}

AudioParameter& AudioParameter::operator = (float v)
{
    setValue(v);

    return *this;
}

float AudioParameter::nextValue()
{
    updateSmoothing();

    if (_smoothing) {
        const float prevValue { _currentValue };
        _currentValue = _targetValue * _frac + _currentValue * (1.0f - _frac);

        if (std::fabsf(_currentValue - prevValue) <= std::numeric_limits<float>::epsilon()) {
            // No advancement - jump to the target
            _currentValue = _targetValue;
        }
    }

    return _currentValue;
}

void AudioParameter::updateSmoothing()
{
    _smoothing = std::fabsf(_currentValue - _targetValue) > std::numeric_limits<float>::epsilon();

    if (!_smoothing)
        _currentValue = _targetValue;
}

//----------------------------------------------------------

AudioParameterPool::AudioParameterPool (size_t size)
    : _params (size)
{
}

AudioParameter& AudioParameterPool::operator[] (int index)
{
    jassert(index >= 0 && index < (int)_params.size());

    if (index >= 0 && index < (int)_params.size())
        return _params.at(index);

    return _dummyParameter;
}

const AudioParameter& AudioParameterPool::operator[] (int index) const
{
    jassert(index >= 0 && index < (int)_params.size());

    if (index >= 0 && index < (int)_params.size())
        return _params.at(index);

    return _dummyParameter;
}

AudioParameter& AudioParameterPool::findByName(const String& n)
{
    for (auto& p : _params) {
        if (p.name() == n)
            return p;
    }

    return _dummyParameter;
}

AEOLUS_NAMESPACE_END
