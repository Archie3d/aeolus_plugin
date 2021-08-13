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

#include "ui/SequencerView.h"

using namespace juce;

namespace ui {

SequencerView::SequencerView(aeolus::Sequencer* sequencer)
    : Component{}
    , _sequencer{sequencer}
    , _stepButtons{}
    , _setButton{"Set"}
    , _advanceButton{">>"}
    , _programMode{false}
    , _listeners{}
{
    jassert(sequencer != nullptr);
    populateStepButtons();

    _setButton.setColour(TextButton::buttonColourId, Colour(0x46, 0x60, 0x16));
    _setButton.onClick = [this]() {
            enterProgramMode();
        };

    _advanceButton.setColour(TextButton::buttonColourId, Colour(0x46, 0x60, 0x16));
    _advanceButton.onClick = [this]() {
            _sequencer->stepForward();
        };
    addAndMakeVisible(_setButton);
    addAndMakeVisible(_advanceButton);
}

void SequencerView::update()
{
    const auto currentStep = _sequencer->getCurrentStep();

    for (int i = 0; i < _stepButtons.size(); ++i)
        _stepButtons[i]->setToggleState(i == currentStep && !_sequencer->isCurrentStepDirty(), juce::dontSendNotification);
}

void SequencerView::cancelProgramMode()
{
    _programMode = false;
    _setButton.setEnabled(true);
    _advanceButton.setEnabled(true);

    _listeners.call([](Listener& listener){ listener.onSequencerLeaveProgramMode(); });
}

void SequencerView::addListener(Listener* listener)
{
    jassert(listener != nullptr);
    _listeners.add(listener);
}

void SequencerView::removeListener(Listener* listener)
{
    jassert(listener != nullptr);
    _listeners.remove(listener);
}

void SequencerView::resized()
{
    constexpr int buttonWidth = 25;
    constexpr int buttonPadding = 3;

    int buttonsWidth = buttonWidth * _sequencer->getStepsCount()
        + buttonPadding * (_sequencer->getStepsCount() - 1);

    // Account for the set and advance button
    buttonsWidth += 4 * (buttonWidth + buttonPadding) + 2 * buttonPadding;

    int x = (getWidth() - buttonsWidth) / 2;

    _setButton.setBounds(x, buttonPadding, 2 * buttonWidth, getHeight() - 2 * buttonPadding);

    x += 2 * (buttonWidth + buttonPadding);

    for (auto* button : _stepButtons) {
        button->setBounds(x, buttonPadding, buttonWidth, getHeight() - 2 * buttonPadding);
        x += buttonWidth + buttonPadding;
    }

    x += buttonPadding;

    _advanceButton.setBounds(x, buttonPadding, 2 * buttonWidth, getHeight() - 2 * buttonPadding);
}

void SequencerView::populateStepButtons()
{
    const auto numSteps = _sequencer->getStepsCount();

    for (int i = 0; i < numSteps; ++i) {
        auto button = std::make_unique<TextButton>(String(i + 1));
        //button->setClickingTogglesState(true);
        button->setColour(TextButton::buttonColourId, Colour(0x40, 0x33, 0x33));
        button->setColour(TextButton::buttonOnColourId, Colour(0xDF, 0xC0, 0x36));
        //button->setRadioGroupId(radioGroupId);

        if (_sequencer->getCurrentStep() == i)
            button->setToggleState(true, juce::dontSendNotification);

        button->onClick = [ptr=button.get(), index = i, this]() {
            if (_programMode) {
                // Program current state
                _sequencer->captureStateToStep(index);
                cancelProgramMode();
            } else {
                // Select a step
                _sequencer->setStep(index, false); // Do not capture current state
                //if (ptr->getToggleState()) {
                //    _sequencer->setStep(index);
                //}
            }
        };

        addAndMakeVisible(button.get());
        _stepButtons.add(button.release());
    }
}

void SequencerView::enterProgramMode()
{
    _programMode = true;
    _setButton.setEnabled(false);
    _advanceButton.setEnabled(false);

    _listeners.call([](Listener& listener){ listener.onSequencerEnterProgramMode(); });
}

} // namespace ui
