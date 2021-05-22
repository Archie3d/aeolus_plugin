#pragma once

#include "aeolus/globals.h"
#include "aeolus/addsynth.h"
#include "aeolus/scale.h"

#include <vector>

AEOLUS_NAMESPACE_BEGIN

/**
 * @brief Single pipe wavetable.
 * 
 * This class represents a single pipe mapped to a model (additive synth),
 * note, and frequency.
 */
class Pipewave
{
public:

    /// Envelope state.
    enum EnvState
    {
        Idle,
        Attack,
        Release,
        Over
    };

    /// PLayback state.
    struct State
    {
        Pipewave *pipewave = nullptr;
        EnvState env = Idle;
        float* playPtr = nullptr;               // _p_p
        float playInterpolation = 0.0;          // _y_p
        float playInterpolationSpeed = 0.0f;    // _z_p
        float* releasePtr = nullptr;            // _p_r
        float releaseInterpolation = 0.0f;      // _y_r
        float releaseGain = 0.0f;               // _g_r
        int releaseCount = 0;                   // _i_r

        void release() { if (pipewave != nullptr) pipewave->release(*this); }
        bool isIdle() const noexcept { return env == Idle; }
        bool isOver() const noexcept { return env == Over; }
        void reset() { pipewave = nullptr; env = Idle;}
    };

    Pipewave() = delete;
    Pipewave(Addsynth& model, int note, float freq);

    int getNote() const noexcept { return _note + _model.getNoteMin(); }

    void prepateToPlay(float sampleRate);

    State trigger();
    void release(Pipewave::State& state);

    void play(State& state, float* out);

private:
    void genwave();

    static void looplen(float f, float sampleRate, int lmax, int& aa, int& bb);
    static void attgain(int n, float p);

    Addsynth& _model;
    int _note;
    float _freq;
    float _sampleRate;

    int _attackLength;          // _l0
    int _loopLength;            // _l1
    int _sampleStep;            // _k_s
    int _releaseLength;         // _k_r
    float _releaseMultiplier;   // _m_r
    float _releaseDetune;       // _d_r
    float _instability;         // _d_p

    std::vector<float> _wavetable;

    float* _attackStartPtr; // _p0
    float* _loopStartPtr;   // _p1
    float* _loopEndPtr;     // _p2

    static std::vector<float> _arg;
    static std::vector<float> _att;
};

//==============================================================================

/**
 * @brief Pipes across the keys range.
 * 
 * This class is a collection of pipes based on the same
 * additive synth model.
 */
class Rankwave
{
public:
    Rankwave(Addsynth& model, const Scale& scale = Scale(Scale::EqualTemp));

    juce::String getStopName() const { return _model.getStopName(); }
    bool isForNote(int note) const noexcept { return note >= _noteMin && note <= _noteMax; }
    int getNoteMin() const noexcept { return _noteMin; }
    int getNoteMax() const noexcept { return _noteMax; }

    void prepareToPlay(float sampleRate);

    Pipewave::State trigger(int note);

private:
    Addsynth& _model;
    int _noteMin;
    int _noteMax;
    Scale _scale;

    juce::OwnedArray<Pipewave> _pipes;
};

AEOLUS_NAMESPACE_END
