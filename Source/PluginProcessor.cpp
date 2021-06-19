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

#include <chrono>

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================
AeolusAudioProcessor::AeolusAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       )
    , _engine{}
    , _parameters(*this)
    , _processLoad{0.0f}
    , _panicRequest{false}
{
    _engine.getMidiKeyboardState().addListener(this);
}

AeolusAudioProcessor::~AeolusAudioProcessor()
{
    // If we don't do this there is a leaked AccessibilityHandler instance detected.
    _engine.getMidiKeyboardState().removeListener(this);
}

//==============================================================================
const juce::String AeolusAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AeolusAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AeolusAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AeolusAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AeolusAudioProcessor::getTailLengthSeconds() const
{
    return _engine.getReverbLengthInSeconds();
}

int AeolusAudioProcessor::getNumPrograms()
{
    // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
    return jmax(1, _engine.getSequencer()->getStepsCount());
}

int AeolusAudioProcessor::getCurrentProgram()
{
    return _engine.getSequencer()->getCurrentStep();
}

void AeolusAudioProcessor::setCurrentProgram (int index)
{
    if (index >= 0 && index < _engine.getSequencer()->getStepsCount())
        _engine.getSequencer()->setStep(index);
}

const juce::String AeolusAudioProcessor::getProgramName (int index)
{
    return juce::String("Sequencer step ") + juce::String(index + 1);
}

void AeolusAudioProcessor::changeProgramName (int /* index */, const juce::String& /* newName */)
{
}

//==============================================================================
void AeolusAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    _engine.prepareToPlay((float)sampleRate, samplesPerBlock);
}

void AeolusAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AeolusAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AeolusAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    using namespace std::chrono;

    auto timestampStart = high_resolution_clock::now();

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    processMidi (midiMessages);

    float* outL = buffer.getWritePointer (0);
    float* outR = outL;

    if (totalNumOutputChannels > 1)
        outR = buffer.getWritePointer (1);

    _engine.setReverbWet(_parameters.reverbWet->get());
    _engine.setVolume(_parameters.volume->get());
    _engine.process(outL, outR, (size_t) buffer.getNumSamples(), isNonRealtime());

    auto timestampStop = high_resolution_clock::now();
    auto duration_us = duration_cast<microseconds> (timestampStop - timestampStart).count();

    float realTime_us = 1e6f * (float) buffer.getNumSamples() / _engine.getSampleRate();
    float load = duration_us / realTime_us;

    _processLoad = jmin (1.0f, 0.99f * _processLoad + 0.01f * load);

    if (_panicRequest) {
        _engine.allNotesOff();
        _panicRequest = false;
    }
}

void AeolusAudioProcessor::processMidi (juce::MidiBuffer& midiMessages)
{
    if (midiMessages.getNumEvents() == 0)
        return;

    for (auto msgIter : midiMessages) {
        const auto msg = msgIter.getMessage();

        _engine.processMIDIMessage(msg);
    }

}

//==============================================================================
bool AeolusAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AeolusAudioProcessor::createEditor()
{
    return new AeolusAudioProcessorEditor (*this);
}

//==============================================================================
void AeolusAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = _engine.getPersistentState();

    if (auto* obj = state.getDynamicObject())
        obj->setProperty("parameters", _parameters.toVar());

    MemoryOutputStream stream(destData, false);

    JSON::writeToStream(stream, state);
}

void AeolusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    MemoryInputStream stream(data, sizeInBytes, false);

    const auto state = JSON::parse(stream);

    if (const auto* obj = state.getDynamicObject()) {
        _parameters.fromVar(obj->getProperty("parameters"));
    }

    _engine.setPersistentState(state);
}

//==============================================================================

void AeolusAudioProcessor::handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float /* velocity */)
{
    if (MessageManager::getInstance()->isThisTheMessageThread()) {
        _engine.postNoteEvent(true, midiNoteNumber, midiChannel);
    } else {
        _engine.noteOn(midiNoteNumber, midiChannel);
    }
}

void AeolusAudioProcessor::handleNoteOff(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float /* velocity */)
{
    if (MessageManager::getInstance()->isThisTheMessageThread()) {
        _engine.postNoteEvent(false, midiNoteNumber, midiChannel);
    } else {
        _engine.noteOff(midiNoteNumber, midiChannel);
    }
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AeolusAudioProcessor();
}
