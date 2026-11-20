#include "HotkeyOverlay.h"

namespace ts
{

HotkeyOverlay::HotkeyOverlay()
{
    setWantsKeyboardFocus (false);
    for (auto* b : { &editButton, &reloadButton, &closeButton })
    {
        b->setWantsKeyboardFocus (false);
        b->setMouseClickGrabsKeyboardFocus (false);
        addAndMakeVisible (*b);
    }

    editButton.onClick = [] {
        const auto f = HotkeyMap::userFile();
        if (f.existsAsFile())
            f.startAsProcess(); // opens in the default text editor
    };
    reloadButton.onClick = [this] { if (onReload) onReload(); };
    closeButton.onClick = [this] { if (onClose) onClose(); };
}

void HotkeyOverlay::show (const HotkeyMap& map, const juce::StringArray& p)
{
    keys.clear();
    labels.clear();
    for (auto& a : HotkeyMap::actions())
    {
        keys.add (map.describe (a.action));
        labels.add (a.label);
    }
    problems = p;
    setVisible (true);
    toFront (false);
    repaint();
}

void HotkeyOverlay::resized()
{
    auto r = getLocalBounds().reduced (30).removeFromBottom (36);
    closeButton.setBounds (r.removeFromRight (120));
    r.removeFromRight (10);
    reloadButton.setBounds (r.removeFromRight (90));
    r.removeFromRight (10);
    editButton.setBounds (r.removeFromRight (160));
}

void HotkeyOverlay::paint (juce::Graphics& g)
{
    auto& skin = Skin::of (*this);
    const auto& pal = skin.pal();
    skin.drawOverlay (g, getLocalBounds().toFloat());

    auto r = getLocalBounds().reduced (30);
    g.setColour (pal.transport);
    g.setFont (skin.font (20.0f, true));
    g.drawText ("Hotkeys", r.removeFromTop (30), juce::Justification::left);
    g.setColour (pal.screenText.withAlpha (0.6f));
    g.setFont (skin.font (12.0f));
    g.drawText ("Click inside the plugin window first so it receives the keys. File: " + HotkeyMap::userFile().getFullPathName(),
                r.removeFromTop (18), juce::Justification::left);
    r.removeFromTop (8);
    r.removeFromBottom (46);

    if (! problems.isEmpty())
    {
        g.setColour (pal.danger);
        g.drawFittedText (problems.joinIntoString ("\n"), r.removeFromBottom (18 * juce::jmin (4, problems.size())),
                          juce::Justification::topLeft, 4);
    }

    const int columns = 3;
    const int rows = (keys.size() + columns - 1) / columns;
    const int colW = r.getWidth() / columns;
    const float rowH = juce::jmin (20.0f, (float) r.getHeight() / (float) juce::jmax (1, rows));

    for (int i = 0; i < keys.size(); ++i)
    {
        const int col = i / rows, row = i % rows;
        auto cell = juce::Rectangle<float> ((float) (r.getX() + col * colW), (float) r.getY() + (float) row * rowH, (float) colW - 10.0f, rowH);
        g.setColour (pal.transport);
        g.setFont (skin.font (12.5f, true));
        g.drawText (keys[i], cell.removeFromLeft (96.0f), juce::Justification::left);
        g.setColour (pal.screenText);
        g.setFont (skin.font (12.5f));
        g.drawText (labels[i], cell, juce::Justification::left);
    }
}

void HotkeyOverlay::mouseDown (const juce::MouseEvent&)
{
    if (onClose)
        onClose();
}

} // namespace ts
