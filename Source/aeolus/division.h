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
#include "aeolus/rankwave.h"

#include <vector>

AEOLUS_NAMESPACE_BEGIN

/**
 * @brief Single keyboard division.
 * 
 * A division may have multiple stops available, which can be enabled or
 * disabled individually.
 */
class Division
{
public:

    struct RankwaveRef
    {
        Rankwave* rankwave = nullptr;
        bool enabled = false;
    };

    Division();

    void addRankwave(Rankwave* ptr, bool ena = false);

    int getStopsCount() const noexcept { return (int)_rankwaves.size(); }
    void enableStop(int i, bool ena) { _rankwaves[i].enabled = ena; }
    bool isStopEnabled(int i) const { return _rankwaves[i].enabled; }
    const RankwaveRef& operator[](int i) const { return _rankwaves[i]; }
    RankwaveRef& operator[](int i) { return _rankwaves[i]; }

    void getAvailableRange(int& minNote, int& maxNote) const noexcept;

private:

    std::vector<RankwaveRef> _rankwaves;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Division)
};

AEOLUS_NAMESPACE_END
