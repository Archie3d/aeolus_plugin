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

#include "PluginProcessor.h"

#include "aeolus/division.h"

#include "Parameters.h"

using namespace juce;

Parameters::Parameters(AeolusAudioProcessor& proc)
    : processor(proc)
{
    processor.addParameter(reverbWet = new AudioParameterFloat("reverb_wet", "Reverb", 0.0f, 1.0f, 0.25f));
    processor.addParameter(volume = new AudioParameterFloat("volume", "Volume", 0.0f, 1.0f, 0.5f));

    auto& engine = proc.getEngine();

    for (int i = 0; i < engine.getDivisionCount(); ++i) {
        auto* division = engine.getDivisionByIndex(i);

        auto param = std::make_unique<AudioParameterFloat>(String("gain_") + String(i), division->getName() + " gain", 0.0f, 1.0f, 0.5f);
        auto* ptr = param.get();
        processor.addParameter(param.release());
        divisionsGain.push_back(ptr);
        division->setParamGain(ptr);
    }
}

var Parameters::toVar() const
{
    Array<var> params;

    for (int i = 0; i < processor.getNumParameters(); ++i) {
        const auto id = processor.getParameterID(i);
        const auto value = processor.getParameter(i);

        auto* obj = new DynamicObject();
        obj->setProperty("id", id);
        obj->setProperty("value", value);

        params.add(var{obj});
    }

    return var{params};
}

void Parameters::fromVar(const var& v)
{
    if (const auto* arr = v.getArray()) {

        const Array<AudioProcessorParameter*>& params = processor.getParameters();

        for (int i = 0; i < arr->size(); ++i) {
            if (const auto* obj = arr->getUnchecked(i).getDynamicObject()) {
                const String id = obj->getProperty("id");
                const float value = obj->getProperty("value");

                for (auto* p : params) {
                    if (auto* paramWithID = dynamic_cast<AudioProcessorParameterWithID*>(p)) {
                        if (paramWithID->paramID == id) {
                            p->setValue(value);
                            break;
                        }
                    }
                }
            }
        }
    }
}