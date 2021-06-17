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

#include "aeolus/sequencer.h"
#include "aeolus/engine.h"
#include "aeolus/division.h"

using namespace juce;

AEOLUS_NAMESPACE_BEGIN

Sequencer::Sequencer(Engine& engine, int numSteps)
    : _engine{engine}
    , _steps(numSteps)
    , _currentStep{0}
{
    jassert(_steps.size() > 0);

    initFromEngine();
}

void Sequencer::captureCurrentStep()
{
    captureState(_steps[_currentStep]);
}

void Sequencer::setStep(int index)
{
    jassert(index >= 0 && index < (int)_steps.size());

    captureCurrentStep();
    _currentStep = index;
    recallState(_steps[_currentStep]);
}

void Sequencer::stepForward()
{
    if (_currentStep < (int)_steps.size() - 1)
        setStep(_currentStep + 1);
}

void Sequencer::initFromEngine()
{
    const auto numDivisions = _engine.getDivisionCount();

    for (auto& step : _steps) {
        step.divisions.resize(numDivisions);

        for (int divIdx = 0; divIdx < numDivisions; ++divIdx) {
            auto* division = _engine.getDivisionByIndex(divIdx);
            step.divisions[divIdx].stops.resize(division->getStopsCount());
        }
    }
}

void Sequencer::captureState(OrganState& organState)
{
    const auto numDivisions = _engine.getDivisionCount();
    jassert(organState.divisions.size() == numDivisions);

    for (int divIdx = 0; divIdx < numDivisions; ++divIdx) {
        auto* const division = _engine.getDivisionByIndex(divIdx);
        auto& divisionState = organState.divisions[divIdx];

        const auto numStops = division->getStopsCount();
        jassert(divisionState.stops.size() == numStops);

        // Capture stops
        for (int stopIdx = 0; stopIdx < numStops; ++stopIdx)
            divisionState.stops[stopIdx] = division->getStopByIndex(stopIdx).enabled;

        // Capture tremulant
        divisionState.tremulant = division->isTremulantEnabled();
    }
}

void Sequencer::recallState(const OrganState& organState)
{
    const auto numDivisions = _engine.getDivisionCount();
    jassert(organState.divisions.size() == numDivisions);

    for (int divIdx = 0; divIdx < numDivisions; ++divIdx) {
        auto* const division = _engine.getDivisionByIndex(divIdx);

        jassert(organState.divisions[divIdx].stops.size() == division->getStopsCount());

        // Restore stops
        for (int i = 0; i < division->getStopsCount(); ++i) 
            division->enableStop(i, organState.divisions[divIdx].stops[i]);

        // REstore tremulant
        division->setTremulantEnabled(organState.divisions[divIdx].tremulant);
    }
}

AEOLUS_NAMESPACE_END
