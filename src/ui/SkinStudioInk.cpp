#include "Skins.h"

namespace ts
{

static Palette studioInkPalette()
{
    Palette p;
    p.background = juce::Colour (0xff0e0f13);
    p.panel = juce::Colour (0xff17181e);
    p.panelEdge = juce::Colour (0xff262932);
    p.text = juce::Colour (0xfff3f3f6);
    p.dim = juce::Colour (0xff868b97);
    p.transport = juce::Colour (0xffff8a2b);
    p.loop = juce::Colour (0xff4f9dff);
    p.repeater = juce::Colour (0xffb57bff);
    p.filter = juce::Colour (0xff38d0c4);
    p.danger = juce::Colour (0xffff4d5e);
    p.padIdle = juce::Colour (0xff21232a);
    p.screen = juce::Colour (0xff0a0b0e);
    p.screenText = p.text;
    return p;
}

StudioInkSkin::StudioInkSkin() : Skin (studioInkPalette()) {}

} // namespace ts
