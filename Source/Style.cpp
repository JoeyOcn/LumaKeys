#include "Style.h"
#include "MidiSkyData.h"

//==============================================================================
static juce::Typeface::Ptr loadFace (const void* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
}

Fonts::Fonts()
{
    regular    = loadFace (MidiSkyData::ZenKakuGothicNewRegular_ttf, MidiSkyData::ZenKakuGothicNewRegular_ttfSize);
    medium     = loadFace (MidiSkyData::ZenKakuGothicNewMedium_ttf,  MidiSkyData::ZenKakuGothicNewMedium_ttfSize);
    bold       = loadFace (MidiSkyData::ZenKakuGothicNewBold_ttf,    MidiSkyData::ZenKakuGothicNewBold_ttfSize);
    serifBold  = loadFace (MidiSkyData::ShipporiMinchoBold_ttf,      MidiSkyData::ShipporiMinchoBold_ttfSize);
    serifXBold = loadFace (MidiSkyData::ShipporiMinchoExtraBold_ttf, MidiSkyData::ShipporiMinchoExtraBold_ttfSize);
    pixelFace  = loadFace (MidiSkyData::PressStart2PRegular_ttf,     MidiSkyData::PressStart2PRegular_ttfSize);
    pixelSmallFace = loadFace (MidiSkyData::Jersey10Regular_ttf,     MidiSkyData::Jersey10Regular_ttfSize);
    bravura    = loadFace (MidiSkyData::Bravura_otf,                 MidiSkyData::Bravura_otfSize);
}

static juce::Font faceFont (const juce::Typeface::Ptr& tf, float height, int fallbackStyle)
{
    if (tf == nullptr)
        return juce::Font (juce::FontOptions (height, fallbackStyle));
    return juce::Font (juce::FontOptions().withTypeface (tf).withHeight (height));
}

juce::Font Fonts::ui (float height, int weight) const
{
    return faceFont (weight >= 2 ? bold : (weight == 1 ? medium : regular), height, weight >= 2 ? juce::Font::bold : juce::Font::plain);
}

juce::Font Fonts::serif (float height, bool extraBold) const
{
    return faceFont (extraBold ? serifXBold : serifBold, height, juce::Font::bold);
}

juce::Font Fonts::pixel (float height) const
{
    // Press Start 2P llena todo el cuadro: se compensa para que mida como las demás
    return faceFont (pixelFace, height * 0.62f, juce::Font::plain);
}

juce::Font Fonts::pixelLabel (float height) const
{
    return faceFont (pixelSmallFace, height * 1.12f, juce::Font::plain);
}

juce::Font Fonts::display (FontKind k, float height) const
{
    switch (k)
    {
        case FontKind::serif: return serif (height, true);
        case FontKind::pixel: return pixel (height * 0.82f);
        case FontKind::sans:  break;
    }
    return ui (height, 2);
}

juce::Font Fonts::label (FontKind k, float height, int weight) const
{
    if (k == FontKind::pixel) return pixelLabel (height);
    return ui (height, weight);
}

juce::Font Fonts::music (float emPixels) const
{
    if (bravura == nullptr)
        return juce::Font (juce::FontOptions (emPixels));
    return juce::Font (juce::FontOptions().withTypeface (bravura).withPointHeight (emPixels));
}

//==============================================================================
juce::Image Textures::get (Tex t)
{
    if (t == Tex::none) return {};
    const int key = (int) t;
    auto it = cache.find (key);
    if (it != cache.end()) return it->second;

    const void* data = nullptr; int size = 0;
    switch (t)
    {
        case Tex::nogal:     data = MidiSkyData::nogal_jpg;     size = MidiSkyData::nogal_jpgSize; break;
        case Tex::hinoki:    data = MidiSkyData::hinoki_jpg;    size = MidiSkyData::hinoki_jpgSize; break;
        case Tex::musgo:     data = MidiSkyData::musgo_jpg;     size = MidiSkyData::musgo_jpgSize; break;
        case Tex::papel:     data = MidiSkyData::papel_jpg;     size = MidiSkyData::papel_jpgSize; break;
        case Tex::washi:     data = MidiSkyData::washi_jpg;     size = MidiSkyData::washi_jpgSize; break;
        case Tex::agua:      data = MidiSkyData::agua_jpg;      size = MidiSkyData::agua_jpgSize; break;
        case Tex::cosmos:    data = MidiSkyData::cosmos_jpg;    size = MidiSkyData::cosmos_jpgSize; break;
        case Tex::metal:     data = MidiSkyData::metal_jpg;     size = MidiSkyData::metal_jpgSize; break;
        case Tex::aluminio:  data = MidiSkyData::aluminio_jpg;  size = MidiSkyData::aluminio_jpgSize; break;
        case Tex::pergamino: data = MidiSkyData::pergamino_jpg; size = MidiSkyData::pergamino_jpgSize; break;
        case Tex::none: break;
    }
    juce::Image img;
    if (data != nullptr)
        img = juce::ImageFileFormat::loadFrom (data, (size_t) size);
    cache[key] = img;
    return img;
}

//==============================================================================
static std::vector<Skin> buildSkins()
{
    std::vector<Skin> v;
    {
        Skin s;
        s.id = "nogal"; s.name = U8 ("Nogal");
        s.body = Tex::nogal; s.paper = Tex::papel; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xfff3e5cd);
        s.dim = juce::Colour (0xffc9ad8a);
        s.rail = juce::Colour ((juce::uint8) 20, (juce::uint8) 10, (juce::uint8) 4, 0.46f);
        s.railBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 222, (juce::uint8) 175, 0.14f);
        s.well = juce::Colour ((juce::uint8) 255, (juce::uint8) 232, (juce::uint8) 200, 0.07f);
        s.wellBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 222, (juce::uint8) 175, 0.16f);
        s.atril = juce::Colour ((juce::uint8) 18, (juce::uint8) 9, (juce::uint8) 3, 0.26f);
        s.lip = juce::Colour (0xff2a160b);
        s.ink = juce::Colour (0xff2c1e12);
        s.noteInk = juce::Colour (0xff3a2414);
        s.fall = juce::Colour ((juce::uint8) 16, (juce::uint8) 8, (juce::uint8) 3, 0.5f);
        s.chord = juce::Colour (0xffe2bd76);
        s.accent = juce::Colour (0xffd6ad63);
        s.accentInk = juce::Colour (0xff24160a);
        s.light = juce::Colour (0xfff0b25c);
        s.felt = juce::Colour (0xff7a1d28);
        s.keyBed = juce::Colour (0xff120a05);
        s.keyWhite = juce::Colour (0xfff2e9d6);
        s.keyShade = juce::Colour (0xffd8cbb0);
        s.keyBlack = juce::Colour (0xff1b110b);
        s.keyLabel = juce::Colour (0xff6b5236);
        s.markOn = juce::Colour (0xffa6762e);
        s.keysBlock = juce::Colour ((juce::uint8) 16, (juce::uint8) 8, (juce::uint8) 3, 0.45f);
        s.cheek = juce::Colour ((juce::uint8) 28, (juce::uint8) 15, (juce::uint8) 6, 0.55f);
        s.knob = juce::Colour (0xffc9a35c);
        s.knobLine = juce::Colour (0xff2a1a0c);
        s.knobCap = juce::Colour (0xfff6ead2);
        s.switchOn = juce::Colour (0xffb98a45);
        s.ring = juce::Colour ((juce::uint8) 255, (juce::uint8) 235, (juce::uint8) 205, 0.05f);
        s.ringLine = juce::Colour ((juce::uint8) 255, (juce::uint8) 225, (juce::uint8) 185, 0.2f);
        s.region = juce::Colour ((juce::uint8) 214, (juce::uint8) 173, (juce::uint8) 99, 0.2f);
        s.regionMinor = juce::Colour ((juce::uint8) 214, (juce::uint8) 173, (juce::uint8) 99, 0.12f);
        s.center = juce::Colour (0xff23130a);
        s.constLine = juce::Colour ((juce::uint8) 255, (juce::uint8) 236, (juce::uint8) 205, 0.75f);
        s.tablet = juce::Colour ((juce::uint8) 255, (juce::uint8) 232, (juce::uint8) 200, 0.06f);
        s.tabletOn = juce::Colour ((juce::uint8) 214, (juce::uint8) 173, (juce::uint8) 99, 0.24f);
        s.tabletBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 222, (juce::uint8) 175, 0.14f);
        s.bodyColour = juce::Colour (0xff3b2213);
        s.bodyColour2 = juce::Colour (0xff24140a);
        s.paperColour = juce::Colour (0xffefe4cf);
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "laca"; s.name = U8 ("Laca y oro");
        s.body = Tex::none; s.paper = Tex::none; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xffefe6d3);
        s.dim = juce::Colour (0xffa8977c);
        s.rail = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.railBorder = juce::Colour ((juce::uint8) 207, (juce::uint8) 166, (juce::uint8) 90, 0.22f);
        s.well = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.04f);
        s.wellBorder = juce::Colour ((juce::uint8) 207, (juce::uint8) 166, (juce::uint8) 90, 0.25f);
        s.atril = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.02f);
        s.lip = juce::Colour (0xff17140f);
        s.ink = juce::Colour (0xffcfa65a);
        s.noteInk = juce::Colour (0xfff1d595);
        s.fall = juce::Colour ((juce::uint8) 0, (juce::uint8) 0, (juce::uint8) 0, 0.35f);
        s.chord = juce::Colour (0xffd9b064);
        s.accent = juce::Colour (0xffcfa65a);
        s.accentInk = juce::Colour (0xff15110a);
        s.light = juce::Colour (0xffe2bd6a);
        s.felt = juce::Colour (0xff2b3f8c);
        s.keyBed = juce::Colour (0xff050505);
        s.keyWhite = juce::Colour (0xfff5f1e8);
        s.keyShade = juce::Colour (0xffddd6c6);
        s.keyBlack = juce::Colour (0xff0d0c0b);
        s.keyLabel = juce::Colour (0xff7c6a4b);
        s.markOn = juce::Colour (0xff2b3f8c);
        s.keysBlock = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.02f);
        s.cheek = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.knob = juce::Colour (0xffcfa65a);
        s.knobLine = juce::Colour (0xff15110a);
        s.knobCap = juce::Colour (0xfff5efe2);
        s.switchOn = juce::Colour (0xff2b3f8c);
        s.ring = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.02f);
        s.ringLine = juce::Colour ((juce::uint8) 207, (juce::uint8) 166, (juce::uint8) 90, 0.35f);
        s.region = juce::Colour ((juce::uint8) 207, (juce::uint8) 166, (juce::uint8) 90, 0.16f);
        s.regionMinor = juce::Colour ((juce::uint8) 207, (juce::uint8) 166, (juce::uint8) 90, 0.09f);
        s.center = juce::Colour (0xff0c0b09);
        s.constLine = juce::Colour ((juce::uint8) 241, (juce::uint8) 213, (juce::uint8) 149, 0.8f);
        s.tablet = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.tabletOn = juce::Colour ((juce::uint8) 207, (juce::uint8) 166, (juce::uint8) 90, 0.2f);
        s.tabletBorder = juce::Colour ((juce::uint8) 207, (juce::uint8) 166, (juce::uint8) 90, 0.22f);
        s.bodyColour = juce::Colour (0xff0e0d0b);
        s.bodyColour2 = juce::Colour (0xff060505);
        s.paperColour = juce::Colour (0xff14110c);
        s.lacquer = true;
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "musgo"; s.name = U8 ("Musgo");
        s.body = Tex::musgo; s.paper = Tex::papel; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xffece9d7);
        s.dim = juce::Colour (0xffaeb393);
        s.rail = juce::Colour ((juce::uint8) 10, (juce::uint8) 13, (juce::uint8) 6, 0.55f);
        s.railBorder = juce::Colour ((juce::uint8) 205, (juce::uint8) 225, (juce::uint8) 170, 0.14f);
        s.well = juce::Colour ((juce::uint8) 230, (juce::uint8) 240, (juce::uint8) 210, 0.06f);
        s.wellBorder = juce::Colour ((juce::uint8) 205, (juce::uint8) 225, (juce::uint8) 170, 0.16f);
        s.atril = juce::Colour ((juce::uint8) 8, (juce::uint8) 10, (juce::uint8) 5, 0.35f);
        s.lip = juce::Colour (0xff1d1e14);
        s.ink = juce::Colour (0xff2a2a1b);
        s.noteInk = juce::Colour (0xff2f3b17);
        s.fall = juce::Colour ((juce::uint8) 8, (juce::uint8) 10, (juce::uint8) 5, 0.55f);
        s.chord = juce::Colour (0xffd7e3b0);
        s.accent = juce::Colour (0xff93b85d);
        s.accentInk = juce::Colour (0xff141a0b);
        s.light = juce::Colour (0xffa9d86e);
        s.felt = juce::Colour (0xff35411f);
        s.keyBed = juce::Colour (0xff0b0c07);
        s.keyWhite = juce::Colour (0xffebe3c9);
        s.keyShade = juce::Colour (0xffcfc6a7);
        s.keyBlack = juce::Colour (0xff181a11);
        s.keyLabel = juce::Colour (0xff5c5a40);
        s.markOn = juce::Colour (0xff5b7b2e);
        s.keysBlock = juce::Colour ((juce::uint8) 8, (juce::uint8) 10, (juce::uint8) 5, 0.5f);
        s.cheek = juce::Colour ((juce::uint8) 16, (juce::uint8) 20, (juce::uint8) 10, 0.6f);
        s.knob = juce::Colour (0xff8d9a6a);
        s.knobLine = juce::Colour (0xff141a0b);
        s.knobCap = juce::Colour (0xffeef0dd);
        s.switchOn = juce::Colour (0xff6f8f3d);
        s.ring = juce::Colour ((juce::uint8) 230, (juce::uint8) 240, (juce::uint8) 210, 0.04f);
        s.ringLine = juce::Colour ((juce::uint8) 205, (juce::uint8) 225, (juce::uint8) 170, 0.22f);
        s.region = juce::Colour ((juce::uint8) 147, (juce::uint8) 184, (juce::uint8) 93, 0.22f);
        s.regionMinor = juce::Colour ((juce::uint8) 147, (juce::uint8) 184, (juce::uint8) 93, 0.13f);
        s.center = juce::Colour (0xff15180e);
        s.constLine = juce::Colour ((juce::uint8) 225, (juce::uint8) 240, (juce::uint8) 195, 0.75f);
        s.tablet = juce::Colour ((juce::uint8) 230, (juce::uint8) 240, (juce::uint8) 210, 0.05f);
        s.tabletOn = juce::Colour ((juce::uint8) 147, (juce::uint8) 184, (juce::uint8) 93, 0.25f);
        s.tabletBorder = juce::Colour ((juce::uint8) 205, (juce::uint8) 225, (juce::uint8) 170, 0.14f);
        s.bodyColour = juce::Colour (0xff2b2a20);
        s.bodyColour2 = juce::Colour (0xff16160f);
        s.paperColour = juce::Colour (0xffe6dcc3);
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "hinoki"; s.name = U8 ("Hinoki y washi");
        s.body = Tex::hinoki; s.paper = Tex::washi; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xff2a2119);
        s.dim = juce::Colour (0xff6c5b46);
        s.rail = juce::Colour ((juce::uint8) 255, (juce::uint8) 251, (juce::uint8) 242, 0.42f);
        s.railBorder = juce::Colour ((juce::uint8) 90, (juce::uint8) 62, (juce::uint8) 32, 0.2f);
        s.well = juce::Colour ((juce::uint8) 255, (juce::uint8) 252, (juce::uint8) 246, 0.55f);
        s.wellBorder = juce::Colour ((juce::uint8) 90, (juce::uint8) 62, (juce::uint8) 32, 0.22f);
        s.atril = juce::Colour ((juce::uint8) 255, (juce::uint8) 250, (juce::uint8) 240, 0.3f);
        s.lip = juce::Colour (0xffb8956a);
        s.ink = juce::Colour (0xff1f1a14);
        s.noteInk = juce::Colour (0xff1f3556);
        s.fall = juce::Colour ((juce::uint8) 255, (juce::uint8) 250, (juce::uint8) 240, 0.38f);
        s.chord = juce::Colour (0xff1f1a14);
        s.accent = juce::Colour (0xff2f4a73);
        s.accentInk = juce::Colour (0xfff7f2e8);
        s.light = juce::Colour (0xff4f7bb7);
        s.felt = juce::Colour (0xff9b2b24);
        s.keyBed = juce::Colour (0xff2a2119);
        s.keyWhite = juce::Colour (0xfffbf8f0);
        s.keyShade = juce::Colour (0xffe5ded0);
        s.keyBlack = juce::Colour (0xff221b14);
        s.keyLabel = juce::Colour (0xff8a7a64);
        s.markOn = juce::Colour (0xff2f4a73);
        s.keysBlock = juce::Colour ((juce::uint8) 90, (juce::uint8) 62, (juce::uint8) 32, 0.18f);
        s.cheek = juce::Colour ((juce::uint8) 255, (juce::uint8) 251, (juce::uint8) 242, 0.5f);
        s.knob = juce::Colour (0xffd8c29b);
        s.knobLine = juce::Colour (0xff2a2119);
        s.knobCap = juce::Colour (0xffffffff);
        s.switchOn = juce::Colour (0xff2f4a73);
        s.ring = juce::Colour ((juce::uint8) 255, (juce::uint8) 252, (juce::uint8) 246, 0.45f);
        s.ringLine = juce::Colour ((juce::uint8) 90, (juce::uint8) 62, (juce::uint8) 32, 0.28f);
        s.region = juce::Colour ((juce::uint8) 47, (juce::uint8) 74, (juce::uint8) 115, 0.16f);
        s.regionMinor = juce::Colour ((juce::uint8) 47, (juce::uint8) 74, (juce::uint8) 115, 0.09f);
        s.center = juce::Colour (0xfff6efe1);
        s.constLine = juce::Colour ((juce::uint8) 31, (juce::uint8) 26, (juce::uint8) 20, 0.65f);
        s.tablet = juce::Colour ((juce::uint8) 255, (juce::uint8) 252, (juce::uint8) 246, 0.5f);
        s.tabletOn = juce::Colour ((juce::uint8) 47, (juce::uint8) 74, (juce::uint8) 115, 0.14f);
        s.tabletBorder = juce::Colour ((juce::uint8) 90, (juce::uint8) 62, (juce::uint8) 32, 0.2f);
        s.bodyColour = juce::Colour (0xffdcc39a);
        s.bodyColour2 = juce::Colour (0xffcdb084);
        s.paperColour = juce::Colour (0xfff4efe4);
        s.seal = true;
        s.dark = false;
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "acuatica"; s.name = U8 ("Acuática");
        s.body = Tex::agua; s.paper = Tex::none; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xffe4f6f7);
        s.dim = juce::Colour (0xff8ec0c6);
        s.rail = juce::Colour ((juce::uint8) 2, (juce::uint8) 22, (juce::uint8) 28, 0.55f);
        s.railBorder = juce::Colour ((juce::uint8) 120, (juce::uint8) 220, (juce::uint8) 225, 0.16f);
        s.well = juce::Colour ((juce::uint8) 160, (juce::uint8) 235, (juce::uint8) 240, 0.07f);
        s.wellBorder = juce::Colour ((juce::uint8) 120, (juce::uint8) 220, (juce::uint8) 225, 0.2f);
        s.atril = juce::Colour ((juce::uint8) 2, (juce::uint8) 18, (juce::uint8) 24, 0.35f);
        s.lip = juce::Colour (0xff031a21);
        s.ink = juce::Colour (0xffd9f4f5);
        s.noteInk = juce::Colour (0xff7ff0ff);
        s.fall = juce::Colour ((juce::uint8) 1, (juce::uint8) 14, (juce::uint8) 19, 0.55f);
        s.chord = juce::Colour (0xffa9f1f4);
        s.accent = juce::Colour (0xff46cfc4);
        s.accentInk = juce::Colour (0xff032026);
        s.light = juce::Colour (0xff5ee6ff);
        s.felt = juce::Colour (0xff0d5566);
        s.keyBed = juce::Colour (0xff021015);
        s.keyWhite = juce::Colour (0xffedf8f8);
        s.keyShade = juce::Colour (0xffcfe6e8);
        s.keyBlack = juce::Colour (0xff062a31);
        s.keyLabel = juce::Colour (0xff4b7d84);
        s.markOn = juce::Colour (0xff1f9aa6);
        s.keysBlock = juce::Colour ((juce::uint8) 1, (juce::uint8) 14, (juce::uint8) 19, 0.5f);
        s.cheek = juce::Colour ((juce::uint8) 2, (juce::uint8) 22, (juce::uint8) 28, 0.6f);
        s.knob = juce::Colour (0xff8fd8dc);
        s.knobLine = juce::Colour (0xff032026);
        s.knobCap = juce::Colour (0xfff2fdfd);
        s.switchOn = juce::Colour (0xff2aa9a3);
        s.ring = juce::Colour ((juce::uint8) 160, (juce::uint8) 235, (juce::uint8) 240, 0.05f);
        s.ringLine = juce::Colour ((juce::uint8) 120, (juce::uint8) 220, (juce::uint8) 225, 0.25f);
        s.region = juce::Colour ((juce::uint8) 70, (juce::uint8) 207, (juce::uint8) 196, 0.2f);
        s.regionMinor = juce::Colour ((juce::uint8) 70, (juce::uint8) 207, (juce::uint8) 196, 0.11f);
        s.center = juce::Colour (0xff03171d);
        s.constLine = juce::Colour ((juce::uint8) 200, (juce::uint8) 250, (juce::uint8) 252, 0.75f);
        s.tablet = juce::Colour ((juce::uint8) 160, (juce::uint8) 235, (juce::uint8) 240, 0.06f);
        s.tabletOn = juce::Colour ((juce::uint8) 70, (juce::uint8) 207, (juce::uint8) 196, 0.24f);
        s.tabletBorder = juce::Colour ((juce::uint8) 120, (juce::uint8) 220, (juce::uint8) 225, 0.18f);
        s.bodyColour = juce::Colour (0xff063440);
        s.bodyColour2 = juce::Colour (0xff021219);
        s.paperColour = juce::Colour ((juce::uint8) 190, (juce::uint8) 240, (juce::uint8) 245, 0.08f);
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "cosmica"; s.name = U8 ("Cósmica");
        s.body = Tex::cosmos; s.paper = Tex::none; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xffece8ff);
        s.dim = juce::Colour (0xffa7a0d4);
        s.rail = juce::Colour ((juce::uint8) 6, (juce::uint8) 4, (juce::uint8) 18, 0.55f);
        s.railBorder = juce::Colour ((juce::uint8) 190, (juce::uint8) 170, (juce::uint8) 255, 0.16f);
        s.well = juce::Colour ((juce::uint8) 210, (juce::uint8) 195, (juce::uint8) 255, 0.07f);
        s.wellBorder = juce::Colour ((juce::uint8) 190, (juce::uint8) 170, (juce::uint8) 255, 0.2f);
        s.atril = juce::Colour ((juce::uint8) 6, (juce::uint8) 4, (juce::uint8) 18, 0.35f);
        s.lip = juce::Colour (0xff0b0820);
        s.ink = juce::Colour (0xffe2d9ff);
        s.noteInk = juce::Colour (0xffffc2ec);
        s.fall = juce::Colour ((juce::uint8) 5, (juce::uint8) 3, (juce::uint8) 14, 0.55f);
        s.chord = juce::Colour (0xffe7dcff);
        s.accent = juce::Colour (0xffb08cff);
        s.accentInk = juce::Colour (0xff120a28);
        s.light = juce::Colour (0xffff9ad9);
        s.felt = juce::Colour (0xff2b1a58);
        s.keyBed = juce::Colour (0xff05030c);
        s.keyWhite = juce::Colour (0xfff3f0ff);
        s.keyShade = juce::Colour (0xffd9d2ef);
        s.keyBlack = juce::Colour (0xff120c24);
        s.keyLabel = juce::Colour (0xff6c6394);
        s.markOn = juce::Colour (0xff8a6be0);
        s.keysBlock = juce::Colour ((juce::uint8) 5, (juce::uint8) 3, (juce::uint8) 14, 0.5f);
        s.cheek = juce::Colour ((juce::uint8) 10, (juce::uint8) 6, (juce::uint8) 26, 0.6f);
        s.knob = juce::Colour (0xffb9a6f0);
        s.knobLine = juce::Colour (0xff120a28);
        s.knobCap = juce::Colour (0xfff7f3ff);
        s.switchOn = juce::Colour (0xff7d5be0);
        s.ring = juce::Colour ((juce::uint8) 210, (juce::uint8) 195, (juce::uint8) 255, 0.04f);
        s.ringLine = juce::Colour ((juce::uint8) 190, (juce::uint8) 170, (juce::uint8) 255, 0.25f);
        s.region = juce::Colour ((juce::uint8) 176, (juce::uint8) 140, (juce::uint8) 255, 0.2f);
        s.regionMinor = juce::Colour ((juce::uint8) 176, (juce::uint8) 140, (juce::uint8) 255, 0.11f);
        s.center = juce::Colour (0xff0b0720);
        s.constLine = juce::Colour ((juce::uint8) 240, (juce::uint8) 230, (juce::uint8) 255, 0.75f);
        s.tablet = juce::Colour ((juce::uint8) 210, (juce::uint8) 195, (juce::uint8) 255, 0.06f);
        s.tabletOn = juce::Colour ((juce::uint8) 176, (juce::uint8) 140, (juce::uint8) 255, 0.24f);
        s.tabletBorder = juce::Colour ((juce::uint8) 190, (juce::uint8) 170, (juce::uint8) 255, 0.18f);
        s.bodyColour = juce::Colour (0xff120a2c);
        s.bodyColour2 = juce::Colour (0xff05030e);
        s.paperColour = juce::Colour ((juce::uint8) 220, (juce::uint8) 205, (juce::uint8) 255, 0.06f);
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "futurista"; s.name = U8 ("Futurista");
        s.body = Tex::metal; s.paper = Tex::none; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xffe6f3f6);
        s.dim = juce::Colour (0xff89a0a8);
        s.rail = juce::Colour ((juce::uint8) 6, (juce::uint8) 9, (juce::uint8) 12, 0.55f);
        s.railBorder = juce::Colour ((juce::uint8) 53, (juce::uint8) 224, (juce::uint8) 255, 0.22f);
        s.well = juce::Colour ((juce::uint8) 53, (juce::uint8) 224, (juce::uint8) 255, 0.05f);
        s.wellBorder = juce::Colour ((juce::uint8) 53, (juce::uint8) 224, (juce::uint8) 255, 0.26f);
        s.atril = juce::Colour ((juce::uint8) 5, (juce::uint8) 8, (juce::uint8) 10, 0.4f);
        s.lip = juce::Colour (0xff0a0e12);
        s.ink = juce::Colour (0xffa7ecff);
        s.noteInk = juce::Colour (0xff35e0ff);
        s.fall = juce::Colour ((juce::uint8) 3, (juce::uint8) 6, (juce::uint8) 8, 0.6f);
        s.chord = juce::Colour (0xffbff6ff);
        s.accent = juce::Colour (0xff35e0ff);
        s.accentInk = juce::Colour (0xff031a20);
        s.light = juce::Colour (0xff35e0ff);
        s.felt = juce::Colour (0xff0b3640);
        s.keyBed = juce::Colour (0xff05080a);
        s.keyWhite = juce::Colour (0xffe8eef0);
        s.keyShade = juce::Colour (0xffc7d1d5);
        s.keyBlack = juce::Colour (0xff0c0f12);
        s.keyLabel = juce::Colour (0xff5a7078);
        s.markOn = juce::Colour (0xff1aa9c4);
        s.keysBlock = juce::Colour ((juce::uint8) 3, (juce::uint8) 6, (juce::uint8) 8, 0.5f);
        s.cheek = juce::Colour ((juce::uint8) 6, (juce::uint8) 9, (juce::uint8) 12, 0.6f);
        s.knob = juce::Colour (0xff9fb4bc);
        s.knobLine = juce::Colour (0xff031a20);
        s.knobCap = juce::Colour (0xffeef6f8);
        s.switchOn = juce::Colour (0xff14a9c6);
        s.ring = juce::Colour ((juce::uint8) 53, (juce::uint8) 224, (juce::uint8) 255, 0.03f);
        s.ringLine = juce::Colour ((juce::uint8) 53, (juce::uint8) 224, (juce::uint8) 255, 0.3f);
        s.region = juce::Colour ((juce::uint8) 53, (juce::uint8) 224, (juce::uint8) 255, 0.16f);
        s.regionMinor = juce::Colour ((juce::uint8) 53, (juce::uint8) 224, (juce::uint8) 255, 0.09f);
        s.center = juce::Colour (0xff071014);
        s.constLine = juce::Colour ((juce::uint8) 170, (juce::uint8) 240, (juce::uint8) 255, 0.8f);
        s.tablet = juce::Colour ((juce::uint8) 53, (juce::uint8) 224, (juce::uint8) 255, 0.04f);
        s.tabletOn = juce::Colour ((juce::uint8) 53, (juce::uint8) 224, (juce::uint8) 255, 0.18f);
        s.tabletBorder = juce::Colour ((juce::uint8) 53, (juce::uint8) 224, (juce::uint8) 255, 0.22f);
        s.bodyColour = juce::Colour (0xff1c2127);
        s.bodyColour2 = juce::Colour (0xff0d1014);
        s.paperColour = juce::Colour ((juce::uint8) 0, (juce::uint8) 0, (juce::uint8) 0, 0.35f);
        s.chamfer = true;
        s.displayFont = FontKind::sans;
        s.radius = 4.0f;
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "retro"; s.name = U8 ("Retro");
        s.body = Tex::aluminio; s.paper = Tex::papel; s.cheekTex = Tex::nogal;
        s.text = juce::Colour (0xff1d1f21);
        s.dim = juce::Colour (0xff545a60);
        s.rail = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.35f);
        s.railBorder = juce::Colour ((juce::uint8) 0, (juce::uint8) 0, (juce::uint8) 0, 0.18f);
        s.well = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.55f);
        s.wellBorder = juce::Colour ((juce::uint8) 0, (juce::uint8) 0, (juce::uint8) 0, 0.2f);
        s.atril = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.22f);
        s.lip = juce::Colour (0xff8d9296);
        s.ink = juce::Colour (0xff2a2520);
        s.noteInk = juce::Colour (0xffb8520f);
        s.fall = juce::Colour (0xff120d06);
        s.chord = juce::Colour (0xffffb03a);
        s.accent = juce::Colour (0xffff7a1a);
        s.accentInk = juce::Colour (0xffffffff);
        s.light = juce::Colour (0xffffb03a);
        s.felt = juce::Colour (0xff8a1f12);
        s.keyBed = juce::Colour (0xff151515);
        s.keyWhite = juce::Colour (0xfff7f4ec);
        s.keyShade = juce::Colour (0xffe0dccf);
        s.keyBlack = juce::Colour (0xff191919);
        s.keyLabel = juce::Colour (0xff7a7468);
        s.markOn = juce::Colour (0xffd8621a);
        s.keysBlock = juce::Colour ((juce::uint8) 0, (juce::uint8) 0, (juce::uint8) 0, 0.12f);
        s.cheek = juce::Colour ((juce::uint8) 0, (juce::uint8) 0, (juce::uint8) 0, 0.15f);
        s.knob = juce::Colour (0xffd9dcdf);
        s.knobLine = juce::Colour (0xff1d1f21);
        s.knobCap = juce::Colour (0xffffffff);
        s.switchOn = juce::Colour (0xffff7a1a);
        s.ring = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.4f);
        s.ringLine = juce::Colour ((juce::uint8) 0, (juce::uint8) 0, (juce::uint8) 0, 0.22f);
        s.region = juce::Colour ((juce::uint8) 255, (juce::uint8) 122, (juce::uint8) 26, 0.18f);
        s.regionMinor = juce::Colour ((juce::uint8) 255, (juce::uint8) 122, (juce::uint8) 26, 0.1f);
        s.center = juce::Colour (0xffe9ebec);
        s.constLine = juce::Colour ((juce::uint8) 30, (juce::uint8) 30, (juce::uint8) 30, 0.65f);
        s.tablet = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.5f);
        s.tabletOn = juce::Colour ((juce::uint8) 255, (juce::uint8) 122, (juce::uint8) 26, 0.16f);
        s.tabletBorder = juce::Colour ((juce::uint8) 0, (juce::uint8) 0, (juce::uint8) 0, 0.18f);
        s.bodyColour = juce::Colour (0xffd4d7d9);
        s.bodyColour2 = juce::Colour (0xffbfc3c6);
        s.paperColour = juce::Colour (0xffefe4cf);
        s.vfd = true;
        s.dark = false;
        s.displayFont = FontKind::sans;
        s.radius = 6.0f;
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "videojuego"; s.name = U8 ("Videojuego");
        s.body = Tex::none; s.paper = Tex::none; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xfff4f4f4);
        s.dim = juce::Colour (0xff9aa0c8);
        s.rail = juce::Colour (0xff1c1e3d);
        s.railBorder = juce::Colour (0xff4a4f8c);
        s.well = juce::Colour (0xff262a52);
        s.wellBorder = juce::Colour (0xff5a60a8);
        s.atril = juce::Colour (0xff161833);
        s.lip = juce::Colour (0xff0d0e1f);
        s.ink = juce::Colour (0xfff4f4f4);
        s.noteInk = juce::Colour (0xff39ff88);
        s.fall = juce::Colour (0xff0f1024);
        s.chord = juce::Colour (0xffffd23f);
        s.accent = juce::Colour (0xffffd23f);
        s.accentInk = juce::Colour (0xff1a1205);
        s.light = juce::Colour (0xff39ff88);
        s.felt = juce::Colour (0xffe43b44);
        s.keyBed = juce::Colour (0xff0b0c1a);
        s.keyWhite = juce::Colour (0xfff4f4f4);
        s.keyShade = juce::Colour (0xffc7c9dd);
        s.keyBlack = juce::Colour (0xff1c1e3d);
        s.keyLabel = juce::Colour (0xff6a6fa8);
        s.markOn = juce::Colour (0xffff4f7b);
        s.keysBlock = juce::Colour (0xff161833);
        s.cheek = juce::Colour (0xff1c1e3d);
        s.knob = juce::Colour (0xffffd23f);
        s.knobLine = juce::Colour (0xff1a1205);
        s.knobCap = juce::Colour (0xfff4f4f4);
        s.switchOn = juce::Colour (0xff39ff88);
        s.ring = juce::Colour (0xff1c1e3d);
        s.ringLine = juce::Colour (0xff4a4f8c);
        s.region = juce::Colour ((juce::uint8) 255, (juce::uint8) 210, (juce::uint8) 63, 0.22f);
        s.regionMinor = juce::Colour ((juce::uint8) 255, (juce::uint8) 210, (juce::uint8) 63, 0.12f);
        s.center = juce::Colour (0xff0f1024);
        s.constLine = juce::Colour ((juce::uint8) 57, (juce::uint8) 255, (juce::uint8) 136, 0.9f);
        s.tablet = juce::Colour (0xff1c1e3d);
        s.tabletOn = juce::Colour (0xff3a3164);
        s.tabletBorder = juce::Colour (0xff4a4f8c);
        s.bodyColour = juce::Colour (0xff12142b);
        s.bodyColour2 = juce::Colour (0xff0b0c1c);
        s.paperColour = juce::Colour (0xff0c0d1d);
        s.pixel = true;
        s.displayFont = FontKind::pixel;
        s.uiFont = FontKind::pixel;
        s.radius = 0.0f;
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "minimalista"; s.name = U8 ("Minimalista");
        s.body = Tex::none; s.paper = Tex::none; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xff141518);
        s.dim = juce::Colour (0xff6b6f76);
        s.rail = juce::Colour (0xffffffff);
        s.railBorder = juce::Colour (0xffe1e3e7);
        s.well = juce::Colour (0xfff4f5f7);
        s.wellBorder = juce::Colour (0xffe1e3e7);
        s.atril = juce::Colour (0xffffffff);
        s.lip = juce::Colour (0xffe9ebee);
        s.ink = juce::Colour (0xff141518);
        s.noteInk = juce::Colour (0xff2f6bff);
        s.fall = juce::Colour (0xffffffff);
        s.chord = juce::Colour (0xff141518);
        s.accent = juce::Colour (0xff2f6bff);
        s.accentInk = juce::Colour (0xffffffff);
        s.light = juce::Colour (0xff2f6bff);
        s.felt = juce::Colour (0xffd6d9de);
        s.keyBed = juce::Colour (0xffd6d9de);
        s.keyWhite = juce::Colour (0xffffffff);
        s.keyShade = juce::Colour (0xfff0f1f3);
        s.keyBlack = juce::Colour (0xff1b1c20);
        s.keyLabel = juce::Colour (0xff9a9ea6);
        s.markOn = juce::Colour (0xff2f6bff);
        s.keysBlock = juce::Colour (0xffffffff);
        s.cheek = juce::Colour (0xfff4f5f7);
        s.knob = juce::Colour (0xffffffff);
        s.knobLine = juce::Colour (0xff141518);
        s.knobCap = juce::Colour (0xffffffff);
        s.switchOn = juce::Colour (0xff2f6bff);
        s.ring = juce::Colour (0xffffffff);
        s.ringLine = juce::Colour (0xffdfe2e6);
        s.region = juce::Colour ((juce::uint8) 47, (juce::uint8) 107, (juce::uint8) 255, 0.1f);
        s.regionMinor = juce::Colour ((juce::uint8) 47, (juce::uint8) 107, (juce::uint8) 255, 0.06f);
        s.center = juce::Colour (0xfff4f5f7);
        s.constLine = juce::Colour ((juce::uint8) 20, (juce::uint8) 21, (juce::uint8) 24, 0.7f);
        s.tablet = juce::Colour (0xfff4f5f7);
        s.tabletOn = juce::Colour ((juce::uint8) 47, (juce::uint8) 107, (juce::uint8) 255, 0.1f);
        s.tabletBorder = juce::Colour (0xffe1e3e7);
        s.bodyColour = juce::Colour (0xffeceef1);
        s.bodyColour2 = juce::Colour (0xffe4e6ea);
        s.paperColour = juce::Colour (0xffffffff);
        s.dark = false;
        s.displayFont = FontKind::sans;
        s.radius = 4.0f;
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "vintage"; s.name = U8 ("Vintage");
        s.body = Tex::pergamino; s.paper = Tex::papel; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xff3a2a18);
        s.dim = juce::Colour (0xff7a6346);
        s.rail = juce::Colour ((juce::uint8) 250, (juce::uint8) 238, (juce::uint8) 210, 0.35f);
        s.railBorder = juce::Colour ((juce::uint8) 90, (juce::uint8) 60, (juce::uint8) 25, 0.3f);
        s.well = juce::Colour ((juce::uint8) 250, (juce::uint8) 240, (juce::uint8) 215, 0.45f);
        s.wellBorder = juce::Colour ((juce::uint8) 90, (juce::uint8) 60, (juce::uint8) 25, 0.3f);
        s.atril = juce::Colour ((juce::uint8) 250, (juce::uint8) 238, (juce::uint8) 210, 0.18f);
        s.lip = juce::Colour (0xff8b6a3e);
        s.ink = juce::Colour (0xff3a2a18);
        s.noteInk = juce::Colour (0xff5a3712);
        s.fall = juce::Colour ((juce::uint8) 60, (juce::uint8) 38, (juce::uint8) 15, 0.1f);
        s.chord = juce::Colour (0xff3a2a18);
        s.accent = juce::Colour (0xff8a5a2b);
        s.accentInk = juce::Colour (0xfff6ead2);
        s.light = juce::Colour (0xffc8902e);
        s.felt = juce::Colour (0xff6b1e1e);
        s.keyBed = juce::Colour (0xff2a1d14);
        s.keyWhite = juce::Colour (0xffefe3c8);
        s.keyShade = juce::Colour (0xffd9c8a3);
        s.keyBlack = juce::Colour (0xff2a1d14);
        s.keyLabel = juce::Colour (0xff8a7253);
        s.markOn = juce::Colour (0xff8a5a2b);
        s.keysBlock = juce::Colour ((juce::uint8) 60, (juce::uint8) 38, (juce::uint8) 15, 0.12f);
        s.cheek = juce::Colour ((juce::uint8) 250, (juce::uint8) 238, (juce::uint8) 210, 0.35f);
        s.knob = juce::Colour (0xffb48a4e);
        s.knobLine = juce::Colour (0xff2a1d14);
        s.knobCap = juce::Colour (0xfff6ead2);
        s.switchOn = juce::Colour (0xff8a5a2b);
        s.ring = juce::Colour ((juce::uint8) 250, (juce::uint8) 240, (juce::uint8) 215, 0.35f);
        s.ringLine = juce::Colour ((juce::uint8) 90, (juce::uint8) 60, (juce::uint8) 25, 0.32f);
        s.region = juce::Colour ((juce::uint8) 138, (juce::uint8) 90, (juce::uint8) 43, 0.18f);
        s.regionMinor = juce::Colour ((juce::uint8) 138, (juce::uint8) 90, (juce::uint8) 43, 0.1f);
        s.center = juce::Colour (0xffefe2c3);
        s.constLine = juce::Colour ((juce::uint8) 58, (juce::uint8) 42, (juce::uint8) 24, 0.7f);
        s.tablet = juce::Colour ((juce::uint8) 250, (juce::uint8) 240, (juce::uint8) 215, 0.4f);
        s.tabletOn = juce::Colour ((juce::uint8) 138, (juce::uint8) 90, (juce::uint8) 43, 0.18f);
        s.tabletBorder = juce::Colour ((juce::uint8) 90, (juce::uint8) 60, (juce::uint8) 25, 0.28f);
        s.bodyColour = juce::Colour (0xffd9c095);
        s.bodyColour2 = juce::Colour (0xffc4a674);
        s.paperColour = juce::Colour (0xffefe4cf);
        s.ornate = true;
        s.dark = false;
        s.radius = 3.0f;
        v.push_back (s);
    }
    return v;
}

const std::vector<Skin>& skins()
{
    static const std::vector<Skin> s = buildSkins();
    return s;
}

int skinIndex (const juce::String& id)
{
    const auto& s = skins();
    for (int i = 0; i < (int) s.size(); ++i)
        if (s[(size_t) i].id == id) return i;
    return 0;
}

const std::vector<Theme>& themes()
{
    static const std::vector<Theme> t {
        { "natural",  "Natural",  U8 ("La luz propia de la skin") },
        { "manana",   U8 ("Mañana"), U8 ("Luz fresca y clara") },
        { "tarde",    "Tarde",    U8 ("Luz cálida entre las hojas") },
        { "murasaki", "Murasaki", U8 ("Violeta con un rayo ámbar") },
        { "noche",    "Noche",    U8 ("Oscuro, con luz de lámpara") },
    };
    return t;
}

int themeIndex (const juce::String& id)
{
    const auto& t = themes();
    for (int i = 0; i < (int) t.size(); ++i)
        if (t[(size_t) i].id == id) return i;
    return 0;
}

juce::Colour Palette::noteColour (int midiNote) const
{
    if (lightMode == 2)
    {
        // Arcoíris ordenado por quintas: los acordes consonantes quedan en colores vecinos.
        const int pc = ((midiNote % 12) + 12) % 12;
        const float pos = (float) ((pc * 7) % 12) / 12.0f;
        return juce::Colour::fromHSV (pos, 0.62f, 1.0f, 1.0f);
    }
    return light;
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
    o.millisecondsBeforeSaving = 400;
    file = std::make_unique<juce::PropertiesFile> (o);

    skin        = file->getValue ("skinId", "nogal");
    theme       = file->getValue ("theme", "natural");
    lightMode   = juce::jlimit (0, 2, file->getIntValue ("lightMode", 0));
    lightCustom = juce::Colour::fromString (file->getValue ("lightCustom", "ff7fb8ff"));
    latin       = file->getBoolValue ("latin", false);
    sevenths    = file->getBoolValue ("sevenths", false);
    midiAuto    = file->getBoolValue ("midiAuto", true);
    hideParams  = file->getBoolValue ("hideParams", false);
    transparent = file->getBoolValue ("transparent", false);
    lowLatencyDone = file->getBoolValue ("lowLatencyDone", false);

    if (skinIndex (skin) == 0 && skin != "nogal") skin = "nogal";
    if (themeIndex (theme) == 0 && theme != "natural") theme = "natural";

    if (auto xml = file->getXmlValue ("floats"))
    {
        for (auto* e : xml->getChildIterator())
        {
            FloatState f;
            f.floating = e->getBoolAttribute ("floating");
            f.onTop = e->getBoolAttribute ("onTop", true);
            f.bounds = { e->getIntAttribute ("x"), e->getIntAttribute ("y"), e->getIntAttribute ("w"), e->getIntAttribute ("h") };
            floats[e->getStringAttribute ("id")] = f;
        }
    }
}

Prefs::~Prefs()
{
    if (file != nullptr)
        file->saveIfNeeded();
}

void Prefs::save (bool notify)
{
    file->setValue ("skinId", skin);
    file->setValue ("theme", theme);
    file->setValue ("lightMode", lightMode);
    file->setValue ("lightCustom", lightCustom.toString());
    file->setValue ("latin", latin);
    file->setValue ("sevenths", sevenths);
    file->setValue ("midiAuto", midiAuto);
    file->setValue ("hideParams", hideParams);
    file->setValue ("transparent", transparent);
    file->setValue ("lowLatencyDone", lowLatencyDone);

    juce::XmlElement xml ("FLOATS");
    for (auto& [id, f] : floats)
    {
        auto* e = xml.createNewChildElement ("F");
        e->setAttribute ("id", id);
        e->setAttribute ("floating", f.floating);
        e->setAttribute ("onTop", f.onTop);
        e->setAttribute ("x", f.bounds.getX());
        e->setAttribute ("y", f.bounds.getY());
        e->setAttribute ("w", f.bounds.getWidth());
        e->setAttribute ("h", f.bounds.getHeight());
    }
    file->setValue ("floats", &xml);
    if (notify) sendChangeMessage();
}

Palette Prefs::palette() const
{
    Palette p;
    static_cast<Skin&> (p) = skins()[(size_t) skinIndex (skin)];
    p.theme = theme;
    p.lightMode = lightMode;
    if (lightMode == 1) p.light = lightCustom;
    if (theme == "murasaki" && lightMode == 0) p.light = juce::Colour (0xffffb24f);
    return p;
}
