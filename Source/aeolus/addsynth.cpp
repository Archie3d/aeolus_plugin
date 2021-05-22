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

#include "addsynth.h"

using namespace juce;

AEOLUS_NAMESPACE_BEGIN

N_func::N_func()
    : _b{}
    , _v{}
{
    reset(0.0f);
}

void N_func::reset(float v)
{
    _b = 16;
    _v.fill(v);
}

void N_func::setValue(int idx, float v)
{
    if (! isPositiveAndBelow(idx, N_NOTES))
        return;

    _v [idx] = v;
    _b |= 1 << idx;

    int j = idx - 1;

    while (j >= 0 && ! (_b & (1 << j)))
        --j;

    if (j < 0) {
        while (++j != idx)
            _v [j] = v;
    } else {
        const float d = (_v [j] - v) / (j - idx);

        while (++j != idx)
            _v [j] = v + (j - idx) * d;
    }

    j = idx + 1;

    while ((j < N_NOTES) && ! (_b & (1 << j)))
        ++j;

    if (j > N_NOTES - 1) {
        while (--j != idx)
            _v [j] = v;
    } else {
        const float d = (_v [j] - v) / (j - idx);

        while (--j != idx)
            _v [j] = v + (j - idx) * d;
    }
}

void N_func::clearValue(int idx)
{
    if (isPositiveAndBelow(idx, N_NOTES))
        return;

    int m = 1 << idx;

    if (! (_b & m) || (_b == m))
        return;

    _b ^= m;

    int j = idx - 1;

    while ((j >= 0) && ! (_b & (1 << j)))
        --j;

    int k = idx + 1;

    while ((k <= N_NOTES - 1) && ! (_b & (1 << k)))
        ++k;

    if ((j >= 0) && (k < N_NOTES)) {
        const float d = (_v [k] - _v [j]) / (k - j);

        for (int i = j + 1; i < k; i++)
            _v [i] = _v [j] + (i - j) * d;
    } else if (j >= 0) {
        const float d = _v [j];

        while (j < N_NOTES - 1)
            _v [++j] = d; 
    } else if (k < N_NOTES) {
        const float d = _v [k];

        while (k > 0)
            _v [--k] = d; 
    }
}

float N_func::getValue(int idx) const
{
    jassert(isPositiveAndBelow(idx, _v.size()));

    return _v[idx];
}

bool N_func::isSet(int idx) const
{
    jassert(isPositiveAndBelow(idx, _v.size()));

    return (_b & (1 << idx)) != 0;
}

float N_func::operator[](int note) const
{
    const int i = note / NOTES_GAP;
    const int k = note - NOTES_GAP * i;
    float v = _v [i];

    if (k) {
        // Apply linear interpolation if falls into the gap.
        jassert(isPositiveAndBelow(i + 1, _v.size()));

        v += k * (_v [i + 1] - v) / NOTES_GAP;
    }

    return v;
}

void N_func::write(OutputStream& stream) const
{
    stream.writeInt(_b);

    for (const auto v : _v)
        stream.writeFloat(v);
}

void N_func::read(InputStream& stream)
{
    _b = stream.readInt();

    for (int i = 0; i < _v.size(); ++i)
        _v[i] = stream.readFloat();
}

//==============================================================================

HN_func::HN_func()
    : _h{}
{
}

void HN_func::reset(float v)
{
    for (auto& h : _h)
        h.reset(v);
}

void HN_func::setValue(int idx, float v)
{
    jassert(isPositiveAndBelow(idx, N_NOTES));

    for (auto& h: _h)
        h.setValue(idx, v);
}

void HN_func::setValue(int harm, int idx, float v)
{
    jassert(isPositiveAndBelow(harm, _h.size()));
    jassert(isPositiveAndBelow(idx, N_NOTES));

    _h[harm].setValue(idx, v);
}

void HN_func::clearValue(int idx)
{
    jassert(isPositiveAndBelow(idx, N_NOTES));

    for (auto& h : _h)
        h.clearValue(idx);
}

void HN_func::clearValue(int harm, int idx)
{
    jassert(isPositiveAndBelow(harm, _h.size()));
    jassert(isPositiveAndBelow(idx, N_NOTES));

    _h[harm].clearValue(idx);
}

float HN_func::getValue(int harm, int idx) const
{
    jassert(isPositiveAndBelow(harm, _h.size()));
    jassert(isPositiveAndBelow(idx, N_NOTES));

    return _h[harm].getValue(idx);
}

bool HN_func::isSet(int harm, int idx) const
{
    jassert(isPositiveAndBelow(harm, _h.size()));
    jassert(isPositiveAndBelow(idx, N_NOTES));

    return _h[harm].isSet(idx);
}

void HN_func::write(OutputStream& stream, int n) const
{
    const auto m = jmin(_h.size(), (size_t)n);

    for (int i = 0; i < m; ++i)
        _h[i].write(stream);
}

void HN_func::read(InputStream& stream, int n)
{
    const auto m = jmin(_h.size(), (size_t)n);

    for (int i = 0; i < m; ++i)
        _h[i].read(stream);
}

//==============================================================================

Addsynth::Addsynth()
{
    reset();
}

void Addsynth::reset()
{
    _noteMin = NOTE_MIN;
    _noteMax = NOTE_MAX;
    _fn = 1;
    _fd = 1;

    _n_vol.reset(-20.0f);
    _n_ins.reset(0.0f);
    _n_off.reset(0.0f);
    _n_att.reset(0.01f);
    _n_atd.reset(0.0f);
    _n_dct.reset(0.01f);
    _n_dcd.reset(0.0f);
    _n_ran.reset(0.0f);
    _h_lev.reset(-100.0f);
    _h_ran.reset(0.0f);
    _h_att.reset(0.050f);
    _h_atp.reset(0.0f);
}

template<size_t L>
static void writeString(const String& string, OutputStream& stream)
{
    char buffer[L] = {0};
    strncpy(buffer, string.toRawUTF8(), jmin(L, (size_t)string.length()));
    stream.write(buffer, L);
}

void Addsynth::write(OutputStream& stream) const
{
    char header[header_length] = {0};

    strncpy(header, "AEOLUS", 6);
    header[7] = defaultVersion;
    header[26] = N_HARM;
    header[28] = (char) _noteMin;
    header[29] = (char) _noteMax;
    header[30] = (char) _fn;
    header[31] = (char) _fd;

    stream.write(header, header_length);

    writeString<stopName_length> (_stopName,  stream);
    writeString<copyright_length>(_copyright, stream);
    writeString<mnemonic_length> (_mnemonic,  stream);
    writeString<comments_length>  (_comments,  stream);
    writeString<reserved_length> (String(), stream);

    _n_vol.write(stream);
    _n_off.write(stream);
    _n_ran.write(stream);
    _n_ins.write(stream);
    _n_att.write(stream);
    _n_atd.write(stream);
    _n_dct.write(stream);
    _n_dcd.write(stream);

    // Store all N_HARM harmonics
    jassert(header[26] == N_HARM);

    _h_lev.write(stream);
    _h_ran.write(stream);
    _h_att.write(stream);
    _h_atp.write(stream);
}

template<size_t L>
static juce::Result readString(String& string, InputStream& stream)
{
    char buffer[L] = {0};
    const auto n = stream.read(buffer, L);

    if (n != L)
        return Result::fail(String("Failed to read string of length ") + String(L)
            + " " + String(n) + " bytes read instead");

    buffer[L - 1] = 0;
    string = String::fromUTF8(buffer, -1);

    return Result::ok();
}

juce::Result Addsynth::read(InputStream& stream)
{
    char header[header_length] = {0};

    auto n = stream.read(header, header_length);

    if (n != header_length)
        return Result::fail("Failed to read the header");

    if (strncmp(header, "AEOLUS", 6) != 0)
        return Result::fail("Invalid header signature");

    int version = header[7];
    int nHarm = header[26];

    if (nHarm == 0)
        nHarm = deprecated::N_HARM;

    _noteMin = header[28];
    _noteMax = header[29];

    if (_noteMax == deprecated::NOTE_MAX)
        _noteMax = NOTE_MAX;

    _fn = header[30];
    _fd = header[31];

    auto res = readString<stopName_length>(_stopName, stream);

    if (res.failed())
        return Result::fail("Failed to read the stop name: " + res.getErrorMessage());

    res = readString<copyright_length>(_copyright, stream);

    if (res.failed())
        return Result::fail("Failed to read the copyright: " + res.getErrorMessage());

    res = readString<mnemonic_length>(_mnemonic, stream);

    if (res.failed())
        return Result::fail("Failed to read the mnemonic: " + res.getErrorMessage());

    res = readString<comments_length>(_comments, stream);

    if (res.failed())
        return Result::fail("Failed to read the comments: " + res.getErrorMessage());

    {
        String reserved;
        res = readString<reserved_length>(reserved, stream);

        if (res.failed())
            return Result::fail("Failed to read reserved field: " + res.getErrorMessage());
    }

    _n_vol.read(stream);
    _n_off.read(stream);
    _n_ran.read(stream);

    if (version >= defaultVersion) {
        _n_ins.read(stream);
        _n_att.read(stream);
        _n_atd.read(stream);
        _n_dct.read(stream);
        _n_dcd.read(stream);
    }

    _h_lev.reset(-100.0f);
    _h_ran.reset(0.0f);
    _h_att.reset(0.050f);
    _h_atp.reset(0.0f);

    _h_lev.read(stream, nHarm);
    _h_ran.read(stream, nHarm);
    _h_att.read(stream, nHarm);
    _h_atp.read(stream, nHarm);

    return Result::ok();
}

Result Addsynth::readFromResource(const String& name)
{
    int size = 0;
    const char* data = BinaryData::getNamedResource(name.toRawUTF8(), size);

    if (data == nullptr)
        return Result::fail("Unable to read embedded resource: " + name);

    MemoryInputStream stream(data, size, false);

    return read(stream);
}

//==============================================================================

Model::Model()
    : _synths()
    , _nameToSynthMap()
{
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i) {
        String filename(BinaryData::originalFilenames[i]);

        if (filename.endsWith(".ae0")) {
            String name(BinaryData::namedResourceList[i]);

            auto synth = std::make_unique<Addsynth>();
            auto res = synth->readFromResource(name);

            DBG(String("Reading ") + name + " --> " + synth->getStopName());

            if (res.wasOk()) {
                auto* ptr = synth.get();

                ptr->setStopName(name.dropLastCharacters(4));

                _synths.add(synth.release());
                _nameToSynthMap[name.dropLastCharacters(4)] = ptr;
            }
        }
    }
}

StringArray Model::getStopNames() const
{
    StringArray stops;

    for (const auto* synth : _synths) {
        stops.add(synth->getStopName());
    }

    return stops;
}

Addsynth* Model::getStopByName(const juce::String& name)
{
    auto it = _nameToSynthMap.find(name);

    if (it == _nameToSynthMap.end())
        return nullptr;

    return it->second;
}

JUCE_IMPLEMENT_SINGLETON(Model)

AEOLUS_NAMESPACE_END
