#pragma once

#include <JuceHeader.h>

class CustomMidiKeyboard : public juce::MidiKeyboardComponent
{
public:
    CustomMidiKeyboard(juce::MidiKeyboardState& state, juce::MidiKeyboardComponent::Orientation orientation);

    void setPlayableRange(int noteMin, int noteMax);

protected:

    // juce::MidiKeyboardComponent
    void drawWhiteNote(int midiNoteNumber,
                       juce::Graphics& g, juce::Rectangle<float> area,
                       bool isDown, bool isOver,
                       juce::Colour lineColour, juce::Colour textColour) override;

    void drawBlackNote(int midiNoteNumber,
                       juce::Graphics& g, juce::Rectangle<float> area,
                       bool isDown, bool isOver,
                       juce::Colour noteFillColour) override;

private:

    bool isNoteWithinPlayableRange(int note) const noexcept;

    juce::Range<int> _playableRange;
};
