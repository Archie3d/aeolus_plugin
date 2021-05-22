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

#include "division.h"

AEOLUS_NAMESPACE_BEGIN

Division::Division()
    : _rankwaves{}
{
}

void Division::addRankwave(Rankwave* ptr, bool ena)
{
    jassert(ptr != nullptr);

    RankwaveRef ref { ptr, ena };
    _rankwaves.push_back(ref);
}

void Division::getAvailableRange(int& minNote, int& maxNote) const noexcept
{
    minNote = -1;
    maxNote = -1;

    for (const auto& ref : _rankwaves) {
        if (ref.enabled) {
            if (minNote < 0 || minNote > ref.rankwave->getNoteMin())
                minNote = ref.rankwave->getNoteMin();
            if (maxNote < 0 || maxNote < ref.rankwave->getNoteMax())
                maxNote = ref.rankwave->getNoteMax();
        }
    }
}

AEOLUS_NAMESPACE_END
