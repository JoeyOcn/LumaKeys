#include "Pieces.h"

using namespace juce;

namespace pieces
{
//==============================================================================
// Mosaicos de textura (se generan una vez). Cada uno guarda luz (+) y sombra (-) por separado.
namespace
{
    struct Tile { Image light, dark; };

    float hashNoise (int x, int y, int seed)
    {
        // Todo en enteros sin signo (con signo, el desbordamiento es indefinido y el compilador hace cosas raras)
        uint32_t h = (uint32_t) x * 374761393u + (uint32_t) y * 668265263u + (uint32_t) seed * 1442695041u;
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;
        return (float) (h & 0xffff) / 65535.0f;
    }

    // Ruido suave (valor interpolado) que se repite cada 'period' pixeles
    float smoothNoise (float x, float y, int period, int seed)
    {
        const int x0 = (int) std::floor (x), y0 = (int) std::floor (y);
        const float fx = x - (float) x0, fy = y - (float) y0;
        auto at = [&] (int xi, int yi) { return hashNoise (((xi % period) + period) % period, ((yi % period) + period) % period, seed); };
        auto s = [] (float t) { return t * t * (3.0f - 2.0f * t); };
        const float a = at (x0, y0), b = at (x0 + 1, y0), c = at (x0, y0 + 1), d = at (x0 + 1, y0 + 1);
        return jmap (s (fy), jmap (s (fx), a, b), jmap (s (fx), c, d));
    }

    Tile makeTile (int w, int h, std::function<float (int, int)> f)   // f devuelve -1..1
    {
        Tile t { Image (Image::ARGB, w, h, true), Image (Image::ARGB, w, h, true) };
        Image::BitmapData L (t.light, Image::BitmapData::writeOnly), D (t.dark, Image::BitmapData::writeOnly);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                const float v = jlimit (-1.0f, 1.0f, f (x, y));
                L.setPixelColour (x, y, Colours::white.withAlpha (jmax (0.0f, v)));
                D.setPixelColour (x, y, Colours::black.withAlpha (jmax (0.0f, -v)));
            }
        return t;
    }

    const Tile& tile (int kind)
    {
        static std::map<int, Tile> tiles;
        auto it = tiles.find (kind);
        if (it != tiles.end()) return it->second;

        Tile t;
        switch (kind)
        {
            case texBrushed:
                t = makeTile (512, 96, [] (int x, int y)
                {
                    // vetas largas horizontales
                    const float row = hashNoise (0, y, 3) - 0.5f;
                    const float streak = smoothNoise ((float) x / 64.0f, (float) y, 8, 7) - 0.5f;
                    const float fine = hashNoise (x, y, 11) - 0.5f;
                    return (row * 0.55f + streak * 0.6f + fine * 0.18f) * 0.55f;
                });
                break;
            case texPaper:
                t = makeTile (256, 256, [] (int x, int y)
                {
                    const float blot = smoothNoise ((float) x / 32.0f, (float) y / 32.0f, 8, 21) - 0.5f;
                    const float fib = smoothNoise ((float) x / 3.0f, (float) y / 12.0f, 85, 23) - 0.5f;
                    const float fine = hashNoise (x, y, 29) - 0.5f;
                    return blot * 0.35f + fib * 0.3f + fine * 0.35f;
                });
                break;
            case texGrain:
                t = makeTile (160, 160, [] (int x, int y) { return (hashNoise (x, y, 41) - 0.5f) * 0.9f; });
                break;
            case texChalk:
                t = makeTile (256, 256, [] (int x, int y)
                {
                    const float smudge = smoothNoise ((float) x / 24.0f, (float) y / 18.0f, 11, 51) - 0.45f;
                    const float dust = hashNoise (x, y, 53) > 0.985f ? 0.9f : 0.0f;
                    return jmax (0.0f, smudge) * 0.7f + dust;
                });
                break;
            case texDots:
                t = makeTile (8, 8, [] (int x, int y)
                {
                    const float d = std::hypot ((float) x - 3.5f, (float) y - 3.5f);
                    return d < 1.3f ? -0.9f : 0.0f;
                });
                break;
            case texLines:
                t = makeTile (4, 3, [] (int, int y) { return y == 2 ? -0.85f : 0.0f; });
                break;
            case texPixelGrid:
                t = makeTile (4, 4, [] (int x, int y) { return (x == 3 || y == 3) ? -0.7f : 0.0f; });
                break;
            case texRuled:
                t = makeTile (24, 24, [] (int x, int y) { return (x == 0 || y == 0) ? -0.55f : 0.0f; });
                break;
            default:
                t = makeTile (4, 4, [] (int, int) { return 0.0f; });
                break;
        }
        return tiles.emplace (kind, std::move (t)).first->second;
    }
}

static void drawRound (Graphics& g, int kind, bool onDark, float strength, Point<float> centre, float r)
{
    if (kind == texGrooves)
    {
        // Surcos de disco: círculos finitos que alternan luz y sombra
        int i = 0;
        for (float rr = r; rr > r * 0.08f; rr -= 2.6f, ++i)
        {
            const float a = (0.05f + 0.05f * hashNoise (i, 0, 5)) * strength * (onDark ? 1.0f : 0.8f);
            g.setColour ((i % 2 == 0 ? Colours::white : Colours::black).withAlpha (a));
            g.drawEllipse (centre.x - rr, centre.y - rr, rr * 2.0f, rr * 2.0f, 1.0f);
        }
        // Brillo cruzado del vinilo
        for (int q = 0; q < 2; ++q)
        {
            const float a0 = MathConstants<float>::pi * (0.25f + (float) q);
            Path wedge;
            wedge.addPieSegment (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, a0 - 0.32f, a0 + 0.32f, 0.0f);
            ColourGradient cg (Colours::white.withAlpha (0.10f * strength), centre.x, centre.y,
                               Colours::white.withAlpha (0.0f), centre.x + r, centre.y, true);
            g.setGradientFill (cg);
            g.fillPath (wedge);
        }
        return;
    }
    if (kind == texRadialMetal)
    {
        // Metal girado: rayitas que salen del centro
        for (int i = 0; i < 240; ++i)
        {
            const float a = MathConstants<float>::twoPi * (float) i / 240.0f;
            const float v = hashNoise (i, 1, 9) - 0.5f;
            g.setColour ((v > 0 ? Colours::white : Colours::black).withAlpha (std::abs (v) * 0.16f * strength));
            g.drawLine (centre.x, centre.y, centre.x + std::cos (a) * r, centre.y + std::sin (a) * r, 1.0f);
        }
        ColourGradient sheen (Colours::white.withAlpha (0.12f * strength), centre.x - r, centre.y - r,
                              Colours::white.withAlpha (0.0f), centre.x, centre.y, false);
        g.setGradientFill (sheen);
        g.fillEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
        return;
    }
}

void texture (Graphics& g, const Path& shape, int kind, bool onDark, float strength, Point<float> centre, float radius)
{
    if (kind == texNone || strength <= 0.0f) return;
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (shape);
    const auto b = shape.getBounds();

    if (kind == texGrooves || kind == texRadialMetal)
    {
        // Estas dos son caras de dibujar: se hacen una vez en una imagen y se reusan
        const float r = radius > 0.0f ? radius : b.getWidth() * 0.5f;
        const float phys = jlimit (1.0f, 3.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
        static std::map<String, Image> roundCache;
        const String key = String (kind) + "|" + String (roundToInt (r)) + "|" + String ((int) onDark) + "|" + String (strength, 2) + "|" + String (phys, 1);
        auto it = roundCache.find (key);
        if (it == roundCache.end())
        {
            if (roundCache.size() > 12) roundCache.clear();
            const int sz = jmax (2, roundToInt (r * 2.0f * phys));
            Image img (Image::ARGB, sz, sz, true);
            {
                Graphics ig (img);
                ig.addTransform (AffineTransform::scale (phys));
                drawRound (ig, kind, onDark, strength, { r, r }, r);
            }
            it = roundCache.emplace (key, img).first;
        }
        g.drawImage (it->second, Rectangle<float> (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f));
        return;
    }
    if (kind == texGlass)
    {
        ColourGradient glass (Colours::white.withAlpha (0.20f * strength), b.getX(), b.getY(),
                              Colours::white.withAlpha (0.0f), b.getX() + b.getWidth() * 0.55f, b.getY() + b.getHeight() * 0.8f, false);
        g.setGradientFill (glass);
        Path diag;
        diag.addTriangle (b.getX(), b.getY(), b.getX() + b.getWidth() * 0.62f, b.getY(), b.getX(), b.getBottom() * 0.98f);
        g.fillPath (diag);
        g.setColour (Colours::white.withAlpha (0.06f * strength));
        g.fillRect (b.withHeight (b.getHeight() * 0.45f));
        return;
    }

    const auto& t = tile (kind);
    // Cuánto se nota cada textura (de día las sombras pesan más; de noche, la luz)
    float lightA = onDark ? 0.10f : 0.22f, darkA = onDark ? 0.30f : 0.12f;
    if (kind == texLines || kind == texPixelGrid) { lightA = 0.0f; darkA = onDark ? 0.5f : 0.22f; }
    if (kind == texDots) { lightA = 0.0f; darkA = onDark ? 0.45f : 0.16f; }
    if (kind == texRuled) { lightA = 0.0f; darkA = onDark ? 0.0f : 0.25f; if (onDark) lightA = 0.0f; }
    if (kind == texChalk) { lightA = onDark ? 0.22f : 0.05f; darkA = onDark ? 0.0f : 0.15f; }
    if (kind == texBrushed) { lightA = onDark ? 0.10f : 0.42f; darkA = onDark ? 0.42f : 0.20f; }

    if (kind == texRuled && onDark)
    {
        // Pautado sobre oscuro: líneas claras
        for (float y = b.getY(); y < b.getBottom(); y += 24.0f)
        {
            g.setColour (Colours::white.withAlpha (0.07f * strength));
            g.fillRect (b.getX(), y, b.getWidth(), 1.0f);
        }
        for (float x = b.getX(); x < b.getRight(); x += 24.0f)
        {
            g.setColour (Colours::white.withAlpha (0.07f * strength));
            g.fillRect (x, b.getY(), 1.0f, b.getHeight());
        }
        return;
    }
    if (lightA > 0.0f)
    {
        g.setTiledImageFill (t.light, (int) b.getX(), (int) b.getY(), jmin (1.0f, lightA * strength));
        g.fillPath (shape);
    }
    if (darkA > 0.0f)
    {
        g.setTiledImageFill (t.dark, (int) b.getX(), (int) b.getY(), jmin (1.0f, darkA * strength));
        g.fillPath (shape);
    }
}

//==============================================================================
static Colour dimFor (Colour text, Colour bg) { return text.interpolatedWith (bg, 0.42f); }

ScreenLook screenLook (const Skin& s)
{
    const auto& p = s.pieces;
    ScreenLook l;
    l.bg = s.screen; l.text = s.scrText; l.dim = s.scrDim;
    l.font = p.scrFont;
    int defTex = texNone;
    switch (p.scrSkin)
    {
        case 1: l.bg = Colour (0xffa3b18a); l.text = Colour (0xff1f2a16); l.dim = Colour (0xff4f5d3f); defTex = texPixelGrid; break;
        case 2: l.bg = Colour (0xff0d0905); l.text = Colour (0xffffb12b); l.dim = Colour (0xff9a6a22); defTex = texLines; l.glow = 0.35f; break;
        case 3: l.bg = Colour (0xff06101c); l.text = Colour (0xff8fe3ff); l.dim = Colour (0xff3f7894); l.glow = 0.25f; break;
        case 4: l.bg = Colour (0xffe9e5da); l.text = Colour (0xff22252b); l.dim = Colour (0xff7a766c); defTex = texPaper; break;
        case 5: l.bg = (s.night ? s.body2.darker (0.35f) : s.body2.darker (0.12f)); l.text = s.text; l.dim = s.dim; l.glass = true; defTex = texGlass; break;
        case 6: l.bg = Colour (0xff0a0618); l.text = Colour (0xffff4fd8); l.dim = Colour (0xff7a4f9e); defTex = texLines; l.glow = 0.6f; break;
        default:
            if (s.neon) l.glow = 0.5f;
            break;
    }
    if (! p.scrBack.isTransparent()) l.bg = p.scrBack;
    if (! p.scrText.isTransparent()) { l.text = p.scrText; l.dim = dimFor (p.scrText, l.bg); }
    l.light = l.bg.getPerceivedBrightness() > 0.55f;
    static const int texMap[6] = { -1, texNone, texLines, texPixelGrid, texGrain, texGlass };
    l.texture = p.scrTexture == 0 ? defTex : texMap[jlimit (0, 5, p.scrTexture)];
    if (p.scrTexture == 5) l.glass = true;
    if (p.scrGlow >= 0.0f) l.glow = p.scrGlow;   // -1 = las luces del diseño
    if (l.light) l.glow *= 0.5f;
    l.bg = s.soften (l.bg); l.text = s.soften (l.text); l.dim = s.soften (l.dim);
    l.glow *= 1.0f - 0.6f * s.soft;
    return l;
}

StaffLook staffLook (const Skin& s)
{
    const auto& p = s.pieces;
    StaffLook l;
    l.skin = p.stfSkin;
    l.paper = s.card; l.ink = s.ink;
    int defTex = s.texture == 2 ? texPaper : texNone;
    switch (p.stfSkin)
    {
        case 1: l.paper = Colour (0xfff4efe2); l.ink = Colour (0xff2a2620); defTex = texPaper; break;
        case 2: l.paper = Colour (0xff23302b); l.ink = Colour (0xffeef0e6); defTex = texChalk; l.card = false; break;
        case 3: l.none = true; l.card = false; l.ink = s.text; defTex = texNone; break;
        case 4: l.paper = Colour (0xff0c0820); l.ink = s.accent.brighter (0.2f); l.glow = 0.6f; l.card = false; break;
        case 5:
        {
            const auto sl = screenLook (s);
            l.paper = sl.bg; l.ink = sl.text; l.glow = sl.glow; l.card = false;
            defTex = sl.texture == texGlass ? texNone : sl.texture;
            break;
        }
        default: break;
    }
    if (! p.stfPaper.isTransparent()) { l.paper = p.stfPaper; l.none = false; }
    if (! p.stfInk.isTransparent()) l.ink = p.stfInk;
    l.paper = s.soften (l.paper); l.ink = s.soften (l.ink);
    l.dim = l.ink.withAlpha (0.62f);
    static const int texMap[6] = { -1, texNone, texPaper, texChalk, texRuled, texDots };
    l.texture = p.stfTexture == 0 ? defTex : texMap[jlimit (0, 5, p.stfTexture)];
    return l;
}

int circleTexture (const Skin& s)
{
    static const int texMap[6] = { -1, texNone, texGrooves, texRadialMetal, texGrain, texDots };
    if (s.pieces.cirTexture > 0) return texMap[jlimit (0, 5, s.pieces.cirTexture)];
    if (s.pieces.cirSkin == 3) return texGrooves;
    if (s.texture == 1) return texRadialMetal;
    if (s.texture == 2) return texPaper;
    return texNone;
}

Colour circleColour (const Skin& s)
{
    return s.pieces.cirColour.isTransparent() ? s.accent : s.pieces.cirColour;
}

//==============================================================================
// Texto con brillo
//==============================================================================
static void boxBlur (Image& img, int radius)
{
    if (radius < 1) return;
    Image::BitmapData bd (img, Image::BitmapData::readWrite);
    const int w = bd.width, h = bd.height;
    std::vector<uint8_t> buf ((size_t) jmax (w, h));
    for (int pass = 0; pass < 3; ++pass)
    {
        for (int y = 0; y < h; ++y)   // horizontal
        {
            auto* row = bd.getLinePointer (y);
            int acc = 0;
            for (int x = -radius; x <= radius; ++x) acc += row[jlimit (0, w - 1, x) * bd.pixelStride];
            for (int x = 0; x < w; ++x)
            {
                buf[(size_t) x] = (uint8_t) (acc / (2 * radius + 1));
                acc += row[jlimit (0, w - 1, x + radius + 1) * bd.pixelStride] - row[jlimit (0, w - 1, x - radius) * bd.pixelStride];
            }
            for (int x = 0; x < w; ++x) row[x * bd.pixelStride] = buf[(size_t) x];
        }
        for (int x = 0; x < w; ++x)   // vertical
        {
            auto px = [&] (int y) -> uint8_t& { return bd.getLinePointer (jlimit (0, h - 1, y))[x * bd.pixelStride]; };
            int acc = 0;
            for (int y = -radius; y <= radius; ++y) acc += px (y);
            for (int y = 0; y < h; ++y)
            {
                buf[(size_t) y] = (uint8_t) (acc / (2 * radius + 1));
                acc += px (y + radius + 1) - px (y - radius);
            }
            for (int y = 0; y < h; ++y) px (y) = buf[(size_t) y];
        }
    }
}

void GlowText::draw (Graphics& g, const String& text, const Font& font, Rectangle<float> box, Justification just,
                     Colour col, float amount, float scale, Point<float> offset)
{
    if (text.isEmpty() || amount <= 0.01f || box.isEmpty()) return;
    const float phys = jlimit (1.0f, 3.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const float pad = font.getHeight() * 0.45f;
    const String key = text + "|" + font.getTypefaceName() + "|" + String (font.getHeight(), 1) + "|"
                       + String (box.getWidth(), 0) + "x" + String (box.getHeight(), 0) + "|" + String (just.getFlags()) + "|" + String (phys, 1);

    Entry* hit = nullptr;
    for (auto& e : cache) if (e.key == key) hit = &e;
    if (hit == nullptr)
    {
        const auto area = box.expanded (pad);
        const int iw = jmax (1, roundToInt (area.getWidth() * phys)), ih = jmax (1, roundToInt (area.getHeight() * phys));
        Image img (Image::SingleChannel, iw, ih, true);
        {
            Graphics ig (img);
            ig.addTransform (AffineTransform::scale (phys));
            ig.setColour (Colours::white);
            ig.setFont (font);
            ig.drawText (text, box.translated (-area.getX(), -area.getY()), just, false);
        }
        boxBlur (img, jmax (1, roundToInt (font.getHeight() * 0.07f * phys)));
        if (cache.size() >= 6)
        {
            auto oldest = std::min_element (cache.begin(), cache.end(), [] (const Entry& a, const Entry& b) { return a.used < b.used; });
            cache.erase (oldest);
        }
        cache.push_back ({ key, img, area, 0 });
        hit = &cache.back();
    }
    hit->used = ++tick;

    Graphics::ScopedSaveState ss (g);
    const auto c = box.getCentre();
    g.addTransform (AffineTransform::scale (scale, scale, c.x, c.y).translated (offset));
    g.setColour (col.withAlpha (jlimit (0.0f, 1.0f, amount)));
    g.drawImage (hit->img, hit->area, RectanglePlacement::stretchToFit, true);
    if (amount > 0.6f)   // mucha luz: una segunda pasada más intensa
    {
        g.setColour (col.withAlpha ((amount - 0.6f) * 1.2f));
        g.drawImage (hit->img, hit->area, RectanglePlacement::stretchToFit, true);
    }
}

//==============================================================================
// Botones de los grados
//==============================================================================
static Colour stepColour (const Skin& s, int i)
{
    // Como las cajas de ritmos: rojo, rojo, naranja, naranja, amarillo, amarillo, crema
    static const int group[7] = { 0, 0, 1, 1, 2, 2, 3 };
    if (s.stepColours) return s.caps[group[i]];
    static const Colour fixed[4] = { Colour (0xffe8452c), Colour (0xfff08a24), Colour (0xfff2c94c), Colour (0xffefe6d2) };
    return s.soften (fixed[group[i]]);
}

void degreeTile (Graphics& g, UiContext& ctx, Rectangle<float> r, int index, const String& roman, const String& symbol,
                 float glow, float press, float hover, float k, GlowText* glowCache)
{
    const auto& pal = ctx.vis.pal;
    const int skin = pal.pieces.degSkin;
    const auto light = pal.pieces.degColour.isTransparent() ? pal.accent : pal.pieces.degColour;
    const auto lightInk = light.getPerceivedBrightness() > 0.7f ? Colour (0xff1a1a1a) : Colours::white;
    // Forma: redondeados, cuadrados, pastilla o círculos
    const int shape = pal.pieces.degShape;
    if (shape == 3) r = r.withSizeKeepingCentre (jmin (r.getWidth(), r.getHeight()), jmin (r.getWidth(), r.getHeight()));
    const float rad = shape == 1 ? 3.0f * k : (shape >= 2 ? jmin (r.getWidth(), r.getHeight()) * 0.5f : 12.0f * k);
    auto inner = r;
    Colour textCol = pal.btnText, dimCol = pal.btnText.withAlpha (0.55f);
    Font symFont = draw::fitUi (ctx.fonts, symbol, 20.0f * k, r.getWidth() - 8.0f, 2);
    String sym = symbol;
    bool screenText = false;
    pieces::ScreenLook sl;

    const bool steps = (skin == 0 && pal.stepColours) || skin == 5;
    if (skin == 0 && ! steps)
    {
        Skin p2 = pal;
        draw::button (g, r, p2, rad, press, false, hover);
        inner = r.withTrimmedBottom (2.5f).translated (0.0f, 2.5f * press);
        if (glow > 0.01f)
        {
            g.setColour (light.withAlpha (glow));
            g.fillRoundedRectangle (inner, rad);
            g.setColour (light.withAlpha (0.35f * glow));
            g.drawRoundedRectangle (inner.expanded (2.0f), rad + 1.0f, 2.0f);
            if (pal.neon)
            {
                g.setColour (light.withAlpha (0.18f * glow));
                g.drawRoundedRectangle (inner.expanded (5.0f), rad + 4.0f, 4.0f);
            }
        }
        textCol = pal.btnText.interpolatedWith (lightInk, glow);
        dimCol = pal.btnText.withAlpha (0.55f).interpolatedWith (lightInk, glow);
    }
    else if (steps)
    {
        // Botones de colores en fila; el que suena se enciende con un led arriba
        Skin p2 = pal;
        const auto c = stepColour (pal, index);
        p2.btn = c; p2.btnEdge = c.darker (0.45f);
        p2.btnText = c.getPerceivedBrightness() > 0.62f ? Colour (0xff1e1c19) : Colours::white;
        draw::button (g, r, p2, shape == 0 ? rad * 0.6f : rad, press, false, hover);
        inner = r.withTrimmedBottom (2.5f).translated (0.0f, 2.5f * press);
        textCol = p2.btnText;
        dimCol = p2.btnText.withAlpha (0.6f);
        const auto led = Rectangle<float> (inner.getCentreX() - 9.0f * k, inner.getY() + 5.0f * k, 18.0f * k, 4.0f * k);
        g.setColour (Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (led, 2.0f * k);
        if (glow > 0.01f)
        {
            const auto ledCol = Colour (0xffff3b1f).interpolatedWith (Colours::white, 0.15f);
            g.setColour (ledCol.withAlpha (glow));
            g.fillRoundedRectangle (led, 2.0f * k);
            ColourGradient halo (ledCol.withAlpha (0.5f * glow), led.getCentreX(), led.getCentreY(),
                                 ledCol.withAlpha (0.0f), led.getCentreX() + 18.0f * k, led.getCentreY(), true);
            g.setGradientFill (halo);
            g.fillEllipse (led.expanded (14.0f * k, 10.0f * k));
            g.setColour (Colours::white.withAlpha (0.18f * glow));
            g.fillRoundedRectangle (inner, shape == 0 ? rad * 0.6f : rad);
        }
    }
    else if (skin == 1)   // Pastilla
    {
        auto face = r.reduced (1.0f).translated (0.0f, press * 1.5f);
        const float pr = shape == 1 ? 3.0f * k : (shape == 0 ? jmin (face.getWidth(), face.getHeight()) * 0.5f : rad);
        g.setColour (pal.btnShadow.withMultipliedAlpha (0.5f));
        g.fillRoundedRectangle (face.translated (0.0f, 2.0f), pr);
        g.setColour (pal.btn.interpolatedWith (light, glow));
        g.fillRoundedRectangle (face, pr);
        g.setColour ((glow > 0.5f ? light.darker (0.3f) : pal.btnEdge).withAlpha (0.8f));
        g.drawRoundedRectangle (face.reduced (0.5f), pr, 1.0f);
        if (hover > 0.0f) { g.setColour (pal.text.withAlpha (0.05f * hover)); g.fillRoundedRectangle (face, pr); }
        inner = face;
        textCol = pal.btnText.interpolatedWith (lightInk, glow);
        dimCol = textCol.withAlpha (0.55f);
    }
    else if (skin == 2)   // Plano: celdas pegadas con rayitas
    {
        auto face = r.expanded (4.0f * k, 0.0f).withTrimmedLeft (index == 0 ? 4.0f * k : 0.0f).withTrimmedRight (index == 6 ? 4.0f * k : 0.0f);
        if (shape != 0) face = r;   // con otra forma, cada celda va suelta
        const float fr = shape == 0 ? 0.0f : rad;
        g.setColour (pal.plate.interpolatedWith (light, glow * 0.9f));
        g.fillRoundedRectangle (face, fr);
        if (hover > 0.0f) { g.setColour (pal.text.withAlpha (0.06f * hover)); g.fillRoundedRectangle (face, fr); }
        if (press > 0.0f) { g.setColour (Colours::black.withAlpha (0.08f * press)); g.fillRoundedRectangle (face, fr); }
        g.setColour (pal.plateLine.withMultipliedAlpha (2.0f));
        if (index > 0 && shape == 0) g.fillRect (face.getX(), face.getY() + 10.0f * k, 1.0f, face.getHeight() - 20.0f * k);
        g.setColour (light.withAlpha (0.9f * glow));
        if (shape == 0) g.fillRect (face.removeFromBottom (3.0f * k));
        else g.fillRect (face.withSizeKeepingCentre (face.getWidth() * 0.4f, 3.0f * k).withY (face.getBottom() - 8.0f * k));
        inner = r;
        textCol = pal.text.interpolatedWith (lightInk, glow);
        dimCol = textCol.withAlpha (0.55f);
    }
    else if (skin == 3)   // Contorno
    {
        auto face = r.reduced (1.5f).translated (0.0f, press * 1.5f);
        if (glow > 0.01f)
        {
            g.setColour (light.withAlpha (0.14f * glow));
            g.fillRoundedRectangle (face, rad);
            g.setColour (light.withAlpha (0.25f * glow));
            g.drawRoundedRectangle (face.expanded (3.0f), rad + 2.0f, 3.0f);
        }
        if (hover > 0.0f) { g.setColour (pal.text.withAlpha (0.05f * hover)); g.fillRoundedRectangle (face, rad); }
        g.setColour (pal.text.withAlpha (0.35f).interpolatedWith (light, glow));
        g.drawRoundedRectangle (face, rad, 1.6f * jmax (1.0f, k));
        inner = face;
        textCol = pal.text.interpolatedWith (light, glow);
        dimCol = pal.text.withAlpha (0.5f).interpolatedWith (light, glow);
    }
    else if (skin == 4)   // Pantallitas
    {
        sl = screenLook (pal);
        Skin p2 = pal;
        p2.screen = sl.bg;
        auto face = r.translated (0.0f, press * 1.5f);
        draw::recess (g, face, pal, rad, pal.body2.darker (pal.night ? 0.2f : 0.04f));
        auto scr = face.reduced (4.0f * k);
        g.setColour (sl.bg.interpolatedWith (light, glow * 0.18f));
        const float sr = shape >= 2 ? jmax (2.0f, rad - 4.0f * k) : rad * 0.6f;
        g.fillRoundedRectangle (scr, sr);
        Path sp; sp.addRoundedRectangle (scr, sr);
        if (sl.texture != texNone) texture (g, sp, sl.texture, ! sl.light, 0.8f);
        if (hover > 0.0f) { g.setColour (sl.text.withAlpha (0.06f * hover)); g.fillRoundedRectangle (scr, sr); }
        inner = scr;
        textCol = sl.text.interpolatedWith (light, glow * 0.6f);
        dimCol = sl.dim;
        screenText = true;
        sym = Fonts::displayText (sl.font, symbol);
        symFont = ctx.fonts.display (sl.font, pal.pixelScreen, 21.0f * k);
        float h = 21.0f * k;
        while (h > 9.0f && draw::textWidth (symFont, sym) > inner.getWidth() - 6.0f)
        {
            h *= 0.92f;
            symFont = ctx.fonts.display (sl.font, pal.pixelScreen, h);
        }
    }

    if (pal.pieces.degLabels == 0)
    {
        // Solo el grado (I a VII), grande y al centro
        const Font rf = screenText ? ctx.fonts.display (sl.font, pal.pixelScreen, 24.0f * k) : ctx.fonts.ui (22.0f * k, 2);
        const auto rt = screenText ? Fonts::displayText (sl.font, roman) : roman;
        auto box = inner.withTrimmedBottom (2.0f * k);
        if (screenText && glowCache != nullptr && (sl.glow > 0.0f || glow > 0.01f))
            glowCache->draw (g, rt, rf, box, Justification::centred, textCol, jmin (1.0f, sl.glow + 0.7f * glow));
        g.setColour (textCol);
        g.setFont (rf);
        g.drawText (rt, box, Justification::centred, false);
        return;
    }
    g.setColour (dimCol);
    g.setFont (ctx.fonts.ui (15.5f * k, 1));
    g.drawText (roman, inner.withHeight (inner.getHeight() * 0.42f).translated (0.0f, 6.0f * k), Justification::centred, false);
    auto symBox = inner.withTrimmedTop (inner.getHeight() * 0.42f).withTrimmedBottom (6.0f * k);
    if (screenText && glowCache != nullptr && (sl.glow > 0.0f || glow > 0.01f))
        glowCache->draw (g, sym, symFont, symBox, Justification::centred, textCol, jmin (1.0f, sl.glow + 0.7f * glow));
    g.setColour (textCol);
    g.setFont (symFont);
    g.drawText (sym, symBox, Justification::centred, false);
}
}
