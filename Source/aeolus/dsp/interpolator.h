
#pragma once

#include "aeolus/globals.h"

AEOLUS_NAMESPACE_BEGIN

namespace dsp {

class Interpolator
{
public:
    Interpolator(float ratio = 1.0f);

    void setRatio(float r) noexcept { _ratio = r; }
    float getRatio() const noexcept { return _ratio; }

    void reset();

    bool canRead() const noexcept;
    bool read(float& l, float& r) noexcept;

    bool canWrite() const noexcept;
    bool write(float l, float r) noexcept;

private:
    float _accL[8];
    float _accR[8];
    int _accIndex;
    float _accFrac;

    float _ratio;
};

} // namespace dsp

AEOLUS_NAMESPACE_END

