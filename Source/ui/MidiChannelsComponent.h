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

#include <JuceHeader.h>

namespace ui {

class MidiChannelsComponent : public juce::Component
{
public:

	MidiChannelsComponent();

	// juce::Component
	void resized() override;

private:

	juce::DrawableButton _midiButton;
	juce::Label _channelsLabel;


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiChannelsComponent)
};

} // namespace ui
