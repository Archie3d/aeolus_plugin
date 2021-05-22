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
