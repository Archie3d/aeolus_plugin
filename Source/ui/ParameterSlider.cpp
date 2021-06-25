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

#include "ui/ParameterSlider.h"

using namespace juce;

namespace ui {

struct ParameterSlider::Impl
{

    /// Helper used to translate slider value to parameter and back.
    struct Handler : public Slider::Listener,
                     public Timer
    {
        ParameterSlider::Impl &impl;
        AudioProcessorParameter &parameter;

        Handler(ParameterSlider::Impl &im, AudioProcessorParameter &p)
            : impl(im)
            , parameter(p)
        {
            startTimerHz (20);
        }

        void sliderValueChanged(Slider *pSlider) override
        {
            if (pSlider->isMouseButtonDown())
                parameter.setValueNotifyingHost(static_cast<float>(pSlider->getValue()));
            else
                parameter.setValue(static_cast<float>(pSlider->getValue()));
        }

        void sliderDragStarted(Slider*) override
        {
            parameter.beginChangeGesture();
        }

        void sliderDragEnded(Slider*) override
        {
            parameter.endChangeGesture();
        }

        void timerCallback() override
        {
            impl.updateSliderPosition();
        }

    };

    ParameterSlider &slider;    // Reference to interface object.
    Handler handler;            // Slider handler

    Impl (ParameterSlider &s, AudioProcessorParameter &p)
        : slider(s)
        , handler(*this, p)
    {
        slider.addListener(&handler);
    }

    void updateSliderPosition()
    {
        auto newValue = handler.parameter.getValue();

        if ((newValue != static_cast<float>(slider.getValue())) && !slider.isMouseButtonDown()) {
            slider.setValue (newValue);
        }
    }

    double getValueFromText(const String &text)
    {
        return handler.parameter.getValueForText(text);
    }

    String getTextFromValue(double value)
    {
        return handler.parameter.getText(static_cast<float>(value), 1024)
            + " " + handler.parameter.getLabel();
    }

};

//----------------------------------------------------------

ParameterSlider::ParameterSlider (AudioProcessorParameter &p, SliderStyle style, TextEntryBoxPosition textBoxPosition)
    : Slider(style, textBoxPosition)
    , d(std::make_unique<Impl>(*this, p))
{
    setRange(0.0, 1.0, 0.0);
    setDoubleClickReturnValue(true, p.getDefaultValue());
}

ParameterSlider::~ParameterSlider() = default;

double ParameterSlider::getValueFromText(const String &text)
{
    return d->getValueFromText(text);
}

String ParameterSlider::getTextFromValue(double value)
{
    return d->getTextFromValue(value);
}

} // namespace ui
