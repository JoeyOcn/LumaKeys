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
        { "Celeste",        juce::Colour (0xff62bfff) },
        { "Ámbar",          juce::Colour (0xffffb23e) },
        { "Fósforo",        juce::Colour (0xff5ef2a0) },
        { "Rosa",           juce::Colour (0xffff5c9a) },
        { "Violeta",        juce::Colour (0xffa58cff) },
        { "Coral",          juce::Colour (0xffff6f52) },
        { "Hielo",          juce::Colour (0xffd6e9ff) },
        { "Arcoíris",       juce::Colour (0xffff6b6b), true },
        { "Personalizado",  juce::Colour (0xff4de1ff), false, true },
    };
    return s;
}

juce::Colour Palette::noteColour (int midiNote) const
{
    if (rainbow)
    {
        // Cada nota con su color, ordenadas por quintas: los acordes consonantes quedan en colores vecinos.
        const int pc = ((midiNote % 12) + 12) % 12;
        const float pos = (float) ((pc * 7) % 12) / 12.0f;
        return juce::Colour::fromHSV (pos, 0.62f, 1.0f, 1.0f);
    }
    return accent;
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
    const auto& s = skins()[(size_t) juce::jlimit (0, (int) skins().size() - 1, skin)];
    p.dark = dark;
    p.rainbow = s.rainbow;
    p.accent = s.custom ? custom : s.light;
    if (p.rainbow) p.accent = juce::Colour (0xffe9e3d6);
    p.onAccent = p.accent.getPerceivedBrightness() > 0.55f ? juce::Colour (0xff111216) : juce::Colours::white;

    if (dark)
    {
        p.bodyTop       = juce::Colour (0xff1b1c20);
        p.bodyBottom    = juce::Colour (0xff121316);
        p.plate         = juce::Colour (0xff202227);
        p.plateEdge     = juce::Colour (0xff0a0b0d);
        p.plateHighlight= juce::Colours::white.withAlpha (0.07f);
        p.well          = juce::Colour (0xff15161a);
        p.wellEdge      = juce::Colour (0xff08090b);
        p.text          = juce::Colour (0xffe8e9ec);
        p.dim           = juce::Colour (0xff8b9099);
        p.faint         = juce::Colours::white.withAlpha (0.13f);
        p.keyWhite      = juce::Colour (0xffeeeeea);
        p.keyWhiteShade = juce::Colour (0xffcfd0cc);
        p.keyEdge       = juce::Colour (0xff6d7078);
        p.keyBlack      = juce::Colour (0xff0e0f12);
        p.keyBlackTop   = juce::Colour (0xff2a2c32);
        p.ink           = juce::Colour (0xffd9dbe0);
    }
    else
    {
        p.bodyTop       = juce::Colour (0xffd8dadd);
        p.bodyBottom    = juce::Colour (0xffc8cbcf);
        p.plate         = juce::Colour (0xffe7e8ea);
        p.plateEdge     = juce::Colour (0xff9da2aa);
        p.plateHighlight= juce::Colours::white.withAlpha (0.85f);
        p.well          = juce::Colour (0xffd2d5d9);
        p.wellEdge      = juce::Colour (0xffa3a8b0);
        p.text          = juce::Colour (0xff1c1e22);
        p.dim           = juce::Colour (0xff5d636c);
        p.faint         = juce::Colour (0xff1c1e22).withAlpha (0.16f);
        p.keyWhite      = juce::Colour (0xfffbfbf8);
        p.keyWhiteShade = juce::Colour (0xffdfe0dc);
        p.keyEdge       = juce::Colour (0xff8d929a);
        p.keyBlack      = juce::Colour (0xff15161a);
        p.keyBlackTop   = juce::Colour (0xff3a3d44);
        p.ink           = juce::Colour (0xff1d1f24);
    }

    p.felt        = juce::Colour (0xff6e1d26);
    p.screen      = juce::Colour (0xff07080a);
    p.screenEdge  = juce::Colour (0xff000000);
    p.screenText  = p.rainbow ? juce::Colour (0xfff1eee6) : p.accent.interpolatedWith (juce::Colours::white, 0.35f);
    p.screenDim   = p.screenText.withAlpha (0.58f);
    p.screenFaint = p.screenText.withAlpha (0.16f);
    return p;
}
