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

#include "ui/PresetView.h"

using namespace juce;

namespace ui {

PresetView::PresetView()
    : Component{}
    , _presetNameLabel{}
{
    addAndMakeVisible(_presetNameLabel);

    _presetNameLabel.setJustificationType(Justification::centred);
    _presetNameLabel.setText("Preset name here", NotificationType::dontSendNotification);
}

void PresetView::resized() {
    auto bounds{ getLocalBounds() };

    _presetNameLabel.setBounds(bounds);
}

void PresetView::paint(Graphics& g)
{
    auto bounds{ getLocalBounds().reduced(1) };
    g.setColour(Colours::darkolivegreen);
    g.drawRoundedRectangle(bounds.toFloat(), 12.0f, 2.0f);
}

} // namespace ui
