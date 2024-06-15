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
#include "aeolus/division.h"
#include "ui/MidiChannelsComponent.h"
#include "ui/ParameterSlider.h"
#include "ui/LevelIndicator.h"

namespace ui {

class DivisionControlPanel : public juce::Component
{
public:
    DivisionControlPanel(aeolus::Division* division = nullptr);

    void update();

    // juce::Component
    void resized() override;
    void paint(juce::Graphics& g) override;

private:

    aeolus::Division* _division;

    juce::TextButton _tremulantButton;
    ui::MidiChannelsComponent _midiChannels;
    ui::ParameterSlider _gainSlider;
    ui::LevelIndicator _volumeLevelL;
    ui::LevelIndicator _volumeLevelR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DivisionControlPanel)
};

} // namespace ui
