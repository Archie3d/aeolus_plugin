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

#include <array>

AEOLUS_NAMESPACE_BEGIN

class Scale
{
public:
    enum Type {
        Pythagorean = 0,
        MeanQuart,
        Werckm3,
        Kirnberg3,
        WellTemp,
        EqualTemp,
        Ahrend,
        Vallotti,
        Kellner,
        Lehman,
        Pure,

        Total
    };

    using Table = std::array<float, 12>;
    using Map = std::map<Type, Table>;

    Scale(Type type = EqualTemp);

    const Table& getTable() const;

private:
    Type _type;

    const static Map _scales;
};

AEOLUS_NAMESPACE_END
