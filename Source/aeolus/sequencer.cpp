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

//==============================================================================

var Sequencer::DivisionState::getPersistentState() const
{
    auto* obj = new DynamicObject();

    Array<var> stopsArr;

    for (const bool s : stops)
        stopsArr.add(s);

    obj->setProperty("stops", stopsArr);
    obj->setProperty("tremulant", tremulant);

    Array<var> linksArr;

    for (const bool l : links)
        linksArr.add(l);

    obj->setProperty("links", linksArr);

    return var{obj};
}

void Sequencer::DivisionState::setPersistentState(const var& v)
{
    if (const auto* obj = v.getDynamicObject()) {
        if (const auto* stopsArr = obj->getProperty("stops").getArray()) {
            if (stopsArr->size() == stops.size()) {
                for (int i = 0; i < stops.size(); ++i)
                    stops[i] = stopsArr->getUnchecked(i);
            }
        }

        tremulant = obj->getProperty("tremulant");

        if (const auto* linksArr = obj->getProperty("links").getArray()) {
            if (linksArr->size() == links.size()) {
                for (int i = 0; i < links.size(); ++i)
                    links[i] = linksArr->getUnchecked(i);
            }
        }
    }
}

//==============================================================================

var Sequencer::OrganState::getPersistentState() const
{
    auto* obj = new DynamicObject();

    Array<var> divisionsArr;

    for (const auto& division : divisions)
        divisionsArr.add(division.getPersistentState());

    obj->setProperty("divisions", divisionsArr);

    return var{obj};
}

void Sequencer::OrganState::setPersistentState(const var& v)
{
    if (const auto* obj = v.getDynamicObject()) {
        if (const auto* divisionsArr = obj->getProperty("divisions").getArray()) {
            if (divisionsArr->size() == divisions.size()) {
                for (int i = 0; i < divisions.size(); ++i)
                    divisions[i].setPersistentState(divisionsArr->getUnchecked(i));
            }
        }
    }
}

//==============================================================================

Sequencer::Sequencer(Engine& engine, int numSteps)
    : _engine{engine}
    , _steps(numSteps)
    , _currentStep{0}
    , _dirty{true}
{
    jassert(_steps.size() > 0);

    initFromEngine();
}

var Sequencer::getPersistentState() const
{
    auto* sequencerObj = new DynamicObject();

    Array<var> stepsArr;

    for (int stepIdx = 0; stepIdx < _steps.size(); ++stepIdx)
        stepsArr.add(_steps[stepIdx].getPersistentState());

    sequencerObj->setProperty("steps", stepsArr);
    sequencerObj->setProperty("current_step", getCurrentStep());
    sequencerObj->setProperty("dirty", _dirty);

    return var{sequencerObj};
}

void Sequencer::setPersistentState(const var& v)
{
    if (auto* sequencerObj = v.getDynamicObject()) {

        if (auto* stepsArr = sequencerObj->getProperty("steps").getArray()) {
            if (stepsArr->size() == _steps.size()) {
                for (int stepIdx = 0; stepIdx < _steps.size(); ++stepIdx)
                    _steps[stepIdx].setPersistentState(stepsArr->getUnchecked(stepIdx));
            }
        }

        const int currentStep = sequencerObj->getProperty("current_step");

        if (currentStep >= 0 && currentStep < (int)_steps.size()) {
            // Don't capture current state as it is unititialised
            // and should not go into the sequencer.
            setStep(currentStep, false);
        }

        setCurrentStepDirty();

        if (auto&& v = sequencerObj->getProperty("dirty"); v.isBool())
            _dirty = (bool)v;
    }
}

void Sequencer::captureCurrentStep()
{
    captureState(_steps[_currentStep]);
    _dirty = false;
}

void Sequencer::captureStateToStep(int index)
{
    jassert(isPositiveAndBelow(index, (int)_steps.size()));

    captureState(_steps[index]);

    // Current state now matches the sequencer step, so we switch to it
    _currentStep = index;
    _dirty = false;
}

void Sequencer::setStep(int index, bool captureCurrentState)
{
    jassert(index >= 0 && index < (int)_steps.size());

    if (captureCurrentState)
        captureCurrentStep();

    _currentStep = index;
    recallState(_steps[_currentStep]);
    _dirty = false;
}

void Sequencer::stepBackward()
{
    if (_currentStep > 0)
        setStep(_currentStep - 1);
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
            step.divisions[divIdx].links.resize(division->getLinksCount());
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
            divisionState.stops[stopIdx] = division->getStopByIndex(stopIdx).isEnabled();

        // Capture tremulant
        divisionState.tremulant = division->isTremulantEnabled();

        // Capture links
        const auto numLinks = division->getLinksCount();
        for (int linkIdx = 0; linkIdx < numLinks; ++linkIdx)
            divisionState.links[linkIdx] = division->getLinkByIndex(linkIdx).enabled;
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

        // Restore tremulant
        division->setTremulantEnabled(organState.divisions[divIdx].tremulant);

        // Restore links
        for (int i = 0; i < division->getLinksCount(); ++i)
            division->enableLink(i, organState.divisions[divIdx].links[i]);
    }
}

AEOLUS_NAMESPACE_END
