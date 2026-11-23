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
    {
        Skin s;
        s.id = "grafito"; s.name = U8 ("Grafito X");
        s.body = Tex::none; s.paper = Tex::none; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xffeef1f4);
        s.dim = juce::Colour (0xff8b939c);
        s.rail = juce::Colour ((juce::uint8) 34, (juce::uint8) 37, (juce::uint8) 42, 0.94f);
        s.railBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.08f);
        s.well = juce::Colour (0xff15171a);
        s.wellBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.1f);
        s.atril = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.025f);
        s.lip = juce::Colour (0xff121417);
        s.ink = juce::Colour (0xffdfe6ee);
        s.noteInk = juce::Colour (0xff5cc8ff);
        s.fall = juce::Colour ((juce::uint8) 10, (juce::uint8) 11, (juce::uint8) 13, 0.6f);
        s.chord = juce::Colour (0xffffffff);
        s.accent = juce::Colour (0xff4cc2ff);
        s.accentInk = juce::Colour (0xff06141c);
        s.light = juce::Colour (0xff4cc2ff);
        s.felt = juce::Colour (0xff2a2e34);
        s.keyBed = juce::Colour (0xff0c0d0f);
        s.keyWhite = juce::Colour (0xffe8ebef);
        s.keyShade = juce::Colour (0xffc9ced5);
        s.keyBlack = juce::Colour (0xff1a1c20);
        s.keyLabel = juce::Colour (0xff7d858f);
        s.markOn = juce::Colour (0xff4cc2ff);
        s.keysBlock = juce::Colour ((juce::uint8) 10, (juce::uint8) 11, (juce::uint8) 13, 0.55f);
        s.cheek = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.knob = juce::Colour (0xff2a2e34);
        s.knobLine = juce::Colour (0xff4cc2ff);
        s.knobCap = juce::Colour (0xffe8ebef);
        s.switchOn = juce::Colour (0xff4cc2ff);
        s.ring = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.025f);
        s.ringLine = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.12f);
        s.region = juce::Colour ((juce::uint8) 76, (juce::uint8) 194, (juce::uint8) 255, 0.18f);
        s.regionMinor = juce::Colour ((juce::uint8) 76, (juce::uint8) 194, (juce::uint8) 255, 0.1f);
        s.center = juce::Colour (0xff15171a);
        s.constLine = juce::Colour ((juce::uint8) 140, (juce::uint8) 220, (juce::uint8) 255, 0.85f);
        s.tablet = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.tabletOn = juce::Colour ((juce::uint8) 76, (juce::uint8) 194, (juce::uint8) 255, 0.18f);
        s.tabletBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.08f);
        s.bodyColour = juce::Colour (0xff1b1d21);
        s.bodyColour2 = juce::Colour (0xff121316);
        s.paperColour = juce::Colour (0xff15171a);
        s.modern = true;
        s.displayFont = FontKind::sans;
        s.radius = 3.0f;
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "wavetable"; s.name = U8 ("Wavetable");
        s.body = Tex::none; s.paper = Tex::none; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xffecebf5);
        s.dim = juce::Colour (0xff9592ad);
        s.rail = juce::Colour ((juce::uint8) 40, (juce::uint8) 40, (juce::uint8) 50, 0.92f);
        s.railBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.06f);
        s.well = juce::Colour (0xff1c1c24);
        s.wellBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.07f);
        s.atril = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.lip = juce::Colour (0xff18181e);
        s.ink = juce::Colour (0xffd9d6f0);
        s.noteInk = juce::Colour (0xffa690ff);
        s.fall = juce::Colour ((juce::uint8) 18, (juce::uint8) 18, (juce::uint8) 24, 0.7f);
        s.chord = juce::Colour (0xfff1eeff);
        s.accent = juce::Colour (0xff9a7dff);
        s.accentInk = juce::Colour (0xff120c26);
        s.light = juce::Colour (0xff3ee6c9);
        s.felt = juce::Colour (0xff2c2a3a);
        s.keyBed = juce::Colour (0xff101015);
        s.keyWhite = juce::Colour (0xffdcdbe6);
        s.keyShade = juce::Colour (0xffbdbbcc);
        s.keyBlack = juce::Colour (0xff25242e);
        s.keyLabel = juce::Colour (0xff7b7894);
        s.markOn = juce::Colour (0xff9a7dff);
        s.keysBlock = juce::Colour ((juce::uint8) 18, (juce::uint8) 18, (juce::uint8) 24, 0.6f);
        s.cheek = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.knob = juce::Colour (0xff2d2c38);
        s.knobLine = juce::Colour (0xff3ee6c9);
        s.knobCap = juce::Colour (0xffecebf5);
        s.switchOn = juce::Colour (0xff9a7dff);
        s.ring = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.ringLine = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.1f);
        s.region = juce::Colour ((juce::uint8) 154, (juce::uint8) 125, (juce::uint8) 255, 0.22f);
        s.regionMinor = juce::Colour ((juce::uint8) 154, (juce::uint8) 125, (juce::uint8) 255, 0.12f);
        s.center = juce::Colour (0xff1a1a22);
        s.constLine = juce::Colour ((juce::uint8) 62, (juce::uint8) 230, (juce::uint8) 201, 0.9f);
        s.tablet = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.035f);
        s.tabletOn = juce::Colour ((juce::uint8) 154, (juce::uint8) 125, (juce::uint8) 255, 0.22f);
        s.tabletBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.07f);
        s.bodyColour = juce::Colour (0xff1e1e26);
        s.bodyColour2 = juce::Colour (0xff15151b);
        s.paperColour = juce::Colour (0xff1a1a22);
        s.modern = true;
        s.displayFont = FontKind::sans;
        s.radius = 9.0f;
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "neon"; s.name = U8 ("Neón");
        s.body = Tex::none; s.paper = Tex::none; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xfff4eefc);
        s.dim = juce::Colour (0xff9a8fb5);
        s.rail = juce::Colour ((juce::uint8) 14, (juce::uint8) 10, (juce::uint8) 24, 0.92f);
        s.railBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 61, (juce::uint8) 154, 0.3f);
        s.well = juce::Colour (0xff120d1f);
        s.wellBorder = juce::Colour ((juce::uint8) 45, (juce::uint8) 226, (juce::uint8) 255, 0.25f);
        s.atril = juce::Colour ((juce::uint8) 10, (juce::uint8) 6, (juce::uint8) 20, 0.5f);
        s.lip = juce::Colour (0xff0a0614);
        s.ink = juce::Colour (0xffe8dcff);
        s.noteInk = juce::Colour (0xffff4fb0);
        s.fall = juce::Colour ((juce::uint8) 8, (juce::uint8) 5, (juce::uint8) 16, 0.75f);
        s.chord = juce::Colour (0xff2de2ff);
        s.accent = juce::Colour (0xffff3d9a);
        s.accentInk = juce::Colour (0xff1a0410);
        s.light = juce::Colour (0xff2de2ff);
        s.felt = juce::Colour (0xffff3d9a);
        s.keyBed = juce::Colour (0xff05030a);
        s.keyWhite = juce::Colour (0xffece6f7);
        s.keyShade = juce::Colour (0xffcfc6e2);
        s.keyBlack = juce::Colour (0xff140e22);
        s.keyLabel = juce::Colour (0xff7a6e98);
        s.markOn = juce::Colour (0xffff3d9a);
        s.keysBlock = juce::Colour ((juce::uint8) 8, (juce::uint8) 5, (juce::uint8) 16, 0.7f);
        s.cheek = juce::Colour ((juce::uint8) 20, (juce::uint8) 12, (juce::uint8) 36, 0.7f);
        s.knob = juce::Colour (0xff1d1530);
        s.knobLine = juce::Colour (0xff2de2ff);
        s.knobCap = juce::Colour (0xfff4eefc);
        s.switchOn = juce::Colour (0xffff3d9a);
        s.ring = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.02f);
        s.ringLine = juce::Colour ((juce::uint8) 45, (juce::uint8) 226, (juce::uint8) 255, 0.22f);
        s.region = juce::Colour ((juce::uint8) 255, (juce::uint8) 61, (juce::uint8) 154, 0.22f);
        s.regionMinor = juce::Colour ((juce::uint8) 45, (juce::uint8) 226, (juce::uint8) 255, 0.12f);
        s.center = juce::Colour (0xff0d0818);
        s.constLine = juce::Colour ((juce::uint8) 45, (juce::uint8) 226, (juce::uint8) 255, 0.9f);
        s.tablet = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.tabletOn = juce::Colour ((juce::uint8) 255, (juce::uint8) 61, (juce::uint8) 154, 0.22f);
        s.tabletBorder = juce::Colour ((juce::uint8) 45, (juce::uint8) 226, (juce::uint8) 255, 0.2f);
        s.bodyColour = juce::Colour (0xff120b22);
        s.bodyColour2 = juce::Colour (0xff07040f);
        s.paperColour = juce::Colour (0xff0d0818);
        s.modern = true;
        s.displayFont = FontKind::sans;
        s.radius = 6.0f;
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "pulso"; s.name = U8 ("Pulso");
        s.body = Tex::none; s.paper = Tex::none; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xffeef0e8);
        s.dim = juce::Colour (0xff8e9486);
        s.rail = juce::Colour ((juce::uint8) 30, (juce::uint8) 32, (juce::uint8) 30, 0.94f);
        s.railBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.07f);
        s.well = juce::Colour (0xff161816);
        s.wellBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.08f);
        s.atril = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.025f);
        s.lip = juce::Colour (0xff121412);
        s.ink = juce::Colour (0xffe2e6da);
        s.noteInk = juce::Colour (0xffc6ff3d);
        s.fall = juce::Colour ((juce::uint8) 12, (juce::uint8) 13, (juce::uint8) 12, 0.65f);
        s.chord = juce::Colour (0xffc6ff3d);
        s.accent = juce::Colour (0xffb8f53a);
        s.accentInk = juce::Colour (0xff121a02);
        s.light = juce::Colour (0xffc6ff3d);
        s.felt = juce::Colour (0xff343832);
        s.keyBed = juce::Colour (0xff0b0c0b);
        s.keyWhite = juce::Colour (0xffe9ebe4);
        s.keyShade = juce::Colour (0xffccd0c4);
        s.keyBlack = juce::Colour (0xff1b1d1a);
        s.keyLabel = juce::Colour (0xff7f8578);
        s.markOn = juce::Colour (0xff9bd62a);
        s.keysBlock = juce::Colour ((juce::uint8) 12, (juce::uint8) 13, (juce::uint8) 12, 0.55f);
        s.cheek = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.knob = juce::Colour (0xff2b2e2a);
        s.knobLine = juce::Colour (0xffc6ff3d);
        s.knobCap = juce::Colour (0xffeef0e8);
        s.switchOn = juce::Colour (0xff9bd62a);
        s.ring = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.025f);
        s.ringLine = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.11f);
        s.region = juce::Colour ((juce::uint8) 184, (juce::uint8) 245, (juce::uint8) 58, 0.18f);
        s.regionMinor = juce::Colour ((juce::uint8) 184, (juce::uint8) 245, (juce::uint8) 58, 0.1f);
        s.center = juce::Colour (0xff161816);
        s.constLine = juce::Colour ((juce::uint8) 198, (juce::uint8) 255, (juce::uint8) 61, 0.85f);
        s.tablet = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.tabletOn = juce::Colour ((juce::uint8) 184, (juce::uint8) 245, (juce::uint8) 58, 0.18f);
        s.tabletBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.08f);
        s.bodyColour = juce::Colour (0xff1d1f1c);
        s.bodyColour2 = juce::Colour (0xff131412);
        s.paperColour = juce::Colour (0xff161816);
        s.modern = true;
        s.displayFont = FontKind::sans;
        s.radius = 5.0f;
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "prisma"; s.name = U8 ("Prisma");
        s.body = Tex::none; s.paper = Tex::none; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xfff2eef8);
        s.dim = juce::Colour (0xff9a93ab);
        s.rail = juce::Colour ((juce::uint8) 28, (juce::uint8) 25, (juce::uint8) 38, 0.94f);
        s.railBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.07f);
        s.well = juce::Colour (0xff16141e);
        s.wellBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.08f);
        s.atril = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.lip = juce::Colour (0xff13111a);
        s.ink = juce::Colour (0xffe6e0f2);
        s.noteInk = juce::Colour (0xffff8a4c);
        s.fall = juce::Colour ((juce::uint8) 14, (juce::uint8) 12, (juce::uint8) 20, 0.7f);
        s.chord = juce::Colour (0xffffb347);
        s.accent = juce::Colour (0xffff7a45);
        s.accentInk = juce::Colour (0xff1c0a02);
        s.light = juce::Colour (0xffffb347);
        s.felt = juce::Colour (0xff7c5cff);
        s.keyBed = juce::Colour (0xff0c0b11);
        s.keyWhite = juce::Colour (0xffebe8f1);
        s.keyShade = juce::Colour (0xffcdc8d8);
        s.keyBlack = juce::Colour (0xff1d1a27);
        s.keyLabel = juce::Colour (0xff837b97);
        s.markOn = juce::Colour (0xff7c5cff);
        s.keysBlock = juce::Colour ((juce::uint8) 14, (juce::uint8) 12, (juce::uint8) 20, 0.6f);
        s.cheek = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.knob = juce::Colour (0xff2a2636);
        s.knobLine = juce::Colour (0xffff7a45);
        s.knobCap = juce::Colour (0xfff2eef8);
        s.switchOn = juce::Colour (0xffff7a45);
        s.ring = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.03f);
        s.ringLine = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.1f);
        s.region = juce::Colour ((juce::uint8) 150, (juce::uint8) 110, (juce::uint8) 255, 0.22f);
        s.regionMinor = juce::Colour ((juce::uint8) 150, (juce::uint8) 110, (juce::uint8) 255, 0.12f);
        s.center = juce::Colour (0xff16141e);
        s.constLine = juce::Colour ((juce::uint8) 255, (juce::uint8) 179, (juce::uint8) 71, 0.9f);
        s.tablet = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.035f);
        s.tabletOn = juce::Colour ((juce::uint8) 255, (juce::uint8) 122, (juce::uint8) 69, 0.2f);
        s.tabletBorder = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.08f);
        s.bodyColour = juce::Colour (0xff1a1724);
        s.bodyColour2 = juce::Colour (0xff100e16);
        s.paperColour = juce::Colour (0xff16141e);
        s.modern = true;
        s.displayFont = FontKind::sans;
        s.radius = 7.0f;
        v.push_back (s);
    }
    {
        Skin s;
        s.id = "cromo"; s.name = U8 ("Cromo");
        s.body = Tex::none; s.paper = Tex::none; s.cheekTex = Tex::none;
        s.text = juce::Colour (0xff141516);
        s.dim = juce::Colour (0xff6c7074);
        s.rail = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.78f);
        s.railBorder = juce::Colour ((juce::uint8) 0, (juce::uint8) 0, (juce::uint8) 0, 0.08f);
        s.well = juce::Colour (0xfff4f5f6);
        s.wellBorder = juce::Colour ((juce::uint8) 0, (juce::uint8) 0, (juce::uint8) 0, 0.1f);
        s.atril = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.45f);
        s.lip = juce::Colour (0xffc9ccd0);
        s.ink = juce::Colour (0xff141516);
        s.noteInk = juce::Colour (0xffff5a1f);
        s.fall = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.72f);
        s.chord = juce::Colour (0xff141516);
        s.accent = juce::Colour (0xffff5a1f);
        s.accentInk = juce::Colour (0xffffffff);
        s.light = juce::Colour (0xffff5a1f);
        s.felt = juce::Colour (0xffff5a1f);
        s.keyBed = juce::Colour (0xff2a2c2f);
        s.keyWhite = juce::Colour (0xfffbfbfc);
        s.keyShade = juce::Colour (0xffe6e7e9);
        s.keyBlack = juce::Colour (0xff1d1e20);
        s.keyLabel = juce::Colour (0xff9a9ea3);
        s.markOn = juce::Colour (0xffff5a1f);
        s.keysBlock = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.5f);
        s.cheek = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.6f);
        s.knob = juce::Colour (0xfff4f5f6);
        s.knobLine = juce::Colour (0xffff5a1f);
        s.knobCap = juce::Colour (0xffffffff);
        s.switchOn = juce::Colour (0xffff5a1f);
        s.ring = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.7f);
        s.ringLine = juce::Colour ((juce::uint8) 0, (juce::uint8) 0, (juce::uint8) 0, 0.1f);
        s.region = juce::Colour ((juce::uint8) 255, (juce::uint8) 90, (juce::uint8) 31, 0.14f);
        s.regionMinor = juce::Colour ((juce::uint8) 255, (juce::uint8) 90, (juce::uint8) 31, 0.08f);
        s.center = juce::Colour (0xfff4f5f6);
        s.constLine = juce::Colour ((juce::uint8) 20, (juce::uint8) 21, (juce::uint8) 22, 0.65f);
        s.tablet = juce::Colour ((juce::uint8) 255, (juce::uint8) 255, (juce::uint8) 255, 0.7f);
        s.tabletOn = juce::Colour ((juce::uint8) 255, (juce::uint8) 90, (juce::uint8) 31, 0.12f);
        s.tabletBorder = juce::Colour ((juce::uint8) 0, (juce::uint8) 0, (juce::uint8) 0, 0.08f);
        s.bodyColour = juce::Colour (0xffdfe1e4);
        s.bodyColour2 = juce::Colour (0xffd2d5d9);
        s.paperColour = juce::Colour (0xffffffff);
        s.modern = true;
        s.dark = false;
        s.displayFont = FontKind::sans;
        s.radius = 6.0f;
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

namespace
{
    using juce::Colour;
    // Pequeño ayudante para escribir los temas de forma legible
    struct L
    {
        ThemeLook t;
        L& wash (uint32_t c, float a)                         { t.wash = Colour (c); t.washA = a; return *this; }
        L& sky (uint32_t top, float ta, uint32_t bot, float ba) { t.skyTop = Colour (top); t.skyTopA = ta; t.skyBottom = Colour (bot); t.skyBottomA = ba; return *this; }
        L& glow (uint32_t c, float a, float x, float y, float r)  { t.glow = Colour (c); t.glowA = a; t.glowPos = { x, y }; t.glowR = r; return *this; }
        L& glow2 (uint32_t c, float a, float x, float y, float r) { t.glow2 = Colour (c); t.glow2A = a; t.glow2Pos = { x, y }; t.glow2R = r; return *this; }
        L& beams (uint32_t c, float a, int n, float angle)    { t.beam = Colour (c); t.beamA = a; t.beams = n; t.beamAngle = angle; return *this; }
        L& vignette (float v)                                 { t.vignette = v; return *this; }
        L& motif (Motif m, uint32_t c, float a, int d)        { t.motif = m; t.motifColour = Colour (c); t.motifA = a; t.density = d; return *this; }
        L& scene (Scene sc, uint32_t c, float a)              { t.scene = sc; t.sceneColour = Colour (c); t.sceneA = a; return *this; }
        L& disc (uint32_t c, float a, float x, float y, float r, bool ring = false) { t.disc = Colour (c); t.discA = a; t.discPos = { x, y }; t.discR = r; t.ring = ring; return *this; }
        L& key (uint32_t c)                                   { t.keyLight = Colour (c); t.hasKeyLight = true; return *this; }
        operator ThemeLook() const { return t; }
    };

    std::vector<Theme> buildThemes()
    {
        std::vector<Theme> v;
        auto add = [&] (const char* id, const char* name, const char* desc, ThemeLook day, ThemeLook night)
        {
            v.push_back ({ id, U8 (name), U8 (desc), day, night });
        };

        add ("natural", "Natural", "La luz propia de la skin",
             L(),
             L().wash (0xff000000, 0.16f).glow (0xffffcd8c, 0.10f, 0.5f, 0.3f, 0.45f).vignette (0.38f));

        add ("fuji", "Fuji al alba", "Cielo rosado y el volcán nevado",
             L().sky (0xffffb7a1, 0.20f, 0xffffd9b0, 0.03f).glow (0xfffff1d6, 0.22f, 0.84f, 0.06f, 0.55f)
                .scene (Scene::fuji, 0xff5a6f9e, 0.32f).disc (0xfffff3dc, 0.6f, 0.86f, 0.02f, 0.07f),
             L().wash (0xff0b1030, 0.22f).sky (0xff1a1f55, 0.25f, 0xffff7a8a, 0.12f).glow (0xffffd7e0, 0.10f, 0.82f, 0.12f, 0.3f)
                .scene (Scene::fuji, 0xff0a0d22, 0.6f).disc (0xfff4f1ff, 0.6f, 0.86f, 0.02f, 0.06f)
                .motif (Motif::stars, 0xffffffff, 0.8f, 70).vignette (0.3f).key (0xffff9ab0));

        add ("komorebi", "Komorebi", "Luz que se cuela entre las hojas",
             L().wash (0xffffb35a, 0.05f).motif (Motif::dapple, 0xffffc878, 0.17f, 6).beams (0xfffff0c8, 0.07f, 4, 24.0f),
             L().wash (0xff04140e, 0.22f).motif (Motif::fireflies, 0xffd8ff7a, 0.95f, 26).glow (0xffa8d8ff, 0.08f, 0.3f, 0.0f, 0.5f)
                .vignette (0.36f).key (0xffd8ff7a));

        add ("sakura", "Sakura", "Pétalos cayendo en primavera",
             L().wash (0xffffd6e4, 0.10f).glow (0xfffff0f5, 0.20f, 0.15f, 0.05f, 0.6f).motif (Motif::petals, 0xffffb7cf, 0.85f, 40),
             L().wash (0xff1a0a1e, 0.22f).sky (0xff3a1240, 0.2f, 0xffff7fb0, 0.06f).glow (0xffffb27a, 0.14f, 0.12f, 0.2f, 0.35f)
                .glow2 (0xffff8ab8, 0.10f, 0.85f, 0.15f, 0.4f).motif (Motif::petals, 0xffffc4dc, 0.75f, 40).vignette (0.3f).key (0xffff8ab8));

        add ("murasaki", "Murasaki", "Violeta con un rayo ámbar",
             L().sky (0xffb9a2ff, 0.16f, 0xff9b7be6, 0.12f).beams (0xffffaa46, 0.16f, 1, 52.0f).key (0xffffb24f),
             L().sky (0xff4b2c9e, 0.30f, 0xff2c1670, 0.40f).beams (0xffffaa46, 0.22f, 1, 52.0f).motif (Motif::stars, 0xffffe6c8, 0.4f, 25)
                .vignette (0.25f).key (0xffffb24f));

        add ("aurora", "Aurora boreal", "Cortinas de luz en el cielo polar",
             L().sky (0xffd8fff1, 0.14f, 0xffbfe6ff, 0.06f).glow (0xffffffff, 0.12f, 0.5f, 0.0f, 0.6f).motif (Motif::aurora, 0xff6dffb0, 0.08f, 3),
             L().wash (0xff02101a, 0.28f).motif (Motif::aurora, 0xff6dffb0, 0.24f, 3).vignette (0.3f).key (0xff6dffb0));

        add ("tormenta", "Tormenta", "Lluvia, nubes y relámpagos",
             L().wash (0xff8a97a8, 0.16f).sky (0xff5c6b7d, 0.18f, 0xff5c6b7d, 0.0f).motif (Motif::rain, 0xffe6f0ff, 0.35f, 90).vignette (0.15f),
             L().wash (0xff050a18, 0.30f).sky (0xff1c2a4a, 0.25f, 0xff000000, 0.0f).glow (0xff9ec3ff, 0.14f, 0.12f, 0.0f, 0.5f)
                .motif (Motif::lightning, 0xffcfe0ff, 0.45f, 110).vignette (0.4f).key (0xff9ec3ff));

        add ("neon", "Neón Tokio", "Letreros de neón en la ciudad",
             L().sky (0xffff4fa3, 0.06f, 0xff2de2ff, 0.06f).glow (0xff2de2ff, 0.10f, 1.0f, 0.5f, 0.55f).glow2 (0xffff3d9a, 0.10f, 0.0f, 0.5f, 0.55f)
                .scene (Scene::city, 0xff2a2440, 0.3f),
             L().wash (0xff07021a, 0.30f).glow (0xffff2e97, 0.24f, 0.0f, 0.75f, 0.55f).glow2 (0xff19e3ff, 0.20f, 1.0f, 0.25f, 0.55f)
                .scene (Scene::city, 0xff05030f, 0.75f).motif (Motif::rain, 0xffff6ac1, 0.25f, 50).vignette (0.35f).key (0xffff3d9a));

        add ("bambu", "Bosque de bambú", "Luz verde entre tallos altos",
             L().wash (0xffc8f0a0, 0.07f).beams (0xfff6ffd8, 0.10f, 5, 14.0f).scene (Scene::bamboo, 0xff2f5a1e, 0.4f),
             L().wash (0xff031208, 0.30f).beams (0xffb8ffd8, 0.05f, 3, 14.0f).scene (Scene::bamboo, 0xff010a03, 0.75f)
                .motif (Motif::fireflies, 0xffe4ff8a, 0.9f, 30).vignette (0.4f).key (0xffb6ff6b));

        add ("oceano", "Océano", "Luz bajo el agua y criaturas que brillan",
             L().sky (0xff9ff3ff, 0.16f, 0xff0b6a8a, 0.14f).beams (0xffe0ffff, 0.10f, 6, -10.0f).motif (Motif::bubbles, 0xffe8ffff, 0.5f, 25),
             L().wash (0xff00101e, 0.35f).sky (0xff000000, 0.0f, 0xff001a3a, 0.3f).glow (0xff1ad4ff, 0.10f, 0.5f, 1.0f, 0.6f)
                .motif (Motif::bubbles, 0xff39f3ff, 0.9f, 45).vignette (0.45f).key (0xff39f3ff));

        add ("volcan", "Volcán", "Calor, brasas y lava",
             L().sky (0xffffcf8a, 0.06f, 0xffff5a1f, 0.18f).glow (0xffffb070, 0.16f, 0.5f, 1.05f, 0.7f).motif (Motif::embers, 0xffffb36b, 0.55f, 30),
             L().wash (0xff120300, 0.30f).glow (0xffff3d0a, 0.30f, 0.5f, 1.1f, 0.75f).motif (Motif::embers, 0xffffa14a, 1.0f, 70)
                .vignette (0.45f).key (0xffff6a2a));

        add ("nieve", "Nieve", "Copos cayendo sobre el templo",
             L().wash (0xfff2f8ff, 0.14f).glow (0xffffffff, 0.20f, 0.5f, 0.0f, 0.7f).motif (Motif::snow, 0xffffffff, 0.85f, 70),
             L().wash (0xff06122a, 0.26f).sky (0xff1d3566, 0.2f, 0xff000000, 0.0f).glow (0xffcfe3ff, 0.14f, 0.85f, 0.08f, 0.35f)
                .disc (0xffeef4ff, 0.55f, 0.86f, 0.02f, 0.06f).motif (Motif::snow, 0xffe6f0ff, 0.8f, 80).vignette (0.3f).key (0xff8fc7ff));

        add ("galaxia", "Galaxia", "Nebulosas y un mar de estrellas",
             L().sky (0xff6b5bd6, 0.14f, 0xffe08ad8, 0.08f).glow (0xffffd2f5, 0.12f, 0.2f, 0.1f, 0.5f).motif (Motif::stars, 0xffffffff, 0.35f, 30),
             L().wash (0xff04021a, 0.35f).glow (0xff8a4dff, 0.25f, 0.25f, 0.3f, 0.5f).glow2 (0xffff4fb8, 0.18f, 0.8f, 0.7f, 0.5f)
                .motif (Motif::stars, 0xffffffff, 1.0f, 140).vignette (0.3f).key (0xffc89bff));

        add ("dunas", "Dunas doradas", "Atardecer dorado en el desierto",
             L().sky (0xffffd98a, 0.14f, 0xffff9a3d, 0.12f).glow (0xfffff0c0, 0.28f, 0.85f, 0.15f, 0.5f)
                .scene (Scene::dunes, 0xffc27a35, 0.35f).disc (0xfffff4d0, 0.7f, 0.86f, 0.02f, 0.08f),
             L().wash (0xff0c0a1e, 0.26f).sky (0xff1b1640, 0.25f, 0xff4a2a3a, 0.12f).glow (0xfffff2d8, 0.16f, 0.15f, 0.12f, 0.32f)
                .scene (Scene::dunes, 0xff0d0a18, 0.7f).disc (0xfffff6e6, 0.6f, 0.14f, 0.02f, 0.06f)
                .motif (Motif::stars, 0xffffffff, 0.7f, 60).key (0xffffcf6b));

        add ("eclipse", "Eclipse", "Un anillo de fuego en el cielo oscuro",
             L().wash (0xff000000, 0.12f).glow (0xffffcc66, 0.12f, 0.5f, 0.0f, 0.45f).disc (0xffffd27a, 0.7f, 0.5f, -0.12f, 0.12f, true).vignette (0.25f),
             L().wash (0xff000000, 0.30f).glow (0xffff7a1a, 0.18f, 0.5f, 0.0f, 0.5f).disc (0xffff9a3a, 1.0f, 0.5f, -0.12f, 0.12f, true)
                .motif (Motif::stars, 0xffffffff, 0.5f, 40).vignette (0.5f).key (0xffffb347));

        add ("linternas", "Linternas", "Faroles de papel flotando",
             L().wash (0xffffe0b0, 0.08f).glow (0xffffd08a, 0.14f, 0.5f, 0.0f, 0.6f).motif (Motif::lanterns, 0xffff9a3d, 0.45f, 10),
             L().wash (0xff0e0510, 0.30f).glow (0xffff8a3a, 0.16f, 0.5f, 1.0f, 0.7f).motif (Motif::lanterns, 0xffffa040, 1.0f, 16)
                .vignette (0.35f).key (0xffffa040));
        return v;
    }
}

const std::vector<Theme>& themes()
{
    static const std::vector<Theme> t = buildThemes();
    return t;
}

int themeIndex (const juce::String& id)
{
    const auto& t = themes();
    for (int i = 0; i < (int) t.size(); ++i)
        if (t[(size_t) i].id == id) return i;
    return 0;
}

const ThemeLook& Palette::look() const
{
    const auto& t = themes()[(size_t) juce::jlimit (0, (int) themes().size() - 1, themeIdx)];
    return night ? t.night : t.day;
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
    dayNight    = juce::jlimit (0, 2, file->getIntValue ("dayNight", -1));
    // Temas de la versión 1.1
    if (! file->containsKey ("dayNight")) dayNight = (theme == "noche") ? 1 : 0;
    if (theme == "manana" || theme == "noche") theme = "natural";
    if (theme == "tarde") theme = "komorebi";
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
    file->setValue ("dayNight", dayNight);
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
    p.themeIdx = themeIndex (theme);
    p.night = isNight();
    if (lightMode == 1) p.light = lightCustom;
    else if (lightMode == 0 && p.look().hasKeyLight) p.light = p.look().keyLight;
    return p;
}

bool Prefs::isNight() const
{
    if (dayNight == 1) return true;
    if (dayNight == 2)
    {
        const int h = juce::Time::getCurrentTime().getHours();
        return h >= 19 || h < 7;
    }
    return false;
}
