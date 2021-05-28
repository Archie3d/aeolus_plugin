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

#include "ui/DivisionControlPanel.h"

using namespace juce;

namespace ui {

DivisionControlPanel::DivisionControlPanel(aeolus::Division* division)
    : _division{division}
    , _midiChannelLabel{{}, "MIDI channel"}
    , _midiChannelComboBox{}
    , _tremulantButton{"Tremulant"}
{
    _midiChannelLabel.setColour(Label::textColourId, Colour(0x99, 0x99, 0x99));
    auto f = _midiChannelLabel.getFont();
    f.setHeight(14);
    _midiChannelLabel.setFont(f);
    addAndMakeVisible(_midiChannelLabel);

    _midiChannelComboBox.addItem("All", 1);
    for (int i = 1; i <= 16; ++i) {
        _midiChannelComboBox.addItem(String(i), i + 1);
    }

    _midiChannelComboBox.setSelectedId(1 + _division->getMIDIChannel(), true);
    _midiChannelComboBox.onChange = [this]() {
        _division->setMIDIChannel(_midiChannelComboBox.getSelectedId() - 1);
    };

    addAndMakeVisible(_midiChannelComboBox);

    _tremulantButton.setClickingTogglesState(true);
    _tremulantButton.setColour(TextButton::buttonOnColourId, Colours::green);
    _tremulantButton.setToggleState(_division->isTremulantEnabled(), false);

    _tremulantButton.onClick = [this]() {
        _division->setTremulantEnabled(_tremulantButton.getToggleState());
    };

    addAndMakeVisible(_tremulantButton);
    _tremulantButton.setVisible(_division->hasTremulant());
}

void DivisionControlPanel::resized()
{
    constexpr int margin = 5;

    auto bounds = getLocalBounds();
    const int itemWidth = bounds.getWidth() - 2 * margin;

    _midiChannelLabel.setBounds(margin, margin, getWidth() - 2 * margin, 20);
    _midiChannelComboBox.setBounds(margin, 2*margin + 20, getWidth() - 2 * margin, 20);

    _tremulantButton.setBounds((getWidth() - 80) / 2, 5*margin + 40, 80, 40);
}

void DivisionControlPanel::paint(juce::Graphics& g)
{
    g.setColour(Colour(0x1F, 0x1F, 0x1F));
    g.fillRect(getLocalBounds());
}

} // namespace ui
