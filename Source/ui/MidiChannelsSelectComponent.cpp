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

MidiChannelsSelectComponent::MidiChannelsSelectComponent()
{
	for (int i = 0; i < 16; ++i) {
		auto checkBox = std::make_unique<ToggleButton>(String(i + 1));
		addAndMakeVisible(checkBox.get());
		_midiChannelButtons.push_back(std::move(checkBox));
	}
}

void MidiChannelsSelectComponent::resized()
{
	auto bounds{ getLocalBounds() };

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
}

} // namespace ui
