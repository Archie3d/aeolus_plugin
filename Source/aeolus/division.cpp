
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
