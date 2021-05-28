/*
  ==============================================================================

    PluginLookAndFeel.cpp
    Created: 27 Nov 2019 2:35:37pm
    Author:  Arthur Benilov

  ==============================================================================
*/

#include "ui/CustomLookAndFeel.h"

using namespace juce;

namespace ui {

CustomLookAndFeel::CustomLookAndFeel()
{
    setColour (Slider::thumbColourId,               Colour (240, 240, 240));
    setColour (Slider::textBoxOutlineColourId,      Colours::transparentWhite);
    setColour (Slider::trackColourId,               Colour (120, 120, 120));
    setColour (Slider::rotarySliderFillColourId,    Colour (60, 255, 200));
    setColour (Slider::rotarySliderOutlineColourId, Colours::white);
    setColour (Slider::backgroundColourId,          Colours::transparentBlack);

    setColour (TextButton::buttonColourId,  Colours::white);
    setColour (TextButton::textColourOffId, Colour (0xff00b5f6));

    setColour (TextButton::buttonOnColourId, findColour (TextButton::textColourOffId));
    setColour (TextButton::textColourOnId,   findColour (TextButton::buttonColourId));
}

void CustomLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float minSliderPos, float maxSliderPos,
                                          const Slider::SliderStyle style, Slider& slider)
{
    g.fillAll (slider.findColour (Slider::backgroundColourId));

    if (style == Slider::LinearBar || style == Slider::LinearBarVertical)
    {
        Path p;

        if (style == Slider::LinearBarVertical)
            p.addRectangle ((float) x, sliderPos, (float) width, 1.0f + height - sliderPos);
        else
            p.addRectangle ((float) x, (float) y, sliderPos - x, (float) height);

        auto baseColour = slider.findColour (Slider::rotarySliderFillColourId)
                                .withMultipliedSaturation (slider.isEnabled() ? 1.0f : 0.5f)
                                .withMultipliedAlpha (0.8);

        g.setColour (baseColour);
        g.fillPath (p);

        auto lineThickness = jmin (15.0f, jmin (width, height) * 0.45f) * 0.1f;
        g.drawRect (slider.getLocalBounds().toFloat(), lineThickness);
    }
    else
    {
        drawLinearSliderBackground (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        drawLinearSliderThumb      (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    }
}

void CustomLookAndFeel::drawLinearSliderThumb (Graphics& g, int x, int y, int width, int height,
                                               float sliderPos, float minSliderPos, float maxSliderPos,
                                               const Slider::SliderStyle style, Slider& slider)
{
    auto sliderRadius = (float) (getSliderThumbRadius (slider) - 2);

    auto isDownOrDragging = slider.isEnabled() && (slider.isMouseOverOrDragging() || slider.isMouseButtonDown());

    auto knobColour = slider.findColour (Slider::thumbColourId)
                            .withMultipliedSaturation ((slider.hasKeyboardFocus (false) || isDownOrDragging) ? 1.3f : 0.9f)
                            .withMultipliedAlpha (slider.isEnabled() ? 1.0f : 0.7f);

    if (style == Slider::LinearHorizontal || style == Slider::LinearVertical)
    {
        float kx, ky;

        if (style == Slider::LinearVertical)
        {
            kx = x + width * 0.5f;
            ky = sliderPos;
        }
        else
        {
            kx = sliderPos;
            ky = y + height * 0.5f;
        }

        auto outlineThickness = slider.isEnabled() ? 0.8f : 0.3f;

        drawRoundThumb (g,
                        kx - sliderRadius,
                        ky - sliderRadius,
                        sliderRadius * 2.0f,
                        knobColour, outlineThickness);
    }
    else
    {
        // Just call the base class for the demo
        LookAndFeel_V2::drawLinearSliderThumb (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
    }
}

void CustomLookAndFeel::drawLinearSliderBackground (Graphics& g, int x, int y, int width, int height,
                                                    float /*sliderPos*/,
                                                    float /*minSliderPos*/,
                                                    float /*maxSliderPos*/,
                                                    const Slider::SliderStyle /*style*/, Slider& slider)
{
    auto sliderRadius = getSliderThumbRadius (slider) - 5.0f;
    Path on, off;

    if (slider.isHorizontal())
    {
        auto iy = y + height * 0.5f - sliderRadius * 0.5f;
        Rectangle<float> r (x - sliderRadius * 0.5f, iy, width + sliderRadius, sliderRadius);
        auto onW = r.getWidth() * ((float) slider.valueToProportionOfLength (slider.getValue()));

        on.addRectangle (r.removeFromLeft (onW));
        off.addRectangle (r);
    }
    else
    {
        auto ix = x + width * 0.5f - sliderRadius * 0.5f;
        Rectangle<float> r (ix, y - sliderRadius * 0.5f, sliderRadius, height + sliderRadius);
        auto onH = r.getHeight() * ((float) slider.valueToProportionOfLength (slider.getValue()));

        on.addRectangle (r.removeFromBottom (onH));
        off.addRectangle (r);
    }

    float intensity = (slider.getValue() - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum());
    intensity = 0.75f * intensity + 0.25f;


    g.setColour (slider.findColour (Slider::rotarySliderFillColourId).withMultipliedBrightness (intensity));
    g.fillPath (on);

    g.setColour (slider.findColour (Slider::trackColourId));
    g.fillPath (off);
}

void CustomLookAndFeel::drawRoundThumb (Graphics& g, float x, float y, float diameter, Colour colour, float outlineThickness)
{
    auto halfThickness = outlineThickness * 0.5f;

    Path p;
    p.addEllipse (x + halfThickness,
                  y + halfThickness,
                  diameter - outlineThickness,
                  diameter - outlineThickness);

    DropShadow (Colours::black, 1, {}).drawForPath (g, p);

    g.setColour (colour);
    g.fillPath (p);

    g.setColour (colour.brighter());
    g.strokePath (p, PathStrokeType (outlineThickness));
}

} // namespace ui