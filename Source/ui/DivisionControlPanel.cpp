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

#include "ui/CustomLookAndFeel.h"
#include "ui/DivisionControlPanel.h"

using namespace juce;

namespace ui {

DivisionControlPanel::DivisionControlPanel(aeolus::Division* division)
    : _division{ division }
    , _tremulantButton{ "Tremulant" }
    , _midiChannels{}
    , _gainSlider{ *division->getParamGain() }
    , _volumeLevelL{ division->volumeLevel().left, LevelIndicator::Orientation::Vertical }
    , _volumeLevelR{ division->volumeLevel().right, LevelIndicator::Orientation::Vertical }
{
    jassert(division);

    addAndMakeVisible(_midiChannels);
    _midiChannels.currentChannelsMaskProvider = [this]() -> int { return _division->getMIDIChannelsMask(); };
    _midiChannels.onChannelsSelectionChanged = [this](int mask) { _division->setMIDIChannelsMask(mask); };
    _midiChannels.updateLabel();

    _tremulantButton.setClickingTogglesState(true);
    _tremulantButton.setColour(TextButton::buttonColourId, Colour(0x66, 0x66, 0x66));
    _tremulantButton.setColour(TextButton::buttonOnColourId,  Colours::darkgreen);
    _tremulantButton.setToggleState(_division->isTremulantEnabled(), juce::dontSendNotification);

    _tremulantButton.onClick = [this]() {
        _division->setTremulantEnabled(_tremulantButton.getToggleState());
    };

    addAndMakeVisible(_tremulantButton);
    _tremulantButton.setVisible(_division->hasTremulant());

    _volumeLevelL.setSkew(0.5f);
    addAndMakeVisible(_volumeLevelL);
    _volumeLevelR.setSkew(0.5f);
    addAndMakeVisible(_volumeLevelR);

    _gainSlider.setSkewFactor(0.5f);
    addAndMakeVisible(_gainSlider);
}

void DivisionControlPanel::update()
{
    _tremulantButton.setToggleState(_division->isTremulantEnabled(), juce::dontSendNotification);
}

void DivisionControlPanel::resized()
{
    constexpr int margin = 5;
    constexpr int gainWidth = 20;

    auto bounds = getLocalBounds();
    const int itemWidth = bounds.getWidth() - 3 * margin - gainWidth;

    _gainSlider.setBounds(margin, margin, margin + gainWidth, 110);

    _volumeLevelL.setBounds(_gainSlider.getX() + 3, margin + 5, 2, _gainSlider.getHeight() - 10);
    _volumeLevelR.setBounds(_gainSlider.getX() + _gainSlider.getWidth() - 5, margin + 5, 2, _gainSlider.getHeight() - 10);

    int offset = _gainSlider.getRight();
   
    _tremulantButton.setBounds(offset + margin, 3 * margin, bounds.getRight() - 3 * margin - offset, 35);

    _midiChannels.setBounds(offset, bounds.getHeight() - 24 - 3 * margin, itemWidth, 24);
}

void DivisionControlPanel::paint(juce::Graphics& g)
{
    g.setColour(Colour(0x1F, 0x1F, 0x1F));
    g.fillRect(getLocalBounds());
}

} // namespace ui
