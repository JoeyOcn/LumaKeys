// Dibujo de los temas: luz de ambiente, partículas y paisajes.
#include "Views.h"

using namespace juce;

namespace
{
    // Tamaño relativo a la ventana normal (1200 x 760), para que las vistas previas también se vean bien
    float detailOf (Rectangle<float> r)
    {
        return jlimit (0.25f, 3.0f, std::sqrt (r.getWidth() * r.getHeight() / (1200.0f * 760.0f)));
    }

    int countFor (int density, Rectangle<float> r)
    {
        const float area = r.getWidth() * r.getHeight() / (1200.0f * 760.0f);
        return jmax (0, roundToInt ((float) density * jlimit (0.3f, 3.0f, area)));
    }

    void radial (Graphics& g, Rectangle<float> clip, Point<float> c, float radius, Colour col, float alpha)
    {
        if (alpha <= 0.0f || radius <= 0.0f) return;
        ColourGradient gr (col.withAlpha (alpha), c.x, c.y, col.withAlpha (0.0f), c.x + radius, c.y, true);
        gr.addColour (0.45, col.withAlpha (alpha * 0.45f));
        g.setGradientFill (gr);
        g.fillRect (clip.getIntersection (Rectangle<float> (c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f)));
    }

    void softDot (Graphics& g, Point<float> c, float coreR, float glowR, Colour col, float alpha)
    {
        if (glowR > coreR)
        {
            ColourGradient gr (col.withAlpha (alpha * 0.45f), c.x, c.y, col.withAlpha (0.0f), c.x + glowR, c.y, true);
            g.setGradientFill (gr);
            g.fillEllipse (c.x - glowR, c.y - glowR, glowR * 2.0f, glowR * 2.0f);
        }
        g.setColour (col.withAlpha (alpha));
        g.fillEllipse (c.x - coreR, c.y - coreR, coreR * 2.0f, coreR * 2.0f);
    }

    void drawBeams (Graphics& g, Rectangle<float> r, const ThemeLook& L, Random& rnd)
    {
        if (L.beamA <= 0.0f || L.beams <= 0) return;
        const float a = degreesToRadians (L.beamAngle);
        const Point<float> d (std::sin (a), std::cos (a));
        const Point<float> n (d.y, -d.x);
        const float len = std::hypot (r.getWidth(), r.getHeight()) * 1.1f;
        for (int i = 0; i < L.beams; ++i)
        {
            const float t = L.beams == 1 ? 0.22f : 0.08f + 0.84f * (float) i / (float) (L.beams - 1) + (rnd.nextFloat() - 0.5f) * 0.06f;
            const Point<float> o (r.getX() + r.getWidth() * t - d.x * r.getHeight() * 0.1f, r.getY() - 4.0f);
            const float w0 = r.getWidth() * (L.beams == 1 ? 0.07f : 0.025f + 0.03f * rnd.nextFloat());
            const float w1 = w0 * 1.9f;
            Path p;
            p.startNewSubPath (o + n * (w0 * 0.5f));
            p.lineTo (o + d * len + n * (w1 * 0.5f));
            p.lineTo (o + d * len - n * (w1 * 0.5f));
            p.lineTo (o - n * (w0 * 0.5f));
            p.closeSubPath();
            const float alpha = L.beamA * (L.beams == 1 ? 1.0f : 0.6f + 0.4f * rnd.nextFloat());
            const auto end = o + d * (len * (L.beams == 1 ? 0.9f : 0.75f));
            ColourGradient gr (L.beam.withAlpha (alpha), o.x, o.y, L.beam.withAlpha (0.0f), end.x, end.y, false);
            g.setGradientFill (gr);
            g.fillPath (p);
        }
    }

    Path petalShape()
    {
        Path p;
        p.startNewSubPath (0.0f, -1.0f);
        p.quadraticTo (0.95f, -0.55f, 0.12f, 1.0f);
        p.lineTo (0.0f, 0.82f);
        p.lineTo (-0.12f, 1.0f);
        p.quadraticTo (-0.95f, -0.55f, 0.0f, -1.0f);
        p.closeSubPath();
        return p;
    }

    void drawMotif (Graphics& g, Rectangle<float> r, const ThemeLook& L, Random& rnd)
    {
        if (L.motif == Motif::none || L.motifA <= 0.0f) return;
        const float s = jmax (0.45f, detailOf (r));
        const int n = countFor (L.density, r);
        const auto col = L.motifColour;
        auto rx = [&] { return r.getX() + rnd.nextFloat() * r.getWidth(); };
        auto ry = [&] { return r.getY() + rnd.nextFloat() * r.getHeight(); };

        switch (L.motif)
        {
            case Motif::stars:
                for (int i = 0; i < n; ++i)
                {
                    const Point<float> c (rx(), r.getY() + std::pow (rnd.nextFloat(), 1.4f) * r.getHeight());
                    const float rad = (0.45f + rnd.nextFloat() * 1.1f) * s;
                    const float a = L.motifA * (0.35f + 0.65f * rnd.nextFloat());
                    g.setColour (col.withAlpha (a));
                    g.fillEllipse (c.x - rad, c.y - rad, rad * 2.0f, rad * 2.0f);
                    if (rnd.nextInt (9) == 0)
                    {
                        const float l = (4.0f + rnd.nextFloat() * 4.0f) * s;
                        g.setColour (col.withAlpha (a * 0.6f));
                        g.drawLine (c.x - l, c.y, c.x + l, c.y, 0.7f * s);
                        g.drawLine (c.x, c.y - l, c.x, c.y + l, 0.7f * s);
                    }
                }
                break;

            case Motif::petals:
            {
                const auto petal = petalShape();
                for (int i = 0; i < n; ++i)
                {
                    const float size = (3.5f + rnd.nextFloat() * 4.5f) * s;
                    const auto t = AffineTransform::scale (size * 0.62f, size)
                                       .rotated (rnd.nextFloat() * MathConstants<float>::twoPi)
                                       .translated (rx(), ry());
                    g.setColour (col.withAlpha (L.motifA * (0.45f + 0.55f * rnd.nextFloat())));
                    g.fillPath (petal, t);
                }
                break;
            }

            case Motif::snow:
                for (int i = 0; i < n; ++i)
                {
                    const float rad = (0.8f + rnd.nextFloat() * rnd.nextFloat() * 2.6f) * s;
                    softDot (g, { rx(), ry() }, rad, rad * 2.6f, col, L.motifA * (0.4f + 0.6f * rnd.nextFloat()));
                }
                break;

            case Motif::rain:
            case Motif::lightning:
            {
                const float ang = degreesToRadians (14.0f);
                for (int i = 0; i < n; ++i)
                {
                    const float l = (12.0f + rnd.nextFloat() * 16.0f) * s;
                    const Point<float> a (rx(), ry());
                    const Point<float> b = a + Point<float> (std::sin (ang), std::cos (ang)) * l;
                    g.setColour (col.withAlpha (L.motifA * (0.35f + 0.65f * rnd.nextFloat())));
                    g.drawLine ({ a, b }, 0.8f * s);
                }
                if (L.motif == Motif::lightning)
                {
                    Path bolt;
                    Point<float> p (r.getX() + r.getWidth() * 0.1f, r.getY());
                    bolt.startNewSubPath (p);
                    Random br (7);
                    while (p.y < r.getY() + r.getHeight() * 0.62f)
                    {
                        p += Point<float> ((br.nextFloat() - 0.35f) * 26.0f * s, (14.0f + br.nextFloat() * 22.0f) * s);
                        bolt.lineTo (p);
                    }
                    g.setColour (col.withAlpha (0.10f));
                    g.strokePath (bolt, PathStrokeType (14.0f * s, PathStrokeType::mitered, PathStrokeType::rounded));
                    g.setColour (col.withAlpha (0.25f));
                    g.strokePath (bolt, PathStrokeType (5.0f * s, PathStrokeType::mitered, PathStrokeType::rounded));
                    g.setColour (Colours::white.withAlpha (0.75f));
                    g.strokePath (bolt, PathStrokeType (1.4f * s, PathStrokeType::mitered, PathStrokeType::rounded));
                }
                break;
            }

            case Motif::fireflies:
                for (int i = 0; i < n; ++i)
                {
                    const float rad = (1.1f + rnd.nextFloat() * 1.0f) * s;
                    softDot (g, { rx(), r.getY() + r.getHeight() * (0.15f + 0.85f * rnd.nextFloat()) }, rad, rad * 7.0f, col,
                             L.motifA * (0.5f + 0.5f * rnd.nextFloat()));
                }
                break;

            case Motif::bubbles:
                for (int i = 0; i < n; ++i)
                {
                    const float rad = (1.5f + rnd.nextFloat() * rnd.nextFloat() * 6.0f) * s;
                    const Point<float> c (rx(), ry());
                    const float a = L.motifA * (0.35f + 0.65f * rnd.nextFloat());
                    softDot (g, c, 0.0f, rad * 3.0f, col, a * 0.7f);
                    g.setColour (col.withAlpha (a));
                    g.drawEllipse (c.x - rad, c.y - rad, rad * 2.0f, rad * 2.0f, 0.8f * s);
                    g.fillEllipse (c.x - rad * 0.45f, c.y - rad * 0.55f, rad * 0.35f, rad * 0.35f);
                }
                break;

            case Motif::embers:
                for (int i = 0; i < n; ++i)
                {
                    const Point<float> c (rx(), r.getY() + r.getHeight() * (1.0f - 0.9f * std::pow (rnd.nextFloat(), 1.8f)));
                    const float rad = (0.7f + rnd.nextFloat() * 1.3f) * s;
                    const float a = L.motifA * (0.4f + 0.6f * rnd.nextFloat());
                    softDot (g, c, rad, rad * 5.0f, col, a);
                    if (rnd.nextInt (3) == 0)
                    {
                        g.setColour (col.withAlpha (a * 0.4f));
                        g.drawLine (c.x, c.y, c.x - 2.0f * s, c.y + 9.0f * s, rad);
                    }
                }
                break;

            case Motif::lanterns:
                for (int i = 0; i < n; ++i)
                {
                    const float w = (7.0f + rnd.nextFloat() * 6.0f) * s, h = w * 1.3f;
                    const Point<float> c (rx(), r.getY() + r.getHeight() * (0.05f + 0.75f * rnd.nextFloat()));
                    const float a = L.motifA * (0.55f + 0.45f * rnd.nextFloat());
                    softDot (g, c, 0.0f, w * 4.5f, col, a * 0.9f);
                    auto body = Rectangle<float> (w, h).withCentre (c);
                    g.setColour (col.brighter (0.3f).withAlpha (a));
                    g.fillRoundedRectangle (body, w * 0.45f);
                    g.setColour (Colours::black.withAlpha (a * 0.45f));
                    g.fillRect (body.withHeight (h * 0.12f).reduced (w * 0.18f, 0.0f));
                    g.fillRect (body.withTrimmedTop (h * 0.88f).reduced (w * 0.18f, 0.0f));
                }
                break;

            case Motif::dapple:
                for (int i = 0; i < jmax (1, L.density); ++i)
                {
                    const Point<float> c (rx(), ry());
                    const float rad = jmax (r.getWidth(), r.getHeight()) * (0.09f + 0.1f * rnd.nextFloat());
                    radial (g, r, c, rad, col, L.motifA);
                }
                break;

            case Motif::aurora:
            {
                const Colour cols[3] = { col, Colour (0xff4de0ff), Colour (0xffb07cff) };
                for (int b = 0; b < jmax (1, L.density); ++b)
                {
                    const auto c = cols[b % 3];
                    const float base = r.getY() + r.getHeight() * (-0.02f + 0.07f * (float) b);
                    const float amp = r.getHeight() * (0.03f + 0.025f * rnd.nextFloat());
                    const float thick = r.getHeight() * (0.13f + 0.06f * rnd.nextFloat());
                    const float f = (1.4f + rnd.nextFloat()) * MathConstants<float>::twoPi / r.getWidth();
                    const float ph = rnd.nextFloat() * 6.0f;
                    Path p;
                    const int steps = 48;
                    for (int i = 0; i <= steps; ++i)
                    {
                        const float x = r.getX() + r.getWidth() * (float) i / (float) steps;
                        const float y = base + amp * std::sin (x * f + ph) + amp * 0.4f * std::sin (x * f * 2.3f + ph * 1.7f);
                        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
                    }
                    for (int i = steps; i >= 0; --i)
                    {
                        const float x = r.getX() + r.getWidth() * (float) i / (float) steps;
                        const float y = base + thick + amp * std::sin (x * f + ph);
                        p.lineTo (x, y);
                    }
                    p.closeSubPath();
                    ColourGradient gr (c.withAlpha (L.motifA * 0.6f), 0.0f, base - amp, c.withAlpha (0.0f), 0.0f, base + thick, false);
                    gr.addColour (0.2, c.withAlpha (L.motifA));
                    gr.addColour (0.5, c.withAlpha (L.motifA * 0.35f));
                    g.setGradientFill (gr);
                    g.fillPath (p);
                }
                break;
            }

            case Motif::none: break;
        }
    }
}

namespace draw
{
    void themeLight (Graphics& g, Rectangle<float> r, const Palette& pal)
    {
        const auto& L = pal.look();
        if (L.isEmpty()) return;
        Random rnd ((int64) (pal.themeIdx * 101 + (pal.night ? 7 : 3)));

        // En skins oscuras la noche no necesita oscurecer tanto: ya lo son
        const float soften = (pal.night && pal.dark) ? 0.6f : 1.0f;
        if (L.washA > 0.0f)
        {
            const float a = L.wash.getPerceivedBrightness() < 0.3f ? L.washA * soften : L.washA;
            g.setColour (L.wash.withAlpha (a));
            g.fillRect (r);
        }
        if (L.skyTopA > 0.0f || L.skyBottomA > 0.0f)
        {
            ColourGradient sky (L.skyTop.withAlpha (L.skyTopA), r.getX(), r.getY(), L.skyBottom.withAlpha (L.skyBottomA), r.getX(), r.getBottom(), false);
            g.setGradientFill (sky);
            g.fillRect (r);
        }
        const float big = jmax (r.getWidth(), r.getHeight());
        radial (g, r, { r.getX() + r.getWidth() * L.glowPos.x, r.getY() + r.getHeight() * L.glowPos.y }, big * L.glowR, L.glow, L.glowA);
        radial (g, r, { r.getX() + r.getWidth() * L.glow2Pos.x, r.getY() + r.getHeight() * L.glow2Pos.y }, big * L.glow2R, L.glow2, L.glow2A);
        drawBeams (g, r, L, rnd);

        // Sol, luna o eclipse: un orbe de luz suave (sin bordes duros que estorben)
        if (L.discA > 0.0f)
        {
            const Point<float> c (r.getX() + r.getWidth() * L.discPos.x, r.getY() + r.getHeight() * L.discPos.y);
            const float rad = jmin (r.getWidth(), r.getHeight()) * L.discR;
            if (L.ring)
            {
                const float R = rad * 3.2f;
                ColourGradient ring (L.disc.withAlpha (0.0f), c.x, c.y, L.disc.withAlpha (0.0f), c.x + R, c.y, true);
                ring.addColour (0.48, L.disc.withAlpha (0.0f));
                ring.addColour (0.56, L.disc.withAlpha (L.discA * 0.55f));
                ring.addColour (0.62, L.disc.withAlpha (L.discA * 0.22f));
                ring.addColour (0.8, L.disc.withAlpha (L.discA * 0.06f));
                g.setGradientFill (ring);
                g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
            }
            else
            {
                const float R = rad * 3.0f;
                ColourGradient orb (L.disc.withAlpha (L.discA * 0.5f), c.x, c.y, L.disc.withAlpha (0.0f), c.x + R, c.y, true);
                orb.addColour (0.22, L.disc.withAlpha (L.discA * 0.32f));
                orb.addColour (0.4, L.disc.withAlpha (L.discA * 0.1f));
                g.setGradientFill (orb);
                g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
            }
        }

        drawMotif (g, r, L, rnd);

        if (L.vignette > 0.0f)
        {
            const auto c = r.getCentre();
            ColourGradient v (Colours::black.withAlpha (0.0f), c.x, c.y, Colours::black.withAlpha (L.vignette * soften), c.x + std::hypot (r.getWidth(), r.getHeight()) * 0.55f, c.y, true);
            v.addColour (0.55, Colours::black.withAlpha (0.0f));
            g.setGradientFill (v);
            g.fillRect (r);
        }
    }

    void themeScene (Graphics& g, Rectangle<float> r, const Palette& pal)
    {
        const auto& L = pal.look();
        if (! L.hasBackground()) return;
        Random rnd ((int64) (pal.themeIdx * 31 + 5));
        const float s = detailOf (r);
        const auto sc = L.sceneColour;
        const float W = r.getWidth(), H = r.getHeight(), X = r.getX(), Y = r.getY();

        if (L.scene == Scene::none || L.sceneA <= 0.0f) return;
        switch (L.scene)
        {
            case Scene::fuji:
            {
                const float base = Y + H * 0.66f, peakX = X + W * 0.52f, peakY = Y + H * 0.2f, top = W * 0.035f;
                Path m;
                m.startNewSubPath (X - W * 0.05f, base);
                m.quadraticTo (X + W * 0.30f, base - H * 0.05f, peakX - top, peakY);
                m.lineTo (peakX + top, peakY);
                m.quadraticTo (X + W * 0.74f, base - H * 0.05f, X + W * 1.05f, base);
                m.closeSubPath();
                ColourGradient gr (sc.withAlpha (L.sceneA), 0.0f, peakY, sc.withAlpha (L.sceneA * 0.45f), 0.0f, base, false);
                g.setGradientFill (gr);
                g.fillPath (m);
                // Nieve en la cima
                Path snow;
                snow.startNewSubPath (peakX - top, peakY);
                snow.lineTo (peakX + top, peakY);
                snow.lineTo (peakX + top * 3.4f, peakY + H * 0.09f);
                for (int i = 0; i < 5; ++i)
                    snow.lineTo (peakX + top * (2.4f - 1.2f * (float) i), peakY + H * (i % 2 == 0 ? 0.06f : 0.1f));
                snow.lineTo (peakX - top * 3.4f, peakY + H * 0.09f);
                snow.closeSubPath();
                g.setColour (Colours::white.withAlpha (jmin (0.85f, L.sceneA * (pal.night ? 0.7f : 1.6f))));
                g.fillPath (snow);
                break;
            }

            case Scene::city:
            {
                float x = X - 4.0f;
                const Colour winCols[3] = { Colour (0xffff4fa3), Colour (0xff2de2ff), Colour (0xffffc46b) };
                while (x < X + W)
                {
                    const float bw = (28.0f + rnd.nextFloat() * 50.0f) * s;
                    const float bh = H * (0.1f + rnd.nextFloat() * 0.26f);
                    auto b = Rectangle<float> (x, Y + H * 0.62f - bh, bw - 3.0f * s, bh);
                    ColourGradient bg (sc.withAlpha (L.sceneA), 0.0f, b.getY(), sc.withAlpha (L.sceneA * 0.3f), 0.0f, b.getBottom(), false);
                    g.setGradientFill (bg);
                    g.fillRect (b);
                    for (float wy = b.getY() + 6.0f * s; wy < b.getY() + bh * 0.5f; wy += 10.0f * s)
                        for (float wx = b.getX() + 4.0f * s; wx < b.getRight() - 6.0f * s; wx += 9.0f * s)
                            if (rnd.nextInt (pal.night ? 6 : 12) == 0)
                            {
                                g.setColour (winCols[rnd.nextInt (3)].withAlpha (pal.night ? 0.55f : 0.18f));
                                g.fillRect (wx, wy, 3.0f * s, 4.0f * s);
                            }
                    x += bw;
                }
                break;
            }

            case Scene::bamboo:
            {
                for (int i = 0; i < 14; ++i)
                {
                    const bool edge = i < 10;
                    float fx = edge ? (i % 2 == 0 ? rnd.nextFloat() * 0.16f : 0.84f + rnd.nextFloat() * 0.16f) : 0.2f + rnd.nextFloat() * 0.6f;
                    const float x = X + W * fx;
                    const float w = (7.0f + rnd.nextFloat() * 8.0f) * s;
                    const float a = L.sceneA * (edge ? 1.0f : 0.45f) * (0.6f + 0.4f * rnd.nextFloat());
                    g.setColour (sc.withAlpha (a));
                    g.fillRect (x, Y, w, H);
                    float ny = Y + rnd.nextFloat() * 60.0f * s;
                    while (ny < Y + H)
                    {
                        g.setColour (sc.darker (0.6f).withAlpha (jmin (1.0f, a * 1.3f)));
                        g.fillRect (x - 1.0f * s, ny, w + 2.0f * s, 2.5f * s);
                        if (rnd.nextInt (3) == 0)
                        {
                            Path leaf;
                            leaf.addEllipse (0.0f, -2.0f * s, 26.0f * s, 4.5f * s);
                            const bool right = rnd.nextBool();
                            g.setColour (sc.withAlpha (a * 0.9f));
                            g.fillPath (leaf, AffineTransform::rotation (right ? 0.35f : MathConstants<float>::pi - 0.35f)
                                                  .translated (right ? x + w : x, ny));
                        }
                        ny += (55.0f + rnd.nextFloat() * 40.0f) * s;
                    }
                }
                break;
            }

            case Scene::dunes:
            {
                for (int layer = 0; layer < 3; ++layer)
                {
                    const float base = Y + H * (0.42f + 0.11f * (float) layer);
                    const float amp = H * (0.05f - 0.01f * (float) layer);
                    const float ph = rnd.nextFloat() * 6.0f;
                    Path p;
                    p.startNewSubPath (X, Y + H);
                    for (int i = 0; i <= 40; ++i)
                    {
                        const float x = X + W * (float) i / 40.0f;
                        p.lineTo (x, base - amp * std::sin ((float) i / 40.0f * 4.2f + ph));
                    }
                    p.lineTo (X + W, Y + H);
                    p.closeSubPath();
                    const float a = L.sceneA * (0.45f + 0.28f * (float) layer);
                    ColourGradient gr (sc.brighter (0.25f * (float) (2 - layer)).withAlpha (a), 0.0f, base - amp, sc.withAlpha (a * 0.6f), 0.0f, Y + H, false);
                    g.setGradientFill (gr);
                    g.fillPath (p);
                }
                break;
            }

            case Scene::none: break;
        }
    }
}

//==============================================================================
void ThemeCache::draw (Graphics& g, Rectangle<float> r, const Palette& pal, float scale)
{
    if (pal.look().isEmpty() || r.isEmpty()) return;
    const int w = jmax (1, roundToInt (r.getWidth() * scale)), h = jmax (1, roundToInt (r.getHeight() * scale));
    const String k = String (pal.themeIdx) + (pal.night ? "n" : "d") + String (w) + "x" + String (h);
    if (k != key || ! image.isValid())
    {
        image = Image (Image::ARGB, w, h, true);
        Graphics gi (image);
        gi.addTransform (AffineTransform::scale ((float) w / r.getWidth(), (float) h / r.getHeight()));
        draw::themeLight (gi, { 0.0f, 0.0f, r.getWidth(), r.getHeight() }, pal);
        key = k;
    }
    g.drawImage (image, r);
}
