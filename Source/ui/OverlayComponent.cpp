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

#include "ui/OverlayComponent.h"

using namespace juce;

namespace ui {

OverlayComponent::OverlayComponent()
    : Component{}
    , onClick{}
{
}

void OverlayComponent::paint(Graphics& g)
{
    g.setColour(Colour(0xCC000000));
    g.fillAll();
}

void OverlayComponent::mouseDown(const MouseEvent& event)
{
}

void OverlayComponent::mouseUp(const MouseEvent& event)
{
    if (onClick)
        onClick();
}

} // namespace ui
