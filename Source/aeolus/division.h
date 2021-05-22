
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
