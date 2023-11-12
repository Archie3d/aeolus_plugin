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

//==============================================================================

class PrepareRankwaveJob : ThreadPoolJob
{
public:

    PrepareRankwaveJob(Rankwave* rw, float sampleRate)
        : ThreadPoolJob(rw->getStopName())
        , _rankwave{rw}
        , _sampleRate{sampleRate}

    {
    }

    JobStatus runJob() override
    {
        _rankwave->prepareToPlay(_sampleRate);
        return JobStatus::jobHasFinished;
    }

private:
    Rankwave* _rankwave;
    float _sampleRate;
};


//==============================================================================

namespace settings {
const static char* tuningFrequency = "tuningFrequency";
const static char* tuningTemperament = "tuningTemperament";
}

EngineGlobal::EngineGlobal()
    : _rankwaves{}
    , _scale(Scale::EqualTemp)
    , _tuningFrequency(TUNING_FREQUENCY_DEFAULT)
    , _globalProperties{}
{
    PropertiesFile::Options options{};

    options.applicationName = ProjectInfo::projectName;
    options.filenameSuffix = ".settings";
    //options.osxLibrarySubFolder = "~/Library/Application Support";
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = PropertiesFile::storeAsXML;

    _globalProperties.setStorageParameters(options);

    loadSettings();

    loadRankwaves();
    loadIRs();
}

EngineGlobal::~EngineGlobal()
{
    saveSettings();
    clearSingletonInstance();
}

void EngineGlobal::registerProcessorProxy(ProcessorProxy* proxy)
{
    jassert(proxy != nullptr);
    _processors.addIfNotAlreadyThere(proxy);
}

void EngineGlobal::unregisterProcessorProxy(ProcessorProxy* proxy)
{
    jassert(proxy != nullptr);
    _processors.removeAllInstancesOf(proxy);
}

void EngineGlobal::loadSettings()
{
    if (auto* propertiesFile = _globalProperties.getUserSettings()) {
        const float tuningFreq = (float)propertiesFile->getDoubleValue(settings::tuningFrequency, TUNING_FREQUENCY_DEFAULT);

        if (tuningFreq >= TUNING_FREQUENCY_MIN && tuningFreq <= TUNING_FREQUENCY_MAX)
            _tuningFrequency = tuningFreq;

        const int scaleType = propertiesFile->getIntValue(settings::tuningTemperament, (int)Scale::EqualTemp);

        if (scaleType >= (int)Scale::First && scaleType < (int)Scale::Total)
            _scale.setType(static_cast<Scale::Type>(scaleType));
    }
}

void EngineGlobal::saveSettings()
{
    if (auto* propertiesFile = _globalProperties.getUserSettings()) {
        propertiesFile->setValue(settings::tuningFrequency, _tuningFrequency);
        propertiesFile->setValue(settings::tuningTemperament, (int)_scale.getType());
    }

    _globalProperties.saveIfNeeded();
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
    _sampleRate = sampleRate;

    ThreadPool threadPool;
    std::atomic<int> done((int)_rankwaves.size());
    WaitableEvent wait;

    for (auto* rw : _rankwaves) {
        threadPool.addJob([sampleRate, rw, &done, &wait]() {
                rw->prepareToPlay(sampleRate);
                done -= 1;
                wait.signal();
            });
    }

    while (done.load() > 0)
        wait.wait();

/*
    // Single-thread equivalent
    for (auto* rw : _rankwaves)
        rw->prepareToPlay(sampleRate);
*/
}

void EngineGlobal::rebuildRankwaves()
{
    // Prepare all the rankwaves to be retuned
    for (auto* rw : _rankwaves) {
        rw->retunePipes(_scale, _tuningFrequency);
    }

    // Kill all the active voices
    int numActiveVoices = 0;

    for (auto* processor : _processors) {
        processor->killAllVoices();
        numActiveVoices += processor->getNumberOfActiveVoices();
    }

    Thread::sleep(100);

    // Wait for the voices to release
    while (numActiveVoices > 0) {
        numActiveVoices = 0;
        for (auto* processor : _processors) {
            processor->killAllVoices();
            numActiveVoices += processor->getNumberOfActiveVoices();
            Thread::sleep(100);
        }
    }

    updateStops(_sampleRate);
}

void EngineGlobal::loadRankwaves()
{
    auto& model = *Model::getInstance();

    for (int i = 0; i < model.getStopsCount(); ++i) {
        auto* synth = model[i];
        jassert(synth);

        auto rankwave = std::make_unique<Rankwave>(*synth);
        rankwave->createPipes(_scale, _tuningFrequency);

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
        "St Laurentius, Molenbeek",
        BinaryData::st_laurentius_molenbeek_wav,
        BinaryData::st_laurentius_molenbeek_wavSize,
        0.8f,
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
        "St George's Episcopal Church",
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

    // A minimum size we can have is a single convolution block.
    _longestIRLength = dsp::Convolver::BlockSize;

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
    : _sampleRate{SAMPLE_RATE_F}
    , _voicePool(*this)
    , _params{NUM_PARAMS}
    , _divisions{}
    , _sequencer{}
    , _subFrameBuffer{N_OUTPUT_CHANNELS, SUB_FRAME_LENGTH}
    , _divisionFrameBuffer{N_OUTPUT_CHANNELS, SUB_FRAME_LENGTH}
    , _voiceFrameBuffer{N_VOICE_CHANNELS, SUB_FRAME_LENGTH}
    , _remainedSamples{0}
    , _tremulantBuffer{1, SUB_FRAME_LENGTH}
    , _tremulantPhase{0.0f}
    , _convolver{}
    , _selectedIR{0}
    , _irSwitchEvents{}
    , _reverbTailCounter{0}
    , _interpolator{1.0f, N_OUTPUT_CHANNELS}
    , _midiKeyboardState{}
    , _volumeLevel{}
    , _midiControlChannel{0}
    , _midiSwellChannel{0}
{
    populateDivisions();

    // Sequencer can be created only after the divisions have been populated.
    _sequencer = std::make_unique<Sequencer>(*this, SEQUENCER_N_STEPS);
}

void Engine::prepareToPlay(float sampleRate, int frameSize)
{
    ignoreUnused(frameSize);

    // Make sure the stops wavetable is updated.
    auto* g = EngineGlobal::getInstance();
    g->updateStops(SAMPLE_RATE_F);

    // Select the first IR for reverb by default
    setReverbIR(_selectedIR);
    _convolver.setDryWet(1.0f, 0.25f, true);

    _interpolator.setRatio(SAMPLE_RATE_F / sampleRate); // 44100 / sampleRate
    _interpolator.reset();

    _sampleRate = sampleRate;
}

void Engine::setReverbIR(int num)
{
    auto* g = EngineGlobal::getInstance();
    const auto& irs = g->getIRs();

    if (num >= 0 && num < irs.size()) {
        const auto& ir = irs[num];
        _convolver.setLength(int(ir.waveform.getNumSamples() / dsp::Convolver::BlockSize + 1) * dsp::Convolver::BlockSize);
        _convolver.prepareToPlay(SAMPLE_RATE_F, SUB_FRAME_LENGTH); // these parameters are irrelevant
        _convolver.setZeroDelay(ir.zeroDelay);
        _convolver.setIR(ir.waveform);

        _reverbTailCounter = _convolver.length();

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

float Engine::getReverbLengthInSeconds() const
{
    return float(_convolver.length()) * SAMPLE_RATE_R;
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

    bool wasAudioGenerated = false;

    while (numFrames > 0)
    {
        if (_remainedSamples > 0)
        {
            const int idx = SUB_FRAME_LENGTH - _remainedSamples;
            const float* subL = _subFrameBuffer.getReadPointer(0, idx);
            const float* subR = _subFrameBuffer.getReadPointer(1, idx);

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
            wasAudioGenerated |= processSubFrame();
            jassert(_remainedSamples > 0);
        }
    }

    // When there is no audio generated we let the reverb tail to
    // sound and stop the reverb processing to avoid convolving with silence.
    if (wasAudioGenerated)
        _reverbTailCounter = _convolver.length();
    else
        _reverbTailCounter = jmax(0, _reverbTailCounter - origNumFrames);

    if (_reverbTailCounter > 0 && _convolver.isAudible()) {
        _convolver.setNonRealtime(isNonRealtime);
        _convolver.process(origOutL, origOutR, origOutL, origOutR, origNumFrames);
    }

    applyVolume(origOutL, origOutR, origNumFrames);

    _volumeLevel.left.process(origOutL, origNumFrames);
    _volumeLevel.right.process(origOutR, origNumFrames);
}

void Engine::process(AudioBuffer<float>& out, bool isNonRealtime)
{
    ignoreUnused(isNonRealtime);

    const int numChannels = out.getNumChannels();
    int numFrames = out.getNumSamples();

    processPendingIRSwitchEvents();
    processPendingNoteEvents();

    bool wasAudioGenerated = false;

    int outIdx = 0;

    while (numFrames > 0) {
        int idx = SUB_FRAME_LENGTH - _remainedSamples;

        while (_remainedSamples > 0 && _interpolator.canWrite()) {
            for (int ch = 0; ch < numChannels; ++ch)
                _interpolator.writeUnchecked(_subFrameBuffer.getReadPointer(ch)[idx], (size_t) ch);

            _interpolator.writeIncrement();

            _remainedSamples -= 1;
            idx += 1;
        }

        while (numFrames > 0 && _interpolator.canRead()) {
            for (int ch = 0; ch < numChannels; ++ch)
                out.getWritePointer(ch)[outIdx] = _interpolator.readUnchecked(ch);

            _interpolator.readIncrement();

            numFrames -= 1;
            outIdx += 1;
        }

        if (_remainedSamples == 0 && numFrames > 0)
        {
            wasAudioGenerated |= processSubFrame();
            jassert(_remainedSamples > 0);
        }

    }

    // Multibus processing does not have a convolver FX

    // Global volume across all the buses
    applyVolume(out);

    _volumeLevel.left.process(out);
    _volumeLevel.right = _volumeLevel.left;
}

void Engine::processMIDIMessage(const MidiMessage& message)
{
    const int ch = getMIDIControlChannel();

    // Process global CCs
    if (ch == 0 || message.getChannel() == 0 || message.getChannel() == ch) {
        processControlMIDIMessage(message);
    }

    if (message.isController()) {
        // Process divisions CCs
        for (auto* division : _divisions)
            division->handleControlMessage(message);
    } else if (message.isNoteOnOrOff()) {
        // Notes on/off
        _midiKeyboardState.processNextMidiEvent(message);
        return;
    }
}

void Engine::noteOn(int note, int midiChannel)
{
    clearDivisionsTriggerFlag();

    const int ch{ getMIDIControlChannel() };

    // Handle key switches
    if (ch == 0 || midiChannel == 0 || midiChannel == ch) {
        if (note == _sequencerStepBackwardKeySwitch)
            _sequencer->stepBackward();
        else if (note == _sequencerStepForwardKeySwitch)
            _sequencer->stepForward();
    }

    // Handle keys
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

std::set<int> Engine::getKeySwitches() const
{
    std::set<int> keySwitches{};

    keySwitches.insert(_sequencerStepBackwardKeySwitch);
    keySwitches.insert(_sequencerStepForwardKeySwitch);

    return keySwitches;
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

    // Save control channel
    obj->setProperty("midi_ctrl_channel", getMIDIControlChannel());
    obj->setProperty("midi_swell_channel", getMIDISwellChannel());

    // Save the IR.
    int irNum = _selectedIR;
    obj->setProperty("ir", irNum);

    // Save divisions.
    Array<var> divisions;

    for (auto* division : _divisions)
        divisions.add(division->getPersistentState());

    obj->setProperty("divisions", divisions);

    obj->setProperty("sequencer", _sequencer->getPersistentState());

    return var{obj};
}

void Engine::setPersistentState(const var& state)
{
    if (const auto* obj = state.getDynamicObject()) {
        // Restore control channel
        setMIDIControlChannel(obj->getProperty("midi_ctrl_channel"));
        setMIDISwellChannel(obj->getProperty("midi_swell_channel"));

        // Restore the IR
        int irNum = obj->getProperty("ir");

        if (MessageManager::getInstance()->isThisTheMessageThread())
            postReverbIR(irNum);
        else
            setReverbIR(irNum);

        postReverbIR(irNum);

        // Restore the sequencer
        _sequencer->setPersistentState(obj->getProperty("sequencer"));

        // Restore the divisions after the sequencer (in case we are restoring
        // from a state that did not have a sequencer before).
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
    const auto configFile = getCustomOrganConfigFile();

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

    if (auto* sequencer = config.getProperty("sequencer", {}).getDynamicObject()) {
        if (var v = sequencer->getProperty("backward_key"); v.isInt())
            _sequencerStepBackwardKeySwitch = (int)v;

        if (var v = sequencer->getProperty("forward_key"); v.isInt())
            _sequencerStepForwardKeySwitch = (int)v;
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

bool Engine::processSubFrame()
{
    jassert(_subFrameBuffer.getNumChannels() == _divisionFrameBuffer.getNumChannels());
    jassert(_subFrameBuffer.getNumSamples() == _divisionFrameBuffer.getNumSamples());

    generateTremulant();

    _subFrameBuffer.clear();

    bool wasAudioGenerated = false;

    for (auto* division : _divisions) {

        _divisionFrameBuffer.clear();

        const bool hasVoices = division->process(_divisionFrameBuffer, _voiceFrameBuffer);
        wasAudioGenerated |= hasVoices;

        if (hasVoices) {
            division->modulate(_divisionFrameBuffer, _tremulantBuffer);

            for (int ch = 0; ch < _subFrameBuffer.getNumChannels(); ++ch)
                _subFrameBuffer.addFrom(ch, 0, _divisionFrameBuffer, ch, 0, SUB_FRAME_LENGTH);
        }

#if AEOLUS_MULTIBUS_OUTPUT
        division->volumeLevel().left.process(_divisionFrameBuffer);
        division->volumeLevel().right = division->volumeLevel().left;
#else
        division->volumeLevel().left.process(_divisionFrameBuffer, 0);
        division->volumeLevel().right.process(_divisionFrameBuffer, 1);
#endif
    }

    _remainedSamples = SUB_FRAME_LENGTH;

    return wasAudioGenerated;
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

void Engine::applyVolume(AudioBuffer<float>& out)
{
    if (_params[VOLUME].isSmoothing()) {
        for (int i = 0; i < out.getNumSamples(); ++i) {
            const float g = _params[VOLUME].nextValue() * VOLUME_GAIN;

            for (int ch = 0; ch < out.getNumChannels(); ++ch)
                out.getWritePointer(ch)[i] *= g;
        }
    } else {
        const float g = _params[VOLUME].target() * VOLUME_GAIN;
        out.applyGain(g);
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

void Engine::processControlMIDIMessage(const MidiMessage& message)
{
    // @note VST3 will not pass the program change MIDI messages through.
    //       Instead program change must be handled at the processor level
    //       via the setCurrentProgram() method.

    // Here we handle the program change message nevertheless
    // in case of a non-VST3 or stand-alone plugin.
    if (message.isProgramChange()) {
        int step = message.getProgramChangeNumber();

        if (step >= 0 && step < _sequencer->getStepsCount())
            _sequencer->setStep(step);
    } else if (message.isController() && message.getControllerNumber() == CC_STOP_BUTTONS) {
        const auto value{ message.getControllerValue() };

        if ((value & 0xC8) == 0x40) {
            // 01mm0ggg
            StopControlMode mode { StopControlMode::Disabled };

            const int modeValue{ (value >> 4) & 0x03 };
            switch (modeValue) {
                case 0: mode = StopControlMode::Disabled; break;
                case 1: mode = StopControlMode::SetOff; break;
                case 2: mode = StopControlMode::SetOn; break;
                case 3: mode = StopControlMode::Toggle; break;
                default: break;
            }

            _stopControlMode = mode;
            _stopControlGroup = value & 0x07;

            if (_stopControlMode == StopControlMode::Disabled) {
                // Disable message does not require a 2nd part and can be processed immeditely.
                processStopControlMessage();

                _stopControlMode.reset();
            }
        } else if (value & 0xE0 == 0) {
            // 000bbbbb
            if (_stopControlMode.has_value()) {
                _stopControlButton = value & 0x1F;

                processStopControlMessage();
            }
        } else {
            _stopControlMode.reset();
        }
    }
}

void Engine::processStopControlMessage()
{
    if (!_stopControlMode.has_value())
        return;

    if (!juce::isPositiveAndBelow(_stopControlGroup, _divisions.size()))
        return;

    auto* division{ _divisions.getUnchecked(_stopControlGroup) };

    const auto mode{ *_stopControlMode };

    switch (mode) {
        case StopControlMode::Disabled:
            division->disableAllStops();
            break;
        case StopControlMode::SetOff:
            division->enableStop(_stopControlButton, false);
            break;
        case StopControlMode::SetOn:
            division->enableStop(_stopControlButton, true);
            break;
        case StopControlMode::Toggle:
            division->enableStop(_stopControlButton, !division->isStopEnabled(_stopControlButton));
            break;
        default:
            break;
    }
}

AEOLUS_NAMESPACE_END
