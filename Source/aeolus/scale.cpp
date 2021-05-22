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

#include "scale.h"

AEOLUS_NAMESPACE_BEGIN

const Scale::Map Scale::_scales = {
    {
        Scale::Pythagorean,
        {
            1.00000000,
            1.06787109,
            1.12500000,
            1.18518519,
            1.26562500,
            1.33333333,
            1.42382812,
            1.50000000,
            1.60180664,
            1.68750000,
            1.77777778,
            1.89843750,
        }
    },
    {
        Scale::MeanQuart,
        {
            1.0000000,
            1.0449067,
            1.1180340,
            1.1962790,
            1.2500000,
            1.3374806,
            1.3975425,
            1.4953488,
            1.5625000,
            1.6718508,
            1.7888544,
            1.8691860,
        }
    },
    {
        Scale::Werckm3,
        {
            1.00000000,
            1.05349794,
            1.11740331,
            1.18518519,
            1.25282725,
            1.33333333,
            1.40466392,
            1.49492696,
            1.58024691,
            1.67043633,
            1.77777778,
            1.87924088,
        }
    },
    {
        Scale::Kirnberg3,
        {
            1.00000000,
            1.05349794,
            1.11848107,
            1.18518519,
            1.25000021,
            1.33333333,
            1.40625000,
            1.49542183,
            1.58024691,
            1.67176840,
            1.77777778,
            1.87500000,
        }
    },
    {
        Scale::WellTemp,
        {
            1.00000000,
            1.05468828,
            1.12246205,
            1.18652432,
            1.25282725,
            1.33483985,
            1.40606829,
            1.49830708,
            1.58203242,
            1.67705161,
            1.77978647,
            1.87711994,
        }
    },
    {
        Scale::EqualTemp,
        {
            1.00000000,
            1.05946309,
            1.12246205,
            1.18920712,
            1.25992105,
            1.33483985,
            1.41421356,
            1.49830708,
            1.58740105,
            1.68179283,
            1.78179744,
            1.88774863,
        }
    },
    {
        Scale::Ahrend,
        {
            1.00000000,
            1.05064661,
            1.11891853,
            1.18518519,
            1.25197868,
            1.33695184,
            1.40086215,
            1.49594019,
            1.57596992,
            1.67383521,
            1.78260246,
            1.87288523,
        }
    },
    {
        Scale::Vallotti,
        {
            1.00000000,
            1.05647631,
            1.12035146,
            1.18808855,
            1.25518740,
            1.33609659,
            1.40890022,
            1.49689777,
            1.58441623,
            1.67705160,
            1.78179744,
            1.87888722,
        }
    },
    {
        Scale::Kellner,
        {
            1.00000000,
            1.05349794,
            1.11891853,
            1.18518519,
            1.25197868,
            1.33333333,
            1.40466392,
            1.49594019,
            1.58024691,
            1.67383521,
            1.77777778,
            1.87796802,
        }
    },
    {
        Scale::Lehman,
        {
            1.00000000,
            1.05826737,
            1.11992982,
            1.18786496,
            1.25424281,
            1.33634808,
            1.41102316,
            1.49661606,
            1.58560949,
            1.67610496,
            1.77978647,
            1.88136421,
        }
    },
    {
        Scale::Pure,
        {
            1.00000000,
            1.04166667,
            1.12500000,
            1.1892,
            1.25000000,
            1.33333333,
            1.40625000,
            1.50000000,
            1.5874,
            1.66666667,
            1.77777778,
            1.87500000,
        }
    }
};

Scale::Scale(Scale::Type type)
    : _type(type)
{
}

const Scale::Table& Scale::getTable() const
{
    const auto it = _scales.find(_type);
    jassert(it != _scales.end());

    return it->second;
}

AEOLUS_NAMESPACE_END
