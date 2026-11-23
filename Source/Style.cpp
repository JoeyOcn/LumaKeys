#include "Style.h"
#include "MidiSkyData.h"

using namespace juce;

//==============================================================================
static Typeface::Ptr loadFace (const void* data, int size)
{
    return Typeface::createSystemTypefaceFor (data, (size_t) size);
}

Fonts::Fonts()
{
    regular   = loadFace (MidiSkyData::ZenKakuGothicNewRegular_ttf, MidiSkyData::ZenKakuGothicNewRegular_ttfSize);
    medium    = loadFace (MidiSkyData::ZenKakuGothicNewMedium_ttf,  MidiSkyData::ZenKakuGothicNewMedium_ttfSize);
    bold      = loadFace (MidiSkyData::ZenKakuGothicNewBold_ttf,    MidiSkyData::ZenKakuGothicNewBold_ttfSize);
    pixelFace = loadFace (MidiSkyData::Jersey10Regular_ttf,         MidiSkyData::Jersey10Regular_ttfSize);
    bravura   = loadFace (MidiSkyData::Bravura_otf,                 MidiSkyData::Bravura_otfSize);
    doto      = loadFace (MidiSkyData::DotoVariable_ttf,            MidiSkyData::DotoVariable_ttfSize);
    vt323     = loadFace (MidiSkyData::VT323Regular_ttf,            MidiSkyData::VT323Regular_ttfSize);
    orbitron  = loadFace (MidiSkyData::OrbitronVariable_ttf,        MidiSkyData::OrbitronVariable_ttfSize);
    monoton   = loadFace (MidiSkyData::MonotonRegular_ttf,          MidiSkyData::MonotonRegular_ttfSize);
    arcade    = loadFace (MidiSkyData::PressStart2PRegular_ttf,     MidiSkyData::PressStart2PRegular_ttfSize);
    sixtyfour = loadFace (MidiSkyData::SixtyfourVariable_ttf,       MidiSkyData::SixtyfourVariable_ttfSize);
}

static Font faceFont (const Typeface::Ptr& tf, float height, int fallbackStyle)
{
    if (tf == nullptr)
        return Font (FontOptions (height, fallbackStyle));
    return Font (FontOptions().withTypeface (tf).withHeight (height));
}

Font Fonts::ui (float height, int weight) const
{
    // Zen Kaku trae mucho aire arriba y abajo: se agranda para que la letra se lea bien
    return faceFont (weight >= 2 ? bold : (weight == 1 ? medium : regular), height * 1.24f, weight >= 2 ? Font::bold : Font::plain);
}

Font Fonts::pixel (float height) const
{
    return faceFont (pixelFace, height * 1.12f, Font::plain);
}

Font Fonts::music (float emPixels) const
{
    if (bravura == nullptr)
        return Font (FontOptions (emPixels));
    return Font (FontOptions().withTypeface (bravura).withPointHeight (emPixels));
}

Font Fonts::display (int which, bool pixelStyle, float height) const
{
    if (which <= 0) which = pixelStyle ? 3 : 1;
    switch (which)
    {
        case 1:  return ui (height, 2);
        case 2:  return ui (height, 0);
        case 3:  return pixel (height);
        case 4:  return faceFont (doto, height * 1.0f, Font::bold);
        case 5:  return faceFont (vt323, height * 1.18f, Font::plain);
        case 6:  return faceFont (orbitron, height * 0.86f, Font::bold);
        case 7:  return faceFont (monoton, height * 0.92f, Font::plain);
        case 8:  return faceFont (arcade, height * 0.62f, Font::plain);
        case 9:  return faceFont (sixtyfour, height * 0.66f, Font::plain);
        default: return ui (height, 2);
    }
}

String Fonts::displayText (int which, const String& text)
{
    // Orbitron no trae algunos signos: se cambian por lo más parecido
    if (which == 6)
        return text.replace (U8 ("ø7"), "m7b5").replace (U8 ("ø"), "m7b5").replace (U8 ("ª"), "a").replace (U8 ("·"), "-");
    return text;
}

//==============================================================================
const std::vector<LookInfo>& looks()
{
    static const std::vector<LookInfo> l {
        { "campo",   "Campo",   U8 ("Gris claro, botones blancos y perillas de color"), keysPastilla },
        { "sampler", "Sampler", U8 ("Botones oscuros y pantalla con letras pixeladas"),  keysBotones },
        { "cristal", "Cristal", U8 ("Blanco lila suave, pulido y brillante"),            keysPlana },
        { "metal",   "Metal",   U8 ("Aluminio cepillado y pantalla azul hielo"),        keysClasica },
        { "ritmo",   "Ritmo",   U8 ("Caja de ritmos: botones rojo, naranja, amarillo y crema"), keysBotones },
        { "neon",    U8 ("Neón"), U8 ("Noche morada con luces rosa y cian"),             keysPlana },
        { "papel",   "Papel",   U8 ("Papel y tinta: plano, con contornos y sin sombras"), keysPlana },
    };
    return l;
}

int lookIndex (const String& id)
{
    const auto& l = looks();
    for (int i = 0; i < (int) l.size(); ++i)
        if (l[(size_t) i].id == id) return i;
    return 0;
}

StringArray keyStyleNames()
{
    return { "Pastilla", "Botones", "Plana", U8 ("Clásica") };
}

const std::vector<QuickPalette>& quickPalettes()
{
    static const std::vector<QuickPalette> p {
        { "Original",       Colour(),           Colour() },
        { "Laca y oro",     Colour (0xff2a1d14), Colour (0xffcfa65a) },
        { U8 ("Neón"),      Colour (0xff3b1d5e), Colour (0xffff3d9a) },
        { "Pulso",          Colour (0xff3a4a20), Colour (0xffa8e22e) },
        { "Prisma",         Colour (0xff3a2a5a), Colour (0xffff7a45) },
        { "Wavetable",      Colour (0xff3a3560), Colour (0xff3ee6c9) },
        { "Grafito",        Colour (0xff2c3440), Colour (0xff4cc2ff) },
        { "Futurista",      Colour (0xff1f3a44), Colour (0xff35e0ff) },
        { "Retro",          Colour (0xff6b5a44), Colour (0xffff7a1a) },
        { "Videojuego",     Colour (0xff2a2d6a), Colour (0xffffd23f) },
        { "Minimalista",    Colour (0xff9aa3ad), Colour (0xff2f6bff) },
        { "Cromo",          Colour (0xff8d9196), Colour (0xffff5a1f) },
    };
    return p;
}

const std::vector<Colour>& swatches()
{
    static const std::vector<Colour> s {
        Colour (0xffff5a1f), Colour (0xffff3d7f), Colour (0xffb05cff), Colour (0xff7d6bff), Colour (0xff2e6bff), Colour (0xff3fc8ff),
        Colour (0xff18c27a), Colour (0xffa8e22e), Colour (0xffffd400), Colour (0xffc9a57d), Colour (0xff22305e), Colour (0xff9a9a98),
        Colour (0xffffffff), Colour (0xff1c1c1e),
    };
    return s;
}

//==============================================================================
// Paletas base de cada estilo (las de las maquetas que eligió Joey)
static Skin baseSkin (int look, bool night)
{
    Skin s;
    s.look = look;
    s.night = night;
    auto C = [] (uint32 c) { return Colour (c); };
    auto A = [] (uint8 r, uint8 g, uint8 b, float a) { return Colour (r, g, b, a); };

    if (look == 0)          // Campo
    {
        s.radius = 30.0f;
        if (! night)
        {
            s.backdrop = C (0xffcfcdc7); s.body1 = C (0xfff1efeb); s.body2 = C (0xffe2e0da); s.edge = C (0xffbdbab3);
            s.text = C (0xff1d1d1d); s.dim = C (0xff76746e);
            s.btn = C (0xfffaf9f6); s.btnEdge = C (0xffd3d0c9); s.btnShadow = A (80, 70, 50, 0.20f); s.btnText = C (0xff1d1d1d);
            s.plate = C (0xffe9e7e2); s.plateLine = A (0, 0, 0, 0.10f); s.card = C (0xfffbfaf7); s.ink = C (0xff1d1d1d);
            s.screen = C (0xff0b0b0c); s.scrText = C (0xfff3f3f1); s.scrDim = C (0xff8a8a88);
            s.accent = C (0xffff5a1f); s.accentInk = C (0xffffffff); s.lit = C (0xffff5a1f);
            s.caps[0] = C (0xff22305e); s.caps[1] = C (0xffc9a57d); s.caps[2] = C (0xffa9a9a7); s.caps[3] = C (0xffff5a1f);
            s.keyW = C (0xfffbfbf9); s.keyWEdge = C (0xffd5d2cb); s.keyB = C (0xff1c1c1e); s.keyBed = C (0xffdcd9d2);
            s.region = A (255, 90, 31, 0.11f); s.ring = A (0, 0, 0, 0.035f);
        }
        else
        {
            s.backdrop = C (0xff0d0d0e); s.body1 = C (0xff2b2b2e); s.body2 = C (0xff1f1f21); s.edge = C (0xff0c0c0d);
            s.text = C (0xffeeede9); s.dim = C (0xff96958f);
            s.btn = C (0xff37373b); s.btnEdge = C (0xff19191b); s.btnShadow = A (0, 0, 0, 0.5f); s.btnText = C (0xffeeede9);
            s.plate = C (0xff252528); s.plateLine = A (255, 255, 255, 0.09f); s.card = C (0xff2f2f33); s.ink = C (0xffecebe7);
            s.screen = C (0xff060606); s.scrText = C (0xfff3f3f1); s.scrDim = C (0xff7d7d7b);
            s.accent = C (0xffff5a1f); s.accentInk = C (0xffffffff); s.lit = C (0xffff5a1f);
            s.caps[0] = C (0xff3b56b8); s.caps[1] = C (0xffc9a57d); s.caps[2] = C (0xff77777a); s.caps[3] = C (0xffff5a1f);
            s.keyW = C (0xff3c3c40); s.keyWEdge = C (0xff18181a); s.keyB = C (0xff121213); s.keyBed = C (0xff18181a);
            s.region = A (255, 90, 31, 0.15f); s.ring = A (255, 255, 255, 0.03f);
        }
    }
    else if (look == 1)     // Sampler
    {
        s.radius = 18.0f;
        s.pixelScreen = true;
        if (! night)
        {
            s.backdrop = C (0xffc7c8c4); s.body1 = C (0xffe3e4e0); s.body2 = C (0xffd2d3cf); s.edge = C (0xffa9aaa6);
            s.text = C (0xff1a1b1d); s.dim = C (0xff66686b);
            s.btn = C (0xff2c2d30); s.btnEdge = C (0xff141416); s.btnShadow = A (0, 0, 0, 0.3f); s.btnText = C (0xfff2f2f0);
            s.plate = C (0xffd9dad6); s.plateLine = A (0, 0, 0, 0.12f); s.card = C (0xfff6f6f3); s.ink = C (0xff1a1b1d);
            s.screen = C (0xff0c0c0d); s.scrText = C (0xffff7a1a); s.scrDim = C (0xff8a6446);
            s.accent = C (0xffff6b1a); s.accentInk = C (0xffffffff); s.lit = C (0xffff6b1a);
            s.caps[0] = C (0xff1a1b1d); s.caps[1] = C (0xff1a1b1d); s.caps[2] = C (0xff1a1b1d); s.caps[3] = C (0xffff6b1a);
            s.keyW = C (0xff34353a); s.keyWEdge = C (0xff151517); s.keyB = C (0xff1d1e21); s.keyBed = C (0xffc8c9c5);
            s.region = A (255, 107, 26, 0.13f); s.ring = A (0, 0, 0, 0.04f);
        }
        else
        {
            s.backdrop = C (0xff0a0a0b); s.body1 = C (0xff1c1c1f); s.body2 = C (0xff121214); s.edge = C (0xff000000);
            s.text = C (0xffe9e9e6); s.dim = C (0xff8c8d91);
            s.btn = C (0xff2a2b2f); s.btnEdge = C (0xff0a0a0b); s.btnShadow = A (0, 0, 0, 0.6f); s.btnText = C (0xffe9e9e6);
            s.plate = C (0xff18181b); s.plateLine = A (255, 255, 255, 0.09f); s.card = C (0xff232327); s.ink = C (0xffe6e6e3);
            s.screen = C (0xff050505); s.scrText = C (0xffff7a1a); s.scrDim = C (0xff7a5636);
            s.accent = C (0xffff6b1a); s.accentInk = C (0xffffffff); s.lit = C (0xffff6b1a);
            s.caps[0] = C (0xff0d0d0e); s.caps[1] = C (0xff0d0d0e); s.caps[2] = C (0xff0d0d0e); s.caps[3] = C (0xffff6b1a);
            s.keyW = C (0xff3d3e44); s.keyWEdge = C (0xff0b0b0c); s.keyB = C (0xff27282d); s.keyBed = C (0xff0e0e10);
            s.region = A (255, 107, 26, 0.15f); s.ring = A (255, 255, 255, 0.03f);
        }
    }
    else if (look == 3)     // Metal
    {
        s.radius = 16.0f;
        s.texture = 1;
        if (! night)
        {
            s.backdrop = C (0xffb9bdc2); s.body1 = C (0xffe4e6e9); s.body2 = C (0xffc9cdd2); s.edge = C (0xff9ca1a8);
            s.text = C (0xff15181c); s.dim = C (0xff5f656d);
            s.btn = C (0xffeef0f2); s.btnEdge = C (0xffa9aeb5); s.btnShadow = A (20, 30, 45, 0.25f); s.btnText = C (0xff15181c);
            s.plate = C (0xffd5d8dc); s.plateLine = A (0, 0, 0, 0.12f); s.card = C (0xfff6f7f8); s.ink = C (0xff15181c);
            s.screen = C (0xff0b1420); s.scrText = C (0xff7fd8ff); s.scrDim = C (0xff4b7a96);
            s.accent = C (0xff2f7bff); s.accentInk = C (0xffffffff); s.lit = C (0xff2f7bff);
            s.caps[0] = C (0xff1d2a3a); s.caps[1] = C (0xff8a96a3); s.caps[2] = C (0xffbfc5cc); s.caps[3] = C (0xff2f7bff);
            s.keyW = C (0xfff3f4f6); s.keyWEdge = C (0xffaeb3ba); s.keyB = C (0xff1b1f24); s.keyBed = C (0xffbcc0c6);
            s.region = A (47, 123, 255, 0.12f); s.ring = A (0, 0, 0, 0.04f);
        }
        else
        {
            s.backdrop = C (0xff0c0e11); s.body1 = C (0xff2b3036); s.body2 = C (0xff1d2126); s.edge = C (0xff0a0b0d);
            s.text = C (0xffe8ecf0); s.dim = C (0xff8d96a0);
            s.btn = C (0xff383e46); s.btnEdge = C (0xff15181c); s.btnShadow = A (0, 0, 0, 0.55f); s.btnText = C (0xffe8ecf0);
            s.plate = C (0xff23282e); s.plateLine = A (255, 255, 255, 0.09f); s.card = C (0xff2f353c); s.ink = C (0xffe8ecf0);
            s.screen = C (0xff05090f); s.scrText = C (0xff7fd8ff); s.scrDim = C (0xff3f6a84);
            s.accent = C (0xff3d8bff); s.accentInk = C (0xffffffff); s.lit = C (0xff3d8bff);
            s.caps[0] = C (0xff11161c); s.caps[1] = C (0xff59636e); s.caps[2] = C (0xff434a53); s.caps[3] = C (0xff3d8bff);
            s.keyW = C (0xff4a5058); s.keyWEdge = C (0xff15181c); s.keyB = C (0xff111316); s.keyBed = C (0xff15181c);
            s.region = A (61, 139, 255, 0.16f); s.ring = A (255, 255, 255, 0.03f);
        }
    }
    else if (look == 4)     // Ritmo (caja de ritmos)
    {
        s.radius = 12.0f;
        s.stepColours = true;
        s.caps[0] = C (0xffe8452c); s.caps[1] = C (0xfff08a24); s.caps[2] = C (0xfff2c94c); s.caps[3] = C (0xffefe6d2);
        if (! night)
        {
            s.backdrop = C (0xffcac6bd); s.body1 = C (0xffe9e5dc); s.body2 = C (0xffd8d3c8); s.edge = C (0xffaea89c);
            s.text = C (0xff1e1c19); s.dim = C (0xff6e685e);
            s.btn = C (0xfff5f1e8); s.btnEdge = C (0xffc4beb2); s.btnShadow = A (60, 50, 30, 0.22f); s.btnText = C (0xff1e1c19);
            s.plate = C (0xffdfdad0); s.plateLine = A (0, 0, 0, 0.11f); s.card = C (0xfffbf8f1); s.ink = C (0xff1e1c19);
            s.screen = C (0xff16130f); s.scrText = C (0xffff8a2a); s.scrDim = C (0xff8a5a30);
            s.accent = C (0xffe8452c); s.accentInk = C (0xffffffff); s.lit = C (0xffe8452c);
            s.keyW = C (0xfffaf7f0); s.keyWEdge = C (0xffcfc8ba); s.keyB = C (0xff2a2724); s.keyBed = C (0xffd3cdc1);
            s.region = A (232, 69, 44, 0.12f); s.ring = A (0, 0, 0, 0.035f);
        }
        else
        {
            s.backdrop = C (0xff0e0d0c); s.body1 = C (0xff2e2c2a); s.body2 = C (0xff22201e); s.edge = C (0xff0b0a09);
            s.text = C (0xfff0ebe1); s.dim = C (0xff9b958a);
            s.btn = C (0xff3a3734); s.btnEdge = C (0xff171513); s.btnShadow = A (0, 0, 0, 0.55f); s.btnText = C (0xfff0ebe1);
            s.plate = C (0xff272523); s.plateLine = A (255, 255, 255, 0.08f); s.card = C (0xff33302d); s.ink = C (0xffefe9de);
            s.screen = C (0xff0a0908); s.scrText = C (0xffff8a2a); s.scrDim = C (0xff7a4c26);
            s.accent = C (0xffe8452c); s.accentInk = C (0xffffffff); s.lit = C (0xffe8452c);
            s.keyW = C (0xff45413c); s.keyWEdge = C (0xff151311); s.keyB = C (0xff161412); s.keyBed = C (0xff151311);
            s.region = A (232, 69, 44, 0.16f); s.ring = A (255, 255, 255, 0.03f);
        }
    }
    else if (look == 5)     // Neón
    {
        s.radius = 22.0f;
        s.neon = true;
        if (night)
        {
            s.backdrop = C (0xff06040f); s.body1 = C (0xff171230); s.body2 = C (0xff0e0a20); s.edge = C (0xff05030c);
            s.text = C (0xfff1ecff); s.dim = C (0xff9a8fc4);
            s.btn = C (0xff221b42); s.btnEdge = C (0xff0b0819); s.btnShadow = A (0, 0, 0, 0.6f); s.btnText = C (0xfff1ecff);
            s.plate = C (0xff130f29); s.plateLine = A (190, 150, 255, 0.14f); s.card = C (0xff1b1538); s.ink = C (0xffefe8ff);
            s.screen = C (0xff07051a); s.scrText = C (0xffff4fd8); s.scrDim = C (0xff8a4f9e);
            s.accent = C (0xffff3fc8); s.accentInk = C (0xffffffff); s.lit = C (0xff35f0ff);
            s.caps[0] = C (0xff2a1f55); s.caps[1] = C (0xff35f0ff); s.caps[2] = C (0xff3b2f6e); s.caps[3] = C (0xffff3fc8);
            s.keyW = C (0xff2b2452); s.keyWEdge = C (0xff0b0819); s.keyB = C (0xff0d0a1f); s.keyBed = C (0xff0b0819);
            s.region = A (255, 63, 200, 0.16f); s.ring = A (190, 150, 255, 0.04f);
        }
        else
        {
            s.backdrop = C (0xffe5dcf5); s.body1 = C (0xfffaf6ff); s.body2 = C (0xffece4fa); s.edge = C (0xffcdbfe6);
            s.text = C (0xff241b3d); s.dim = C (0xff7a6b9c);
            s.btn = C (0xffffffff); s.btnEdge = C (0xffdccff2); s.btnShadow = A (120, 60, 200, 0.18f); s.btnText = C (0xff241b3d);
            s.plate = C (0xfff1eafc); s.plateLine = A (90, 40, 160, 0.10f); s.card = C (0xffffffff); s.ink = C (0xff241b3d);
            s.screen = C (0xff160f2e); s.scrText = C (0xffff4fd8); s.scrDim = C (0xff8f6aa8);
            s.accent = C (0xffff2fb9); s.accentInk = C (0xffffffff); s.lit = C (0xff12c8e8);
            s.caps[0] = C (0xff3a2a6e); s.caps[1] = C (0xff12c8e8); s.caps[2] = C (0xffd7cbee); s.caps[3] = C (0xffff2fb9);
            s.keyW = C (0xffffffff); s.keyWEdge = C (0xffd8ccee); s.keyB = C (0xff2a1f55); s.keyBed = C (0xffded3f2);
            s.region = A (255, 47, 185, 0.12f); s.ring = A (90, 40, 160, 0.035f);
        }
    }
    else if (look == 6)     // Papel
    {
        s.radius = 10.0f;
        s.flat = true;
        s.texture = 2;
        if (! night)
        {
            s.backdrop = C (0xffd9d5cc); s.body1 = C (0xfff3f0e8); s.body2 = C (0xffece8de); s.edge = C (0xff2a2d33);
            s.text = C (0xff1f2228); s.dim = C (0xff6d6a63);
            s.btn = C (0xfff7f5ef); s.btnEdge = C (0xff2a2d33); s.btnShadow = A (0, 0, 0, 0.0f); s.btnText = C (0xff1f2228);
            s.plate = C (0xffefebe2); s.plateLine = A (30, 33, 40, 0.22f); s.card = C (0xfffbf9f4); s.ink = C (0xff1f2228);
            s.screen = C (0xffe6e2d7); s.scrText = C (0xff1f2228); s.scrDim = C (0xff7a766d);
            s.accent = C (0xff2f55ff); s.accentInk = C (0xffffffff); s.lit = C (0xff2f55ff);
            s.caps[0] = C (0xff1f2228); s.caps[1] = C (0xff2f55ff); s.caps[2] = C (0xffd8d3c7); s.caps[3] = C (0xff2f55ff);
            s.keyW = C (0xfffbf9f4); s.keyWEdge = C (0xff2a2d33); s.keyB = C (0xff1f2228); s.keyBed = C (0xffe4dfd3);
            s.region = A (47, 85, 255, 0.10f); s.ring = A (0, 0, 0, 0.0f);
        }
        else
        {
            s.backdrop = C (0xff0d0f14); s.body1 = C (0xff1c2029); s.body2 = C (0xff171a22); s.edge = C (0xffd9d3c3);
            s.text = C (0xffece6d6); s.dim = C (0xff9a9586);
            s.btn = C (0xff1f232d); s.btnEdge = C (0xffd9d3c3); s.btnShadow = A (0, 0, 0, 0.0f); s.btnText = C (0xffece6d6);
            s.plate = C (0xff1a1e26); s.plateLine = A (236, 230, 214, 0.25f); s.card = C (0xff20242e); s.ink = C (0xffece6d6);
            s.screen = C (0xff12151c); s.scrText = C (0xffece6d6); s.scrDim = C (0xff8a8576);
            s.accent = C (0xff7d9bff); s.accentInk = C (0xff0d0f14); s.lit = C (0xff7d9bff);
            s.caps[0] = C (0xff2b303c); s.caps[1] = C (0xff7d9bff); s.caps[2] = C (0xff3a3f4b); s.caps[3] = C (0xff7d9bff);
            s.keyW = C (0xff262b36); s.keyWEdge = C (0xffd9d3c3); s.keyB = C (0xff0f1218); s.keyBed = C (0xff12151c);
            s.region = A (125, 155, 255, 0.14f); s.ring = A (0, 0, 0, 0.0f);
        }
    }
    else                    // Cristal
    {
        s.radius = 34.0f;
        if (! night)
        {
            s.backdrop = C (0xffd9d7e8); s.body1 = C (0xfff5f4fc); s.body2 = C (0xffe6e4f4); s.edge = C (0xffc9c6de);
            s.text = C (0xff1d1b33); s.dim = C (0xff73708f);
            s.btn = C (0xffffffff); s.btnEdge = C (0xffdcd9ee); s.btnShadow = A (80, 70, 160, 0.17f); s.btnText = C (0xff1d1b33);
            s.plate = C (0xffeeedf8); s.plateLine = A (40, 30, 120, 0.10f); s.card = C (0xffffffff); s.ink = C (0xff1d1b33);
            s.screen = C (0xff17152e); s.scrText = C (0xfff2f0ff); s.scrDim = C (0xff8783b3);
            s.accent = C (0xff7d6bff); s.accentInk = C (0xffffffff); s.lit = C (0xff7d6bff);
            s.caps[0] = C (0xff7d6bff); s.caps[1] = C (0xff3fc8ff); s.caps[2] = C (0xffc9c6de); s.caps[3] = C (0xffff7ab6);
            s.keyW = C (0xffffffff); s.keyWEdge = C (0xffd7d4ec); s.keyB = C (0xff2a2750); s.keyBed = C (0xffd8d5ec);
            s.region = A (125, 107, 255, 0.12f); s.ring = A (40, 30, 120, 0.03f);
        }
        else
        {
            s.backdrop = C (0xff07061a); s.body1 = C (0xff1d1b38); s.body2 = C (0xff121128); s.edge = C (0xff05041a);
            s.text = C (0xffecebff); s.dim = C (0xff9692c0);
            s.btn = C (0xff26244a); s.btnEdge = C (0xff0c0b1f); s.btnShadow = A (0, 0, 0, 0.55f); s.btnText = C (0xffecebff);
            s.plate = C (0xff1a1834); s.plateLine = A (200, 190, 255, 0.12f); s.card = C (0xff211f40); s.ink = C (0xffe6e3ff);
            s.screen = C (0xff0a0920); s.scrText = C (0xfff2f0ff); s.scrDim = C (0xff7a76a8);
            s.accent = C (0xff8f7dff); s.accentInk = C (0xffffffff); s.lit = C (0xff3fd4ff);
            s.caps[0] = C (0xff8f7dff); s.caps[1] = C (0xff3fd4ff); s.caps[2] = C (0xff4a4775); s.caps[3] = C (0xffff7ab6);
            s.keyW = C (0xffd9d6f2); s.keyWEdge = C (0xff0c0b1f); s.keyB = C (0xff0f0e24); s.keyBed = C (0xff0c0b1f);
            s.region = A (143, 125, 255, 0.16f); s.ring = A (200, 190, 255, 0.03f);
        }
    }
    return s;
}

// Lleva un color al tono elegido conservando su brillo (así el día sigue claro y la noche oscura)
static Colour tinted (Colour base, Colour tint, float strength)
{
    if (tint.isTransparent()) return base;
    const float sat = jlimit (0.0f, 1.0f, tint.getSaturation() * strength);
    const float bri = base.getBrightness();
    return Colour::fromHSV (tint.getHue(), jmax (base.getSaturation() * (1.0f - strength), sat), bri, base.getFloatAlpha());
}

Colour Skin::noteColour (int midiNote) const
{
    if (rainbow)
    {
        // Arcoíris ordenado por quintas: los acordes consonantes quedan en colores vecinos
        const int pc = ((midiNote % 12) + 12) % 12;
        const float pos = (float) ((pc * 7) % 12) / 12.0f;
        return Colour::fromHSV (pos, 0.66f, 1.0f, 1.0f);
    }
    return lit;
}

Colour Skin::noteTag (int index) const
{
    if (pixelScreen) return scrText;
    static const Colour cols[5] = { Colour (0xffff5a1f), Colour (0xff2e6bff), Colour (0xff18c27a), Colour (0xffffd400), Colour (0xffff3d7f) };
    return cols[((index % 5) + 5) % 5];
}

//==============================================================================
StringArray screenSkinNames()    { return { "Del estilo", "LCD verde", U8 ("Ámbar"), "Hielo", "Papel", "Vidrio", U8 ("Neón") }; }
StringArray screenFontNames()    { return { "Del estilo", "Redonda", "Fina", "Pixel", "LCD", "Terminal", "Futurista", U8 ("Neón"), "Arcade", "Retro" }; }
StringArray screenTextureNames() { return { U8 ("Del diseño"), "Lisa", U8 ("Líneas"), "Puntos", "Grano", "Reflejo" }; }
StringArray screenMotionNames()  { return { "Deslizar", "Fundido", "Zoom", "Escribir", "Quieto" }; }
StringArray degreeSkinNames()    { return { "Del estilo", "Pastilla", "Plano", "Contorno", "Pantallitas", "Colores en fila" }; }
StringArray circleSkinNames()    { return { U8 ("Clásico"), U8 ("Líneas"), "Color", "Vinilo", U8 ("Neón") }; }
StringArray circleTextureNames() { return { U8 ("Del diseño"), "Liso", "Surcos", "Metal", "Grano", "Puntos" }; }
StringArray circleRouteNames()   { return { "Rodean el centro", "Por debajo", "Se difuminan", "Por encima" }; }
StringArray circleNodeNames()    { return { "Con brillo", "Anillos", "Sin puntos" }; }
StringArray staffSkinNames()     { return { "Tarjeta", "Papel", "Pizarra", "Transparente", U8 ("Neón"), "Pantalla" }; }
StringArray staffTextureNames()  { return { U8 ("Del diseño"), "Liso", "Papel", "Tiza", "Pautado", "Puntos" }; }
StringArray screenShapeNames()   { return { "Redondeada", "Recta", "Pastilla", "Ovalada" }; }
StringArray degreeShapeNames()   { return { "Redondeados", "Cuadrados", "Pastilla", U8 ("Círculos") }; }
StringArray frameShapeNames()    { return { "Redondeado", "Recto", "Muy redondo", "Biselado" }; }
StringArray plateShapeNames()    { return { "Redondeada", "Recta", "Muy redonda", "Sin placa" }; }
StringArray buttonShapeNames()   { return { "Redondeados", U8 ("Círculos"), "Cuadrados" }; }
StringArray keyMotionNames()     { return { "Suave", "Al instante", "Con estela" }; }

static Colour readColour (const XmlElement& e, const char* key)
{
    const auto v = e.getStringAttribute (key);
    return v.isEmpty() ? Colour() : Colour::fromString (v);
}

static void writeColour (XmlElement& e, const char* key, Colour c)
{
    if (! c.isTransparent()) e.setAttribute (key, c.toString());
}

void PieceStyle::read (const XmlElement& e)
{
    scrSkin = jlimit (0, 6, e.getIntAttribute ("scrSkin", 0));
    scrFont = jlimit (0, 9, e.getIntAttribute ("scrFont", 0));
    scrTexture = jlimit (0, 5, e.getIntAttribute ("scrTexture", 0));
    scrText = readColour (e, "scrText");
    scrBack = readColour (e, "scrBack");
    scrGlow = jlimit (-1.0f, 1.0f, (float) e.getDoubleAttribute ("scrGlow", -1.0));
    scrMotion = jlimit (0, 4, e.getIntAttribute ("scrMotion", 0));
    degSkin = jlimit (0, 5, e.getIntAttribute ("degSkin", 0));
    degColour = readColour (e, "degColour");
    cirSkin = jlimit (0, 4, e.getIntAttribute ("cirSkin", 0));
    cirTexture = jlimit (0, 5, e.getIntAttribute ("cirTexture", 0));
    cirColour = readColour (e, "cirColour");
    cirRoute = jlimit (0, 3, e.getIntAttribute ("cirRoute", 0));
    cirLineW = jlimit (0.5f, 2.5f, (float) e.getDoubleAttribute ("cirLineW", 1.0));
    cirKnob = jlimit (0.5f, 1.15f, (float) e.getDoubleAttribute ("cirKnob", 1.0));
    cirNodes = jlimit (0, 2, e.getIntAttribute ("cirNodes", 0));
    stfSkin = jlimit (0, 5, e.getIntAttribute ("stfSkin", 0));
    stfTexture = jlimit (0, 5, e.getIntAttribute ("stfTexture", 0));
    stfInk = readColour (e, "stfInk");
    stfPaper = readColour (e, "stfPaper");
    scrShape = jlimit (0, 3, e.getIntAttribute ("scrShape", 0));
    degShape = jlimit (0, 3, e.getIntAttribute ("degShape", 0));
    stfShape = jlimit (0, 3, e.getIntAttribute ("stfShape", 0));
    cirShape = jlimit (0, 3, e.getIntAttribute ("cirShape", 0));
    btnShape = jlimit (0, 2, e.getIntAttribute ("btnShape", 0));
    anim = e.getBoolAttribute ("anim", true);
    animSpeed = jlimit (0.4f, 2.5f, (float) e.getDoubleAttribute ("animSpeed", 1.0));
    keyGlow = jlimit (0.0f, 1.0f, (float) e.getDoubleAttribute ("keyGlow", 0.5));
    keyMotion = jlimit (0, 2, e.getIntAttribute ("keyMotion", 0));
    cirFlow = e.getBoolAttribute ("cirFlow", true);
    panelMotion = e.getBoolAttribute ("panelMotion", true);
}

void PieceStyle::write (XmlElement& e) const
{
    e.setAttribute ("scrSkin", scrSkin);
    e.setAttribute ("scrFont", scrFont);
    e.setAttribute ("scrTexture", scrTexture);
    writeColour (e, "scrText", scrText);
    writeColour (e, "scrBack", scrBack);
    e.setAttribute ("scrGlow", (double) scrGlow);
    e.setAttribute ("scrMotion", scrMotion);
    e.setAttribute ("degSkin", degSkin);
    writeColour (e, "degColour", degColour);
    e.setAttribute ("cirSkin", cirSkin);
    e.setAttribute ("cirTexture", cirTexture);
    writeColour (e, "cirColour", cirColour);
    e.setAttribute ("cirRoute", cirRoute);
    e.setAttribute ("cirLineW", (double) cirLineW);
    e.setAttribute ("cirKnob", (double) cirKnob);
    e.setAttribute ("cirNodes", cirNodes);
    e.setAttribute ("stfSkin", stfSkin);
    e.setAttribute ("stfTexture", stfTexture);
    writeColour (e, "stfInk", stfInk);
    writeColour (e, "stfPaper", stfPaper);
    e.setAttribute ("scrShape", scrShape);
    e.setAttribute ("degShape", degShape);
    e.setAttribute ("stfShape", stfShape);
    e.setAttribute ("cirShape", cirShape);
    e.setAttribute ("btnShape", btnShape);
    e.setAttribute ("anim", anim);
    e.setAttribute ("animSpeed", (double) animSpeed);
    e.setAttribute ("keyGlow", (double) keyGlow);
    e.setAttribute ("keyMotion", keyMotion);
    e.setAttribute ("cirFlow", cirFlow);
    e.setAttribute ("panelMotion", panelMotion);
}

String Prefs::keyName (int code)
{
    if (code <= 0) return U8 ("—");
    if (code == KeyPress::spaceKey) return "Espacio";
    if (code >= 'a' && code <= 'z') return String::charToString ((juce_wchar) (code - 'a' + 'A'));
    if (code > 32 && code < 127) return String::charToString ((juce_wchar) code);
    return KeyPress (code).getTextDescription();
}

String Prefs::padName (int code)
{
    if (code < 0) return U8 ("—");
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int note = code & 0x7f, ch = code >> 8;
    String t = String (names[note % 12]) + String (note / 12 - 2) + "  (" + String (note) + ")";
    if (ch > 0) t << U8 (" · canal ") << ch;
    return t;
}

//==============================================================================
static std::map<String, FloatState> readFloats (const XmlElement& xml)
{
    std::map<String, FloatState> out;
    for (auto* e : xml.getChildIterator())
    {
        FloatState f;
        f.floating = e->getBoolAttribute ("floating");
        f.onTop = e->getBoolAttribute ("onTop", true);
        f.collapsed = e->getBoolAttribute ("collapsed", false);
        f.bounds = { e->getIntAttribute ("x"), e->getIntAttribute ("y"), e->getIntAttribute ("w"), e->getIntAttribute ("h") };
        out[e->getStringAttribute ("id")] = f;
    }
    return out;
}

static std::unique_ptr<XmlElement> writeFloats (const std::map<String, FloatState>& floats)
{
    auto xml = std::make_unique<XmlElement> ("FLOATS");
    for (auto& [id, f] : floats)
    {
        auto* e = xml->createNewChildElement ("F");
        e->setAttribute ("id", id);
        e->setAttribute ("floating", f.floating);
        e->setAttribute ("onTop", f.onTop);
        e->setAttribute ("collapsed", f.collapsed);
        e->setAttribute ("x", f.bounds.getX());
        e->setAttribute ("y", f.bounds.getY());
        e->setAttribute ("w", f.bounds.getWidth());
        e->setAttribute ("h", f.bounds.getHeight());
    }
    return xml;
}

Prefs::Prefs()
{
    PropertiesFile::Options o;
    o.applicationName = "MidiSky";
    o.filenameSuffix = ".settings";
    o.folderName = "MidiSky";   // se queda el nombre viejo para no perder tus ajustes
    o.osxLibrarySubFolder = "Application Support";
    o.commonToAllUsers = false;
    o.millisecondsBeforeSaving = 400;
    file = std::make_unique<PropertiesFile> (o);

    auto colourOf = [this] (const char* key)
    {
        const auto s = file->getValue (key, "");
        return s.isEmpty() ? Colour() : Colour::fromString (s);
    };

    look        = file->getValue ("look", "campo");
    if (lookIndex (look) == 0 && look != "campo") look = "campo";
    dayNight    = jlimit (0, 2, file->getIntValue ("dayNight", 0));
    keyStyle    = jlimit (-1, 3, file->getIntValue ("keyStyle", -1));
    octaves     = jlimit (2, 4, file->getIntValue ("octaves", 3));
    startC      = jlimit (12, 108 - 12 * octaves, file->getIntValue ("startC", 48));
    startC      -= startC % 12;
    uiTint      = colourOf ("uiTint");
    uiAccent    = colourOf ("uiAccent");
    keyLit      = colourOf ("keyLit");
    keyWhite    = colourOf ("keyWhite");
    keyBlack    = colourOf ("keyBlack");
    rainbow     = file->getBoolValue ("rainbow", false);
    latin       = file->getBoolValue ("latin", false);
    sevenths    = file->getBoolValue ("sevenths", false);
    midiAuto    = file->getBoolValue ("midiAuto", true);
    hideParams  = file->getBoolValue ("hideParams", false);
    transparent = file->getBoolValue ("transparent", false);
    lowLatencyDone = file->getBoolValue ("lowLatencyDone", false);

    // Ajustes de versiones anteriores
    if (! file->containsKey ("look"))
    {
        if (file->getIntValue ("lightMode", 0) == 2) rainbow = true;
        if (file->getIntValue ("lightMode", 0) == 1) keyLit = Colour::fromString (file->getValue ("lightCustom", "ff7fb8ff"));
    }

    if (auto xml = file->getXmlValue ("floats"))
        floats = readFloats (*xml);

    metroBpm        = jlimit (20.0f, 300.0f, (float) file->getDoubleValue ("metroBpm", 100.0));
    metroBeats      = jlimit (1, 12, file->getIntValue ("metroBeats", 4));
    metroSound      = jlimit (0, 5, file->getIntValue ("metroSound", 0));
    metroVolume     = jlimit (-40.0f, 6.0f, (float) file->getDoubleValue ("metroVolume", -8.0));
    metroFollowHost = file->getBoolValue ("metroFollowHost", true);
    countIn         = file->getBoolValue ("countIn", true);
    recordMouse     = file->getBoolValue ("recordMouse", true);
    recordAudio     = false;   // la opción de grabar audio se quitó
    degreeOctave    = jlimit (-2, 2, file->getIntValue ("degreeOctave", 0));

    if (auto xml = file->getXmlValue ("pieces"))
        pieces.read (*xml);
    degreeKeysOn = file->getBoolValue ("degreeKeysOn", true);
    degreePadsOn = file->getBoolValue ("degreePadsOn", true);
    {
        auto keys = StringArray::fromTokens (file->getValue ("degreeKeys", ""), ",", "");
        if (keys.size() == 7)
            for (int i = 0; i < 7; ++i) degreeKeys[(size_t) i] = keys[i].getIntValue();
        auto pads = StringArray::fromTokens (file->getValue ("degreePads", ""), ",", "");
        if (pads.size() == 7)
            for (int i = 0; i < 7; ++i) degreePads[(size_t) i] = jlimit (-1, 16 * 256 + 127, pads[i].getIntValue());
    }

    if (auto xml = file->getXmlValue ("themes"))
        for (auto* e : xml->getChildIterator())
        {
            ThemePreset t;
            t.name = e->getStringAttribute ("name");
            t.look = e->getStringAttribute ("look", "campo");
            t.dayNight = jlimit (0, 2, e->getIntAttribute ("dayNight"));
            t.keyStyle = jlimit (-1, 3, e->getIntAttribute ("keyStyle", -1));
            auto col = [e] (const char* k) { const auto v = e->getStringAttribute (k); return v.isEmpty() ? Colour() : Colour::fromString (v); };
            t.uiTint = col ("uiTint"); t.uiAccent = col ("uiAccent");
            t.keyLit = col ("keyLit"); t.keyWhite = col ("keyWhite"); t.keyBlack = col ("keyBlack");
            t.rainbow = e->getBoolAttribute ("rainbow");
            if (auto* pe = e->getChildByName ("PIECES")) t.pieces.read (*pe);
            if (t.name.isNotEmpty()) themes.push_back (t);
        }

    if (auto xml = file->getXmlValue ("layouts"))
        for (auto* e : xml->getChildIterator())
        {
            LayoutPreset l;
            l.name = e->getStringAttribute ("name");
            l.transparent = e->getBoolAttribute ("transparent");
            l.hideParams = e->getBoolAttribute ("hideParams");
            l.editorW = e->getIntAttribute ("editorW");
            l.editorH = e->getIntAttribute ("editorH");
            if (auto* fx = e->getChildByName ("FLOATS")) l.floats = readFloats (*fx);
            if (l.name.isNotEmpty()) layouts.push_back (l);
        }
}

ThemePreset Prefs::captureTheme (const String& name) const
{
    ThemePreset t;
    t.name = name;
    t.look = look; t.dayNight = dayNight; t.keyStyle = keyStyle;
    t.uiTint = uiTint; t.uiAccent = uiAccent;
    t.keyLit = keyLit; t.keyWhite = keyWhite; t.keyBlack = keyBlack;
    t.rainbow = rainbow;
    t.pieces = pieces;
    return t;
}

void Prefs::applyTheme (const ThemePreset& t)
{
    look = t.look; dayNight = t.dayNight; keyStyle = t.keyStyle;
    uiTint = t.uiTint; uiAccent = t.uiAccent;
    keyLit = t.keyLit; keyWhite = t.keyWhite; keyBlack = t.keyBlack;
    rainbow = t.rainbow;
    pieces = t.pieces;
    save();
}

Prefs::~Prefs()
{
    if (file != nullptr)
        file->saveIfNeeded();
}

void Prefs::save (bool notify)
{
    auto putColour = [this] (const char* key, Colour c) { file->setValue (key, c.isTransparent() ? String() : c.toString()); };
    file->setValue ("look", look);
    file->setValue ("dayNight", dayNight);
    file->setValue ("keyStyle", keyStyle);
    file->setValue ("octaves", octaves);
    file->setValue ("startC", startC);
    putColour ("uiTint", uiTint);
    putColour ("uiAccent", uiAccent);
    putColour ("keyLit", keyLit);
    putColour ("keyWhite", keyWhite);
    putColour ("keyBlack", keyBlack);
    file->setValue ("rainbow", rainbow);
    file->setValue ("latin", latin);
    file->setValue ("sevenths", sevenths);
    file->setValue ("midiAuto", midiAuto);
    file->setValue ("hideParams", hideParams);
    file->setValue ("transparent", transparent);
    file->setValue ("lowLatencyDone", lowLatencyDone);

    auto xml = writeFloats (floats);
    file->setValue ("floats", xml.get());

    file->setValue ("metroBpm", (double) metroBpm);
    file->setValue ("metroBeats", metroBeats);
    file->setValue ("metroSound", metroSound);
    file->setValue ("metroVolume", (double) metroVolume);
    file->setValue ("metroFollowHost", metroFollowHost);
    file->setValue ("countIn", countIn);
    file->setValue ("recordMouse", recordMouse);
    file->setValue ("degreeOctave", degreeOctave);
    {
        XmlElement px ("PIECES");
        pieces.write (px);
        file->setValue ("pieces", &px);
        StringArray keys, pads;
        for (auto k : degreeKeys) keys.add (String (k));
        for (auto pd : degreePads) pads.add (String (pd));
        file->setValue ("degreeKeys", keys.joinIntoString (","));
        file->setValue ("degreePads", pads.joinIntoString (","));
        file->setValue ("degreeKeysOn", degreeKeysOn);
        file->setValue ("degreePadsOn", degreePadsOn);
    }

    XmlElement tx ("THEMES");
    for (auto& t : themes)
    {
        auto* e = tx.createNewChildElement ("T");
        auto putC = [e] (const char* k, Colour c) { e->setAttribute (k, c.isTransparent() ? String() : c.toString()); };
        e->setAttribute ("name", t.name);
        e->setAttribute ("look", t.look);
        e->setAttribute ("dayNight", t.dayNight);
        e->setAttribute ("keyStyle", t.keyStyle);
        putC ("uiTint", t.uiTint); putC ("uiAccent", t.uiAccent);
        putC ("keyLit", t.keyLit); putC ("keyWhite", t.keyWhite); putC ("keyBlack", t.keyBlack);
        e->setAttribute ("rainbow", t.rainbow);
        t.pieces.write (*e->createNewChildElement ("PIECES"));
    }
    file->setValue ("themes", &tx);

    XmlElement lx ("LAYOUTS");
    for (auto& l : layouts)
    {
        auto* e = lx.createNewChildElement ("L");
        e->setAttribute ("name", l.name);
        e->setAttribute ("transparent", l.transparent);
        e->setAttribute ("hideParams", l.hideParams);
        e->setAttribute ("editorW", l.editorW);
        e->setAttribute ("editorH", l.editorH);
        e->addChildElement (writeFloats (l.floats).release());
    }
    file->setValue ("layouts", &lx);
    if (notify) sendChangeMessage();
}

bool Prefs::isNight() const
{
    if (dayNight == 1) return true;
    if (dayNight == 2)
    {
        const int h = Time::getCurrentTime().getHours();
        return h >= 19 || h < 7;
    }
    return false;
}

int Prefs::effectiveKeyStyle() const
{
    if (keyStyle >= 0) return keyStyle;
    return looks()[(size_t) lookIndex (look)].defaultKeys;
}

Skin Prefs::palette (const String& lookOverride) const
{
    const auto& lk = lookOverride.isNotEmpty() ? lookOverride : look;
    Skin s = baseSkin (lookIndex (lk), isNight());

    // Tono propio del cuerpo de la interfaz
    if (! uiTint.isTransparent())
    {
        for (auto* c : { &s.backdrop, &s.body1, &s.body2, &s.edge, &s.plate, &s.keyBed })
            *c = tinted (*c, uiTint, 0.75f);
        s.card = tinted (s.card, uiTint, 0.2f);
        if (s.look != 1) s.btn = tinted (s.btn, uiTint, 0.25f);
        else             s.btn = tinted (s.btn, uiTint, 0.5f);
        s.btnEdge = tinted (s.btnEdge, uiTint, 0.6f);
        s.dim = tinted (s.dim, uiTint, 0.35f);
        if (s.look == 2) s.screen = tinted (s.screen, uiTint, 0.6f);
    }
    // Color de acento propio
    if (! uiAccent.isTransparent())
    {
        s.accent = uiAccent;
        s.accentInk = uiAccent.getPerceivedBrightness() > 0.72f ? Colour (0xff1a1a1a) : Colours::white;
        s.region = uiAccent.withAlpha (s.night ? 0.16f : 0.12f);
        s.caps[3] = uiAccent;
        if (s.pixelScreen) s.scrText = uiAccent;
        s.lit = uiAccent;
    }
    // Teclas
    if (! keyLit.isTransparent())   s.lit = keyLit;
    if (! keyWhite.isTransparent()) { s.keyW = keyWhite; s.keyWEdge = keyWhite.darker (0.25f); }
    if (! keyBlack.isTransparent()) s.keyB = keyBlack;
    s.rainbow = rainbow;
    s.keyStyle = keyStyle >= 0 ? keyStyle : looks()[(size_t) lookIndex (lk)].defaultKeys;
    s.pieces = pieces;
    return s;
}
