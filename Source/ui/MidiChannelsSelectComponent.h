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

#pragma once

#include  <JuceHeader.h>
#include <functional>

namespace ui {

class MidiChannelsSelectComponent : public juce::Component
{
public:
	MidiChannelsSelectComponent(int midiChannelsMask = 0);

	// juce::Component
	void resized() override;

	std::function<void(int)> onSelectionChanged{};

private:

	void selectAll();
	void clear();
	void updateToggleButtons();

	void toggleChannel(int index, bool shouldBeSelected);
	void notifySelectionChanged();

	int _midiChannelsMask{};

	std::vector<std::unique_ptr<juce::ToggleButton>> _midiChannelButtons{};

	juce::TextButton _selectAllButton;
	juce::TextButton _clearButton;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiChannelsSelectComponent)
};

} // namespace ui
