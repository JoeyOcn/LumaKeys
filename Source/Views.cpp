#include "Views.h"

using namespace juce;

//==============================================================================
namespace draw
{
    float textWidth (const Font& f, const String& s) { return GlyphArrangement::getStringWidth (f, s); }

    Font fitUi (const Fonts& fonts, const String& text, float maxH, float maxW, int weight)
    {
        float h = maxH;
        auto f = fonts.ui (h, weight);
        while (h > 8.0f && textWidth (f, text) > maxW)
        {
            h *= 0.93f;
            f = fonts.ui (h, weight);
        }
        return f;
    }

    Path devicePath (Rectangle<float> r, const Skin& pal, float k)
    {
        Path p;
        p.addRoundedRectangle (r, jlimit (8.0f, 44.0f, pal.radius * k));
        return p;
    }

    void device (Graphics& g, Rectangle<float> r, const Skin& pal, float k, bool backdrop)
    {
        if (backdrop)
        {
            g.setColour (pal.backdrop.darker (pal.night ? 0.2f : 0.35f));
            g.fillRect (r);
        }
        auto outer = r.reduced (0.5f);
        const auto path = devicePath (outer, pal, k);
        {
            Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (path);
            ColourGradient body (pal.body1, 0.0f, r.getY(), pal.body2, 0.0f, r.getBottom(), false);
            g.setGradientFill (body);
            g.fillRect (r);
            // Luz suave arriba y sombra abajo, como un aparato de aluminio
            ColourGradient top (Colours::white.withAlpha (pal.night ? 0.04f : 0.35f), 0.0f, r.getY(),
                                Colours::white.withAlpha (0.0f), 0.0f, r.getY() + 60.0f * k, false);
            g.setGradientFill (top);
            g.fillRect (r);
        }
        g.setColour (pal.edge);
        g.strokePath (path, PathStrokeType (1.0f));
        auto inner = outer.reduced (1.2f);
        ColourGradient hl (Colours::white.withAlpha (pal.night ? 0.12f : 0.85f), 0.0f, inner.getY(),
                           Colours::white.withAlpha (pal.night ? 0.02f : 0.25f), 0.0f, inner.getBottom(), false);
        g.setGradientFill (hl);
        g.strokePath (devicePath (inner, pal, k * 0.95f), PathStrokeType (1.0f));
    }

    void button (Graphics& g, Rectangle<float> r, const Skin& pal, float radius, float press, bool active, float hover)
    {
        const float depth = 2.5f * (1.0f - press);
        auto face = r.withTrimmedBottom (2.5f).translated (0.0f, 2.5f - depth);
        // Sombra suave
        g.setColour (pal.btnShadow.withMultipliedAlpha (0.55f * (1.0f - press * 0.6f)));
        g.fillRoundedRectangle (face.translated (0.0f, depth + 2.0f).expanded (0.5f, 0.0f), radius + 1.0f);
        // Canto del botón
        const auto edge = active ? pal.accent.darker (0.35f) : pal.btnEdge;
        g.setColour (edge);
        g.fillRoundedRectangle (face.translated (0.0f, depth), radius);
        // Cara
        auto top = active ? pal.accent : pal.btn;
        if (hover > 0.0f) top = top.interpolatedWith (pal.night || pal.look == 1 ? Colours::white : Colours::white, 0.06f * hover);
        ColourGradient fg (top, 0.0f, face.getY(), top.darker (0.04f), 0.0f, face.getBottom(), false);
        g.setGradientFill (fg);
        g.fillRoundedRectangle (face, radius);
        g.setColour (edge.withMultipliedAlpha (0.8f));
        g.drawRoundedRectangle (face.reduced (0.5f), radius, 1.0f);
        g.setColour (Colours::white.withAlpha (active ? 0.25f : ((pal.night || pal.look == 1) ? 0.08f : 0.6f)));
        g.drawLine (face.getX() + radius, face.getY() + 1.2f, face.getRight() - radius, face.getY() + 1.2f, 1.0f);
    }

    void recess (Graphics& g, Rectangle<float> r, const Skin& pal, float radius, Colour fill)
    {
        g.setColour (fill);
        g.fillRoundedRectangle (r, radius);
        Graphics::ScopedSaveState ss (g);
        Path clip;
        clip.addRoundedRectangle (r, radius);
        g.reduceClipRegion (clip);
        ColourGradient sh (Colours::black.withAlpha (pal.night ? 0.35f : 0.12f), 0.0f, r.getY(),
                           Colours::black.withAlpha (0.0f), 0.0f, r.getY() + 9.0f, false);
        g.setGradientFill (sh);
        g.fillRect (r);
        g.setColour (Colours::black.withAlpha (pal.night ? 0.3f : 0.06f));
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
        g.setColour (Colours::white.withAlpha (pal.night ? 0.05f : 0.45f));
        g.drawLine (r.getX() + radius, r.getBottom() - 0.5f, r.getRight() - radius, r.getBottom() - 0.5f, 1.0f);
    }

    void card (Graphics& g, Rectangle<float> r, const Skin& pal, float radius)
    {
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (pal.btnShadow.withMultipliedAlpha (0.18f));
            g.fillRoundedRectangle (r.translated (0.0f, (float) i * 2.0f).expanded ((float) i), radius + (float) i);
        }
        g.setColour (pal.btnEdge);
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f), radius);
        g.setColour (pal.card);
        g.fillRoundedRectangle (r, radius);
        g.setColour (pal.btnEdge.withMultipliedAlpha (0.8f));
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    }

    void screen (Graphics& g, Rectangle<float> r, const Skin& pal, float k)
    {
        recess (g, r, pal, 20.0f * k, pal.body2.darker (pal.night ? 0.2f : 0.04f));
        auto in = r.reduced (8.0f * k);
        g.setColour (pal.screen);
        g.fillRoundedRectangle (in, 14.0f * k);
        ColourGradient glass (Colours::white.withAlpha (0.07f), in.getX(), in.getY(),
                              Colours::white.withAlpha (0.0f), in.getX() + in.getWidth() * 0.45f, in.getY() + in.getHeight() * 0.7f, false);
        g.setGradientFill (glass);
        g.fillRoundedRectangle (in, 14.0f * k);
        g.setColour (Colours::white.withAlpha (0.05f));
        g.drawRoundedRectangle (in.reduced (0.5f), 14.0f * k, 1.0f);
    }

    void knobBody (Graphics& g, Point<float> c, float r, const Skin& pal, Colour cap)
    {
        g.setColour (Colours::black.withAlpha (pal.night ? 0.45f : 0.18f));
        g.fillEllipse (c.x - r, c.y - r + 2.5f, r * 2.0f, r * 2.0f);
        g.setColour (pal.btn);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (pal.btnEdge);
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.0f);
        const float cr = r * 0.74f;
        ColourGradient cg (cap.brighter (0.12f), c.x, c.y - cr, cap.darker (0.12f), c.x, c.y + cr, false);
        g.setGradientFill (cg);
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
    }

    void icon (Graphics& g, const String& name, Rectangle<float> r, Colour c, float stroke)
    {
        const float s = jmin (r.getWidth(), r.getHeight());
        const auto sq = r.withSizeKeepingCentre (s, s);
        auto P = [&] (float x, float y) { return Point<float> (sq.getX() + x / 24.0f * s, sq.getY() + y / 24.0f * s); };
        Path p;
        g.setColour (c);
        const PathStrokeType st (stroke * jmax (0.8f, s / 22.0f), PathStrokeType::curved, PathStrokeType::rounded);

        if (name == "sun")
        {
            const auto cc = P (12, 12);
            p.addEllipse (cc.x - s * 0.17f, cc.y - s * 0.17f, s * 0.34f, s * 0.34f);
            for (int i = 0; i < 8; ++i)
            {
                const float a = MathConstants<float>::pi * 0.25f * (float) i;
                p.startNewSubPath (cc + Point<float> (std::cos (a), std::sin (a)) * (s * 0.29f));
                p.lineTo (cc + Point<float> (std::cos (a), std::sin (a)) * (s * 0.40f));
            }
        }
        else if (name == "moon")
        {
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
        else if (name == "palette")
        {
            const auto cc = P (12, 12);
            p.addEllipse (cc.x - s * 0.36f, cc.y - s * 0.36f, s * 0.72f, s * 0.72f);
            g.strokePath (p, st);
            const Point<float> dots[4] = { P (8.5f, 9.5f), P (12.5f, 7.5f), P (16, 10.5f), P (15, 15) };
            for (auto d : dots) g.fillEllipse (d.x - s * 0.06f, d.y - s * 0.06f, s * 0.12f, s * 0.12f);
            return;
        }
        else if (name == "keys")
        {
            p.addRoundedRectangle (Rectangle<float> (P (3.5f, 5), P (20.5f, 19)), s * 0.06f);
            for (float x : { 9.2f, 14.8f }) { p.startNewSubPath (P (x, 12.5f)); p.lineTo (P (x, 19)); }
            g.strokePath (p, st);
            for (float x : { 9.2f, 14.8f })
                g.fillRect (Rectangle<float> (P (x - 1.6f, 5), P (x + 1.6f, 12.5f)));
            return;
        }
        else if (name == "transp")
        {
            p.addRoundedRectangle (Rectangle<float> (P (4, 4), P (20, 20)), s * 0.08f);
            g.strokePath (p, st);
            g.fillRect (Rectangle<float> (P (6.5f, 6.5f), P (11.5f, 11.5f)));
            g.fillRect (Rectangle<float> (P (12.5f, 12.5f), P (17.5f, 17.5f)));
            return;
        }
        else if (name == "split")
        {
            p.addRoundedRectangle (Rectangle<float> (P (3, 4), P (21, 11)), s * 0.06f);
            p.addRoundedRectangle (Rectangle<float> (P (3, 15), P (21, 20)), s * 0.05f);
        }
        else if (name == "windows" || name == "detach")
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
        else if (name == "chev")  { p.startNewSubPath (P (7, 10)); p.lineTo (P (12, 15)); p.lineTo (P (17, 10)); }
        else if (name == "left")  { p.startNewSubPath (P (14, 6)); p.lineTo (P (8, 12)); p.lineTo (P (14, 18)); }
        else if (name == "right") { p.startNewSubPath (P (10, 6)); p.lineTo (P (16, 12)); p.lineTo (P (10, 18)); }
        else if (name == "close") { p.startNewSubPath (P (6.5f, 6.5f)); p.lineTo (P (17.5f, 17.5f)); p.startNewSubPath (P (17.5f, 6.5f)); p.lineTo (P (6.5f, 17.5f)); }
        else if (name == "minimize") { p.startNewSubPath (P (6, 16)); p.lineTo (P (18, 16)); }
        else if (name == "expand")   { p.addRoundedRectangle (Rectangle<float> (P (6, 6), P (18, 18)), s * 0.05f); }
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
        else if (name == "restore")
        {
            const auto cc = P (12, 12.5f);
            p.addCentredArc (cc.x, cc.y, s * 0.32f, s * 0.32f, 0.0f, -2.4f, 2.6f, true);
            const auto a = cc + Point<float> (std::sin (-2.4f), -std::cos (-2.4f)) * (s * 0.32f);
            p.startNewSubPath (a + Point<float> (s * 0.17f, 0.0f));
            p.lineTo (a);
            p.lineTo (a + Point<float> (0.0f, -s * 0.17f));
        }
        else if (name == "metro")
        {
            // metrónomo de péndulo
            p.startNewSubPath (P (8.5f, 20)); p.lineTo (P (10.6f, 4.5f)); p.lineTo (P (13.4f, 4.5f)); p.lineTo (P (15.5f, 20)); p.closeSubPath();
            p.startNewSubPath (P (12, 16)); p.lineTo (P (17.5f, 7.5f));
            p.startNewSubPath (P (9.6f, 14)); p.lineTo (P (14.4f, 14));
        }
        else if (name == "rec")
        {
            const auto cc = P (12, 12);
            g.fillEllipse (cc.x - s * 0.22f, cc.y - s * 0.22f, s * 0.44f, s * 0.44f);
            return;
        }
        else if (name == "stop")
        {
            g.fillRoundedRectangle (Rectangle<float> (P (7, 7), P (17, 17)), s * 0.06f);
            return;
        }
        else if (name == "play")
        {
            p.startNewSubPath (P (8, 5.5f)); p.lineTo (P (18.5f, 12)); p.lineTo (P (8, 18.5f)); p.closeSubPath();
            g.fillPath (p);
            g.strokePath (p, st);
            return;
        }
        else if (name == "folder")
        {
            p.startNewSubPath (P (3.5f, 7)); p.lineTo (P (3.5f, 18.5f)); p.lineTo (P (20.5f, 18.5f)); p.lineTo (P (20.5f, 8.5f));
            p.lineTo (P (11.5f, 8.5f)); p.lineTo (P (9.5f, 5.5f)); p.lineTo (P (3.5f, 5.5f)); p.closeSubPath();
        }
        else if (name == "trash")
        {
            p.startNewSubPath (P (4.5f, 7)); p.lineTo (P (19.5f, 7));
            p.startNewSubPath (P (9.5f, 7)); p.lineTo (P (10, 4.5f)); p.lineTo (P (14, 4.5f)); p.lineTo (P (14.5f, 7));
            p.startNewSubPath (P (6.5f, 7)); p.lineTo (P (7.5f, 20)); p.lineTo (P (16.5f, 20)); p.lineTo (P (17.5f, 7));
            p.startNewSubPath (P (10.5f, 11)); p.lineTo (P (10.5f, 16.5f));
            p.startNewSubPath (P (13.5f, 11)); p.lineTo (P (13.5f, 16.5f));
        }
        else if (name == "plug")
        {
            p.startNewSubPath (P (9, 3.5f)); p.lineTo (P (9, 8));
            p.startNewSubPath (P (15, 3.5f)); p.lineTo (P (15, 8));
            p.startNewSubPath (P (6, 8)); p.lineTo (P (18, 8)); p.lineTo (P (18, 12));
            p.quadraticTo (P (18, 17), P (12, 17)); p.quadraticTo (P (6, 17), P (6, 12)); p.closeSubPath();
            p.startNewSubPath (P (12, 17)); p.lineTo (P (12, 21));
        }
        else if (name == "save")
        {
            p.startNewSubPath (P (5, 4.5f)); p.lineTo (P (16.5f, 4.5f)); p.lineTo (P (19.5f, 7.5f)); p.lineTo (P (19.5f, 19.5f));
            p.lineTo (P (4.5f, 19.5f)); p.lineTo (P (4.5f, 4.5f)); p.closeSubPath();
            p.startNewSubPath (P (8, 4.5f)); p.lineTo (P (8, 9)); p.lineTo (P (15, 9)); p.lineTo (P (15, 4.5f));
            p.addRoundedRectangle (Rectangle<float> (P (8, 13), P (16, 19.5f)), s * 0.02f);
        }
        else if (name == "pencil")
        {
            p.startNewSubPath (P (5, 19)); p.lineTo (P (6, 15)); p.lineTo (P (16, 5)); p.lineTo (P (19, 8)); p.lineTo (P (9, 18)); p.closeSubPath();
            p.startNewSubPath (P (14, 7)); p.lineTo (P (17, 10));
        }
        else if (name == "cursor")
        {
            p.startNewSubPath (P (6, 4)); p.lineTo (P (6, 18)); p.lineTo (P (10, 14.5f)); p.lineTo (P (12.5f, 20)); p.lineTo (P (15, 19));
            p.lineTo (P (12.5f, 13.5f)); p.lineTo (P (17.5f, 13.5f)); p.closeSubPath();
        }
        else if (name == "knife")
        {
            p.startNewSubPath (P (4, 20)); p.lineTo (P (16, 8)); p.lineTo (P (19, 5)); p.lineTo (P (20, 9)); p.lineTo (P (8, 21)); p.closeSubPath();
            p.startNewSubPath (P (12, 4)); p.lineTo (P (12, 6.5f));
            p.startNewSubPath (P (3.5f, 12)); p.lineTo (P (6, 12));
        }
        else if (name == "eraser")
        {
            p.startNewSubPath (P (9, 19.5f)); p.lineTo (P (4, 14.5f)); p.lineTo (P (13, 5.5f)); p.lineTo (P (20, 12.5f)); p.lineTo (P (13, 19.5f)); p.closeSubPath();
            p.startNewSubPath (P (8, 10.5f)); p.lineTo (P (15, 17.5f));
            p.startNewSubPath (P (13, 19.5f)); p.lineTo (P (20, 19.5f));
        }
        else if (name == "undo" || name == "redo")
        {
            const bool u = name == "undo";
            auto X = [u] (float x) { return u ? x : 24.0f - x; };
            p.startNewSubPath (P (X (5), 10)); p.lineTo (P (X (14), 10)); p.quadraticTo (P (X (20), 10), P (X (20), 15)); p.quadraticTo (P (X (20), 19.5f), P (X (14), 19.5f)); p.lineTo (P (X (10), 19.5f));
            p.startNewSubPath (P (X (9), 6)); p.lineTo (P (X (5), 10)); p.lineTo (P (X (9), 14));
        }
        else if (name == "magnet")
        {
            p.startNewSubPath (P (6, 4.5f)); p.lineTo (P (6, 12)); p.quadraticTo (P (6, 19.5f), P (12, 19.5f)); p.quadraticTo (P (18, 19.5f), P (18, 12)); p.lineTo (P (18, 4.5f));
            p.startNewSubPath (P (10, 4.5f)); p.lineTo (P (10, 12)); p.quadraticTo (P (10, 15.5f), P (12, 15.5f)); p.quadraticTo (P (14, 15.5f), P (14, 12)); p.lineTo (P (14, 4.5f));
            p.startNewSubPath (P (6, 8)); p.lineTo (P (10, 8));
            p.startNewSubPath (P (14, 8)); p.lineTo (P (18, 8));
        }
        else if (name == "quant")
        {
            for (float x : { 6.0f, 12.0f, 18.0f }) { p.startNewSubPath (P (x, 4)); p.lineTo (P (x, 20)); }
            g.strokePath (p, PathStrokeType (st.getStrokeThickness() * 0.6f));
            Path q;
            q.addRoundedRectangle (Rectangle<float> (P (6, 9), P (12, 15)), s * 0.03f);
            g.fillPath (q);
            return;
        }
        else if (name == "export")
        {
            p.startNewSubPath (P (12, 15)); p.lineTo (P (12, 3.5f));
            p.startNewSubPath (P (7.5f, 8)); p.lineTo (P (12, 3.5f)); p.lineTo (P (16.5f, 8));
            p.startNewSubPath (P (5, 12)); p.lineTo (P (5, 20)); p.lineTo (P (19, 20)); p.lineTo (P (19, 12));
        }
        else if (name == "plus")  { p.startNewSubPath (P (12, 6)); p.lineTo (P (12, 18)); p.startNewSubPath (P (6, 12)); p.lineTo (P (18, 12)); }
        else if (name == "minus") { p.startNewSubPath (P (6, 12)); p.lineTo (P (18, 12)); }
        else if (name == "tap")
        {
            const auto cc = P (12, 12);
            p.addEllipse (cc.x - s * 0.3f, cc.y - s * 0.3f, s * 0.6f, s * 0.6f);
            g.strokePath (p, st);
            g.fillEllipse (cc.x - s * 0.1f, cc.y - s * 0.1f, s * 0.2f, s * 0.2f);
            return;
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

    void catLogo (Graphics& g, Rectangle<float> r, Colour line, Colour eyes)
    {
        // El mismo gato del ícono de la app (cabeza rellena, ojos y rayitas de color)
        const float s = jmin (r.getWidth(), r.getHeight());
        const auto cen = r.getCentre();
        const float sc = s / 680.0f;
        auto P = [&] (float x, float y) { return Point<float> (cen.x + x * sc, cen.y + (y + 30.0f) * sc); };
        const bool darkHead = line.getPerceivedBrightness() < 0.5f;
        const auto face = darkHead ? Colour (0xffe8e5de) : Colour (0xff1c1c1e);

        g.setColour (line);
        for (float side : { -1.0f, 1.0f })
        {
            Path ear;
            ear.startNewSubPath (P (side * 268, -30));
            ear.cubicTo (P (side * 282, -150), P (side * 262, -250), P (side * 236, -296));
            ear.cubicTo (P (side * 226, -312), P (side * 210, -314), P (side * 196, -304));
            ear.cubicTo (P (side * 150, -270), P (side * 100, -222), P (side * 70, -180));
            ear.closeSubPath();
            g.fillPath (ear);
            g.strokePath (ear, PathStrokeType (24.0f * sc, PathStrokeType::curved, PathStrokeType::rounded));
        }
        const auto hc = P (0, 20);
        g.fillEllipse (hc.x - 290.0f * sc, hc.y - 232.0f * sc, 580.0f * sc, 464.0f * sc);

        g.setColour (eyes);
        for (float side : { -1.0f, 1.0f })
        {
            Path inner;
            inner.startNewSubPath (P (side * 232, -90));
            inner.cubicTo (P (side * 238, -160), P (side * 228, -220), P (side * 214, -250));
            inner.cubicTo (P (side * 180, -226), P (side * 140, -192), P (side * 116, -160));
            inner.closeSubPath();
            g.fillPath (inner);
        }
        // Rayitas de la frente
        for (auto st : { Rectangle<float> (-56, -196, 22, 70), Rectangle<float> (-11, -206, 22, 84), Rectangle<float> (34, -196, 22, 70) })
        {
            const auto a = P (st.getX(), st.getY());
            g.fillRoundedRectangle (a.x, a.y, st.getWidth() * sc, st.getHeight() * sc, 11.0f * sc);
        }
        // Ojos
        for (float side : { -1.0f, 1.0f })
        {
            const auto e = P (side * 112, 10);
            g.setColour (eyes);
            g.fillEllipse (e.x - 56.0f * sc, e.y - 60.0f * sc, 112.0f * sc, 120.0f * sc);
            g.setColour (line);
            const auto pu = P (side * 112, 12);
            g.fillEllipse (pu.x - 16.0f * sc, pu.y - 46.0f * sc, 32.0f * sc, 92.0f * sc);
            if (s > 22.0f)
            {
                g.setColour (Colours::white);
                const auto hl = P (side * 112 + 20, -14);
                g.fillEllipse (hl.x - 11.0f * sc, hl.y - 11.0f * sc, 22.0f * sc, 22.0f * sc);
            }
        }
        // Nariz y boca
        Path nose;
        nose.startNewSubPath (P (-28, 92));
        nose.lineTo (P (28, 92));
        nose.lineTo (P (0, 130));
        nose.closeSubPath();
        g.setColour (Colour (0xffff9c80));
        g.fillPath (nose);
        Path mouth;
        mouth.startNewSubPath (P (0, 128));
        mouth.cubicTo (P (0, 150), P (-14, 164), P (-36, 164));
        mouth.cubicTo (P (-52, 164), P (-62, 154), P (-64, 142));
        mouth.startNewSubPath (P (0, 128));
        mouth.cubicTo (P (0, 150), P (14, 164), P (36, 164));
        mouth.cubicTo (P (52, 164), P (62, 154), P (64, 142));
        g.setColour (face);
        g.strokePath (mouth, PathStrokeType (jmax (1.0f, 12.0f * sc), PathStrokeType::curved, PathStrokeType::rounded));
        // Bigotes
        Path wh;
        for (float side : { -1.0f, 1.0f })
        {
            wh.startNewSubPath (P (side * 170, 96));  wh.lineTo (P (side * 330, 72));
            wh.startNewSubPath (P (side * 170, 124)); wh.lineTo (P (side * 334, 134));
        }
        g.setColour (face.interpolatedWith (line, 0.3f));
        g.strokePath (wh, PathStrokeType (jmax (1.0f, 10.0f * sc), PathStrokeType::curved, PathStrokeType::rounded));
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
}

//==============================================================================
// Controles
//==============================================================================
void Switch::setOn (bool v)
{
    if (on != v) { on = v; animate(); }
}

bool Switch::stepAnimation (float dt)
{
    pos = approach (pos, on ? 1.0f : 0.0f, 0.28f, dt);
    repaint();
    return pos != (on ? 1.0f : 0.0f);
}

void Switch::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat();
    const float h = jmin (b.getHeight(), b.getWidth() * 0.54f);
    auto track = b.withSizeKeepingCentre (h * 1.86f, h);
    const auto off = pal.night ? pal.plate.brighter (0.45f) : pal.plate.darker (0.14f);
    g.setColour (off.interpolatedWith (pal.accent, pos));
    g.fillRoundedRectangle (track, h * 0.5f);
    ColourGradient in (Colours::black.withAlpha (0.2f), 0.0f, track.getY(), Colours::black.withAlpha (0.0f), 0.0f, track.getY() + h * 0.4f, false);
    g.setGradientFill (in);
    g.fillRoundedRectangle (track, h * 0.5f);
    const float d = h - 4.0f;
    const float x = track.getX() + 2.0f + (track.getWidth() - d - 4.0f) * pos;
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillEllipse (x, track.getY() + 3.0f, d, d);
    g.setColour (Colours::white);
    g.fillEllipse (x, track.getY() + 2.0f, d, d);
}

void Switch::mouseUp (const MouseEvent& e)
{
    if (! getLocalBounds().contains (e.getPosition())) return;
    on = ! on;
    animate();
    if (onChange) onChange (on);
}

//------------------------------------------------------------------------------
void Knob::setValue (float normalised, String display)
{
    if (std::abs (normalised - value) > 1.0e-4f || display != valueText)
    {
        value = normalised;
        valueText = std::move (display);
        if (dragging) shown = value;
        animate();
        repaint();
    }
}

bool Knob::stepAnimation (float dt)
{
    shown = approach (shown, value, 0.3f, dt);
    repaint();
    return shown != value;
}

void Knob::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat();
    const float d = jmin (b.getWidth(), b.getHeight());
    const auto c = b.getCentre();
    const float r = d * 0.5f - jmax (4.0f, d * 0.1f);
    const float a0 = -MathConstants<float>::pi * 0.75f, a1 = MathConstants<float>::pi * 0.75f;
    const float av = a0 + (a1 - a0) * shown;
    const float ar = r + jmax (3.0f, d * 0.07f);
    const float thick = jmax (2.2f, d * 0.05f);

    Path track, val;
    track.addCentredArc (c.x, c.y, ar, ar, 0.0f, a0, a1, true);
    val.addCentredArc (c.x, c.y, ar, ar, 0.0f, a0, av, true);
    g.setColour (pal.dim.withAlpha (0.25f));
    g.strokePath (track, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (pal.accent);
    g.strokePath (val, PathStrokeType (thick, PathStrokeType::curved, PathStrokeType::rounded));

    draw::knobBody (g, c, r, pal, pal.caps[(size_t) jlimit (0, 3, capIndex)]);
    const auto p0 = c + Point<float> (std::sin (av), -std::cos (av)) * (r * 0.22f);
    const auto p1 = c + Point<float> (std::sin (av), -std::cos (av)) * (r * 0.62f);
    g.setColour (Colours::white);
    g.drawLine ({ p0, p1 }, jmax (2.0f, r * 0.09f));
}

void Knob::mouseDown (const MouseEvent&)        { dragging = true; dragStart = value; if (onGestureStart) onGestureStart(); }
void Knob::mouseUp (const MouseEvent&)          { dragging = false; if (onGestureEnd) onGestureEnd(); }
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
void Segmented::setSelected (int i)
{
    if (i != selected) { selected = i; animate(); }
}

bool Segmented::stepAnimation (float dt)
{
    pos = approach (pos, (float) selected, 0.3f, dt);
    repaint();
    return pos != (float) selected;
}

void Segmented::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat().reduced (0.5f);
    const float rad = jmin (10.0f, b.getHeight() * 0.3f);
    draw::recess (g, b, pal, rad, pal.night ? pal.plate.darker (0.15f) : pal.plate.darker (0.03f));
    const int n = jmax (1, options.size());
    const float w = b.getWidth() / (float) n;
    auto sel = Rectangle<float> (b.getX() + pos * w, b.getY(), w, b.getHeight()).reduced (3.0f);
    draw::button (g, sel, pal, rad * 0.8f, 0.0f, true);
    for (int i = 0; i < n; ++i)
    {
        auto r = Rectangle<float> (b.getX() + (float) i * w, b.getY(), w, b.getHeight()).reduced (3.0f);
        const float closeness = jlimit (0.0f, 1.0f, 1.0f - std::abs (pos - (float) i));
        g.setColour (pal.dim.interpolatedWith (pal.accentInk, closeness));
        g.setFont (draw::fitUi (ctx.fonts, options[i], jmin (14.5f * fontScale, r.getHeight() * 0.5f), r.getWidth() - 6.0f, 2));
        g.drawText (options[i], r.translated (0.0f, -1.0f), Justification::centred, false);
    }
}

void Segmented::mouseUp (const MouseEvent& e)
{
    auto b = getLocalBounds().toFloat();
    if (! b.contains (e.position)) return;
    const int i = jlimit (0, options.size() - 1, (int) (e.position.x / (b.getWidth() / (float) options.size())));
    if (i != selected) { selected = i; animate(); if (onChange) onChange (i); }
}

//------------------------------------------------------------------------------
PushButton::PushButton (UiContext& c, String iconName, String tip, String text)
    : ctx (c), iconId (std::move (iconName)), label (std::move (text))
{
    setTooltip (tip);
    setTitle (tip);
}

bool PushButton::stepAnimation (float dt)
{
    press = approach (press, down ? 1.0f : 0.0f, 0.5f, dt);
    hover = approach (hover, hoverT, 0.25f, dt);
    repaint();
    return press != (down ? 1.0f : 0.0f) || hover != hoverT;
}

void PushButton::mouseDown (const MouseEvent&) { down = true; animate(); }

bool PushButton::inMoreZone (Point<int> p) const
{
    // La flechita de abajo a la derecha abre los ajustes
    return onMore != nullptr && p.x > getWidth() * 0.62f && p.y > getHeight() * 0.52f;
}

void PushButton::mouseUp (const MouseEvent& e)
{
    down = false;
    animate();
    if (! getLocalBounds().contains (e.getPosition())) return;
    if (onMore != nullptr && (e.mods.isPopupMenu() || inMoreZone (e.getPosition()))) { onMore(); return; }
    if (onClick) onClick();
}

void PushButton::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    auto content = b;
    Colour col = lightOnDark ? Colours::white : pal.btnText;

    if (framed)
    {
        Skin p2 = pal;
        if (! activeColour.isTransparent()) { p2.accent = activeColour; p2.accentInk = Colours::white; }
        draw::button (g, b, p2, jmin (b.getHeight(), b.getWidth()) * radiusScale, press, active, hover);
        content = b.withTrimmedBottom (2.5f).translated (0.0f, 2.5f * press);
        if (active) col = p2.accentInk;
        else if (! iconColour.isTransparent()) col = iconColour;
        if (flash > 0.01f)
        {
            g.setColour ((active ? Colours::white : p2.accent).withAlpha (0.55f * flash));
            g.drawRoundedRectangle (content.reduced (2.0f), jmin (b.getHeight(), b.getWidth()) * radiusScale * 0.85f, 2.5f);
        }
    }
    else
    {
        if (hover > 0.01f || press > 0.01f)
        {
            g.setColour ((lightOnDark ? Colours::white : pal.text).withAlpha (0.10f * hover + 0.08f * press));
            g.fillRoundedRectangle (b, jmin (b.getWidth(), b.getHeight()) * 0.3f);
        }
        col = lightOnDark ? Colours::white : (active ? pal.accent : pal.text);
    }

    const bool showText = label.isNotEmpty() && content.getWidth() > content.getHeight() * 1.8f;
    if (showText)
    {
        auto ir = content.removeFromLeft (content.getHeight());
        if (! swatch.isTransparent())
        {
            const float d = ir.getHeight() * 0.42f;
            auto sr = ir.withSizeKeepingCentre (d, d).translated (ir.getHeight() * 0.08f, 0.0f);
            g.setColour (swatch);
            g.fillEllipse (sr);
            g.setColour (Colours::black.withAlpha (0.25f));
            g.drawEllipse (sr, 1.0f);
        }
        else if (iconId.isNotEmpty())
        {
            draw::icon (g, iconId, ir.reduced (ir.getWidth() * 0.26f).translated (ir.getHeight() * 0.06f, 0.0f), col, 1.6f);
        }
        g.setColour (col);
        g.setFont (draw::fitUi (ctx.fonts, label, jmin (14.5f, content.getHeight() * 0.5f), content.getWidth() - 8.0f, 2));
        g.drawText (label, content.withTrimmedRight (6.0f), Justification::centredLeft, false);
    }
    else if (iconId.isNotEmpty())
    {
        auto ir = content.reduced (content.getHeight() * 0.25f);
        if (onMore != nullptr) ir = ir.translated (-content.getWidth() * 0.06f, -content.getHeight() * 0.03f);
        draw::icon (g, iconId, ir, col, 1.6f);
    }
    else if (label.isNotEmpty())
    {
        g.setColour (col);
        g.setFont (draw::fitUi (ctx.fonts, label, jmin (14.5f, content.getHeight() * 0.48f), content.getWidth() - 6.0f, 2));
        g.drawText (label, content, Justification::centred, false);
    }

    if (onMore != nullptr)
    {
        const float cs = content.getHeight() * 0.26f;
        auto cr = Rectangle<float> (content.getRight() - cs - content.getWidth() * 0.07f, content.getBottom() - cs - content.getHeight() * 0.06f, cs, cs);
        Path tri;
        tri.addTriangle (cr.getX(), cr.getY() + cs * 0.3f, cr.getRight(), cr.getY() + cs * 0.3f, cr.getCentreX(), cr.getBottom() - cs * 0.05f);
        g.setColour (col.withAlpha (0.7f));
        g.fillPath (tri);
    }
}

//------------------------------------------------------------------------------
bool FieldButton::stepAnimation (float dt)
{
    press = approach (press, down ? 1.0f : 0.0f, 0.5f, dt);
    repaint();
    return press != (down ? 1.0f : 0.0f);
}

void FieldButton::mouseUp (const MouseEvent& e)
{
    down = false;
    animate();
    if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
}

void FieldButton::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    draw::button (g, b, pal, 12.0f * k, press, false);
    auto inner = b.withTrimmedBottom (2.5f).translated (0.0f, 2.5f * press).reduced (12.0f * k, 5.0f * k);
    auto chev = inner.removeFromRight (14.0f * k);
    draw::icon (g, "chev", chev.withSizeKeepingCentre (16.0f * k, 16.0f * k), pal.btnText.withAlpha (0.6f), 1.8f);

    auto labelRow = inner.removeFromTop (inner.getHeight() * 0.42f);
    g.setColour (pal.btnText.withAlpha (0.58f));
    g.setFont (ctx.fonts.ui (13.5f * k, 1));
    g.drawText (text, labelRow, Justification::bottomLeft, false);

    if (led >= 0)
    {
        auto lr = inner.removeFromLeft (15.0f * k);
        const auto c = Point<float> (lr.getX() + 4.5f * k, inner.getCentreY());
        const auto col = led == 2 ? pal.accent : (led == 1 ? Colour (0xff3fd17a) : pal.dim.withAlpha (0.4f));
        if (led > 0)
        {
            ColourGradient glow (col.withAlpha (0.55f), c.x, c.y, col.withAlpha (0.0f), c.x + 9.0f * k, c.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (c.x - 9.0f * k, c.y - 9.0f * k, 18.0f * k, 18.0f * k);
        }
        g.setColour (col);
        g.fillEllipse (c.x - 4.0f * k, c.y - 4.0f * k, 8.0f * k, 8.0f * k);
    }
    g.setColour (pal.btnText);
    g.setFont (draw::fitUi (ctx.fonts, value, 15.0f * k, inner.getWidth(), 2));
    g.drawText (value, inner, Justification::centredLeft, true);
}

//==============================================================================
// Barra de arriba
//==============================================================================
RailBar::RailBar (UiContext& c)
    : midi (c, "MIDI"), key (c, "Tonalidad"), scale (c, "Escala"), latency (c, "Latencia"),
      recBtn (c, "rec", U8 ("Grabar MIDI (la flechita abre tus grabaciones)")),
      metroBtn (c, "metro", U8 ("Metrónomo (la flechita abre sus ajustes)")),
      dayBtn (c, "sun", U8 ("Modo día / noche")),
      colourBtn (c, "palette", U8 ("Color de la interfaz")),
      transpBtn (c, "transp", "Modo transparente"),
      splitBtn (c, "split", "Separar o unir el teclado"),
      windowsBtn (c, "windows", "Separar secciones en ventanas"),
      resetBtn (c, "restore", U8 ("Regresar todo a su lugar"), "Restablecer"),
      hideBtn (c, "eyeoff", U8 ("Ocultar parámetros")),
      menuBtn (c, "menu", U8 ("Menú")),
      ctx (c)
{
    for (Component* comp : std::initializer_list<Component*> { &midi, &key, &scale, &latency, &recBtn, &metroBtn, &dayBtn, &colourBtn, &transpBtn, &splitBtn, &windowsBtn, &resetBtn, &hideBtn, &menuBtn })
        addAndMakeVisible (comp);
    resetBtn.setActive (true);
    recBtn.activeColour = Colour (0xffe5383b);
    recBtn.iconColour = Colour (0xffe5383b);
    midi.setLed (0);
}

void RailBar::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto logo = logoArea;
    const float s = jmin (logo.getHeight(), 46.0f * k);
    draw::catLogo (g, logo.removeFromLeft (s).withSizeKeepingCentre (s, s), pal.text, pal.accent);
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (26.0f * k, 2));
    g.drawText ("Midi Cat", logo.withTrimmedLeft (10.0f * k), Justification::centredLeft, false);
}

void RailBar::resized()
{
    auto b = getLocalBounds();
    const int gap = roundToInt (8.0f * k);
    const int ib = roundToInt (48.0f * k);
    const int y = (b.getHeight() - ib) / 2;
    logoArea = b.removeFromLeft (roundToInt (180.0f * k)).toFloat();
    b.removeFromLeft (gap);

    auto placeR = [&] (Component& c, int w) { c.setBounds (b.getRight() - w, y, w, ib); b.removeFromRight (w + gap); };
    auto place  = [&] (Component& c, int w) { c.setBounds (b.getX(), y, w, ib); b.removeFromLeft (w + gap); };

    for (auto* f : { &midi, &key, &scale, &latency }) f->k = k;

    placeR (menuBtn, ib);
    placeR (hideBtn, ib);
    const bool show = ! ctx.vis.hideParams;
    resetBtn.setVisible (showReset);
    if (showReset)
    {
        // Con texto si cabe; si no, solo el ícono (para no apachurrar los campos)
        const float fieldsWant = (172.0f + 150.0f + 146.0f + 96.0f) * k + 4.0f * (float) gap;
        const float iconsWant = 7.0f * (float) (ib + gap);
        const bool roomForText = ! show || (float) b.getWidth() - iconsWant - 140.0f * k - (float) gap >= fieldsWant * 0.9f;
        resetBtn.setLabel (roomForText ? U8 ("Restablecer") : String());
        placeR (resetBtn, roomForText ? roundToInt (140.0f * k) : ib);
    }

    for (auto* c : std::initializer_list<Component*> { &midi, &key, &scale, &latency, &dayBtn, &colourBtn, &transpBtn, &splitBtn, &windowsBtn })
        c->setVisible (show);

    if (show)
    {
        for (auto* btn : { &windowsBtn, &splitBtn, &transpBtn, &colourBtn, &dayBtn })
            placeR (*btn, ib);
        b.removeFromRight (roundToInt (6.0f * k));
    }
    // Grabar y metrónomo siempre a la mano
    placeR (metroBtn, ib);
    placeR (recBtn, ib);

    if (show)
    {
        const int avail = b.getWidth();
        const float want = (172.0f + 150.0f + 146.0f + 96.0f) * k + 4.0f * (float) gap;
        const float f = jlimit (0.3f, 1.0f, (float) avail / want);
        place (midi, roundToInt (172.0f * k * f));
        place (key, roundToInt (150.0f * k * f));
        place (scale, roundToInt (146.0f * k * f));
        place (latency, roundToInt (96.0f * k * f));
    }
}

//==============================================================================
// Panel flotante (se cierra con la X, Esc o clic afuera)
//==============================================================================
Popover::Popover (UiContext& c, const String& t, std::unique_ptr<Component> cont, Rectangle<int> a)
    : ctx (c), title (t), content (std::move (cont)), anchor (a), closeBtn (c, "close", "Cerrar")
{
    setWantsKeyboardFocus (true);
    addAndMakeVisible (*content);
    closeBtn.framed = true;
    closeBtn.radiusScale = 0.5f;
    closeBtn.onClick = [this] { requestClose(); };
    addAndMakeVisible (closeBtn);
}

void Popover::requestClose()
{
    MessageManager::callAsync ([sp = SafePointer<Popover> (this)]
    {
        if (sp != nullptr && sp->onClose) sp->onClose();
    });
}

void Popover::resized()
{
    if (getParentComponent() == nullptr) return;
    const int pad = 18, head = 50;
    int w = content->getWidth() + pad * 2;
    int h = content->getHeight() + pad + head;
    const auto area = getLocalBounds().reduced (10);
    float scale = 1.0f;
    if (w > area.getWidth() || h > area.getHeight())
        scale = jmin ((float) area.getWidth() / (float) w, (float) area.getHeight() / (float) h);
    const int sw = roundToInt ((float) w * scale), sh = roundToInt ((float) h * scale);

    int x = jlimit (area.getX(), area.getRight() - sw, anchor.getCentreX() - sw / 2);
    int y = anchor.getBottom() + 10;
    if (y + sh > area.getBottom()) y = anchor.getY() - 10 - sh;
    y = jlimit (area.getY(), area.getBottom() - sh, y);
    panel = { x, y, sw, sh };

    content->setTransform (AffineTransform::scale (scale).translated ((float) x + (float) pad * scale, (float) y + (float) head * scale));
    content->setTopLeftPosition (0, 0);
    const int cb = roundToInt (34.0f * scale);
    closeBtn.setBounds (panel.getRight() - cb - roundToInt (12.0f * scale), panel.getY() + roundToInt (10.0f * scale), cb, cb);
}

void Popover::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    g.setColour (Colours::black.withAlpha (pal.night ? 0.35f : 0.16f));
    g.fillAll();

    auto p = panel.toFloat();
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (Colours::black.withAlpha (0.06f));
        g.fillRoundedRectangle (p.translated (0.0f, (float) i * 2.5f).expanded ((float) i * 1.5f), 22.0f);
    }
    const auto bg = pal.night ? pal.body1.brighter (0.06f) : pal.body1;
    ColourGradient gr (bg.brighter (0.04f), 0.0f, p.getY(), bg, 0.0f, p.getBottom(), false);
    g.setGradientFill (gr);
    g.fillRoundedRectangle (p, 20.0f);
    g.setColour (pal.edge);
    g.drawRoundedRectangle (p.reduced (0.5f), 20.0f, 1.0f);
    g.setColour (Colours::white.withAlpha (pal.night ? 0.08f : 0.8f));
    g.drawLine (p.getX() + 20.0f, p.getY() + 1.5f, p.getRight() - 20.0f, p.getY() + 1.5f, 1.0f);

    const float scale = (float) panel.getWidth() / (float) jmax (1, content->getWidth() + 36);
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (18.0f * scale, 2));
    g.drawText (title, p.withHeight (54.0f * scale).withTrimmedLeft (20.0f * scale).withTrimmedRight (60.0f * scale), Justification::centredLeft, true);
}

void Popover::mouseDown (const MouseEvent& e)
{
    if (! panel.contains (e.getPosition()))
        requestClose();
}

bool Popover::keyPressed (const KeyPress& k)
{
    if (k == KeyPress::escapeKey) { requestClose(); return true; }
    return false;
}

//==============================================================================
// Selector de color
//==============================================================================
ColourPicker::ColourPicker (UiContext& c) : ctx (c)
{
    selector.addChangeListener (this);
    addAndMakeVisible (selector);
    setSize (330, 262);
}

ColourPicker::~ColourPicker()
{
    selector.removeChangeListener (this);
}

void ColourPicker::setColour (Colour c)
{
    current = c;
    settingProgrammatically = true;
    selector.setCurrentColour (c.isTransparent() ? Colours::white : c, dontSendNotification);
    settingProgrammatically = false;
    repaint();
}

Rectangle<float> ColourPicker::swatchRect (int i) const
{
    const int perRow = 7;
    const float d = 30.0f, gap = (330.0f - d * (float) perRow) / (float) (perRow - 1);
    return { (float) (i % perRow) * (d + gap), (float) (i / perRow) * (d + 8.0f), d, d };
}

void ColourPicker::resized()
{
    selector.setBounds (0, 82, getWidth(), getHeight() - 82);
}

void ColourPicker::paint (Graphics& g)
{
    const auto& sw = swatches();
    for (int i = 0; i < (int) sw.size(); ++i)
    {
        auto r = swatchRect (i);
        g.setColour (Colours::black.withAlpha (0.15f));
        g.fillEllipse (r.translated (0.0f, 1.5f));
        g.setColour (sw[(size_t) i]);
        g.fillEllipse (r);
        g.setColour (Colours::black.withAlpha (0.18f));
        g.drawEllipse (r.reduced (0.5f), 1.0f);
        if (! current.isTransparent() && current.getARGB() == sw[(size_t) i].getARGB())
        {
            g.setColour (ctx.vis.pal.text);
            g.drawEllipse (r.expanded (3.0f), 2.0f);
        }
    }
}

void ColourPicker::mouseUp (const MouseEvent& e)
{
    const auto& sw = swatches();
    for (int i = 0; i < (int) sw.size(); ++i)
        if (swatchRect (i).contains (e.position))
        {
            setColour (sw[(size_t) i]);
            if (onColour) onColour (sw[(size_t) i]);
            return;
        }
}

void ColourPicker::changeListenerCallback (ChangeBroadcaster*)
{
    if (settingProgrammatically) return;
    current = selector.getCurrentColour().withAlpha (1.0f);
    repaint();
    if (onColour) onColour (current);
}

//==============================================================================
// Panel: color de toda la interfaz
//==============================================================================
InterfacePanel::InterfacePanel (UiContext& c, Prefs& p)
    : ctx (c), prefs (p),
      mode (c, { U8 ("Día"), "Noche", "Auto" }),
      target (c, { "Tono del cuerpo", "Acento" }),
      picker (c),
      resetBtn (c, "restore", U8 ("Quitar mis colores"), "Volver al original"),
      saveThemeBtn (c, "save", U8 ("Guardar estos colores como un tema tuyo"), "Guardar este tema"),
      confirmBtn (c, "save", "Guardar", "Guardar"),
      cancelBtn (c, "close", "Cancelar")
{
    for (Component* comp : std::initializer_list<Component*> { &mode, &target, &picker, &resetBtn, &saveThemeBtn })
        addAndMakeVisible (comp);
    addChildComponent (nameEditor);
    addChildComponent (confirmBtn);
    addChildComponent (cancelBtn);
    nameEditor.setFont (ctx.fonts.ui (16.0f, 1));
    nameEditor.setColour (TextEditor::backgroundColourId, ctx.vis.pal.night ? ctx.vis.pal.plate.brighter (0.1f) : Colours::white);
    nameEditor.setColour (TextEditor::textColourId, ctx.vis.pal.text);
    nameEditor.setColour (TextEditor::outlineColourId, ctx.vis.pal.edge);
    nameEditor.setColour (TextEditor::focusedOutlineColourId, ctx.vis.pal.accent);
    nameEditor.setColour (CaretComponent::caretColourId, ctx.vis.pal.accent);
    nameEditor.setIndents (12, 0);
    nameEditor.setJustification (Justification::centredLeft);
    confirmBtn.setActive (true);
    auto commit = [this]
    {
        const auto name = nameEditor.getText().trim();
        if (name.isEmpty()) return;
        auto t = prefs.captureTheme (name);
        bool replaced = false;
        for (auto& th : prefs.themes) if (th.name == name) { th = t; replaced = true; }
        if (! replaced) prefs.themes.push_back (t);
        prefs.save (false);
        setNaming (false);
    };
    nameEditor.onReturnKey = commit;
    confirmBtn.onClick = commit;
    cancelBtn.onClick = [this] { setNaming (false); };
    saveThemeBtn.onClick = [this] { setNaming (true); };
    mode.setSelected (prefs.dayNight);
    mode.onChange = [this] (int i) { prefs.dayNight = i; prefs.save(); repaint(); };
    target.onChange = [this] (int) { refreshPicker(); };
    picker.onColour = [this] (Colour col)
    {
        if (target.getSelected() == 0) prefs.uiTint = col; else prefs.uiAccent = col;
        prefs.save();
        repaint();
    };
    resetBtn.onClick = [this] { prefs.uiTint = Colour(); prefs.uiAccent = Colour(); prefs.save(); refreshPicker(); repaint(); };
    refreshPicker();
    setSize (740, 488);
}

void InterfacePanel::setNaming (bool n)
{
    naming = n;
    nameEditor.setVisible (n);
    confirmBtn.setVisible (n);
    cancelBtn.setVisible (n);
    saveThemeBtn.setVisible (! n);
    if (n)
    {
        nameEditor.setText ("Mi tema " + String ((int) prefs.themes.size() + 1), false);
        nameEditor.grabKeyboardFocus();
        nameEditor.selectAll();
    }
    repaint();
}

Rectangle<float> InterfacePanel::themeChip (int i) const
{
    // Los temas guardados, en fila (los que quepan antes del botón de guardar)
    return { (float) i * 150.0f, 438.0f, 142.0f, 44.0f };
}

void InterfacePanel::refreshPicker()
{
    picker.setColour (target.getSelected() == 0 ? prefs.uiTint : prefs.uiAccent);
}

Rectangle<float> InterfacePanel::lookTile (int i) const
{
    return { (float) i * 122.0f, 26.0f, 112.0f, 92.0f };
}

Rectangle<float> InterfacePanel::paletteChip (int i) const
{
    return { (float) (i % 3) * 122.0f, 214.0f + (float) (i / 3) * 44.0f, 112.0f, 36.0f };
}

void InterfacePanel::resized()
{
    mode.setBounds (0, 146, 356, 38);
    target.setBounds (390, 0, 350, 38);
    picker.setBounds (390, 52, 350, 290);
    resetBtn.setBounds (390, 356, 350, 42);
    saveThemeBtn.setBounds (getWidth() - 200, 438, 200, 44);
    nameEditor.setBounds (0, 438, getWidth() - 220, 44);
    confirmBtn.setBounds (getWidth() - 212, 438, 150, 44);
    cancelBtn.setBounds (getWidth() - 52, 438, 52, 44);
}

void InterfacePanel::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto label = [&] (const String& t, float y)
    {
        g.setColour (pal.dim);
        g.setFont (ctx.fonts.ui (14.0f, 2));
        g.drawText (t, Rectangle<float> (0.0f, y, 360.0f, 20.0f), Justification::centredLeft, false);
    };
    label ("Estilo", 0.0f);
    label (U8 ("Modo día o noche"), 124.0f);
    label (U8 ("Paletas rápidas"), 192.0f);
    label (naming ? U8 ("Nombre de tu tema (Enter para guardar)") : U8 ("Mis temas"), 412.0f);

    if (! naming)
    {
        const int maxChips = (getWidth() - 210) / 150;
        if (prefs.themes.empty())
        {
            g.setColour (pal.dim.withAlpha (0.8f));
            g.setFont (ctx.fonts.ui (14.0f, 1));
            g.drawText (U8 ("Aquí aparecen los temas que guardes (estilo, día/noche y colores)."),
                        Rectangle<float> (0.0f, 438.0f, (float) getWidth() - 214.0f, 44.0f), Justification::centredLeft, true);
        }
        for (int i = 0; i < (int) prefs.themes.size() && i < maxChips; ++i)
        {
            const auto& t = prefs.themes[(size_t) i];
            auto r = themeChip (i);
            draw::button (g, r, pal, 10.0f, 0.0f, false);
            auto in = r.withTrimmedBottom (2.5f).reduced (10.0f, 0.0f);
            auto x = in.removeFromRight (18.0f);
            const auto body = t.uiTint.isTransparent() ? prefs.palette (t.look).body2 : t.uiTint;
            const auto acc = t.uiAccent.isTransparent() ? prefs.palette (t.look).accent : t.uiAccent;
            auto dots = in.removeFromLeft (28.0f);
            g.setColour (body); g.fillEllipse (dots.getX(), dots.getCentreY() - 7.0f, 14.0f, 14.0f);
            g.setColour (acc);  g.fillEllipse (dots.getX() + 10.0f, dots.getCentreY() - 7.0f, 14.0f, 14.0f);
            g.setColour (confirmDelete == i ? Colour (0xffe5383b) : pal.btnText);
            g.setFont (draw::fitUi (ctx.fonts, confirmDelete == i ? U8 ("¿Borrar?") : t.name, 14.0f, in.getWidth(), 1));
            g.drawText (confirmDelete == i ? U8 ("¿Borrar?") : t.name, in, Justification::centredLeft, true);
            draw::icon (g, "close", x.withSizeKeepingCentre (14.0f, 14.0f), (confirmDelete == i ? Colour (0xffe5383b) : pal.btnText.withAlpha (0.5f)), 1.4f);
        }
    }

    const auto cur = lookIndex (prefs.look);
    for (int i = 0; i < 3; ++i)
    {
        auto r = lookTile (i);
        const Skin s = prefs.palette (looks()[(size_t) i].id);
        auto dev = r.withTrimmedBottom (22.0f);
        draw::device (g, dev, s, 0.35f, false);
        auto scr = dev.reduced (dev.getWidth() * 0.28f, dev.getHeight() * 0.2f).withTrimmedBottom (dev.getHeight() * 0.22f);
        g.setColour (s.screen);
        g.fillRoundedRectangle (scr, 4.0f);
        g.setColour (s.scrText);
        g.setFont (s.pixelScreen ? ctx.fonts.pixel (17.0f) : ctx.fonts.ui (15.0f, 2));
        g.drawText ("Dm", scr, Justification::centred, false);
        auto keys = dev.reduced (8.0f, 0.0f).withTrimmedTop (dev.getHeight() * 0.74f).withTrimmedBottom (6.0f);
        const float kw = keys.getWidth() / 8.0f;
        for (int kk = 0; kk < 8; ++kk)
        {
            g.setColour (kk == 2 || kk == 5 ? s.lit : s.keyW);
            g.fillRoundedRectangle (keys.getX() + (float) kk * kw + 1.0f, keys.getY(), kw - 2.0f, keys.getHeight(), 2.0f);
        }
        g.setColour (i == cur ? pal.accent : pal.edge);
        g.drawRoundedRectangle (dev.expanded (3.0f), 12.0f, i == cur ? 2.5f : 1.0f);
        g.setColour (i == cur ? pal.text : pal.dim);
        g.setFont (ctx.fonts.ui (14.0f, i == cur ? 2 : 1));
        g.drawText (looks()[(size_t) i].name, r.withTop (r.getBottom() - 20.0f), Justification::centred, false);
    }

    const auto& qp = quickPalettes();
    for (int i = 0; i < (int) qp.size(); ++i)
    {
        auto r = paletteChip (i);
        const bool sel = (i == 0 && prefs.uiTint.isTransparent() && prefs.uiAccent.isTransparent())
                         || (i > 0 && prefs.uiTint == qp[(size_t) i].tint && prefs.uiAccent == qp[(size_t) i].accent);
        draw::button (g, r, pal, 10.0f, 0.0f, false);
        auto in = r.withTrimmedBottom (2.5f).reduced (8.0f, 0.0f);
        auto dots = in.removeFromLeft (30.0f);
        const auto t = qp[(size_t) i].tint.isTransparent() ? pal.body2 : qp[(size_t) i].tint;
        const auto a = qp[(size_t) i].accent.isTransparent() ? pal.accent : qp[(size_t) i].accent;
        g.setColour (t);
        g.fillEllipse (dots.getX(), dots.getCentreY() - 7.0f, 14.0f, 14.0f);
        g.setColour (a);
        g.fillEllipse (dots.getX() + 11.0f, dots.getCentreY() - 7.0f, 14.0f, 14.0f);
        g.setColour (sel ? pal.accent : pal.btnText);
        g.setFont (draw::fitUi (ctx.fonts, qp[(size_t) i].name, 13.0f, in.getWidth(), sel ? 2 : 1));
        g.drawText (qp[(size_t) i].name, in, Justification::centredLeft, false);
    }
}

void InterfacePanel::mouseUp (const MouseEvent& e)
{
    if (! naming)
    {
        const int maxChips = (getWidth() - 210) / 150;
        for (int i = 0; i < (int) prefs.themes.size() && i < maxChips; ++i)
        {
            auto r = themeChip (i);
            if (! r.contains (e.position)) continue;
            if (e.position.x > r.getRight() - 28.0f)
            {
                if (confirmDelete == i) { prefs.themes.erase (prefs.themes.begin() + i); prefs.save (false); confirmDelete = -1; }
                else confirmDelete = i;
                repaint();
                return;
            }
            confirmDelete = -1;
            prefs.applyTheme (prefs.themes[(size_t) i]);
            mode.setSelected (prefs.dayNight);
            refreshPicker();
            repaint();
            return;
        }
        if (confirmDelete >= 0) { confirmDelete = -1; repaint(); }
    }
    for (int i = 0; i < 3; ++i)
        if (lookTile (i).contains (e.position))
        {
            prefs.look = looks()[(size_t) i].id;
            prefs.save();
            repaint();
            return;
        }
    const auto& qp = quickPalettes();
    for (int i = 0; i < (int) qp.size(); ++i)
        if (paletteChip (i).contains (e.position))
        {
            prefs.uiTint = qp[(size_t) i].tint;
            prefs.uiAccent = qp[(size_t) i].accent;
            prefs.save();
            refreshPicker();
            repaint();
            return;
        }
}

//==============================================================================
// Panel: color y estilo de las teclas
//==============================================================================
KeysPanel::KeysPanel (UiContext& c, Prefs& p)
    : ctx (c), prefs (p),
      style (c, keyStyleNames()),
      target (c, { "Encendidas", "Blancas", "Negras" }),
      rainbow (c),
      picker (c),
      resetBtn (c, "restore", U8 ("Colores de fábrica"), U8 ("Colores de fábrica"))
{
    for (Component* comp : std::initializer_list<Component*> { &style, &target, &rainbow, &picker, &resetBtn })
        addAndMakeVisible (comp);
    style.setSelected (prefs.effectiveKeyStyle());
    style.onChange = [this] (int i) { prefs.keyStyle = i; prefs.save(); repaint(); };
    rainbow.setOn (prefs.rainbow);
    rainbow.onChange = [this] (bool on) { prefs.rainbow = on; prefs.save(); };
    target.onChange = [this] (int) { refreshPicker(); };
    picker.onColour = [this] (Colour col)
    {
        const int t = target.getSelected();
        if (t == 0) { prefs.keyLit = col; prefs.rainbow = false; rainbow.setOn (false); }
        else if (t == 1) prefs.keyWhite = col;
        else prefs.keyBlack = col;
        prefs.save();
    };
    resetBtn.onClick = [this]
    {
        prefs.keyLit = prefs.keyWhite = prefs.keyBlack = Colour();
        prefs.rainbow = false;
        rainbow.setOn (false);
        prefs.save();
        refreshPicker();
    };
    refreshPicker();
    setSize (740, 360);
}

void KeysPanel::refreshPicker()
{
    const int t = target.getSelected();
    picker.setColour (t == 0 ? prefs.keyLit : (t == 1 ? prefs.keyWhite : prefs.keyBlack));
}

void KeysPanel::resized()
{
    style.setBounds (0, 26, 356, 40);
    rainbowRow = { 0, 116, 356, 40 };
    rainbow.setBounds (rainbowRow.getRight() - 60, rainbowRow.getY() + 6, 58, 30);
    target.setBounds (390, 0, 350, 38);
    picker.setBounds (390, 52, 350, 262);
    resetBtn.setBounds (390, 318, 350, 42);
}

void KeysPanel::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    g.setColour (pal.dim);
    g.setFont (ctx.fonts.ui (14.0f, 2));
    g.drawText ("Forma de las teclas", Rectangle<float> (0.0f, 0.0f, 356.0f, 20.0f), Justification::centredLeft, false);
    g.drawText ("Encendidas", Rectangle<float> (0.0f, 90.0f, 356.0f, 20.0f), Justification::centredLeft, false);
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (15.0f, 1));
    g.drawText (U8 ("Arcoíris: cada nota de un color"), rainbowRow.toFloat().withTrimmedRight (70.0f), Justification::centredLeft, false);

    // Muestra de cómo se ven
    auto demo = Rectangle<float> (0.0f, 184.0f, 356.0f, 150.0f);
    draw::recess (g, demo, pal, 14.0f, pal.keyBed);
    auto kr = demo.reduced (12.0f);
    const int whites = 8;
    const float ww = kr.getWidth() / (float) whites;
    const Skin s = prefs.palette();
    for (int i = 0; i < whites; ++i)
    {
        const bool lit = i == 0 || i == 2 || i == 4;
        auto r = Rectangle<float> (kr.getX() + (float) i * ww + 2.0f, kr.getY() + kr.getHeight() * 0.32f, ww - 4.0f, kr.getHeight() * 0.68f);
        g.setColour (lit ? s.noteColour (60 + i * 2) : s.keyW);
        g.fillRoundedRectangle (r, jmin (ww * 0.42f, 12.0f));
    }
    for (int i : { 1, 2, 4, 5, 6 })
    {
        const float x = kr.getX() + (float) i * ww;
        g.setColour (i == 4 ? s.noteColour (66) : s.keyB);
        g.fillEllipse (x - ww * 0.3f, kr.getY() + 4.0f, ww * 0.6f, ww * 0.6f);
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
    slider.setTextBoxStyle (Slider::TextBoxRight, false, 72, 26);
    slider.setColour (Slider::trackColourId, ctx.vis.pal.accent);
    slider.setColour (Slider::thumbColourId, ctx.vis.pal.accent);
    slider.setColour (Slider::backgroundColourId, ctx.vis.pal.plate.darker (0.1f));
    attach = std::make_unique<SliderParameterAttachment> (p, slider);
    addAndMakeVisible (slider);

    if (standalone && setBuffer)
    {
        for (int bs : { 128, 256, 512 })
        {
            auto* b = bufferButtons.add (new PushButton (c, {}, String (bs) + " muestras", String (bs)));
            b->onClick = [this, bs] { setBuffer (bs); repaint(); };
            addAndMakeVisible (b);
        }
    }
    setSize (440, standalone ? 270 : 190);
}

LatencyPanel::~LatencyPanel() = default;

void LatencyPanel::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (14.5f, 0));
    g.drawFittedText (U8 ("Si Midi Cat suena o se enciende antes que el otro instrumento con el que lo agrupas, "
                          "súbele hasta que vayan juntos. Retrasa el MIDI (sonido y luces) sin cambiar nada más."),
                      Rectangle<int> (0, 0, getWidth(), 66), Justification::topLeft, 3);
    if (standalone)
    {
        g.setColour (pal.dim);
        g.setFont (ctx.fonts.ui (13.5f, 1));
        g.drawText (U8 ("Buffer de audio (menos = responde más rápido)"), Rectangle<int> (0, 134, getWidth(), 20), Justification::centredLeft, false);
        if (deviceInfo)
            g.drawText (deviceInfo(), Rectangle<int> (0, 214, getWidth(), 20), Justification::centredLeft, true);
    }
}

void LatencyPanel::resized()
{
    slider.setBounds (0, 80, getWidth(), 36);
    int x = 0;
    for (auto* b : bufferButtons)
    {
        b->setBounds (x, 160, 100, 44);
        x += 112;
    }
}

//==============================================================================
TextPanel::TextPanel (UiContext& c, String t, int width) : ctx (c), text (std::move (t))
{
    const auto f = ctx.fonts.ui (15.0f, 0);
    GlyphArrangement ga;
    ga.addJustifiedText (f, text, 0.0f, 0.0f, (float) width, Justification::topLeft, 6.0f);
    setSize (width, (int) ga.getBoundingBox (0, -1, true).getBottom() + 10);
}

void TextPanel::paint (Graphics& g)
{
    g.setColour (ctx.vis.pal.text);
    GlyphArrangement ga;
    ga.addJustifiedText (ctx.fonts.ui (15.0f, 0), text, 0.0f, 0.0f, (float) getWidth(), Justification::topLeft, 6.0f);
    ga.draw (g);
}
