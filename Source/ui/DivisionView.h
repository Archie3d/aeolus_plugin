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

#include "ui/DivisionControlPanel.h"
#include "ui/StopButton.h"

namespace ui {

class DivisionView : public juce::Component
{
public:

    DivisionView(aeolus::Division* division = nullptr);

    void update();

    void cancelAllStops();
    void cancelAllLinks();
    void cancelTremulant();

    int getEstimatedHeightForWidth(int width) const;

    // juce::Component
    void resized() override;
    void paint(juce::Graphics&) override;

private:

    void populateStopButtons();
    void populateLinkButtons();

    aeolus::Division* _division;

    juce::Label _nameLabel;
    juce::TextButton _cancelButton;         ///< Disable all stops.
    juce::OwnedArray<juce::TextButton> _linkButtons;
    DivisionControlPanel _controlPanel;
    juce::OwnedArray<StopButton> _stopButtons;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DivisionView)
};

} // namespace ui
