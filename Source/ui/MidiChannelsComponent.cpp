// ----------------------------------------------------------------------------
//
//  Copyright (C) 2024 Arthur Benilov <arthur.benilov@gmail.com>
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

#include "ui/MidiChannelsSelectComponent.h"
#include "ui/MidiChannelsComponent.h"

using namespace juce;

namespace ui {

MidiChannelsComponent::MidiChannelsComponent()
	: _midiButton{ "midiButton", DrawableButton::ImageFitted },
	  _channelsLabel{}
{
	addAndMakeVisible(_midiButton);
	addAndMakeVisible(_channelsLabel);

    auto loadSVG = [](const char* data, size_t size) -> std::unique_ptr<Drawable> {
        if (auto xml = parseXML(String::fromUTF8(data, (int)size))) {
            return Drawable::createFromSVG(*xml);
        }
        return nullptr;
    };

    auto normalIcon = loadSVG(BinaryData::midi_svg, BinaryData::midi_svgSize);
    auto hoverIcon = loadSVG(BinaryData::midihover_svg, BinaryData::midihover_svgSize);
    _midiButton.setImages(normalIcon.get(), hoverIcon.get());
    _midiButton.setMouseCursor(MouseCursor::PointingHandCursor);

    _midiButton.onClick = [this] {
        int mask{};

        if (currentChannelsMaskProvider)
            mask = currentChannelsMaskProvider();

        auto content = std::make_unique<ui::MidiChannelsSelectComponent>(mask);
        content->setSize(240, 190);
        auto* contentPtr = content.get();
        
        content->onSelectionChanged = [this](int mask) {
            if (onChannelsSelectionChanged)
                onChannelsSelectionChanged(mask);

            updateLabel();
        };

        auto* parent = getParentComponent();
        while (parent != nullptr && dynamic_cast<juce::AudioProcessorEditor*>(parent) == nullptr)
            parent = parent->getParentComponent();

        if (parent == nullptr)
            parent = getTopLevelComponent();

        jassert(parent != nullptr);

        if (parent == nullptr)
            return;

        auto bounds = parent->getLocalArea(&_midiButton, _midiButton.getBounds());

        auto& box = CallOutBox::launchAsynchronously(
            std::move(content),
            bounds,
            parent);
    };

    _channelsLabel.setColour(Label::textColourId, Colours::lightyellow);

    auto f = _channelsLabel.getFont();
    f.setHeight(14);
    _channelsLabel.setFont(f);
}

void MidiChannelsComponent::resized()
{
    auto bounds = getLocalBounds();

    _midiButton.setBounds(bounds.removeFromLeft(32));

    _channelsLabel.setBounds(bounds);
}

void MidiChannelsComponent::updateLabel()
{
    _channelsLabel.setText(getMidiChannelsAsText(), juce::dontSendNotification);
}

String MidiChannelsComponent::getMidiChannelsAsText() const
{
    if (!currentChannelsMaskProvider)
        return {};

    int midiChannelsMask = currentChannelsMaskProvider();

    if (midiChannelsMask == 0)
        return "None";

    if (midiChannelsMask == ((1 << 16) - 1))
        return "Any";

    String text{};
    int prevChannel = 0;
    int prevCounter = 0;

    for (int i = 0; i < 16; i++) {
        int mask = 1 << i;

        if ((midiChannelsMask & mask) == 0) {
            if (prevCounter == 2)
                text += "," + String(prevChannel + 1);
            else if (prevCounter > 2)
                text += "-" + String(prevChannel + 1);
            prevCounter = 0;
            continue;
        }

        if (prevCounter == 0) {
            if (text.length() > 0)
                text += ",";
            text += String(i + 1);
            prevChannel = i;
            prevCounter = 1;
        } else {
            if (prevChannel == i - 1) {
                prevChannel = i;
                prevCounter += 1;
            } else {
                if (prevCounter < 2)
                    text += ",";
                else
                    text += "-";
                text += String(i + 1);
                prevChannel = i;
                prevCounter = 0;
            }
        }
    }

    if (prevCounter == 2)
        text += "," + String(prevChannel + 1);
    else if (prevCounter > 2)
        text += "-" + String(prevChannel + 1);

    return text;
}

} // namespace ui

