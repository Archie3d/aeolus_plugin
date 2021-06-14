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

#include "aeolus/engine.h"

using namespace juce;

AEOLUS_NAMESPACE_BEGIN

EngineGlobal::EngineGlobal()
    : _rankwaves{}
{
    loadRankwaves();
    loadIRs();
}

StringArray EngineGlobal::getAllStopNames() const
{
    StringArray names;

    for (const auto* const rankwave : _rankwaves)
        names.add(rankwave->getStopName());

    return names;
}

Rankwave* EngineGlobal::getStopByName(const String& name)
{
    if (!_rankwavesByName.contains(name))
        return nullptr;

    return _rankwavesByName[name];
}

void EngineGlobal::updateStops(float sampleRate)
{
    for (auto* rw : _rankwaves)
        rw->prepareToPlay(sampleRate);
}

void EngineGlobal::loadRankwaves()
{
    auto& model = *Model::getInstance();

    for (int i = 0; i < model.getStopsCount(); ++i) {
        auto* synth = model[i];
        jassert(synth);

        auto rankwave = std::make_unique<Rankwave>(*synth);
        auto* ptr = rankwave.get();
        _rankwaves.add(rankwave.release());
        _rankwavesByName.set(ptr->getStopName(), ptr);
    }
}

void EngineGlobal::loadIRs()
{
    _irs.clear();

    _irs.push_back({
        "York Guildhall Council Chamber",
        BinaryData::york_council_chamber_wav,
        BinaryData::york_council_chamber_wavSize,
        0.25f,
        true,
        {}
    });

    _irs.push_back({
        "St Andrew's Church",
        BinaryData::st_andrews_church_wav,
        BinaryData::st_andrews_church_wavSize,
        1.0f,
        true,
        {}
    });

    _irs.push_back({
        "St. George's Episcopal Church",
        BinaryData::st_georges_far_wav,
        BinaryData::st_georges_far_wavSize,
        1.0f,
        true,
        {}
    });

    _irs.push_back({
        "Lady Chapel, St Albans Cathedral",
        BinaryData::lady_chapel_stalbans_wav,
        BinaryData::lady_chapel_stalbans_wavSize,
        1.0f,
        true,
        {}
    });

    _irs.push_back({
        "1st Baptist Church, Nashville",
        BinaryData::_1st_baptist_nashville_balcony_wav,
        BinaryData::_1st_baptist_nashville_balcony_wavSize,
        1.0f,
        true,
        {}
    });

    _irs.push_back({
        "Elveden Hall, Suffolk",
        BinaryData::elveden_hall_suffolk_england_wav,
        BinaryData::elveden_hall_suffolk_england_wavSize,
        0.1f,
        false,
        {}
    });

    _irs.push_back({
        "R1 Nuclear Reactor Hall",
        BinaryData::r1_nuclear_reactor_hall_wav,
        BinaryData::r1_nuclear_reactor_hall_wavSize,
        0.4f,
        true,
        {}
    });

    _irs.push_back({
        "Sports Centre, University of York",
        BinaryData::york_uni_sportscentre_wav,
        BinaryData::york_uni_sportscentre_wavSize,
        0.4f,
        true,
        {}
    });

    _irs.push_back({
        "York Minster",
        BinaryData::york_minster_wav,
        BinaryData::york_minster_wavSize,
        0.3f,
        true,
        {}
    });

    AudioFormatManager manager;
    manager.registerBasicFormats();

    _longestIRLength = 4096;

    for (auto& ir : _irs) {
        std::unique_ptr<InputStream> stream = std::make_unique<MemoryInputStream>(ir.data, ir.size, false);
        std::unique_ptr<AudioFormatReader> reader{manager.createReaderFor(std::move(stream))};
        ir.waveform.setSize(reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&ir.waveform, 0, ir.waveform.getNumSamples(), 0, true, true);

        ir.waveform.applyGain(ir.gain);

        _longestIRLength = jmax(_longestIRLength, ir.waveform.getNumSamples());
    }

}

JUCE_IMPLEMENT_SINGLETON(EngineGlobal)

//==============================================================================

Engine::Engine()
    : _sampleRate{SAMPLE_RATE}
    , _voicePool(*this)
    , _params{NUM_PARAMS}
    , _divisions{}
    , _subFrameBuffer{2, SUB_FRAME_LENGTH}
    , _divisionFrameBuffer{2, SUB_FRAME_LENGTH}
    , _voiceFrameBuffer{2, SUB_FRAME_LENGTH}
    , _remainedSamples{0}
    , _tremulantBuffer{1, SUB_FRAME_LENGTH}
    , _tremulantPhase{0.0f}
    , _convolver{}
    , _selectedIR{0}
    , _irSwitchEvents{}
    , _interpolator{1.0f}
    , _midiKeyboardState{}
    , _volumeLevel{}
{
    populateDivisions();
}

void Engine::prepareToPlay(float sampleRate, int frameSize)
{
    // Make sure the stops wavetable is updated.
    auto* g = EngineGlobal::getInstance();
    g->updateStops(SAMPLE_RATE);

    // Select the first IR for reverb by default
    setReverbIR(_selectedIR);
    _convolver.setDryWet(1.0f, 0.25f, true);

    _interpolator.setRatio(SAMPLE_RATE / sampleRate); // 44100 / sampleRate
    _interpolator.reset();

    _sampleRate = sampleRate;
}

void Engine::setReverbIR(int num)
{
    auto* g = EngineGlobal::getInstance();
    const auto& irs = g->getIRs();

    if (num >= 0 && num < irs.size()) {
        const auto& ir = irs[num];
        _convolver.setLength(int(ir.waveform.getNumSamples()/ 4096 + 1) * 4096);
        _convolver.prepareToPlay(SAMPLE_RATE, SUB_FRAME_LENGTH); // these parameters are irrelevant
        _convolver.setZeroDelay(ir.zeroDelay);
        _convolver.setIR(ir.waveform);

        _selectedIR = num;
    }
}

void Engine::postReverbIR(int num)
{
    // Anticipate the IR change that will happen later.
    // This is required for the UI to be updated correctly.
    _selectedIR = num;

    _irSwitchEvents.send({num});
}

void Engine::setReverbWet(float v)
{
    _convolver.setDryWet(1.0f, v);
}

void Engine::setVolume(float v)
{
    _params[VOLUME].setValue(v);
}

void Engine::process(float* outL, float* outR, int numFrames, bool isNonRealtime)
{
    jassert(outL != nullptr);
    jassert(outR != nullptr);

    float* origOutL = outL;
    float* origOutR = outR;
    int origNumFrames = numFrames;

    processPendingIRSwitchEvents();
    processPendingNoteEvents();

    while (numFrames > 0)
    {
        if (_remainedSamples > 0)
        {
            const int idx = SUB_FRAME_LENGTH - _remainedSamples;
            const float* subL = _subFrameBuffer.getReadPointer(0, idx);
            const float* subR = _subFrameBuffer.getReadPointer(1, idx);

            const int n = jmin(_remainedSamples, numFrames);

            while (_remainedSamples > 0 && _interpolator.canWrite()) {
                _interpolator.write(*subL, *subR);
                --_remainedSamples;
                subL += 1;
                subR += 1;
            }

            while (numFrames > 0 && _interpolator.canRead()) {
                _interpolator.read(*outL, *outR);
                numFrames -= 1;
                outL += 1;
                outR += 1;
            }
        }

        if (_remainedSamples == 0 && numFrames > 0)
        {
            processSubFrame();
            jassert(_remainedSamples > 0);
        }
    }

    if (_convolver.isAudible()) {
        _convolver.setNonRealtime(isNonRealtime);
        _convolver.process(origOutL, origOutR, origOutL, origOutR, origNumFrames);
    }

    applyVolume(origOutL, origOutR, origNumFrames);

    _volumeLevel.left.process(origOutL, origNumFrames);
    _volumeLevel.right.process(origOutR, origNumFrames);
}

void Engine::noteOn(int note, int midiChannel)
{
    clearDivisionsTriggerFlag();

    for (auto* division : _divisions)
        division->noteOn(note, midiChannel);
}

void Engine::noteOff(int note, int midiChannel)
{
    clearDivisionsTriggerFlag();

    for (auto* division : _divisions) {
        division->noteOff(note, midiChannel);
    }
}

void Engine::allNotesOff()
{
    for (auto* division : _divisions)
        division->allNotesOff();

    _midiKeyboardState.allNotesOff(0);
}

Range<int> Engine::getMidiKeyboardRange() const
{
    int minNote = -1;
    int maxNote = -1;

    for (auto* division : _divisions) {
        int min, max;
        division->getAvailableRange(min, max);

        if (min >= 0 && max >= 0) {
            if (minNote < 0 || minNote > min)
                minNote = min;

            if (maxNote < 0 || maxNote < max)
                maxNote = max;
        }
    }
    
    return Range<int>(minNote, maxNote);
}

Division* Engine::getDivisionByName(const String& name)
{
    for (auto* division : _divisions) {
        if (division->getName() == name)
            return division;
    }

    return nullptr;
}

var Engine::getPersistentState() const
{
    auto* obj = new DynamicObject();

    int irNum = _selectedIR;
    obj->setProperty("ir", irNum);

    Array<var> divisions;

    for (auto* division : _divisions)
        divisions.add(division->getPersistentState());

    obj->setProperty("divisions", divisions);

    return var{obj};
}

void Engine::setPersistentState(const var& state)
{
    if (const auto* obj = state.getDynamicObject()) {

        int irNum = obj->getProperty("ir");

        if (MessageManager::getInstance()->isThisTheMessageThread())
            postReverbIR(irNum);
        else
            setReverbIR(irNum);

        postReverbIR(irNum);

        if (const auto* divisions = obj->getProperty("divisions").getArray()) {

            if (divisions->size() != _divisions.size()) {
                DBG("Saved state is invalid and will be ignored");
                return;
            }

            for (int divIdx = 0; divIdx < _divisions.size(); ++divIdx) {
                auto* division = _divisions.getUnchecked(divIdx);
                division->setPersistentState(divisions->getReference(divIdx));
            }
        }
    }
}

void Engine::populateDivisions()
{
    auto wd = File::getCurrentWorkingDirectory();
    auto configFile = wd.getChildFile("organ_config.json");

    if (configFile.exists()) {
        FileInputStream stream(configFile);
        loadDivisionsFromConfig(stream);
    } else {
        MemoryInputStream stream(BinaryData::default_organ_json, BinaryData::default_organ_jsonSize, false);
        loadDivisionsFromConfig(stream);
    }

    // Update division links after they've been loaded.
    for (auto* division : _divisions) {
        division->populateLinkedDivisions();
    }
}

void Engine::loadDivisionsFromConfig(InputStream& stream)
{
    // Load organ config JSON
    auto config = JSON::parse(stream);

    if (auto* divisions = config.getProperty("divisions", {}).getArray()) {
        for (int i = 0; i < divisions->size(); ++i) {
            if (auto* divisionObj = divisions->getUnchecked(i).getDynamicObject()) {
                auto division = std::make_unique<Division>(*this);

                division->initFromVar(divisions->getUnchecked(i));

                _divisions.add(division.release());
            }
        }
    }
}

void Engine::clearDivisionsTriggerFlag()
{
    for (auto* division : _divisions)
        division->clearTriggerFlag();
}

void Engine::postNoteEvent(bool onOff, int note, int midiChannel)
{
    _pendingNoteEvents.send({onOff, note, midiChannel});
}

void Engine::processSubFrame()
{
    generateTremulant();

    _subFrameBuffer.clear();

    for (auto* division : _divisions) {

        _divisionFrameBuffer.clear();

        auto& activeVoices = division->getActiveVoices();
        
        auto* voice = activeVoices.first();
        bool hasVoices = (voice != nullptr);

        while (voice != nullptr) {
            _voiceFrameBuffer.clear();
            float* outL = _voiceFrameBuffer.getWritePointer(0);
            float* outR = _voiceFrameBuffer.getWritePointer(1);

            voice->process(outL, outR);

            _divisionFrameBuffer.addFrom(0, 0, _voiceFrameBuffer, 0, 0, SUB_FRAME_LENGTH);
            _divisionFrameBuffer.addFrom(1, 0, _voiceFrameBuffer, 1, 0, SUB_FRAME_LENGTH);

            if (voice->isOver()) {
                auto* nextVoice = activeVoices.removeAndReturnNext(voice);
                voice->resetAndReturnToPool();
                voice = nextVoice;
            } else {
                voice = voice->next();
            }
        }

        if (hasVoices) {
            modulateDivision(division);

            _subFrameBuffer.addFrom(0, 0, _divisionFrameBuffer, 0, 0, SUB_FRAME_LENGTH);
            _subFrameBuffer.addFrom(1, 0, _divisionFrameBuffer, 1, 0, SUB_FRAME_LENGTH);
        }

        division->volumeLevel().left.process(_divisionFrameBuffer, 0);
        division->volumeLevel().right.process(_divisionFrameBuffer, 1);

    }

    _remainedSamples = SUB_FRAME_LENGTH;
}

void Engine::processPendingNoteEvents()
{
    NoteEvent event;

    while (_pendingNoteEvents.receive(event)) {
        if (event.on)
            noteOn(event.note, event.midiChannel);
        else
            noteOff(event.note, event.midiChannel);
    }
}

void Engine::processPendingIRSwitchEvents()
{
    IRSwithEvent event;
    bool received = false;

    while (_irSwitchEvents.receive(event)) {
        received = true;
    }

    if (received) {
        setReverbIR(event.num);
    }
}

void Engine::generateTremulant()
{
    float* buf = _tremulantBuffer.getWritePointer(0);
    jassert(buf != nullptr);

    for (int i = 0; i < SUB_FRAME_LENGTH; ++i) {
        const float s = sinf(_tremulantPhase);
        buf[i] = s * TREMULANT_LEVEL;
        _tremulantPhase += TREMULANT_PHASE_INCREMENT;

        if (_tremulantPhase >= juce::MathConstants<float>::twoPi)
            _tremulantPhase -= juce::MathConstants<float>::twoPi;
    }
}

void Engine::modulateDivision(Division* division)
{
    jassert(division != nullptr);

    float* outL = _divisionFrameBuffer.getWritePointer(0);
    float* outR = _divisionFrameBuffer.getWritePointer(1);

    jassert(outL != nullptr);
    jassert(outR != nullptr);

    const float* gain = _tremulantBuffer.getReadPointer(0);
    jassert(gain != nullptr);

    const float lvl = division->getTremulantLevel(true);

    auto& paramGain = division->parameters()[Division::GAIN];
    paramGain.setValue(division->getParamGain()->get());

    for (int i = 0; i < SUB_FRAME_LENGTH; ++i) {
        float g = (1.0f + gain[i] * lvl) * paramGain.nextValue();
        outL[i] *= g;
        outR[i] *= g;
    }
}

void Engine::applyVolume(float* outL, float* outR, int numFrames)
{
    if (_params[VOLUME].isSmoothing()) {
        for (int i = 0; i < numFrames; ++i) {
            const float g = _params[VOLUME].nextValue() * VOLUME_GAIN;
            outL[i] *= g;
            outR[i] *= g;
        }
    } else {
        const float g = _params[VOLUME].target() * VOLUME_GAIN;

        for (int i = 0; i < numFrames; ++i) {
            outL[i] *= g;
            outR[i] *= g;
        }
    }
}

AEOLUS_NAMESPACE_END
