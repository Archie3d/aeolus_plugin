// ----------------------------------------------------------------------------
//
//  Copyright (C) 2025 Arthur Benilov <arthur.benilov@gmail.com>
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

#include "ui/FxComponent.h"

using namespace juce;

namespace ui {

FxComponent::FxComponent()
    : _fxLabel {{}, "Effects"}
    , _okButton{"OK"}
    , _cancelButton{"Cancel"}
{
    auto* g = aeolus::EngineGlobal::getInstance();

    addAndMakeVisible(_fxLabel);
    _fxLabel.setJustificationType(Justification::centred);
    _fxLabel.setColour(Label::textColourId, Colours::lightyellow);
    auto font = _fxLabel.getFont();
    font.setHeight(font.getHeight() * 1.2f);
    _fxLabel.setFont(font);


    addAndMakeVisible(_okButton);
    _okButton.onClick = [this] {
        if (onOk) onOk();
    };

    addAndMakeVisible(_cancelButton);
    _cancelButton.onClick = [this] {
        if (onCancel) onCancel();
    };

    _cancelButton.setColour(TextButton::buttonColourId, Colour(0x66, 0x66, 0x33));
    _okButton.setColour(TextButton::buttonColourId, Colour(0x66, 0x66, 0x33));
}

void FxComponent::resized()
{
    constexpr int margin = 6;

    auto bounds = getLocalBounds();
    bounds.reduce(margin, margin);

    _fxLabel.setBounds(bounds.removeFromTop(20));

    bounds.removeFromTop(3 * margin);

    auto row = bounds.removeFromTop(20);

    row = bounds.removeFromBottom(20);
    _cancelButton.setBounds(row.removeFromRight(60));
    row.removeFromRight(margin);
    _okButton.setBounds(row.removeFromRight(60));
}


} // namespace ui
