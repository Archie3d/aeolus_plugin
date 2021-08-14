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

#pragma once

#include "aeolus/globals.h"

#include <stdint.h>
#include <array>

AEOLUS_NAMESPACE_BEGIN

/**
 * @brief Interpolated per-note look-up table.
 * 
 * This class stores a float parameter across the
 * N_NOTES points. Notes in between get interpolated linearly.
 */
class N_func final
{
public:
    N_func();
    void reset(float v);
    void setValue(int idx, float v);    // setv(i, v)
    void clearValue(int idx);           // clrv(i)
    float getValue(int idx) const;      // vs(i)
    bool isSet(int idx) const;          // st(i)

    /// Returns interpolated value for a note number (starting from 0).
    float operator[](int note) const;   // vi(n)

    juce::var toVar() const;
    void fromVar(const juce::var& v);

    void write(juce::OutputStream& stream) const;
    void read(juce::InputStream& stream);

private:
    int _b;
    std::array<float, N_NOTES> _v;
};

//==============================================================================

/**
 * @brief Interpolated per-note look-up table for harmonics.
 * 
 * This class keeps a per-note LUT for each of the N_HARM harmonics.
 */
class HN_func final
{
public:
    HN_func();
    void reset(float v);
    void setValue(int idx, float v);            // setv(i, v)
    void setValue(int harm, int idx, float v);  // setv(h, i, v)
    void clearValue(int idx);                   // clrv(i)
    void clearValue(int harm, int idx);         // clrv(h, i);
    float getValue(int harm, int idx) const;    // vs(h, i);
    bool isSet(int harm, int idx) const;        // st(h, i)

    N_func& operator[](int harm) { jassert(juce::isPositiveAndBelow(harm, _h.size())); return _h[harm]; }
    const N_func& operator[](int harm) const { jassert(juce::isPositiveAndBelow(harm, _h.size())); return _h[harm]; }

    juce::var toVar(int n = N_HARM) const;
    void fromVar(const juce::var& v);

    void write(juce::OutputStream& stream, int n = N_HARM) const;
    void read(juce::InputStream& stream, int n = N_HARM);

private:
    std::array<N_func, N_HARM> _h;
};

//==============================================================================

class Addsynth final
{
public:
    Addsynth();

    void reset();

    juce::String getStopName() const { return _stopName; }
    void setStopName(const juce::String& n) { _stopName = n; }
    juce::String getCopyright() const { return _copyright; }
    juce::String getMnemonic() const { return _mnemonic; }
    juce::String getComments() const { return _comments; }

    int getNoteMin() const noexcept { return _noteMin; }
    int getNoteMax() const noexcept { return _noteMax; }

    juce::var toVar() const;
    void fromVar(const juce::var& v);

    void write(juce::OutputStream& stream) const;
    juce::Result read(juce::InputStream& stream);

    juce::Result readFromResource(const juce::String& name);

    float getNoteVolume(int n) const noexcept { return _n_vol[n]; }
    float getNoteAttack(int n) const noexcept { return _n_att[n]; }
    float getNoteOffset(int n) const noexcept { return _n_off[n]; }
    float getNoteRandomisation(int n) const noexcept { return _n_ran[n]; }
    float getNoteInstability(int n) const noexcept { return _n_ins[n]; }
    float getNoteAttackDetune(int n) const noexcept { return _n_atd[n]; }
    float getNoteRelease(int n) const noexcept { return _n_dct[n]; }
    float getNoteReleaseDetune(int n) const noexcept { return _n_dcd[n]; }

    float getHarmonicLevel(int h, int n) const noexcept { return _h_lev[h][n]; }
    float getHarmonicAttack(int h, int n) const noexcept { return _h_att[h][n]; }
    float getHarmonicRandomisation(int h, int n) const noexcept { return _h_ran[h][n]; }
    float getHarmonicAttackProfile(int h, int n) const noexcept { return _h_atp[h][n]; }

    /// Frequency ration nominator.
    int getFn() const noexcept { return _fn; }

    ///  Frequency ratio denominator.
    int getFd() const noexcept { return _fd; }

private:

    constexpr static int defaultVersion = 2;

    constexpr static size_t header_length    = 32;
    constexpr static size_t stopName_length  = 32;
    constexpr static size_t copyright_length = 56;
    constexpr static size_t mnemonic_length  = 8;
    constexpr static size_t comments_length  = 56;
    constexpr static size_t reserved_length  = 8;

    juce::String _stopName;
    juce::String _copyright;
    juce::String _mnemonic;
    juce::String _comments;

    int _noteMin;   // _n0;
    int _noteMax;   // _n1;
    int _fn;
    int _fd;

    N_func  _n_vol;
    N_func  _n_off;
    N_func  _n_ran;
    N_func  _n_ins;
    N_func  _n_att; ///< Attack time.
    N_func  _n_atd; ///< Attack detune.
    N_func  _n_dct; ///< Releate time.
    N_func  _n_dcd; ///< Release detune.

    HN_func _h_lev; ///< Harmonic level
    HN_func _h_ran; ///< Harmonic level randomization.
    HN_func _h_att; ///< Harmonic attack time
    HN_func _h_atp; ///< Harmonic attack profile.
};

//==============================================================================

/**
 * @brief A collection of all available stops.
 * 
 * This class holds a collection of all available stops.
 * These stops models are shared among all the plugin instances.
 */
class Model : public juce::DeletedAtShutdown
{
public:
    juce::StringArray getStopNames() const;
    Addsynth* getStopByName(const juce::String& name);

    int getStopsCount() const { return _synths.size(); }
    Addsynth* operator[](int idx) { return _synths[idx]; }
    const Addsynth* operator[](int idx) const { return _synths[idx]; }

    JUCE_DECLARE_SINGLETON (Model, false)

private:
    Model();
    ~Model() override  { clearSingletonInstance(); }

    juce::OwnedArray<Addsynth> _synths;
    std::map<juce::String, Addsynth*> _nameToSynthMap;
};

AEOLUS_NAMESPACE_END
