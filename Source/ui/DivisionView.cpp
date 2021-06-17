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

#include "aeolus/engine.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/DivisionView.h"

using namespace juce;

namespace ui {

constexpr int paddingTop = 30;
constexpr int paddingBottom = 5;
constexpr int buttonSize = 80;

DivisionView::DivisionView(aeolus::Division* division)
    : _division(division)
    , _nameLabel{{}, division->getName()}
    , _cancelButton{"All OFF"}
    , _controlPanel(division)
    , _stopButtons{}
{
    _nameLabel.setJustificationType(Justification::centred);
    _nameLabel.setColour(Label::textColourId, Colour(0xCC, 0xCC, 0x99));
    auto font = CustomLookAndFeel::getManualLabelFont();
    font.setHeight(22);
    _nameLabel.setFont(font);

    addAndMakeVisible(_nameLabel);
    addAndMakeVisible(_cancelButton);
    _cancelButton.setColour(TextButton::buttonColourId, Colour(0x66, 0x66, 0x33));
    _cancelButton.onClick = [this]() {
        cancelAllStops();
    };

    addAndMakeVisible(_controlPanel);

    populateStopButtons();
    populateLinkButtons();
}

void DivisionView::cancelAllStops()
{
    if (_division == nullptr)
        return;

    for (int i = 0; i < _division->getStopsCount(); ++i) {
        auto& stop = _division->getStopByIndex(i);

        auto* button = _stopButtons.getUnchecked(i);

        stop.enabled = false;
        button->setToggleState(false, dontSendNotification);
    }
}

void DivisionView::cancelAllLinks()
{
    if (_division == nullptr)
        return;

    _division->cancelAllLinks();

    for (auto& button : _linkButtons)
        button->setToggleState(false, dontSendNotification);
}

constexpr int controlPanelWidth = 130;

int DivisionView::getEstimatedHeightForWidth(int width) const
{
    const int nButtonsInRow = (width - controlPanelWidth) / buttonSize;
    const int nRows = _stopButtons.size() / nButtonsInRow + (_stopButtons.size() % nButtonsInRow > 0 ? 1 : 0);
    return nRows * buttonSize + paddingTop + paddingBottom;
}

void DivisionView::resized()
{
    _nameLabel.setBounds(0, 0, getWidth() - controlPanelWidth, paddingTop);

    _cancelButton.setBounds(getWidth() - controlPanelWidth - 50, 10, 40, 15);

    int x = 10;

    for (auto* linkButton : _linkButtons) {
        linkButton->setBounds(x, 10, 40, 15);
        x += 50;
    }

    FlexBox fbox;
    fbox.flexWrap = FlexBox::Wrap::wrap;
    fbox.justifyContent = FlexBox::JustifyContent::center;
    fbox.alignContent = FlexBox::AlignContent::center;

    for (auto* button : _stopButtons)
        fbox.items.add(FlexItem(*button).withWidth(buttonSize).withHeight(buttonSize));

    auto bounds = getLocalBounds();
    bounds.setTop(paddingTop);
    bounds.setBottom(bounds.getHeight() - paddingTop - paddingBottom);
    bounds.setWidth(bounds.getWidth() - controlPanelWidth);

    fbox.performLayout(bounds.toFloat());

    // For some reason the performLayout ignores the target rectangle
    // top and left coordinates, so we reposition all the buttons to respect
    // the padding.
    for (auto* button : _stopButtons) {
        auto b = button->getBounds();
        b.setY(b.getY() + paddingTop);
        button->setBounds(b);
    }

    _controlPanel.setBounds(getWidth() - controlPanelWidth, 0, controlPanelWidth, getHeight());
}

void DivisionView::paint(Graphics& g)
{
    ColourGradient grad = ColourGradient::vertical(Colour(0x31, 0x2F, 0x2F), 0, Colour(0x1F, 0x1F, 0x1F), (float)getHeight());
    g.setGradientFill(grad);
    g.fillRect(getLocalBounds());
}

void DivisionView::populateStopButtons()
{
    _stopButtons.clear();

    if (_division == nullptr)
        return;

    for (int i = 0; i < _division->getStopsCount(); ++i) {
        auto button = std::make_unique<StopButton>(*_division, i);
        auto* ptr = button.get();

        _stopButtons.add(button.release());
        addAndMakeVisible(ptr);
    }
}

void DivisionView::populateLinkButtons()
{
    jassert(_division != nullptr);
    
    for (int i = 0; i < _division->getLinksCount(); ++i) {
        auto& link = _division->getLinkByIndex(i);

        const String caption = _division->getMnemonic() + " + " + link.division->getMnemonic();
        auto button = std::make_unique<TextButton>(caption);
        auto* ptr = button.get();
        ptr->setColour(TextButton::textColourOffId, Colour(0x99, 0x99, 0x99));
        ptr->setColour(TextButton::textColourOnId, Colour(0xFF, 0xFF, 0xFF));
        ptr->setColour(TextButton::buttonColourId, Colour(0x33, 0x33, 0x33));
        ptr->setColour(TextButton::buttonOnColourId, Colours::darkgreen);

        ptr->setClickingTogglesState(true);
        ptr->setToggleState(link.enabled, juce::dontSendNotification);

        button->onClick = [division=_division, i, ptr] {
            division->enableLink(i, ptr->getToggleState());
        };

        _linkButtons.add(button.release());
        addAndMakeVisible(ptr);
    }

    auto& engine = _division->getEngine();

    for (int i = 0; i < engine.getDivisionCount(); ++i) {
        auto* div = engine.getDivisionByIndex(i);

        if (div == _division)
            break;
    }
}

} // namespace ui