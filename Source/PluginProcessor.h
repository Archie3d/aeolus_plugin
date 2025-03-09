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

#include <JuceHeader.h>

#include "aeolus/engine.h"

#include "Parameters.h"

//==============================================================================
/**
*/
class AeolusAudioProcessor : public juce::AudioProcessor,
                                    juce::MidiKeyboardState::Listener,
                                    aeolus::EngineGlobal::ProcessorProxy
{
public:
    //==============================================================================
    AeolusAudioProcessor();
    ~AeolusAudioProcessor() override;

    Parameters& getParametersContainer() noexcept { return _parameters; }

    void panic() noexcept { _panicRequest = true; }

    //==============================================================================
    // aeolus::EngineGlobal::ProcessorProxy
    juce::AudioProcessor* getAudioProcessor() override { return this; }
    aeolus::Engine& getEngine() override { return _engine; }
    void killAllVoices() override { panic(); }
    int getNumberOfActiveVoices() override { return _engine.getVoiceCount(); }

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool canAddBus(bool isInput) const override;
    bool canRemoveBus(bool isInput) const override;
    bool canApplyBusCountChange(bool isInput, bool isAdding, BusProperties& outProperties) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processorLayoutsChanged() override;
#endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processMidi (juce::MidiBuffer& midiMessages);

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float getProcessLoad() const noexcept { return _processLoad; }
    int getActiveVoiceCount() const noexcept { return (int) _engine.getVoiceCount(); }

    // juce::MidiKeyboardState::Listener
    void handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;

private:

    static BusesProperties getBusesProperties();

    aeolus::Engine _engine;

    juce::dsp::Limiter<float> _limiter;

    Parameters _parameters;

    std::atomic<float> _processLoad;
    std::atomic<bool> _panicRequest;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AeolusAudioProcessor)
};
