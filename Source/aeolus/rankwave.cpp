// ----------------------------------------------------------------------------
//
//  Copyright (C) 2021 Arthur Benilov <arthur.benilov@gmail.com>
//  Copyright (C) 2003-2013 Fons Adriaensen <fons@linuxaudio.org>
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

#include "rankwave.h"
#include "engine.h"

using namespace juce;

AEOLUS_NAMESPACE_BEGIN


Pipewave::Pipewave(Addsynth& model, int note, float freq)
    : _model(model)
    , _note(note)
    , _freq(freq)
    , _needsToBeRebuilt{true}
{

}

float Pipewave::getPipeFrequency() const noexcept
{
    return _freq * _model.getFn() / _model.getFd();
}

void Pipewave::prepateToPlay(float sampleRate)
{
    if (_wavetable.size() == 0 || _sampleRate != sampleRate || _needsToBeRebuilt.load()) {
        _sampleRate = sampleRate;

        genwave();
    }
}

Pipewave::State Pipewave::trigger()
{
    Pipewave::State state = {};

    if (!_needsToBeRebuilt.load()) {
        state.pipewave = this;
        state.env = Pipewave::Attack;
    }

    return state;
}

void Pipewave::release(Pipewave::State& state)
{
    jassert(state.env == Pipewave::Attack || state.env == Pipewave::Release);
    jassert(state.pipewave == this);
    state.env = Pipewave::Release;
}

void Pipewave::play(Pipewave::State& state, float* out)
{
    static Random rnd;

    jassert(out != nullptr);
    jassert(state.env != Pipewave::Idle);
    jassert(_attackStartPtr != nullptr);
    jassert(_loopStartPtr != nullptr);
    jassert(_loopEndPtr != nullptr);

    float* p = state.playPtr;
    float* r = state.releasePtr;

    if (state.env == Pipewave::Attack) {
        if (p == nullptr) {
            p = _attackStartPtr;
            state.playInterpolation = 0.0f;
            state.playInterpolationSpeed = 0.0f;
        }
    } else if (state.env == Pipewave::Release) {
        if (r == nullptr) {
            r = p;
            p = nullptr;
            state.releaseGain = 1.0f;
            state.releaseInterpolation = state.playInterpolation;
            state.releaseCount = _releaseLength;
        }
    } else {
        jassertfalse; // Invalid envelope state
    }

    if (r != nullptr) {
        int k = SUB_FRAME_LENGTH;
        float* q = out; 
        float g = state.releaseGain;
        int i = state.releaseCount - 1;

        float dg = g / SUB_FRAME_LENGTH;  

        if (i > 0)
            dg *= _releaseMultiplier;

        if (r < _loopStartPtr) {

            while (k--) {
                *q++ += g * *r++;
                g -= dg;
            }

        } else {

            float y = state.releaseInterpolation;
            float dy = _releaseDetune;

            while (k--) {
                y += dy;

                if (y > 1.0f) {
                    y -= 1.0f;
                    r += 1;
                } else if (y < 0.0f) {
                    y += 1.0f;
                    r -= 1;
                }

                *q++ += g * (r [0] + y * (r [1] - r [0]));
                g -= dg;
                r += _sampleStep;

                if (r >= _loopEndPtr)
                    r -= _loopLength;
            }

            state.releaseInterpolation = y;
        }

        if (i > 0) {
            state.releaseGain = g;
            state.releaseCount = i;
        } else {
            r = nullptr;
            state.env = Pipewave::Over;
        }
    }

    if (p != nullptr) { 
        int k = SUB_FRAME_LENGTH;
        float* q = out;

        if (p < _loopStartPtr) {
            while (k--) {
                *q++ += *p++;
            }
        } else {
            float y = state.playInterpolation;
            state.playInterpolationSpeed += _instability * 0.0005f * (0.05f * _instability * (rnd.nextFloat() - 0.5f) - state.playInterpolationSpeed);
            float dy = state.playInterpolationSpeed * _sampleStep;

            while (k--) {
                y += dy;

                if (y > 1.0f) {
                    y -= 1.0f;
                    p += 1;
                } else if (y < 0.0f) {
                    y += 1.0f;
                    p -= 1;
                }

                *q++ += p [0] + y * (p [1] - p [0]);
                p += _sampleStep;

                if (p >= _loopEndPtr)
                    p -= _loopLength;
            }

            state.playInterpolation = y;
        }
    }

    if (p == nullptr && r == nullptr)
        state.env = Pipewave::Over;

    state.playPtr = p;
    state.releasePtr = r;
}

void Pipewave::genwave()
{
    thread_local static Random rnd;

    const float sampleRate_r = 1.0f / _sampleRate;

    float m = _model.getNoteAttack(_note);

    for (int h = 0; h < N_HARM; ++h) {
        float t = _model.getHarmonicAttack(h, _note);

        if (t > m)
            m = t;
    }

    // Attack length aligned to the processing sub-frames
    _attackLength = (int)(_sampleRate * m + 0.5f);
    _attackLength = (_attackLength + SUB_FRAME_LENGTH - 1) & ~(SUB_FRAME_LENGTH - 1);

    // Target frequency
    float f1 = (_freq + _model.getNoteOffset(_note) + _model.getNoteRandomisation(_note) * (2.0f * rnd.nextFloat() + 1.0f)) * sampleRate_r;

    // Attack frequency (detuned)
    float f0 = f1 * math::exp2ap(_model.getNoteAttackDetune(_note) / 1200.0f);

    float f = 0.0f;

    for (int h = N_HARM - 1; h >= 0; --h) {
        f = (h + 1) * f1;

        if (f < 0.45f && _model.getHarmonicLevel(h, _note) >= -40.0f)
            break;
    }

    if (f > 0.25f)
        _sampleStep = 3;
    else if (f > 0.125f)
        _sampleStep = 2;
    else
        _sampleStep = 1;

    int nc = 0;

    Pipewave::looplen(f1 * _sampleRate, _sampleStep * _sampleRate, (int)(_sampleRate / 6.0f), _loopLength, nc);
    jassert(_loopLength > 0);

    if (_loopLength < _sampleStep * SUB_FRAME_LENGTH) {
        int k = (_sampleStep * SUB_FRAME_LENGTH - 1) / _loopLength + 1;
        _loopLength *= k;
        nc *= k;
    }

    int wavetableLength = _attackLength + _loopLength + _sampleStep * (SUB_FRAME_LENGTH + 4);
    _wavetable.resize(wavetableLength);

    std::vector<float> _arg(wavetableLength);
    std::vector<float> _att(wavetableLength);

    _attackStartPtr = _wavetable.data();
    _loopStartPtr = _attackStartPtr + _attackLength;
    _loopEndPtr = _loopStartPtr + _loopLength;

    memset(_attackStartPtr, 0, sizeof(float) * _wavetable.size());

    _releaseLength = (int)(ceilf (_model.getNoteRelease(_note) * _sampleRate / SUB_FRAME_LENGTH) + 1);
    _releaseMultiplier = 1.0f - powf (0.1f, 1.0f / _releaseLength); 
    _releaseDetune = _sampleStep * (math::exp2ap (_model.getNoteReleaseDetune(_note) / 1200.0f) - 1.0f);
    _instability = _model.getNoteInstability(_note);

    int k = (int)(_sampleRate * _model.getNoteAttack(_note) + 0.5);

    // _arg[i] will contain phase steps along the generated wavetable

    {
        float t = 0.0f;

        // Interpolate from frequency f1 to f0 during the attack
        for (int i = 0; i <= _attackLength; ++i) {
            _arg [i] = t - floorf(t + 0.5f);
            t += (i < k) ? (((k - i) * f0 + i * f1) / k) : f1;
        }
    }

    // Generate phase steps of the sustained loop
    for (int i = 1; i < _loopLength; ++i) {
        float t = _arg[_attackLength] + (float)i * nc / _loopLength;
        _arg[i + _attackLength] = t - floorf(t + 0.5f);
    }

    float v0 = math::exp2ap(0.1661f * _model.getNoteVolume(_note));

    for (int h = 0; h < N_HARM; ++h) {
        if ((h + 1) * f1 > 0.45f)
            break;

        float v = _model.getHarmonicLevel(h, _note);

        if (v < -80.0f)
            continue;

        v = v0 * math::exp2ap(0.1661f * (v + _model.getHarmonicRandomisation(h, _note) * (2.0f * rnd.nextFloat() - 1.0f)));
        k = (int)(_sampleRate * _model.getHarmonicAttack(h, _note) + 0.5f);

        if (k > _att.size())
            _att.resize(k);

        attgain(_att.data(), k, _model.getHarmonicAttackProfile(h, _note));

        for (int i = 0; i < _attackLength + _loopLength; ++i) {
            float t = _arg[i] * (h + 1);
            t -= floorf(t);
            m = v * sinf(MathConstants<float>::twoPi * t);

            if (i < k)
                m *= _att[i];

            _attackStartPtr[i] += m;
        }
    }

    for (int i = 0; i < _sampleStep * (SUB_FRAME_LENGTH + 4); ++i)
        _attackStartPtr[i + _attackLength + _loopLength] = _attackStartPtr[i + _attackLength];

    _needsToBeRebuilt = false;
}

void Pipewave::looplen(float f, float sampleRate, int lmax, int& aa, int& bb)
{
    constexpr int N = 8;
    int z[N];
    int a, b;
    float d;

    float g = sampleRate / f;

    for (int i = 0; i < N; ++i) {
        a = z[i] = (int)(floor (g + 0.5));
        g -= a;
        b = 1;
        int j = i;

        while (j > 0) {
            int t = a;
            a = z[--j] * a + b;
            b = t;
        }

        if (a < 0) {
            a = -a;
            b = -b;
        }

        if (a <= lmax) {
            d = sampleRate * b / a - f;

            if (fabs(d) < 0.1f && fabs(d) < 3e-4f * f)
                break;

            g = (fabs(g) < 1e-6f) ? 1e6f : 1.0f / g;
        } else  {
            b = (int)(lmax * f / sampleRate);
            a = (int)(b * sampleRate / f + 0.5f);
            d = sampleRate * b / a - f; 
            break; 
        }
    }

    aa = a;
    bb = b;
}

void Pipewave::attgain(float* att, int n, float p)
{
    float w = 0.05f;
    float y = 0.6f;

    if (p > 0.0f)
        y += 0.11f * p;

    float z = 0.0;
    int j = 0;

    for (int i = 1; i <= 24; i++)
    {
        int k = n * i / 24;
        float x =  1.0f - z - 1.5f * y;
        y += w * x;
        float d = k == j ? 0.0f : w * y * p / (k - j);

        while (j < k) {
            float m = (float) j / n;
            att[j++] = (1.0f - m) * z + m;
            z += d;
        }
    }
}

//==============================================================================

Rankwave::Rankwave(Addsynth& model)
    : _model(model)
    , _noteMin(model.getNoteMin())
    , _noteMax(model.getNoteMax())
    , _pipes{}
{
    jassert(_noteMax - _noteMin + 1 > 0);
}

void Rankwave::createPipes(const Scale& scale, float tuningFrequency)
{
    _pipes.clear();

    const auto fn = _model.getFn();
    const auto fd = _model.getFd();

    const auto& s = scale.getTable();
    float fbase = tuningFrequency * fn / (fd * s[9]);

    for (int i = _noteMin; i <= _noteMax; ++i) {
        auto pipe = std::make_unique<Pipewave>(_model, i - _noteMin, ldexpf(fbase * s[i % 12], i/12 - 5));

        _pipes.add(pipe.release());
    }
}

void Rankwave::retunePipes(const Scale& scale, float tuningFrequency)
{
    const auto fn = _model.getFn();
    const auto fd = _model.getFd();

    const auto& s = scale.getTable();
    float fbase = tuningFrequency * fn / (fd * s[9]);

    for (int i = _noteMin; i <= _noteMax; ++i) {
        Pipewave* pipe = _pipes[i - _noteMin];
        pipe->setFrequency(ldexpf(fbase * s[i % 12], i/12 - 5));
        pipe->setNeedsToBeRebuilt(true);
    }
}

void Rankwave::prepareToPlay(float sampleRate)
{
    for (auto* pipe : _pipes) {
        pipe->prepateToPlay(sampleRate);
    }
}

Pipewave::State Rankwave::trigger(int note)
{
    if (note < _noteMin || note > _noteMax)
        return {};

    int index = note - _noteMin;
    jassert(index >= 0 && index < (int)_pipes.size());

    Pipewave* pipe = _pipes[note - _noteMin];
    return pipe->trigger();
}

AEOLUS_NAMESPACE_END
