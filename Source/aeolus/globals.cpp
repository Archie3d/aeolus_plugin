#include "aeolus/globals.h"

AEOLUS_NAMESPACE_BEGIN

namespace math {

float exp2ap(float x)
{
    int i = (int)(floor (x));
    x -= i;
    // return ldexp (1 + x * (0.66 + 0.34 * x), i);
    return ldexp (1 + x * (0.6930 + x * (0.2416 + x * (0.0517 + x * 0.0137))), i);
}

} // namespace math

AEOLUS_NAMESPACE_END
