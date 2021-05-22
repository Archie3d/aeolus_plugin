#include "CustomMidiKeyboard.h"

using namespace juce;

CustomMidiKeyboard::CustomMidiKeyboard(juce::MidiKeyboardState& state, juce::MidiKeyboardComponent::Orientation orientation)
    : MidiKeyboardComponent(state, orientation)
    , _playableRange(getRangeStart(), getRangeEnd())
{
    //setColour(MidiKeyboardComponent::mouseOverKeyOverlayColourId, Colours::pink);
    //setColour(MidiKeyboardComponent::keyDownOverlayColourId,      Colours::red);
}

void CustomMidiKeyboard::setPlayableRange(int noteMin, int noteMax)
{
    if (_playableRange.getStart() != noteMin || _playableRange.getEnd() != noteMax) {
        _playableRange = {noteMin, noteMax};
        repaint();
    }
}

void CustomMidiKeyboard::drawWhiteNote (int midiNoteNumber,
    Graphics& g, Rectangle<float> area,
    bool isDown, bool isOver,
    Colour lineColour, Colour textColour)
{
    if (!isNoteWithinPlayableRange(midiNoteNumber)) {
        g.setColour(Colours::lightgrey.darker());
        g.fillRect (area);
    }

    juce::MidiKeyboardComponent::drawWhiteNote(midiNoteNumber, g, area, isDown, isOver, lineColour, textColour);
}

void CustomMidiKeyboard::drawBlackNote (int midiNoteNumber,
    Graphics& g, Rectangle<float> area,
    bool isDown, bool isOver,
    Colour noteFillColour)
{
    auto colour = noteFillColour;

    if (!isNoteWithinPlayableRange(midiNoteNumber)) {
        colour = Colours::darkgrey.darker();
    }

    juce::MidiKeyboardComponent::drawBlackNote(midiNoteNumber, g, area, isDown, isOver, colour);
}

bool CustomMidiKeyboard::isNoteWithinPlayableRange(int note) const noexcept
{
    return note >= _playableRange.getStart() && note <= _playableRange.getEnd();
}