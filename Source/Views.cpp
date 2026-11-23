#include "Views.h"

using namespace juce;

//==============================================================================
namespace draw
{
    float textWidth (const Font& f, const String& s) { return GlyphArrangement::getStringWidth (f, s); }

    Font fit (const Fonts& fonts, FontKind kind, const String& text, float maxH, float maxW, bool display)
    {
        float h = maxH;
        auto make = [&] (float hh) { return display ? fonts.display (kind, hh) : fonts.label (kind, hh, 1); };
        auto f = make (h);
        while (h > 8.0f && textWidth (f, text) > maxW)
        {
            h *= 0.92f;
            f = make (h);
        }
        return f;
    }

    Path panelPath (Rectangle<float> r, const Palette& pal, float k, float radiusScale)
    {
        Path p;
        if (pal.chamfer)
        {
            const float c = jmin (12.0f * k, r.getHeight() * 0.3f, r.getWidth() * 0.3f);
            p.startNewSubPath (r.getX() + c, r.getY());
            p.lineTo (r.getRight(), r.getY());
            p.lineTo (r.getRight(), r.getBottom() - c);
            p.lineTo (r.getRight() - c, r.getBottom());
            p.lineTo (r.getX(), r.getBottom());
            p.lineTo (r.getX(), r.getY() + c);
            p.closeSubPath();
        }
        else if (pal.pixel)
        {
            const float s = jmax (2.0f, 3.0f * k);
            p.startNewSubPath (r.getX() + s, r.getY());
            p.lineTo (r.getRight() - s, r.getY());
            p.lineTo (r.getRight() - s, r.getY() + s);
            p.lineTo (r.getRight(), r.getY() + s);
            p.lineTo (r.getRight(), r.getBottom() - s);
            p.lineTo (r.getRight() - s, r.getBottom() - s);
            p.lineTo (r.getRight() - s, r.getBottom());
            p.lineTo (r.getX() + s, r.getBottom());
            p.lineTo (r.getX() + s, r.getBottom() - s);
            p.lineTo (r.getX(), r.getBottom() - s);
            p.lineTo (r.getX(), r.getY() + s);
            p.lineTo (r.getX() + s, r.getY() + s);
            p.closeSubPath();
        }
        else
        {
            p.addRoundedRectangle (r, jmin (pal.radius * k * radiusScale, r.getHeight() * 0.5f, r.getWidth() * 0.5f));
        }
        return p;
    }

    void panel (Graphics& g, Rectangle<float> r, Colour fill, Colour border, const Palette& pal, float k, float radiusScale)
    {
        auto p = panelPath (r, pal, k, radiusScale);
        g.setColour (fill);
        g.fillPath (p);
        g.setColour (border);
        g.strokePath (p, PathStrokeType (pal.pixel ? jmax (2.0f, 2.0f * k) : 1.0f));
        if (pal.modern && r.getHeight() > 16.0f && r.getWidth() > 24.0f)
        {
            const float rad = jmin (pal.radius * k * radiusScale, r.getHeight() * 0.5f);
            g.setColour (Colours::white.withAlpha (pal.dark ? 0.07f : 0.8f));
            g.fillRect (r.getX() + rad, r.getY() + 1.0f, r.getWidth() - rad * 2.0f, 1.0f);
        }
        if (pal.ornate && r.getHeight() > 30.0f)
        {
            auto inner = panelPath (r.reduced (4.0f * k), pal, k, radiusScale * 0.6f);
            g.setColour (border.withMultipliedAlpha (0.8f));
            g.strokePath (inner, PathStrokeType (0.8f));
        }
    }

    void texture (Graphics& g, const Image& img, Rectangle<float> r, float alpha)
    {
        if (! img.isValid()) return;
        g.setOpacity (alpha);
        g.drawImage (img, r, RectanglePlacement::fillDestination);
        g.setOpacity (1.0f);
    }

    void body (Graphics& g, Rectangle<float> r, const Palette& pal, Textures& tex)
    {
        ColourGradient base (pal.bodyColour, r.getX(), r.getY(), pal.bodyColour2, r.getX(), r.getBottom(), false);
        g.setGradientFill (base);
        g.fillRect (r);
        if (pal.body != Tex::none)
            texture (g, tex.get (pal.body), r);

        if (pal.lacquer)
        {
            ColourGradient top (Colours::white.withAlpha (0.07f), r.getX(), r.getY(), Colours::white.withAlpha (0.0f), r.getX(), r.getY() + r.getHeight() * 0.25f, false);
            g.setGradientFill (top);
            g.fillRect (r);
            ColourGradient band (Colours::white.withAlpha (0.0f), r.getX(), r.getY(), Colours::white.withAlpha (0.0f), r.getRight(), r.getY(), false);
            band.addColour (0.48, Colours::white.withAlpha (0.035f));
            g.setGradientFill (band);
            g.fillRect (r);
        }
        if (pal.modern)
        {
            static Image dotsDark, dotsLight;
            auto& tile = pal.dark ? dotsDark : dotsLight;
            if (! tile.isValid())
            {
                tile = Image (Image::ARGB, 14, 14, true);
                Graphics tg (tile);
                tg.setColour (pal.dark ? Colours::white.withAlpha (0.05f) : Colours::black.withAlpha (0.06f));
                tg.fillRect (6, 6, 2, 2);
            }
            g.setTiledImageFill (tile, roundToInt (r.getX()), roundToInt (r.getY()), 1.0f);
            g.fillRect (r);
            ColourGradient sheen (Colours::white.withAlpha (pal.dark ? 0.035f : 0.25f), r.getX(), r.getY(),
                                  Colours::white.withAlpha (0.0f), r.getX(), r.getY() + r.getHeight() * 0.35f, false);
            g.setGradientFill (sheen);
            g.fillRect (r);
        }
        if (pal.pixel)
        {
            g.setColour (Colours::white.withAlpha (0.035f));
            for (float y = r.getY(); y < r.getBottom(); y += 8.0f)
                for (float x = r.getX() + (float) (((int) (y / 8.0f)) % 2) * 4.0f; x < r.getRight(); x += 8.0f)
                    g.fillRect (x, y, 2.0f, 2.0f);
        }
    }

    Path chassisPath (Rectangle<float> r, const Palette& pal, float k)
    {
        const float R = jlimit (8.0f, 30.0f, 16.0f * k);
        Path p;
        if (pal.pixel)
        {
            // Esquinas redondas "pixeladas", en escalones
            const float st = jmax (3.0f, 4.0f * k);
            const int n = 4;
            p.startNewSubPath (r.getX() + st * n, r.getY());
            p.lineTo (r.getRight() - st * n, r.getY());
            for (int i = n - 1; i >= 0; --i) { p.lineTo (r.getRight() - st * (float) i, r.getY() + st * (float) (n - 1 - i)); p.lineTo (r.getRight() - st * (float) i, r.getY() + st * (float) (n - i)); }
            p.lineTo (r.getRight(), r.getBottom() - st * n);
            for (int i = 1; i <= n; ++i) { p.lineTo (r.getRight() - st * (float) (i - 1), r.getBottom() - st * (float) (n - i)); p.lineTo (r.getRight() - st * (float) i, r.getBottom() - st * (float) (n - i)); }
            p.lineTo (r.getX() + st * n, r.getBottom());
            for (int i = n - 1; i >= 0; --i) { p.lineTo (r.getX() + st * (float) i, r.getBottom() - st * (float) (n - 1 - i)); p.lineTo (r.getX() + st * (float) i, r.getBottom() - st * (float) (n - i)); }
            p.lineTo (r.getX(), r.getY() + st * n);
            for (int i = 1; i <= n; ++i) { p.lineTo (r.getX() + st * (float) (i - 1), r.getY() + st * (float) (n - i)); p.lineTo (r.getX() + st * (float) i, r.getY() + st * (float) (n - i)); }
            p.closeSubPath();
            return p;
        }
        p.addRoundedRectangle (r, R);
        return p;
    }

    void chassis (Graphics& g, Rectangle<float> r, const Palette& pal, float k, Textures& tex, bool backdrop)
    {
        if (backdrop)
        {
            g.setColour (Colour (0xff0b0b0d));
            g.fillRect (r);
        }
        auto outer = r.reduced (0.5f);
        const auto path = chassisPath (outer, pal, k);
        {
            Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (path);
            body (g, r, pal, tex);
            themeScene (g, r, pal);

            // Sombra interior suave en todo el borde: da profundidad al marco
            if (! pal.pixel)
            {
                const float depth = (pal.modern ? 5.0f : 9.0f) * k;
                const int steps = 6;
                for (int i = 0; i < steps; ++i)
                {
                    const float t = (float) i / (float) steps;
                    auto rr = outer.reduced (depth * t);
                    g.setColour (Colours::black.withAlpha ((pal.dark ? 0.10f : 0.045f) * (1.0f - t)));
                    g.strokePath (chassisPath (rr, pal, k * (1.0f - t * 0.4f)), PathStrokeType (depth / (float) steps + 0.6f));
                }
            }
        }

        // Bisel: filo oscuro por fuera y luz por dentro (más brillante arriba)
        g.setColour (Colours::black.withAlpha (pal.dark ? 0.75f : 0.35f));
        g.strokePath (path, PathStrokeType (pal.pixel ? jmax (2.0f, 2.0f * k) : 1.0f));
        if (! pal.pixel)
        {
            auto inner = outer.reduced (1.2f);
            ColourGradient hl (Colours::white.withAlpha (pal.dark ? (pal.modern ? 0.13f : 0.17f) : 0.75f), 0.0f, inner.getY(),
                               Colours::white.withAlpha (pal.dark ? 0.02f : 0.15f), 0.0f, inner.getBottom(), false);
            g.setGradientFill (hl);
            g.strokePath (chassisPath (inner, pal, k * 0.94f), PathStrokeType (1.0f));
        }

        // Detalles por estilo
        if (pal.lacquer || pal.ornate)
        {
            auto rim = outer.reduced (6.0f * k);
            g.setColour (pal.accent.withAlpha (pal.lacquer ? 0.38f : 0.32f));
            g.strokePath (chassisPath (rim, pal, k * 0.65f), PathStrokeType (pal.lacquer ? 0.9f : 0.8f));
            if (pal.ornate)
                g.strokePath (chassisPath (rim.reduced (3.0f * k), pal, k * 0.5f), PathStrokeType (0.6f));
        }
        else if (pal.modern)
        {
            g.setColour ((pal.dark ? Colours::white : Colours::black).withAlpha (0.06f));
            g.strokePath (chassisPath (outer.reduced (5.0f * k), pal, k * 0.7f), PathStrokeType (1.0f));
        }
    }

    void shadowText (Graphics& g, const String& text, Rectangle<float> r, Justification j, Colour c, bool shadow)
    {
        if (shadow)
        {
            g.setColour (Colours::black.withAlpha (0.6f));
            g.drawText (text, r.translated (0.0f, 1.5f), j, false);
            g.drawText (text, r.translated (1.0f, 0.5f), j, false);
        }
        g.setColour (c);
        g.drawText (text, r, j, false);
    }

    void icon (Graphics& g, const String& name, Rectangle<float> r, Colour c, float stroke)
    {
        const float s = jmin (r.getWidth(), r.getHeight());
        const auto sq = r.withSizeKeepingCentre (s, s);
        auto P = [&] (float x, float y) { return Point<float> (sq.getX() + x / 24.0f * s, sq.getY() + y / 24.0f * s); };
        Path p;
        g.setColour (c);
        const PathStrokeType st (stroke, PathStrokeType::curved, PathStrokeType::rounded);

        if (name == "sun")
        {
            const auto cc = P (12, 12);
            p.addEllipse (cc.x - s * 0.17f, cc.y - s * 0.17f, s * 0.34f, s * 0.34f);
            for (int i = 0; i < 8; ++i)
            {
                const float a = MathConstants<float>::pi * 0.25f * (float) i;
                p.startNewSubPath (cc + Point<float> (std::cos (a), std::sin (a)) * (s * 0.30f));
                p.lineTo (cc + Point<float> (std::cos (a), std::sin (a)) * (s * 0.40f));
            }
        }
        else if (name == "moon")
        {
            // Luna creciente: un círculo al que se le recorta otro
            const auto cc = P (12, 12);
            const float R = s * 0.33f;
            Path outerC, both;
            outerC.addEllipse (cc.x - R, cc.y - R, R * 2.0f, R * 2.0f);
            both.addEllipse (cc.x - R, cc.y - R, R * 2.0f, R * 2.0f);
            both.addEllipse (cc.x - R * 0.35f, cc.y - R * 1.25f, R * 1.9f, R * 1.9f);
            both.setUsingNonZeroWinding (false);
            Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (outerC);
            g.fillPath (both);
            return;
        }
        else if (name == "theme")
        {
            p.startNewSubPath (P (2.5f, 19)); p.lineTo (P (9, 9)); p.lineTo (P (13, 14.5f)); p.lineTo (P (15.5f, 11.5f)); p.lineTo (P (21.5f, 19));
            p.closeSubPath();
            const auto sc = P (17, 6);
            p.addEllipse (sc.x - s * 0.09f, sc.y - s * 0.09f, s * 0.18f, s * 0.18f);
        }
        else if (name == "restore")
        {
            const auto cc = P (12, 12.5f);
            p.addCentredArc (cc.x, cc.y, s * 0.32f, s * 0.32f, 0.0f, -2.4f, 2.6f, true);
            const auto a = cc + Point<float> (std::sin (-2.4f), -std::cos (-2.4f)) * (s * 0.32f);
            p.startNewSubPath (a + Point<float> (s * 0.17f, 0.0f));
            p.lineTo (a);
            p.lineTo (a + Point<float> (0.0f, -s * 0.17f));
        }
        else if (name == "palette")
        {
            const auto cc = P (12, 12);
            p.addEllipse (cc.x - s * 0.36f, cc.y - s * 0.36f, s * 0.72f, s * 0.72f);
            g.strokePath (p, st);
            const Point<float> dots[4] = { P (8.5f, 9), P (12.5f, 7.5f), P (16, 10.5f), P (15, 15) };
            for (auto d : dots) g.fillEllipse (d.x - s * 0.06f, d.y - s * 0.06f, s * 0.12f, s * 0.12f);
            return;
        }
        else if (name == "transp")
        {
            p.addRoundedRectangle (Rectangle<float> (P (3.5f, 3.5f), P (20.5f, 20.5f)), s * 0.08f);
            g.strokePath (p, st);
            const float cell = s * (17.0f / 24.0f) / 3.0f;
            for (int yy = 0; yy < 3; ++yy)
                for (int xx = 0; xx < 3; ++xx)
                    if ((xx + yy) % 2 == 0)
                    {
                        const auto o = P (3.5f, 3.5f);
                        g.fillRect (Rectangle<float> (o.x + (float) xx * cell, o.y + (float) yy * cell, cell, cell).reduced (cell * 0.18f));
                    }
            return;
        }
        else if (name == "split")
        {
            p.addRoundedRectangle (Rectangle<float> (P (3, 4), P (21, 11)), s * 0.06f);
            p.addRoundedRectangle (Rectangle<float> (P (3, 15), P (21, 20)), s * 0.05f);
            for (float x : { 8.0f, 12.0f, 16.0f })
            {
                p.startNewSubPath (P (x, 12.6f));
                p.lineTo (P (x, 13.6f));
            }
        }
        else if (name == "windows")
        {
            p.addRoundedRectangle (Rectangle<float> (P (3, 3), P (14, 12)), s * 0.06f);
            p.addRoundedRectangle (Rectangle<float> (P (10, 12), P (21, 21)), s * 0.06f);
        }
        else if (name == "eyeoff" || name == "eye")
        {
            p.startNewSubPath (P (3, 12));
            p.quadraticTo (P (12, 3.5f), P (21, 12));
            p.quadraticTo (P (12, 20.5f), P (3, 12));
            const auto cc = P (12, 12);
            p.addEllipse (cc.x - s * 0.11f, cc.y - s * 0.11f, s * 0.22f, s * 0.22f);
            if (name == "eyeoff") { p.startNewSubPath (P (4, 4)); p.lineTo (P (20, 20)); }
        }
        else if (name == "menu")
        {
            for (float y : { 7.0f, 12.0f, 17.0f }) { p.startNewSubPath (P (5, y)); p.lineTo (P (19, y)); }
        }
        else if (name == "dock")
        {
            p.addRoundedRectangle (Rectangle<float> (P (3, 3), P (21, 21)), s * 0.08f);
            p.startNewSubPath (P (16, 8)); p.lineTo (P (9, 15));
            p.startNewSubPath (P (9, 10)); p.lineTo (P (9, 15)); p.lineTo (P (14, 15));
        }
        else if (name == "reset")
        {
            p.addRoundedRectangle (Rectangle<float> (P (6, 7), P (18, 17)), s * 0.05f);
            p.startNewSubPath (P (3, 6)); p.lineTo (P (3, 3)); p.lineTo (P (6, 3));
            p.startNewSubPath (P (21, 18)); p.lineTo (P (21, 21)); p.lineTo (P (18, 21));
        }
        else if (name == "pin")
        {
            p.startNewSubPath (P (9, 3)); p.lineTo (P (15, 3));
            p.startNewSubPath (P (10, 3)); p.lineTo (P (10, 10)); p.lineTo (P (7, 14)); p.lineTo (P (17, 14)); p.lineTo (P (14, 10)); p.lineTo (P (14, 3));
            p.startNewSubPath (P (12, 14)); p.lineTo (P (12, 21));
        }
        else if (name == "close")
        {
            p.startNewSubPath (P (6, 6)); p.lineTo (P (18, 18));
            p.startNewSubPath (P (18, 6)); p.lineTo (P (6, 18));
        }
        else if (name == "chev")
        {
            p.startNewSubPath (P (7, 10)); p.lineTo (P (12, 15)); p.lineTo (P (17, 10));
        }
        else if (name == "grip")
        {
            for (float x : { 9.0f, 15.0f })
                for (float y : { 7.0f, 12.0f, 17.0f })
                {
                    const auto d = P (x, y);
                    g.fillEllipse (d.x - s * 0.05f, d.y - s * 0.05f, s * 0.1f, s * 0.1f);
                }
            return;
        }
        g.strokePath (p, st);
    }
}

//==============================================================================
// Controles
//==============================================================================
void Switch::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat();
    const float h = jmin (b.getHeight(), b.getWidth() * 0.55f);
    auto track = b.withSizeKeepingCentre (h * 1.82f, h);
    const auto shape = draw::panelPath (track, pal, 1.0f, pal.pixel ? 1.0f : 100.0f);
    g.setColour (on ? pal.switchOn : pal.well.withMultipliedAlpha (1.6f));
    g.fillPath (shape);
    g.setColour (pal.wellBorder);
    g.strokePath (shape, PathStrokeType (1.0f));

    const float d = h - 4.0f;
    const float x = on ? track.getRight() - d - 2.0f : track.getX() + 2.0f;
    auto cap = Rectangle<float> (x, track.getCentreY() - d * 0.5f, d, d);
    g.setColour (Colours::black.withAlpha (0.35f));
    if (pal.pixel) g.fillRect (cap.translated (0.0f, 1.0f)); else g.fillEllipse (cap.translated (0.0f, 1.0f));
    g.setColour (pal.knobCap);
    if (pal.pixel) g.fillRect (cap); else g.fillEllipse (cap);
}

void Switch::mouseUp (const MouseEvent& e)
{
    if (! getLocalBounds().contains (e.getPosition())) return;
    on = ! on;
    repaint();
    if (onChange) onChange (on);
}

//------------------------------------------------------------------------------
void Knob::setValue (float normalised, String display)
{
    if (std::abs (normalised - value) > 1.0e-4f || display != valueText)
    {
        value = normalised;
        valueText = std::move (display);
        repaint();
    }
}

void Knob::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat();
    const float d = jmin (b.getWidth(), b.getHeight());
    auto kb = b.withSizeKeepingCentre (d, d);
    const auto c = kb.getCentre();
    const float r = d * 0.5f;
    const float a0 = -MathConstants<float>::pi * 0.75f, a1 = MathConstants<float>::pi * 0.75f;
    const float av = a0 + (a1 - a0) * value;

    if (pal.modern)
    {
        const float ar = r - 2.0f;
        const float thick = jmax (2.0f, d * 0.07f);
        Path track, val;
        track.addCentredArc (c.x, c.y, ar, ar, 0.0f, a0, a1, true);
        val.addCentredArc (c.x, c.y, ar, ar, 0.0f, a0, av, true);
        g.setColour ((pal.dark ? Colours::white : Colours::black).withAlpha (0.10f));
        g.strokePath (track, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (pal.accent);
        g.strokePath (val, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));
        const float ir = r * 0.62f;
        g.setColour (pal.knob);
        g.fillEllipse (c.x - ir, c.y - ir, ir * 2.0f, ir * 2.0f);
        g.setColour ((pal.dark ? Colours::white : Colours::black).withAlpha (0.10f));
        g.drawEllipse (c.x - ir, c.y - ir, ir * 2.0f, ir * 2.0f, 1.0f);
        const auto p0 = c + Point<float> (std::sin (av), -std::cos (av)) * (ir * 0.35f);
        const auto p1 = c + Point<float> (std::sin (av), -std::cos (av)) * (ir - 2.5f);
        g.setColour (pal.knobLine);
        g.drawLine ({ p0, p1 }, 2.2f);
        return;
    }

    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 10.0f;
        const bool lit = (float) i / 10.0f <= value + 0.001f;
        const auto p0 = c + Point<float> (std::sin (a), -std::cos (a)) * (r - 0.5f);
        const auto p1 = c + Point<float> (std::sin (a), -std::cos (a)) * (r - r * 0.16f);
        g.setColour (lit ? pal.accent : pal.dim.withAlpha (0.35f));
        g.drawLine ({ p0, p1 }, 1.3f);
    }

    const float ir = r * 0.68f;
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillEllipse (c.x - ir, c.y - ir + 1.5f, ir * 2.0f, ir * 2.0f);
    ColourGradient bodyGrad (pal.knob.brighter (0.25f), c.x, c.y - ir, pal.knob.darker (0.35f), c.x, c.y + ir, false);
    g.setGradientFill (bodyGrad);
    if (pal.pixel) g.fillRect (c.x - ir, c.y - ir, ir * 2.0f, ir * 2.0f);
    else           g.fillEllipse (c.x - ir, c.y - ir, ir * 2.0f, ir * 2.0f);

    const auto p0 = c + Point<float> (std::sin (av), -std::cos (av)) * (ir * 0.2f);
    const auto p1 = c + Point<float> (std::sin (av), -std::cos (av)) * (ir - 2.0f);
    g.setColour (pal.knobLine);
    g.drawLine ({ p0, p1 }, 2.0f);
}

void Knob::mouseDown (const MouseEvent&)        { dragStart = value; if (onGestureStart) onGestureStart(); }
void Knob::mouseUp (const MouseEvent&)          { if (onGestureEnd) onGestureEnd(); }
void Knob::mouseDoubleClick (const MouseEvent&) { if (onReset) onReset(); }

void Knob::mouseDrag (const MouseEvent& e)
{
    const float sens = e.mods.isShiftDown() ? 800.0f : 220.0f;
    if (onValue) onValue (jlimit (0.0f, 1.0f, dragStart - (float) e.getDistanceFromDragStartY() / sens));
}

void Knob::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    if (onGestureStart) onGestureStart();
    if (onValue) onValue (jlimit (0.0f, 1.0f, value + w.deltaY * 0.12f));
    if (onGestureEnd) onGestureEnd();
}

//------------------------------------------------------------------------------
void Segmented::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    draw::panel (g, b, pal.well, pal.wellBorder, pal, 0.5f);
    const float w = b.getWidth() / (float) options.size();
    for (int i = 0; i < options.size(); ++i)
    {
        auto r = Rectangle<float> (b.getX() + (float) i * w, b.getY(), w, b.getHeight()).reduced (2.0f);
        if (i == selected)
        {
            g.setColour (pal.accent);
            g.fillPath (draw::panelPath (r, pal, 0.4f));
        }
        g.setColour (i == selected ? pal.accentInk : pal.dim);
        g.setFont (ctx.fonts.label (pal.uiFont, jmin (13.0f, r.getHeight() * 0.62f), 2));
        g.drawText (options[i], r, Justification::centred, false);
    }
}

void Segmented::mouseUp (const MouseEvent& e)
{
    auto b = getLocalBounds().toFloat();
    if (! b.contains (e.position)) return;
    const int i = jlimit (0, options.size() - 1, (int) (e.position.x / (b.getWidth() / (float) options.size())));
    if (i != selected) { selected = i; repaint(); if (onChange) onChange (i); }
}

//------------------------------------------------------------------------------
void IconButton::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    if (framed)
    {
        draw::panel (g, b, active ? pal.accent.withAlpha (0.28f) : pal.well, active ? pal.accent : pal.wellBorder, pal, 0.6f);
        if (hover) { g.setColour (pal.text.withAlpha (0.07f)); g.fillPath (draw::panelPath (b, pal, 0.6f)); }
    }
    else if (hover)
    {
        g.setColour ((lightOnDark ? Colours::white : pal.text).withAlpha (0.14f));
        g.fillEllipse (b);
    }
    const auto col = lightOnDark ? Colours::white : (active && label.isNotEmpty() ? pal.text : pal.text);
    if (label.isNotEmpty() && b.getWidth() > b.getHeight() * 2.2f)
    {
        auto ir = b.removeFromLeft (b.getHeight());
        draw::icon (g, iconId, ir.reduced (ir.getWidth() * 0.24f).translated (b.getHeight() * 0.06f, 0.0f), col, 1.6f);
        g.setColour (col);
        g.setFont (draw::fit (ctx.fonts, pal.uiFont, label, jmin (14.0f, b.getHeight() * 0.54f), b.getWidth() - 6.0f, false));
        g.drawText (label, b.withTrimmedRight (6.0f), Justification::centredLeft, false);
        return;
    }
    draw::icon (g, iconId, b.reduced (b.getWidth() * 0.22f), col, 1.6f);
}

void IconButton::mouseUp (const MouseEvent& e)
{
    if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
}

//------------------------------------------------------------------------------
void FieldButton::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    draw::panel (g, b, pal.well, hover ? pal.accent.withAlpha (0.7f) : pal.wellBorder, pal, 0.7f);

    auto inner = b.reduced (10.0f * k, 4.0f * k);
    auto chev = inner.removeFromRight (12.0f * k);
    draw::icon (g, "chev", chev.withSizeKeepingCentre (14.0f * k, 14.0f * k), pal.dim, 1.8f);

    auto labelRow = inner.removeFromTop (inner.getHeight() * 0.42f);
    if (led >= 0)
    {
        auto lr = inner.removeFromLeft (14.0f * k);
        const auto c = Point<float> (lr.getX() + 4.0f * k, inner.getCentreY());
        const auto col = led == 2 ? pal.accent : (led == 1 ? Colour (0xff5fcf7e) : pal.dim.withAlpha (0.4f));
        if (led > 0)
        {
            ColourGradient glow (col.withAlpha (0.55f), c.x, c.y, col.withAlpha (0.0f), c.x + 9.0f * k, c.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (c.x - 9.0f * k, c.y - 9.0f * k, 18.0f * k, 18.0f * k);
        }
        g.setColour (col);
        g.fillEllipse (c.x - 4.0f * k, c.y - 4.0f * k, 8.0f * k, 8.0f * k);
    }

    g.setColour (pal.dim);
    g.setFont (ctx.fonts.label (pal.uiFont, 10.5f * k, 1));
    g.drawText (text, labelRow, Justification::bottomLeft, false);
    g.setColour (pal.text);
    g.setFont (draw::fit (ctx.fonts, pal.uiFont, value, 14.0f * k, inner.getWidth(), false));
    g.drawText (value, inner, Justification::centredLeft, true);
}

void FieldButton::mouseUp (const MouseEvent& e)
{
    if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
}

//==============================================================================
// Barra de arriba
//==============================================================================
RailBar::RailBar (UiContext& c)
    : midi (c, "MIDI"), key (c, "Tonalidad"), scale (c, "Escala"), latency (c, "Latencia"),
      dayBtn (c, "sun", U8 ("Modo día / noche")),
      themeBtn (c, "theme", U8 ("Temas de luz")),
      skinBtn (c, "palette", "Skin"),
      transpBtn (c, "transp", "Modo transparente"),
      splitBtn (c, "split", "Separar o unir el teclado"),
      windowsBtn (c, "windows", "Separar secciones en ventanas"),
      resetBtn (c, "restore", U8 ("Regresar todo a su lugar")),
      hideBtn (c, "eyeoff", U8 ("Ocultar parámetros")),
      menuBtn (c, "menu", U8 ("Menú")),
      ctx (c)
{
    for (Component* comp : std::initializer_list<Component*> { &midi, &key, &scale, &latency, &dayBtn, &themeBtn, &skinBtn, &transpBtn, &splitBtn, &windowsBtn, &resetBtn, &hideBtn, &menuBtn })
        addAndMakeVisible (comp);
    resetBtn.label = "Restablecer";
    resetBtn.setActive (true);
    midi.setLed (0);
}

void RailBar::setStatus (const String& s, double seconds)
{
    status = s;
    statusUntil = Time::getMillisecondCounterHiRes() + seconds * 1000.0;
    repaint();
}

void RailBar::tick (double nowMs)
{
    if (status.isNotEmpty() && nowMs > statusUntil)
    {
        status.clear();
        repaint();
    }
}

void RailBar::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat();
    draw::panel (g, b.reduced (0.5f), pal.rail, pal.railBorder, pal, k);

    // Marca: el conector MIDI con los pines unidos como constelación, y el nombre
    auto logo = b.reduced (14.0f * k, 0.0f).withWidth (176.0f * k);
    {
        const float s = 30.0f * k;
        const auto c = Point<float> (logo.getX() + s * 0.5f, b.getCentreY());
        const float R = s * 0.43f;
        g.setColour (pal.text);
        g.drawEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f, 1.5f);
        g.fillRect (c.x - s * 0.08f, c.y - R - s * 0.06f, s * 0.16f, s * 0.12f);
        Point<float> pins[5];
        const float angles[5] = { 90.0f, 135.0f, 180.0f, 225.0f, 270.0f };
        for (int i = 0; i < 5; ++i)
        {
            const float a = degreesToRadians (angles[i] - 90.0f);
            pins[i] = c + Point<float> (std::cos (a), std::sin (a)) * (R * 0.6f);
            pins[i].y += s * 0.03f;
        }
        Path line;
        line.startNewSubPath (pins[0]);
        for (int i = 1; i < 5; ++i) line.lineTo (pins[i]);
        g.setColour (pal.light.withAlpha (0.8f));
        g.strokePath (line, PathStrokeType (1.1f));
        for (int i = 0; i < 5; ++i)
        {
            const float rr = (i == 2 ? 2.3f : 1.7f) * k;
            g.setColour (i == 2 ? pal.light : pal.text);
            g.fillEllipse (pins[i].x - rr, pins[i].y - rr, rr * 2.0f, rr * 2.0f);
        }
        g.setColour (pal.text);
        g.setFont (pal.displayFont == FontKind::pixel ? ctx.fonts.pixel (19.0f * k)
                   : (pal.displayFont == FontKind::sans ? ctx.fonts.ui (22.0f * k, 2) : ctx.fonts.serif (24.0f * k, true)));
        g.drawText ("MidiSky", logo.withTrimmedLeft (s + 10.0f * k), Justification::centredLeft, false);
    }
}

void RailBar::resized()
{
    auto b = getLocalBounds().reduced (roundToInt (10.0f * k), roundToInt (8.0f * k));
    const int gap = roundToInt (8.0f * k);
    const int ib = b.getHeight();
    b.removeFromLeft (roundToInt (176.0f * k));
    b.removeFromLeft (gap);

    auto place  = [&] (Component& c, int w) { c.setBounds (b.removeFromLeft (w)); b.removeFromLeft (gap); };
    auto placeR = [&] (Component& c, int w) { c.setBounds (b.removeFromRight (w)); b.removeFromRight (gap); };

    for (auto* f : { &midi, &key, &scale, &latency }) f->k = k;

    placeR (menuBtn, ib);
    placeR (hideBtn, ib);
    resetBtn.setVisible (showReset);
    if (showReset) placeR (resetBtn, roundToInt (136.0f * k));

    const bool show = ! ctx.vis.hideParams;
    for (auto* c : std::initializer_list<Component*> { &midi, &key, &scale, &latency, &dayBtn, &themeBtn, &skinBtn, &transpBtn, &splitBtn, &windowsBtn })
        c->setVisible (show);

    if (show)
    {
        const int sib = roundToInt ((float) ib * 0.92f);
        placeR (windowsBtn, sib);
        placeR (splitBtn, sib);
        placeR (transpBtn, sib);
        placeR (skinBtn, sib);
        placeR (themeBtn, sib);
        placeR (dayBtn, sib);

        const int avail = b.getWidth();
        const float want = (220.0f + 140.0f + 160.0f + 110.0f) * k + 4.0f * (float) gap;
        const float f = jlimit (0.3f, 1.0f, (float) avail / want);
        place (midi, roundToInt (220.0f * k * f));
        place (key, roundToInt (140.0f * k * f));
        place (scale, roundToInt (160.0f * k * f));
        place (latency, roundToInt (110.0f * k * f));
    }
    statusArea = b.toFloat();
}

//==============================================================================
// Aviso corto
//==============================================================================
void Toast::show (const String& t, double seconds)
{
    text = t;
    until = Time::getMillisecondCounterHiRes() + seconds * 1000.0;
    setVisible (true);
    toFront (false);
    repaint();
}

void Toast::tick (double nowMs)
{
    if (isVisible() && nowMs > until) setVisible (false);
}

void Toast::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto f = ctx.fonts.label (pal.uiFont, 13.5f, 1);
    const float w = jmin ((float) getWidth(), draw::textWidth (f, text) + 40.0f);
    auto r = getLocalBounds().toFloat().withSizeKeepingCentre (w, (float) getHeight()).reduced (1.0f);
    const auto bg = pal.dark ? Colour (0xff17171a) : Colour (0xfff8f8f8);
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), r.getHeight() * 0.5f);
    g.setColour (bg.withAlpha (0.96f));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (pal.accent.withAlpha (0.7f));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    g.setColour (pal.dark ? Colour (0xffefeff2) : Colour (0xff17171a));
    g.setFont (f);
    g.drawText (text, r, Justification::centred, true);
}

//==============================================================================
// Selector de skins
//==============================================================================
static int firstModernSkin()
{
    const auto& l = skins();
    for (int i = 0; i < (int) l.size(); ++i) if (l[(size_t) i].modern) return i;
    return (int) l.size();
}

SkinPicker::SkinPicker (UiContext& c, int cur) : ctx (c), current (cur)
{
    const int fm = firstModernSkin();
    const int rowsA = (fm + 3) / 4, rowsB = ((int) skins().size() - fm + 3) / 4;
    setSize (4 * 112 + 24, 40 + rowsA * 96 + (rowsB > 0 ? 30 + rowsB * 96 : 0) + 10);
}

Rectangle<float> SkinPicker::cell (int i) const
{
    const int fm = firstModernSkin();
    const int rowsA = (fm + 3) / 4;
    if (i < fm) return { 12.0f + (float) (i % 4) * 112.0f, 40.0f + (float) (i / 4) * 96.0f, 112.0f, 96.0f };
    const int j = i - fm;
    return { 12.0f + (float) (j % 4) * 112.0f, 40.0f + (float) rowsA * 96.0f + 30.0f + (float) (j / 4) * 96.0f, 112.0f, 96.0f };
}

void SkinPicker::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (15.0f, 2));
    g.drawText (U8 ("Skins japonesas y clásicas"), Rectangle<float> (16.0f, 10.0f, 300.0f, 22.0f), Justification::centredLeft, false);
    const int fm = firstModernSkin();
    if (fm < (int) skins().size())
        g.drawText (U8 ("Modernas (estilo sintetizador)"), Rectangle<float> (16.0f, cell (fm).getY() - 26.0f, 300.0f, 22.0f), Justification::centredLeft, false);

    const auto& list = skins();
    for (int i = 0; i < (int) list.size(); ++i)
    {
        const auto& s = list[(size_t) i];
        auto r = cell (i).reduced (5.0f);
        if (i == hover) { g.setColour (pal.text.withAlpha (0.07f)); g.fillRoundedRectangle (r, 6.0f); }

        auto sw = r.removeFromTop (r.getHeight() - 22.0f).reduced (8.0f, 4.0f);
        Palette sp;
        static_cast<Skin&> (sp) = s;
        {
            Graphics::ScopedSaveState ss (g);
            Path clip;
            clip.addRoundedRectangle (sw, 5.0f);
            g.reduceClipRegion (clip);
            draw::body (g, sw, sp, ctx.tex);
            auto railR = sw.reduced (5.0f, 0.0f).withHeight (9.0f).translated (0.0f, 5.0f);
            g.setColour (s.rail);
            g.fillRoundedRectangle (railR, jmin (3.0f, s.radius * 0.4f));
            g.setColour (s.accent);
            g.fillEllipse (railR.getRight() - 8.0f, railR.getY() + 2.5f, 4.0f, 4.0f);
            auto keyR = sw.withTrimmedTop (sw.getHeight() * 0.45f).reduced (sw.getWidth() * 0.12f, 0.0f);
            g.setColour (s.keyWhite);
            g.fillRect (keyR);
            const float kw = keyR.getWidth() / 5.0f;
            g.setColour (s.light);
            g.fillRect (keyR.getX() + kw * 2.0f + 1.0f, keyR.getY(), kw - 2.0f, keyR.getHeight());
            g.setColour (s.keyBlack);
            for (int bk = 1; bk < 5; ++bk)
                if (bk != 3) g.fillRect (keyR.getX() + kw * (float) bk - kw * 0.3f, keyR.getY(), kw * 0.6f, keyR.getHeight() * 0.6f);
            g.setColour (s.felt);
            g.fillRect (keyR.getX(), keyR.getY() - 3.0f, keyR.getWidth(), 3.0f);
        }
        g.setColour (i == current ? pal.accent : pal.text.withAlpha (0.2f));
        g.drawRoundedRectangle (sw, 5.0f, i == current ? 2.0f : 1.0f);

        g.setColour (i == current ? pal.text : pal.dim);
        g.setFont (ctx.fonts.ui (12.5f, 1));
        g.drawText (s.name, r, Justification::centred, false);
    }
}

void SkinPicker::mouseMove (const MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < (int) skins().size(); ++i)
        if (cell (i).contains (e.position)) h = i;
    if (h != hover) { hover = h; repaint(); }
}

void SkinPicker::mouseUp (const MouseEvent& e)
{
    for (int i = 0; i < (int) skins().size(); ++i)
        if (cell (i).contains (e.position))
        {
            current = i;
            repaint();
            if (onPick) onPick (i);
            return;
        }
}

//==============================================================================
// Selector de temas (con modo día / noche / automático)
//==============================================================================
ThemePicker::ThemePicker (UiContext& c, int cur, int dn)
    : ctx (c), mode (c, { U8 ("Día"), "Noche", U8 ("Auto") }), current (cur), modeSel (dn)
{
    addAndMakeVisible (mode);
    mode.setSelected (dn);
    mode.onChange = [this] (int i) { modeSel = i; repaint(); if (onMode) onMode (i); };
    const int rows = ((int) themes().size() + 3) / 4;
    setSize (4 * 152 + 24, 52 + rows * 112 + 40);
}

bool ThemePicker::showsNight() const
{
    if (modeSel == 1) return true;
    if (modeSel == 2) { const int h = Time::getCurrentTime().getHours(); return h >= 19 || h < 7; }
    return false;
}

void ThemePicker::resized()
{
    mode.setBounds (getWidth() - 12 - 230, 10, 230, 30);
}

Rectangle<float> ThemePicker::cell (int i) const
{
    return { 12.0f + (float) (i % 4) * 152.0f, 52.0f + (float) (i / 4) * 112.0f, 152.0f, 112.0f };
}

void ThemePicker::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (15.0f, 2));
    g.drawText ("Temas de luz", Rectangle<float> (16.0f, 14.0f, 240.0f, 22.0f), Justification::centredLeft, false);

    const bool night = showsNight();
    const auto& list = themes();
    Palette base = pal;
    static_cast<Skin&> (base) = skins()[(size_t) skinIndex (pal.id)];

    for (int i = 0; i < (int) list.size(); ++i)
    {
        auto r = cell (i).reduced (5.0f);
        if (i == hover) { g.setColour (pal.text.withAlpha (0.07f)); g.fillRoundedRectangle (r, 7.0f); }
        auto sw = r.removeFromTop (r.getHeight() - 22.0f).reduced (6.0f, 4.0f);

        Palette tp = base;
        tp.themeIdx = i;
        tp.night = night;
        if (pal.lightMode == 0 && tp.look().hasKeyLight) tp.light = tp.look().keyLight;
        {
            Graphics::ScopedSaveState ss (g);
            Path clip;
            clip.addRoundedRectangle (sw, 6.0f);
            g.reduceClipRegion (clip);
            draw::body (g, sw, tp, ctx.tex);
            draw::themeScene (g, sw, tp);
            // Maqueta pequeña de la interfaz
            auto railR = sw.reduced (6.0f, 0.0f).withHeight (8.0f).translated (0.0f, 5.0f);
            g.setColour (tp.rail); g.fillRoundedRectangle (railR, 2.0f);
            auto atrilR = Rectangle<float> (sw.getX() + 6.0f, railR.getBottom() + 4.0f, sw.getWidth() - 12.0f, sw.getHeight() * 0.36f);
            g.setColour (tp.atril); g.fillRoundedRectangle (atrilR, 2.0f);
            const float cr = atrilR.getHeight() * 0.36f;
            g.setColour (tp.ringLine); g.drawEllipse (atrilR.getX() + 6.0f, atrilR.getCentreY() - cr, cr * 2.0f, cr * 2.0f, 1.0f);
            auto keyR = Rectangle<float> (sw.getX() + 6.0f, atrilR.getBottom() + 5.0f, sw.getWidth() - 12.0f, sw.getBottom() - atrilR.getBottom() - 10.0f);
            g.setColour (tp.keyWhite); g.fillRect (keyR);
            const float kw = keyR.getWidth() / 10.0f;
            g.setColour (tp.light);
            for (int lk : { 2, 4, 6 }) g.fillRect (keyR.getX() + kw * (float) lk + 0.5f, keyR.getY(), kw - 1.0f, keyR.getHeight());
            g.setColour (tp.keyBlack);
            for (int bk = 1; bk < 10; ++bk)
                if (bk % 7 != 3 && bk % 7 != 0) g.fillRect (keyR.getX() + kw * (float) bk - kw * 0.28f, keyR.getY(), kw * 0.56f, keyR.getHeight() * 0.6f);
            draw::themeLight (g, sw, tp);
        }
        g.setColour (i == current ? pal.accent : pal.text.withAlpha (0.2f));
        g.drawRoundedRectangle (sw, 6.0f, i == current ? 2.0f : 1.0f);
        g.setColour (i == current ? pal.text : pal.dim);
        g.setFont (ctx.fonts.ui (12.5f, i == current ? 2 : 1));
        g.drawText (list[(size_t) i].name, r, Justification::centred, false);
    }

    const int shown = hover >= 0 ? hover : current;
    if (shown >= 0 && shown < (int) list.size())
    {
        g.setColour (pal.dim);
        g.setFont (ctx.fonts.ui (13.0f, 0));
        g.drawText (list[(size_t) shown].name + ": " + list[(size_t) shown].description
                        + (night ? U8 (" (noche)") : U8 (" (día)")),
                    Rectangle<float> (16.0f, (float) getHeight() - 34.0f, (float) getWidth() - 32.0f, 24.0f), Justification::centredLeft, true);
    }
}

void ThemePicker::mouseMove (const MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < (int) themes().size(); ++i)
        if (cell (i).contains (e.position)) h = i;
    if (h != hover) { hover = h; repaint(); }
}

void ThemePicker::mouseUp (const MouseEvent& e)
{
    for (int i = 0; i < (int) themes().size(); ++i)
        if (cell (i).contains (e.position))
        {
            current = i;
            repaint();
            if (onPick) onPick (i);
            return;
        }
}

//==============================================================================
// Panel de latencia
//==============================================================================
LatencyPanel::LatencyPanel (UiContext& c, RangedAudioParameter& p, bool sa,
                            std::function<String()> info, std::function<void (int)> setBuf)
    : ctx (c), standalone (sa), deviceInfo (std::move (info)), setBuffer (std::move (setBuf))
{
    slider.setSliderStyle (Slider::LinearHorizontal);
    slider.setTextBoxStyle (Slider::TextBoxRight, false, 70, 24);
    slider.setColour (Slider::trackColourId, ctx.vis.pal.accent);
    slider.setColour (Slider::thumbColourId, ctx.vis.pal.accent);
    slider.setColour (Slider::backgroundColourId, ctx.vis.pal.text.withAlpha (0.15f));
    slider.setColour (Slider::textBoxTextColourId, ctx.vis.pal.text);
    addAndMakeVisible (slider);
    attach = std::make_unique<SliderParameterAttachment> (p, slider, nullptr);

    if (standalone)
    {
        for (int bs : { 128, 256, 512 })
        {
            auto* b = bufferButtons.add (new TextButton (String (bs)));
            b->onClick = [this, bs] { if (setBuffer) setBuffer (bs); repaint(); };
            addAndMakeVisible (b);
        }
    }
    setSize (400, standalone ? 252 : 156);
}

LatencyPanel::~LatencyPanel() = default;

void LatencyPanel::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat().reduced (16.0f, 12.0f);
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (15.0f, 2));
    g.drawText ("Compensar latencia", b.removeFromTop (22.0f), Justification::centredLeft, false);
    g.setColour (pal.dim);
    g.setFont (ctx.fonts.ui (13.0f));
    g.drawFittedText (U8 ("Si MidiSky suena o se ilumina antes que tu otro instrumento, súbele hasta que coincidan. Retrasa el sonido y las luces de MidiSky juntos."),
                      b.removeFromTop (54.0f).toNearestInt(), Justification::topLeft, 3);
    if (standalone)
    {
        g.setColour (pal.text);
        g.setFont (ctx.fonts.ui (13.5f, 2));
        g.drawText ("Buffer de audio (menos = menos latencia)", Rectangle<float> (16.0f, 140.0f, (float) getWidth() - 32.0f, 20.0f), Justification::centredLeft, false);
        g.setColour (pal.dim);
        g.setFont (ctx.fonts.ui (12.5f));
        g.drawText (deviceInfo ? deviceInfo() : String(), Rectangle<float> (16.0f, 214.0f, (float) getWidth() - 32.0f, 20.0f), Justification::centredLeft, true);
    }
}

void LatencyPanel::resized()
{
    slider.setBounds (16, 98, getWidth() - 32, 30);
    int x = 16;
    for (auto* b : bufferButtons)
    {
        b->setBounds (x, 168, 80, 30);
        x += 88;
    }
}
