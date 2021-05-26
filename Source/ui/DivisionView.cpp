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

#include "DivisionView.h"

#include "aeolus/division.h"

using namespace juce;

namespace ui {

DivisionView::DivisionView(aeolus::Division* division)
    : _division(division)
    , _stopButtons{}
    , _tremulantButton{"Tremulant"}
{
}

void DivisionView::resized()
{
}

void DivisionView::populateStopButtons()
{
    _stopButtons.clear();

    if (_division == nullptr)
        return;

    for (int i = 0; i < _division->getStopsCount(); ++i) {
        auto& stop = _division->getStopByIndex(i);

        auto button = std::make_unique<TextButton>(stop.name);

        button->setToggleState(stop.enabled, juce::dontSendNotification);
        button->setClickingTogglesState(true);

        auto* ptr = button.get();

        ptr->setClickingTogglesState(true);

        button->onClick = [division=_division, i, ptr]() {
            division->getStopByIndex(i).enabled = ptr->getToggleState();
        };

        _stopButtons.add(button.release());
        addAndMakeVisible(ptr);
    }
}

} // namespace ui