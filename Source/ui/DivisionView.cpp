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

#include "ui/DivisionView.h"

using namespace juce;

namespace ui {

DivisionView::DivisionView(aeolus::Division* division)
    : _division(division)
    , _stopButtons{}
    , _tremulantButton{"Tremulant"}
{
    populateStopButtons();
}

void DivisionView::resized()
{
    FlexBox fbox;
    fbox.flexWrap = FlexBox::Wrap::wrap;
    fbox.justifyContent = FlexBox::JustifyContent::center;
    fbox.alignContent = FlexBox::AlignContent::center;

    for (auto* button : _stopButtons)
        fbox.items.add(FlexItem(*button).withMinWidth(80).withMinHeight(80));

    fbox.performLayout(getLocalBounds().toFloat());
}

void DivisionView::populateStopButtons()
{
    _stopButtons.clear();

    if (_division == nullptr)
        return;

    for (int i = 0; i < _division->getStopsCount(); ++i) {
        auto& stop = _division->getStopByIndex(i);

        auto button = std::make_unique<StopButton>(stop.name);

        button->setToggleState(stop.enabled, juce::dontSendNotification);
        auto* ptr = button.get();

        button->onClick = [division=_division, i, ptr]() {
            division->getStopByIndex(i).enabled = ptr->getToggleState();
        };

        _stopButtons.add(button.release());
        addAndMakeVisible(ptr);
    }
}

} // namespace ui