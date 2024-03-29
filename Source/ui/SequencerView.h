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

#include "aeolus/globals.h"
#include "aeolus/sequencer.h"

namespace ui {

class SequencerView : public juce::Component
{
public:

    class Listener
    {
    public:
        virtual ~Listener() = default;

        virtual void onSequencerEnterProgramMode() = 0;
        virtual void onSequencerLeaveProgramMode() = 0;
    };

    SequencerView(aeolus::Sequencer* sequencer);

    void update();
    void setDirty() { _sequencer->setCurrentStepDirty(); }

    void cancelProgramMode();

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    int getOptimalWidth() const;

    // juce::Component
    void resized() override;

    constexpr static int buttonWidth = 25;
    constexpr static int buttonPadding = 3;

private:

    constexpr static int radioGroupId = 1001;

    void populateStepButtons();

    void enterProgramMode();

    aeolus::Sequencer* _sequencer;
    juce::OwnedArray<juce::TextButton> _stepButtons;

    juce::TextButton _setButton;
    juce::TextButton _backwardButton;
    juce::TextButton _forwardButton;

    bool _programMode;

    juce::ListenerList<Listener> _listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequencerView)
};

} // namespace ui
