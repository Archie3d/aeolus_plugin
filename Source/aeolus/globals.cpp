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

#include "aeolus/globals.h"

AEOLUS_NAMESPACE_BEGIN

juce::File getCustomOrganConfigFile()
{
    const static String organConfigJSON{ "organ_config.json" };

    // Try current working directory first
    auto file{ juce::File::File::getCurrentWorkingDirectory()
        .getChildFile(organConfigJSON)
    };

    if (file.existsAsFile())
        return file;

    // Fall back onto the global configuiration
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("Aeolus")
        .getChildFile("organ_config.json");
}

//==============================================================================

namespace math {

float exp2ap(float x)
{
    int i = (int)(floor (x));
    x -= i;
    // return ldexp (1 + x * (0.66 + 0.34 * x), i);
    return ldexp (1 + x * (0.6930f + x * (0.2416f + x * (0.0517f + x * 0.0137f))), i);
}

} // namespace math

AEOLUS_NAMESPACE_END
