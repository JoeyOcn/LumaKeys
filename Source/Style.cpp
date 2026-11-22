#include "Style.h"
#include "MidiSkyData.h"

//==============================================================================
Fonts::Fonts()
{
    regular  = juce::Typeface::createSystemTypefaceFor (MidiSkyData::OutfitRegular_ttf,  (size_t) MidiSkyData::OutfitRegular_ttfSize);
    semibold = juce::Typeface::createSystemTypefaceFor (MidiSkyData::OutfitSemiBold_ttf, (size_t) MidiSkyData::OutfitSemiBold_ttfSize);
    bold     = juce::Typeface::createSystemTypefaceFor (MidiSkyData::OutfitBold_ttf,     (size_t) MidiSkyData::OutfitBold_ttfSize);
    bravura  = juce::Typeface::createSystemTypefaceFor (MidiSkyData::Bravura_otf,        (size_t) MidiSkyData::Bravura_otfSize);
}

juce::Font Fonts::ui (float height, int weight) const
{
    auto tf = weight >= 2 ? bold : (weight == 1 ? semibold : regular);
    if (tf == nullptr)
        return juce::Font (juce::FontOptions (height, weight > 0 ? juce::Font::bold : juce::Font::plain));
    return juce::Font (juce::FontOptions().withTypeface (tf).withHeight (height));
}

juce::Font Fonts::music (float emPixels) const
{
    if (bravura == nullptr)
        return juce::Font (juce::FontOptions (emPixels));
    return juce::Font (juce::FontOptions().withTypeface (bravura).withPointHeight (emPixels));
}

//==============================================================================
const std::vector<Skin>& skins()
{
    static const std::vector<Skin> s {
        { "Noche",          juce::Colour (0xff5b8cff), juce::Colour (0xffb07cff) },
        { "Aurora",         juce::Colour (0xff2bf0b5), juce::Colour (0xff6c7bff) },
        { "Atardecer",      juce::Colour (0xffff7a50), juce::Colour (0xffffc54d) },
        { "Nebulosa",       juce::Colour (0xffff4fb8), juce::Colour (0xff7d5bff) },
        { "Amanecer",       juce::Colour (0xffffb070), juce::Colour (0xffff6e9c) },
        { "Tormenta",       juce::Colour (0xff62d8ff), juce::Colour (0xffd6f3ff) },
        { "Arcoíris",       juce::Colour (0xffff5a5a), juce::Colour (0xff5a8cff), true },
        { "Personalizado",  juce::Colour (0xff4de1ff), juce::Colour (0xff4de1ff), false, true },
    };
    return s;
}

juce::Colour Palette::noteColour (int midiNote) const
{
    if (rainbow)
    {
        const int pc = ((midiNote % 12) + 12) % 12;
        // Cada nota de la escala cromática con su propio color, ordenado por quintas para que
        // los acordes consonantes queden en colores vecinos.
        const float pos = (float) ((pc * 7) % 12) / 12.0f;
        return juce::Colour::fromHSV (pos, dark ? 0.72f : 0.80f, 1.0f, 1.0f);
    }
    const float t = juce::jlimit (0.0f, 1.0f, (float) (midiNote - 21) / 87.0f);
    return a.interpolatedWith (b, t);
}

//==============================================================================
Prefs::Prefs()
{
    juce::PropertiesFile::Options o;
    o.applicationName = "MidiSky";
    o.filenameSuffix = ".settings";
    o.folderName = "MidiSky";
    o.osxLibrarySubFolder = "Application Support";
    o.commonToAllUsers = false;
    o.millisecondsBeforeSaving = 300;
    file = std::make_unique<juce::PropertiesFile> (o);

    skin     = juce::jlimit (0, (int) skins().size() - 1, file->getIntValue ("skin", 0));
    dark     = file->getBoolValue ("dark", true);
    latin    = file->getBoolValue ("latin", false);
    sevenths = file->getBoolValue ("sevenths", false);
    midiAuto = file->getBoolValue ("midiAuto", true);
    custom   = juce::Colour::fromString (file->getValue ("custom", "ff4de1ff"));
}

Prefs::~Prefs()
{
    if (file != nullptr)
        file->saveIfNeeded();
}

void Prefs::save()
{
    file->setValue ("skin", skin);
    file->setValue ("dark", dark);
    file->setValue ("latin", latin);
    file->setValue ("sevenths", sevenths);
    file->setValue ("midiAuto", midiAuto);
    file->setValue ("custom", custom.toString());
    sendChangeMessage();
}

Palette Prefs::palette() const
{
    Palette p;
    const auto& s = skins()[(size_t) skin];
    p.dark = dark;
    p.rainbow = s.rainbow;

    if (s.custom)
    {
        p.a = custom;
        p.b = custom.withRotatedHue (0.10f).withMultipliedBrightness (dark ? 1.0f : 0.92f);
    }
    else
    {
        p.a = s.a;
        p.b = s.b;
    }

    if (dark)
    {
        p.skyTop       = juce::Colour (0xff050817).interpolatedWith (p.a, 0.10f);
        p.skyBottom    = juce::Colour (0xff0d0a22).interpolatedWith (p.b, 0.13f);
        p.panel        = juce::Colours::white.withAlpha (0.045f);
        p.panelEdge    = juce::Colours::white.withAlpha (0.10f);
        p.panelShine   = juce::Colours::white.withAlpha (0.05f);
        p.text         = juce::Colour (0xffeef2ff);
        p.dim          = juce::Colour (0xff9aa6c8);
        p.faint        = juce::Colours::white.withAlpha (0.22f);
        p.keyWhite     = juce::Colour (0xffe6eaf4);
        p.keyWhiteShade= juce::Colour (0xffb9c0d4);
        p.keyEdge      = juce::Colour (0xff3a4160);
        p.keyBlack     = juce::Colour (0xff121522);
        p.keyBlackTop  = juce::Colour (0xff2b3047);
        p.ink          = juce::Colour (0xffdfe5f7);
        p.screenTop    = juce::Colour (0xff04050d).interpolatedWith (p.a, 0.06f);
        p.screenBottom = juce::Colour (0xff070817).interpolatedWith (p.b, 0.12f);
    }
    else
    {
        p.skyTop       = juce::Colour (0xffb9d8ff).interpolatedWith (p.a, 0.14f);
        p.skyBottom    = juce::Colour (0xfff3f7ff).interpolatedWith (p.b, 0.07f);
        p.panel        = juce::Colours::white.withAlpha (0.66f);
        p.panelEdge    = juce::Colour (0xff0b1a3a).withAlpha (0.10f);
        p.panelShine   = juce::Colours::white.withAlpha (0.55f);
        p.text         = juce::Colour (0xff111c38);
        p.dim          = juce::Colour (0xff56637f);
        p.faint        = juce::Colour (0xff0b1a3a).withAlpha (0.18f);
        p.keyWhite     = juce::Colour (0xfffbfcff);
        p.keyWhiteShade= juce::Colour (0xffd5dbe8);
        p.keyEdge      = juce::Colour (0xff9aa3ba);
        p.keyBlack     = juce::Colour (0xff1b1f30);
        p.keyBlackTop  = juce::Colour (0xff3b4159);
        p.ink          = juce::Colour (0xff1a2340);
        p.screenTop    = juce::Colour (0xff0d1838).interpolatedWith (p.a, 0.10f);
        p.screenBottom = juce::Colour (0xff1c2a5a).interpolatedWith (p.b, 0.16f);
    }

    p.screenEdge = p.a.withAlpha (dark ? 0.30f : 0.45f);
    p.screenText = juce::Colour (0xfff3f6ff);
    p.screenDim  = juce::Colour (0xffa9b4d6);
    return p;
}
