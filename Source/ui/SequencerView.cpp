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
{
    jassert(sequencer != nullptr);
    populateStepButtons();
}

void SequencerView::resized()
{
    constexpr int buttonWidth = 25;
    constexpr int buttonPadding = 3;

    int buttonsWidth = buttonWidth * _sequencer->getStepsCount()
        + buttonPadding * (_sequencer->getStepsCount() - 1);

    int x = (getWidth() - buttonsWidth) / 2;

    for (auto* button : _stepButtons) {
        button->setBounds(x, buttonPadding, buttonWidth, getHeight() - 2 * buttonPadding);
        x += buttonWidth + buttonPadding;
    }
}

void SequencerView::populateStepButtons()
{
    const auto numSteps = _sequencer->getStepsCount();

    for (int i = 0; i < numSteps; ++i) {
        auto button = std::make_unique<TextButton>(String(i + 1));
        addAndMakeVisible(button.get());
        _stepButtons.add(button.release());
    }
}

} // namespace ui
