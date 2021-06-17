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

#pragma once

#include "aeolus/globals.h"
#include "aeolus/division.h"

namespace ui {

/**
 * @brief Toggle button associated with a particular stop on a division.
 */
class StopButton final : public juce::Button,
                         public aeolus::Division::Listener
{
public:
    StopButton(aeolus::Division& division, int stopIndex);
    ~StopButton();
    void setMargin(int m) noexcept { _margin = m; }

    // aeolus::Division::Listener
    void stopEnablementChanged(int stopIndex) override;

protected:
    // juce::Button
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:

    aeolus::Division& _division;
    int _stopIndex;
    aeolus::Division::Stop& _stop;

    int _margin;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StopButton)
};

} // namespace ui
