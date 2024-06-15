// ----------------------------------------------------------------------------
//
//  Copyright (C) 2024 Arthur Benilov <arthur.benilov@gmail.com>
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

#include "ui/MidiChannelsSelectComponent.h"

using namespace juce;

namespace ui {

MidiChannelsSelectComponent::MidiChannelsSelectComponent(int midiChannelsMask)
	: _midiChannelsMask{ midiChannelsMask }
	, _titleLabel{{}, "MIDI channels"}
	, _selectAllButton{ "Select all" }
	, _clearButton{ "Clear" }
{
	for (int i = 0; i < 16; ++i) {
		auto checkBox = std::make_unique<ToggleButton>(String(i + 1));
		checkBox->onStateChange = [this, i]() {
				const bool selected{ _midiChannelButtons[i]->getToggleState() };
				toggleChannel(i, selected);
			};

		addAndMakeVisible(checkBox.get());
		_midiChannelButtons.push_back(std::move(checkBox));
	}

	addAndMakeVisible(_titleLabel);
	addAndMakeVisible(_selectAllButton);
	addAndMakeVisible(_clearButton);

	_titleLabel.setJustificationType(juce::Justification::centred);

	_selectAllButton.onClick = [&]() { selectAll(); };
	_clearButton.onClick = [&]() { clear(); };

	_selectAllButton.setColour(TextButton::buttonColourId, Colour(0x66, 0x66, 0x33));
	_clearButton.setColour(TextButton::buttonColourId, Colour(0x66, 0x66, 0x33));

	updateToggleButtons();
}

void MidiChannelsSelectComponent::resized()
{
	auto bounds{ getLocalBounds() };

	_titleLabel.setBounds(bounds.removeFromTop(20));
	bounds.removeFromTop(6);

	auto buttonsRow{ bounds.removeFromBottom(26).removeFromBottom(20) };

	int idx = 0;

	while (idx < _midiChannelButtons.size()) {
		auto row = bounds.removeFromTop(32);
		const auto w = row.getWidth() / 4;

		for (int i = 0; i < 4; ++i) {
			if (i < 3)
				_midiChannelButtons[idx++]->setBounds(row.removeFromLeft(w));
			else
				_midiChannelButtons[idx++]->setBounds(row);
		}
	}

	_clearButton.setBounds(buttonsRow.removeFromRight(60));
	buttonsRow.removeFromRight(6);
	_selectAllButton.setBounds(buttonsRow.removeFromRight(80));
}

void MidiChannelsSelectComponent::selectAll()
{
	_midiChannelsMask = (1 << _midiChannelButtons.size()) - 1;
	updateToggleButtons();
	notifySelectionChanged();
}

void MidiChannelsSelectComponent::clear()
{
	_midiChannelsMask = 0;
	updateToggleButtons();
	notifySelectionChanged();
}

void MidiChannelsSelectComponent::updateToggleButtons()
{
	int mask = 1;

	for (auto& button : _midiChannelButtons) {
		bool shouldBeOn{ (_midiChannelsMask & mask) != 0 };
		button->setToggleState(shouldBeOn, juce::dontSendNotification);
		mask = mask << 1;
	}
}

void MidiChannelsSelectComponent::toggleChannel(int index, bool shouldBeSelected)
{
	const int mask{ 1 << index };

	if (shouldBeSelected) {
		if ((_midiChannelsMask & mask) != 0)
			return;

		_midiChannelsMask |= mask;
	} else {
		if ((_midiChannelsMask & mask) == 0)
			return;

		_midiChannelsMask &= ~(mask);
	}

	notifySelectionChanged();
}

void MidiChannelsSelectComponent::notifySelectionChanged()
{
	if (onSelectionChanged)
		onSelectionChanged(_midiChannelsMask);
}

} // namespace ui
