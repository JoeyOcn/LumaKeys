#include "Views.h"

using namespace juce;

//==============================================================================
// Ayudas de dibujo
//==============================================================================
namespace draw
{
    void glassPanel (Graphics& g, Rectangle<float> r, const Palette& pal, float radius)
    {
        g.setColour (pal.panel);
        g.fillRoundedRectangle (r, radius);

        ColourGradient shine (pal.panelShine, r.getX(), r.getY(),
                              pal.panelShine.withAlpha (0.0f), r.getX(), r.getY() + r.getHeight() * 0.45f, false);
        g.setGradientFill (shine);
        g.fillRoundedRectangle (r, radius);

        g.setColour (pal.panelEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    }

    Path star (Point<float> c, float r, float inner)
    {
        Path p;
        const float ri = r * inner;
        for (int i = 0; i < 8; ++i)
        {
            const float ang = MathConstants<float>::pi * 0.25f * (float) i - MathConstants<float>::halfPi;
            const float rr = (i % 2 == 0) ? r : ri;
            const auto pt = c + Point<float> (std::cos (ang) * rr, std::sin (ang) * rr);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        p.closeSubPath();
        return p;
    }

    void title (Graphics& g, Rectangle<float> area, const String& text, const Palette& pal, const Fonts& fonts, Colour textColour)
    {
        const float h = area.getHeight();
        const auto c = Point<float> (area.getX() + h * 0.35f, area.getCentreY());
        ColourGradient grad (pal.a, c.x - 5, c.y - 5, pal.b, c.x + 5, c.y + 5, false);
        g.setGradientFill (grad);
        g.fillPath (star (c, h * 0.36f, 0.3f));

        g.setColour (textColour);
        g.setFont (fonts.ui (h * 0.74f, 1).withExtraKerningFactor (0.06f));
        g.drawText (text, area.withTrimmedLeft (h * 0.9f), Justification::centredLeft, false);
    }

    void stars (Graphics& g, Rectangle<float> r, int count, int seed, Colour tint, float alphaScale)
    {
        Random rng (seed);
        for (int i = 0; i < count; ++i)
        {
            const float x = r.getX() + rng.nextFloat() * r.getWidth();
            const float y = r.getY() + rng.nextFloat() * r.getHeight();
            const float s = 0.5f + rng.nextFloat() * rng.nextFloat() * 1.8f;
            const float a = (0.15f + 0.65f * rng.nextFloat()) * alphaScale;
            const auto col = (rng.nextInt (5) == 0 ? tint : Colours::white).withAlpha (a);
            g.setColour (col);
            if (rng.nextInt (16) == 0)
                g.fillPath (star ({ x, y }, s * 2.6f, 0.22f));
            else
                g.fillEllipse (x - s * 0.5f, y - s * 0.5f, s, s);
        }
    }
}

static void drawGlyphF (Graphics& g, const Font& f, juce_wchar cp, float x, float baseline)
{
    GlyphArrangement ga;
    ga.addLineOfText (f, String::charToString (cp), x, baseline);
    ga.draw (g);
}

static float glyphWidth (const Font& f, juce_wchar cp)
{
    return GlyphArrangement::getStringWidth (f, String::charToString (cp));
}

static float textWidth (const Font& f, const String& s)
{
    return GlyphArrangement::getStringWidth (f, s);
}

static Font fitFont (const Fonts& fonts, const String& text, float maxH, float maxW, int weight)
{
    float h = maxH;
    auto f = fonts.ui (h, weight);
    while (h > 10.0f && textWidth (f, text) > maxW)
    {
        h *= 0.92f;
        f = fonts.ui (h, weight);
    }
    return f;
}

//==============================================================================
// Pentagrama
//==============================================================================
void StaffView::paint (Graphics& g)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;
    auto b = getLocalBounds().toFloat();
    draw::glassPanel (g, b, pal);
    draw::title (g, b.reduced (16.0f, 12.0f).removeFromTop (18.0f), "Pentagrama", pal, ctx.fonts, pal.dim);

    auto area = b.reduced (16.0f, 0.0f).withTrimmedTop (44.0f).withTrimmedBottom (40.0f);
    const float sp = jmin (area.getHeight() / 17.5f, area.getWidth() / 12.5f);
    const float midY = area.getCentreY();
    auto yOf = [&] (int d) { return midY - (float) (d - 28) * sp * 0.5f; };

    const float lineW = jmax (1.0f, sp * 0.09f);
    const float left = area.getX() + sp * 1.4f;
    const float right = area.getRight() - sp * 0.4f;
    const auto inkCol = pal.ink;

    g.setColour (inkCol.withAlpha (0.55f));
    for (int d : { 30, 32, 34, 36, 38, 18, 20, 22, 24, 26 })
        g.fillRect (left, yOf (d) - lineW * 0.5f, right - left, lineW);
    g.fillRect (left - lineW * 0.5f, yOf (38), lineW * 1.4f, yOf (18) - yOf (38));
    g.fillRect (right - lineW, yOf (38), lineW, yOf (18) - yOf (38));

    const auto mf = ctx.fonts.music (sp * 4.0f);

    // Llave del sistema
    {
        const float sysH = yOf (18) - yOf (38);
        const auto braceFont = ctx.fonts.music (sysH);
        const float bw = glyphWidth (braceFont, 0xE000);
        g.setColour (inkCol.withAlpha (0.85f));
        drawGlyphF (g, braceFont, 0xE000, left - bw - sp * 0.25f, yOf (18));
    }

    g.setColour (inkCol);
    drawGlyphF (g, mf, 0xE050, left + sp * 0.5f, yOf (32)); // clave de sol
    drawGlyphF (g, mf, 0xE062, left + sp * 0.5f, yOf (24)); // clave de fa

    // Armadura
    static const int trebleSharp[7] = { 38, 35, 39, 36, 33, 37, 34 };
    static const int trebleFlat[7]  = { 34, 37, 33, 36, 32, 35, 31 };
    static const int bassSharp[7]   = { 24, 21, 25, 22, 19, 23, 20 };
    static const int bassFlat[7]    = { 20, 23, 19, 22, 18, 21, 17 };

    float x = left + sp * 3.7f;
    const auto accGlyph = v.sig.flats ? (juce_wchar) 0xE260 : (juce_wchar) 0xE262;
    const float accStep = v.sig.flats ? sp * 1.0f : sp * 1.1f;
    for (int i = 0; i < v.sig.count; ++i)
    {
        drawGlyphF (g, mf, accGlyph, x, yOf (v.sig.flats ? trebleFlat[i] : trebleSharp[i]));
        drawGlyphF (g, mf, accGlyph, x, yOf (v.sig.flats ? bassFlat[i] : bassSharp[i]));
        x += accStep;
    }
    const float sigEnd = x;

    // Notas
    struct N { int midi; int d; bool treble; int acc; bool showAcc; bool sustainedOnly; bool shifted; int ottava; float xOff; int accCol; };
    std::vector<N> ns;
    for (auto m : v.active)
    {
        const auto s = theory::spell (m, v.sig);
        N n;
        n.midi = m;
        n.d = s.diatonic();
        n.treble = m >= 60;
        n.acc = s.acc;
        n.showAcc = s.acc != v.sig.acc[(size_t) s.letter];
        n.sustainedOnly = v.down[(size_t) m] == 0;
        n.shifted = false;
        n.ottava = 0;
        if (n.treble && n.d > 45) { n.d -= 7; n.ottava = 1; }
        if (! n.treble && n.d < 11) { n.d += 7; n.ottava = -1; }
        n.xOff = 0.0f;
        n.accCol = 0;
        ns.push_back (n);
    }
    std::sort (ns.begin(), ns.end(), [] (const N& a, const N& b2) { return a.d < b2.d; });

    const float headW = glyphWidth (mf, 0xE0A2);
    const float noteX = jmax (sigEnd + sp * 3.2f, (left + right) * 0.5f + sp * 0.8f);

    // Segundas: desplazar la cabeza
    for (size_t i = 1; i < ns.size(); ++i)
    {
        auto& a = ns[i - 1];
        auto& c = ns[i];
        if (a.treble == c.treble && c.d - a.d <= 1 && ! a.shifted)
        {
            c.shifted = true;
            c.xOff = headW * 0.98f;
        }
    }

    // Columnas de alteraciones (de arriba hacia abajo)
    {
        std::vector<std::pair<int, int>> placed; // d, columna
        for (int i = (int) ns.size() - 1; i >= 0; --i)
        {
            auto& n = ns[(size_t) i];
            if (! n.showAcc) continue;
            int col = 0;
            for (;;)
            {
                bool clash = false;
                for (auto& p : placed)
                    if (p.second == col && std::abs (p.first - n.d) < 6) clash = true;
                if (! clash) break;
                ++col;
            }
            n.accCol = col;
            placed.push_back ({ n.d, col });
        }
    }

    for (auto& n : ns)
    {
        const float nx = noteX + n.xOff;
        const float ny = yOf (n.d);

        // Líneas adicionales
        g.setColour (inkCol.withAlpha (0.7f));
        auto ledger = [&] (int d)
        {
            g.fillRect (noteX - sp * 0.45f, yOf (d) - lineW * 0.5f, headW + sp * 0.9f + (n.shifted ? headW : 0.0f), lineW);
        };
        if (n.treble)
        {
            if (n.d <= 28) for (int d = 28; d >= n.d; d -= 2) ledger (d);
            if (n.d >= 40) for (int d = 40; d <= n.d; d += 2) ledger (d);
        }
        else
        {
            if (n.d >= 28) for (int d = 28; d <= n.d; d += 2) ledger (d);
            if (n.d <= 16) for (int d = 16; d >= n.d; d -= 2) ledger (d);
        }

        auto col = pal.noteColour (n.midi);
        if (! pal.dark) col = col.withMultipliedBrightness (0.78f).withMultipliedSaturation (1.15f);
        const float alpha = n.sustainedOnly ? 0.45f : 1.0f;

        // brillo
        {
            const auto c = Point<float> (nx + headW * 0.5f, ny);
            ColourGradient glow (col.withAlpha (0.45f * alpha), c.x, c.y, col.withAlpha (0.0f), c.x + sp * 1.8f, c.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (c.x - sp * 1.8f, c.y - sp * 1.8f, sp * 3.6f, sp * 3.6f);
        }

        g.setColour (col.withAlpha (alpha));
        drawGlyphF (g, mf, 0xE0A2, nx, ny);

        if (n.showAcc)
        {
            const juce_wchar ag = n.acc > 0 ? 0xE262 : (n.acc < 0 ? 0xE260 : 0xE261);
            g.setColour (inkCol.withAlpha (alpha));
            drawGlyphF (g, mf, ag, noteX - sp * 1.35f - (float) n.accCol * sp * 1.15f, ny);
        }

        if (n.ottava != 0)
        {
            g.setColour (pal.dim);
            g.setFont (ctx.fonts.ui (sp * 1.1f, 1));
            const float ty = n.ottava > 0 ? ny - sp * 2.4f : ny + sp * 1.2f;
            g.drawText (n.ottava > 0 ? "8va" : "8vb", Rectangle<float> (nx - sp, ty, headW + sp * 2.0f, sp * 1.3f), Justification::centred);
        }
    }

    // Pie: armadura en palabras
    String sigText;
    if (v.sig.count == 0)
        sigText = U8 ("Armadura: sin alteraciones");
    else
    {
        static const int sharpOrder[7] = { 3, 0, 4, 1, 5, 2, 6 };
        static const int flatOrder[7]  = { 6, 2, 5, 1, 4, 0, 3 };
        StringArray names;
        for (int i = 0; i < v.sig.count; ++i)
            names.add (theory::letterName (v.sig.flats ? flatOrder[i] : sharpOrder[i], v.latin) + (v.sig.flats ? "b" : "#"));
        sigText = "Armadura: " + String (v.sig.count) + (v.sig.flats ? (v.sig.count == 1 ? U8 (" bemol") : U8 (" bemoles"))
                                                                    : (v.sig.count == 1 ? U8 (" sostenido") : U8 (" sostenidos")))
                  + "  (" + names.joinIntoString (" ") + ")";
    }
    g.setColour (pal.dim);
    g.setFont (ctx.fonts.ui (13.0f));
    g.drawText (sigText, b.reduced (18.0f, 12.0f).removeFromBottom (20.0f), Justification::centredLeft, true);
}

//==============================================================================
// Pantalla central
//==============================================================================
Rectangle<float> ChordScreen::chipArea() const
{
    auto b = getLocalBounds().toFloat().reduced (16.0f);
    const float h = jlimit (54.0f, 74.0f, b.getHeight() * 0.2f);
    return b.removeFromBottom (h);
}

Rectangle<float> ChordScreen::chipBounds (int i, int count) const
{
    auto a = chipArea().reduced (8.0f, 8.0f);
    const float gap = 6.0f;
    const float w = (a.getWidth() - gap * (float) (count - 1)) / (float) jmax (1, count);
    return { a.getX() + (float) i * (w + gap), a.getY(), w, a.getHeight() };
}

void ChordScreen::paint (Graphics& g)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;
    auto& fonts = ctx.fonts;
    auto b = getLocalBounds().toFloat();
    const float radius = 18.0f;

    ColourGradient bg (pal.screenTop, b.getX(), b.getY(), pal.screenBottom, b.getX(), b.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (b, radius);

    {
        Graphics::ScopedSaveState ss (g);
        Path clip;
        clip.addRoundedRectangle (b, radius);
        g.reduceClipRegion (clip);
        draw::stars (g, b, 90, 7, pal.b, 0.75f);

        // Resplandor detrás del acorde (más intenso si hay notas)
        const float glowA = v.active.empty() ? 0.12f : 0.26f;
        const auto c = b.getCentre().translated (0.0f, -b.getHeight() * 0.08f);
        ColourGradient glow (pal.a.withAlpha (glowA), c.x, c.y, pal.b.withAlpha (0.0f), c.x + b.getWidth() * 0.42f, c.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (c.x - b.getWidth() * 0.42f, c.y - b.getHeight() * 0.34f, b.getWidth() * 0.84f, b.getHeight() * 0.68f);
    }

    g.setColour (pal.screenEdge);
    g.drawRoundedRectangle (b.reduced (0.6f), radius, 1.2f);

    auto inner = b.reduced (18.0f, 12.0f);
    auto head = inner.removeFromTop (18.0f);
    draw::title (g, head, "Acorde", pal, fonts, pal.screenDim);

    {
        auto keyText = theory::keyTitle (v.keyRoot, v.scale, v.latin);
        if (v.autoKey) keyText = (v.autoHasData ? U8 ("Auto · ") : U8 ("Auto · esperando notas · ")) + keyText;
        g.setFont (fonts.ui (13.0f, 1));
        g.setColour (pal.screenDim);
        g.drawText (keyText, head, Justification::centredRight, true);
    }

    // Área de contenido
    auto content = b.reduced (22.0f, 0.0f).withTrimmedTop (40.0f);
    content.setBottom (chipArea().getY() - 6.0f);

    const int count = (int) v.active.size();
    String big, sub, chip;
    int bigWeight = 2;

    if (count == 0)
    {
        big = theory::keyTitle (v.keyRoot, v.scale, v.latin);
        StringArray sn;
        for (auto s : theory::scales()[(size_t) v.scale].steps)
            sn.add (theory::pcName (v.keyRoot + s, v.sig, v.latin));
        sub = "Escala: " + sn.joinIntoString ("  ");
        chip = U8 ("Toca una nota o un acorde");
        bigWeight = 1;
    }
    else if (count == 1)
    {
        const int m = v.active.front();
        big = theory::noteName (m, v.sig, v.latin, true);
        const double hz = 440.0 * std::pow (2.0, (m - 69) / 12.0);
        sub = theory::noteName (m, v.sig, true, false) + (m == 60 ? U8 (" central") : String()) + U8 ("  ·  ") + String (hz, 1) + " Hz";
        chip = theory::noteDegree (theory::pcOf (m), v.keyRoot, v.scale);
    }
    else if (v.chord.valid && (count >= 3 || v.chord.suffix == "5"))
    {
        big = v.chord.symbol;
        sub = theory::pcName (v.chord.root, v.sig, true) + " " + v.chord.quality;
        if (v.chord.inversion != "estado fundamental")
            sub << U8 ("  ·  ") << v.chord.inversion;
        chip = v.degree.roman + U8 ("  ·  ") + v.degree.function;
    }
    else if (count == 2)
    {
        const int iv = v.active[1] - v.active[0];
        big = theory::intervalName (iv);
        sub = theory::noteName (v.active[0], v.sig, v.latin) + U8 ("  →  ") + theory::noteName (v.active[1], v.sig, v.latin)
              + U8 ("  ·  ") + String (iv) + (iv == 1 ? " semitono" : " semitonos");
        chip = U8 ("Intervalo");
    }
    else
    {
        StringArray sn;
        for (int pc = 0; pc < 12; ++pc)
            if (v.activeMask & (1u << (unsigned) pc)) sn.add (theory::pcName (pc, v.sig, v.latin));
        big = sn.joinIntoString (" ");
        sub = String (count) + U8 (" notas · sin nombre común de acorde");
        chip = U8 ("Prueba quitar una nota");
        bigWeight = 1;
    }

    // Texto grande con degradado
    const float bigMaxH = jlimit (28.0f, 120.0f, content.getHeight() * 0.34f);
    const auto bigFont = fitFont (fonts, big, bigMaxH, content.getWidth() * 0.94f, bigWeight);
    const float bigH = bigFont.getHeight();

    const float blockH = bigH + 30.0f + 34.0f + 40.0f;
    float y = content.getY() + jmax (0.0f, (content.getHeight() - blockH) * 0.45f);

    auto bigRect = Rectangle<float> (content.getX(), y, content.getWidth(), bigH);
    {
        ColourGradient tg (Colours::white, bigRect.getX(), bigRect.getY(),
                           pal.a.interpolatedWith (Colours::white, 0.45f), bigRect.getX(), bigRect.getBottom(), false);
        g.setGradientFill (tg);
        g.setFont (bigFont);
        g.drawText (big, bigRect, Justification::centred, false);
    }
    y += bigH + 4.0f;

    g.setColour (pal.screenDim);
    g.setFont (fitFont (fonts, sub, 16.0f, content.getWidth() * 0.96f, 0));
    g.drawText (sub, Rectangle<float> (content.getX(), y, content.getWidth(), 24.0f), Justification::centred, false);
    y += 32.0f;

    if (chip.isNotEmpty())
    {
        const auto cf = fonts.ui (13.5f, 1);
        const float cw = textWidth (cf, chip) + 28.0f;
        auto cr = Rectangle<float> (content.getCentreX() - cw * 0.5f, y, cw, 26.0f);
        g.setColour (pal.a.withAlpha (count > 0 ? 0.20f : 0.08f));
        g.fillRoundedRectangle (cr, 13.0f);
        g.setColour (pal.a.withAlpha (count > 0 ? 0.65f : 0.30f));
        g.drawRoundedRectangle (cr.reduced (0.5f), 13.0f, 1.0f);
        g.setColour (pal.screenText.withAlpha (count > 0 ? 1.0f : 0.7f));
        g.setFont (cf);
        g.drawText (chip, cr, Justification::centred, false);
        y += 36.0f;
    }

    // Pastillas con las notas tocadas
    if (count > 0)
    {
        const auto pf = fonts.ui (13.0f, 1);
        std::vector<std::pair<String, Colour>> pills;
        for (auto m : v.active)
            pills.push_back ({ theory::noteName (m, v.sig, v.latin), v.pal.noteColour (m) });
        if (pills.size() > 10) pills.resize (10);

        float total = 0.0f;
        for (auto& p : pills) total += textWidth (pf, p.first) + 30.0f;
        total += 6.0f * (float) (pills.size() - 1);
        float px = content.getCentreX() - total * 0.5f;
        for (auto& p : pills)
        {
            const float w = textWidth (pf, p.first) + 30.0f;
            auto r = Rectangle<float> (px, y, w, 24.0f);
            g.setColour (Colours::white.withAlpha (0.07f));
            g.fillRoundedRectangle (r, 12.0f);
            g.setColour (p.second);
            g.fillEllipse (r.getX() + 9.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f);
            g.setColour (pal.screenText);
            g.setFont (pf);
            g.drawText (p.first, r.withTrimmedLeft (20.0f).withTrimmedRight (6.0f), Justification::centred, false);
            px += w + 6.0f;
        }
    }

    // Acordes de la tonalidad
    auto ca = chipArea();
    g.setColour (Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (ca, 12.0f);
    g.setColour (Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (ca.reduced (0.5f), 12.0f, 1.0f);

    const int nChips = (int) v.diatonic.size();
    for (int i = 0; i < nChips; ++i)
    {
        const auto& d = v.diatonic[(size_t) i];
        auto r = chipBounds (i, nChips);
        const bool activeChip = v.chord.valid && v.chord.root == d.rootPc && (d.mask & ~v.activeMask) == 0;

        if (activeChip)
        {
            ColourGradient cg (pal.a.withAlpha (0.55f), r.getX(), r.getY(), pal.b.withAlpha (0.55f), r.getRight(), r.getBottom(), false);
            g.setGradientFill (cg);
            g.fillRoundedRectangle (r, 9.0f);
        }
        else if (i == hoverChip)
        {
            g.setColour (Colours::white.withAlpha (0.10f));
            g.fillRoundedRectangle (r, 9.0f);
        }

        g.setColour (activeChip ? pal.screenText : pal.screenDim);
        g.setFont (fonts.ui (jmin (13.0f, r.getHeight() * 0.28f), 1));
        g.drawText (d.roman, r.withHeight (r.getHeight() * 0.45f).withTrimmedTop (3.0f), Justification::centred, false);
        g.setColour (pal.screenText);
        g.setFont (fitFont (fonts, d.symbol, jmin (17.0f, r.getHeight() * 0.36f), r.getWidth() - 6.0f, 1));
        g.drawText (d.symbol, r.withTrimmedTop (r.getHeight() * 0.42f).withTrimmedBottom (4.0f), Justification::centred, false);
    }
}

void ChordScreen::mouseDown (const MouseEvent& e)
{
    const int n = (int) ctx.vis.diatonic.size();
    for (int i = 0; i < n; ++i)
        if (chipBounds (i, n).contains (e.position))
        {
            if (onChipClicked) onChipClicked (i);
            return;
        }
}

void ChordScreen::mouseMove (const MouseEvent& e)
{
    int h = -1;
    const int n = (int) ctx.vis.diatonic.size();
    for (int i = 0; i < n; ++i)
        if (chipBounds (i, n).contains (e.position)) h = i;
    if (h != hoverChip)
    {
        hoverChip = h;
        setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
        repaint();
    }
}

void ChordScreen::mouseExit (const MouseEvent&)
{
    if (hoverChip != -1) { hoverChip = -1; repaint(); }
}

//==============================================================================
// Círculo de quintas
//==============================================================================
void CircleView::geometry (Point<float>& c, float& r) const
{
    auto b = getLocalBounds().toFloat().withTrimmedTop (38.0f).withTrimmedBottom (30.0f).reduced (14.0f, 4.0f);
    c = b.getCentre();
    r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
}

static int circleIndexOfPc (int pc) { return (theory::pcOf (pc) * 7) % 12; }
static int majorPcAt (int i)        { return (i * 7) % 12; }

bool CircleView::hitTestRing (Point<float> p, int& index, bool& minor) const
{
    Point<float> c; float r;
    geometry (c, r);
    const float dist = p.getDistanceFrom (c);
    if (dist > r || dist < r * 0.36f) return false;
    float ang = std::atan2 (p.y - c.y, p.x - c.x) + MathConstants<float>::halfPi; // 0 arriba
    if (ang < 0) ang += MathConstants<float>::twoPi;
    index = (int) std::floor ((ang + MathConstants<float>::pi / 12.0f) / (MathConstants<float>::twoPi / 12.0f)) % 12;
    minor = dist < r * 0.68f;
    return true;
}

void CircleView::paint (Graphics& g)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;
    auto& fonts = ctx.fonts;
    auto b = getLocalBounds().toFloat();
    draw::glassPanel (g, b, pal);
    draw::title (g, b.reduced (16.0f, 12.0f).removeFromTop (18.0f), U8 ("Círculo de quintas"), pal, fonts, pal.dim);

    Point<float> c; float r;
    geometry (c, r);
    const float rOut = r, rMid = r * 0.68f, rIn = r * 0.40f;
    const float seg = MathConstants<float>::twoPi / 12.0f;

    const auto& sc = theory::scales()[(size_t) v.scale];
    const int parentMajor = theory::pcOf (v.keyRoot - sc.parentOffset);
    const int region = circleIndexOfPc (parentMajor);
    const int tonicIndex = sc.minorFlavour ? circleIndexOfPc (theory::pcOf (v.keyRoot + 3)) : circleIndexOfPc (v.keyRoot);
    const bool tonicMinor = sc.minorFlavour;

    auto ringSeg = [&] (int i, float r0, float r1)
    {
        Path p;
        const float a0 = (float) i * seg - seg * 0.5f;
        p.addPieSegment (c.x - r1, c.y - r1, r1 * 2.0f, r1 * 2.0f, a0, a0 + seg, r0 / r1);
        return p;
    };

    for (int i = 0; i < 12; ++i)
    {
        const int dist = std::min ((i - region + 12) % 12, (region - i + 12) % 12);
        for (int ring = 0; ring < 2; ++ring)
        {
            const bool minorRing = ring == 1;
            auto p = minorRing ? ringSeg (i, rIn, rMid) : ringSeg (i, rMid, rOut);
            const bool isTonic = (i == tonicIndex && minorRing == tonicMinor);

            if (isTonic)
            {
                ColourGradient tg (pal.a, c.x, c.y - rOut, pal.b, c.x, c.y + rOut, false);
                g.setGradientFill (tg);
                g.fillPath (p);
            }
            else if (dist <= 1)
            {
                g.setColour (pal.a.withAlpha (minorRing ? 0.10f : 0.16f));
                g.fillPath (p);
            }
            else
            {
                g.setColour (pal.dark ? Colours::white.withAlpha (minorRing ? 0.015f : 0.03f)
                                      : Colour (0xff0b1a3a).withAlpha (minorRing ? 0.02f : 0.035f));
                g.fillPath (p);
            }

            if (i == hoverIndex && minorRing == hoverMinor)
            {
                g.setColour (pal.text.withAlpha (0.10f));
                g.fillPath (p);
            }

            g.setColour (pal.faint);
            g.strokePath (p, PathStrokeType (1.0f));

            // etiquetas
            const float ang = (float) i * seg - MathConstants<float>::halfPi;
            const float lr = minorRing ? (rIn + rMid) * 0.5f : (rMid + rOut) * 0.5f;
            const auto lp = c + Point<float> (std::cos (ang) * lr, std::sin (ang) * lr);
            const int pc = minorRing ? theory::pcOf (majorPcAt (i) + 9) : majorPcAt (i);
            // Nombre preferido según la armadura de esa tonalidad
            const auto sigHere = theory::keySignature (pc, minorRing ? 1 : 0);
            auto name = theory::pcName (pc, sigHere, v.latin) + (minorRing ? "m" : "");
            const float fh = minorRing ? jmax (10.0f, r * 0.085f) : jmax (12.0f, r * 0.11f);
            g.setFont (fonts.ui (fh, isTonic ? 2 : 1));
            g.setColour (isTonic ? Colours::white : (minorRing ? pal.dim : pal.text));
            g.drawText (name, Rectangle<float> (lp.x - 40.0f, lp.y - fh * 0.6f, 80.0f, fh * 1.2f), Justification::centred, false);
        }
    }

    // Constelación: las notas que suenan, unidas como estrellas
    std::vector<int> idx;
    for (int pc = 0; pc < 12; ++pc)
        if (v.activeMask & (1u << (unsigned) pc)) idx.push_back (circleIndexOfPc (pc));
    std::sort (idx.begin(), idx.end());

    if (! idx.empty())
    {
        const float sr = (rMid + rOut) * 0.5f + (rOut - rMid) * 0.32f;
        std::vector<Point<float>> pts;
        for (auto i : idx)
        {
            const float ang = (float) i * seg - MathConstants<float>::halfPi;
            pts.push_back (c + Point<float> (std::cos (ang) * sr, std::sin (ang) * sr));
        }

        if (pts.size() >= 2)
        {
            Path poly;
            poly.startNewSubPath (pts[0]);
            for (size_t k = 1; k < pts.size(); ++k) poly.lineTo (pts[k]);
            if (pts.size() >= 3) poly.closeSubPath();
            g.setColour (pal.a.withAlpha (pal.dark ? 0.16f : 0.12f));
            if (pts.size() >= 3) g.fillPath (poly);
            ColourGradient lg (pal.a.withAlpha (0.9f), c.x - r, c.y, pal.b.withAlpha (0.9f), c.x + r, c.y, false);
            g.setGradientFill (lg);
            g.strokePath (poly, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded));
        }

    }

    // Centro: tonalidad y armadura
    {
        g.setColour (pal.dark ? Colour (0xff070917).withAlpha (0.55f) : Colours::white.withAlpha (0.7f));
        g.fillEllipse (c.x - rIn, c.y - rIn, rIn * 2.0f, rIn * 2.0f);
        g.setColour (pal.faint);
        g.drawEllipse (c.x - rIn, c.y - rIn, rIn * 2.0f, rIn * 2.0f, 1.0f);

        const auto root = theory::pcName (v.keyRoot, v.sig, v.latin);
        const auto scName = String (CharPointer_UTF8 (sc.shortName));
        g.setColour (pal.text);
        g.setFont (fonts.ui (rIn * 0.46f, 2));
        g.drawText (root, Rectangle<float> (c.x - rIn, c.y - rIn * 0.62f, rIn * 2.0f, rIn * 0.58f), Justification::centred, false);
        g.setColour (pal.dim);
        g.setFont (fitFont (fonts, scName, rIn * 0.22f, rIn * 1.7f, 0));
        g.drawText (scName, Rectangle<float> (c.x - rIn, c.y - rIn * 0.02f, rIn * 2.0f, rIn * 0.3f), Justification::centred, false);
        const String sigTxt = v.sig.count == 0 ? U8 ("sin alteraciones") : String (v.sig.count) + (v.sig.flats ? "b" : "#");
        g.setFont (fonts.ui (rIn * 0.2f, 1));
        g.setColour (pal.a);
        g.drawText (sigTxt, Rectangle<float> (c.x - rIn, c.y + rIn * 0.3f, rIn * 2.0f, rIn * 0.3f), Justification::centred, false);
    }

    if (! idx.empty())
    {
        const float sr = (rMid + rOut) * 0.5f + (rOut - rMid) * 0.32f;
        std::vector<Point<float>> pts;
        for (auto i : idx)
        {
            const float ang = (float) i * seg - MathConstants<float>::halfPi;
            pts.push_back (c + Point<float> (std::cos (ang) * sr, std::sin (ang) * sr));
        }
        for (size_t k = 0; k < pts.size(); ++k)
        {
            const int pc = majorPcAt (idx[k]);
            const auto col = pal.noteColour (60 + pc);
            ColourGradient glow (col.withAlpha (0.55f), pts[k].x, pts[k].y, col.withAlpha (0.0f), pts[k].x + 16.0f, pts[k].y, true);
            g.setGradientFill (glow);
            g.fillEllipse (pts[k].x - 16.0f, pts[k].y - 16.0f, 32.0f, 32.0f);
            g.setColour (Colours::white);
            g.fillPath (draw::star (pts[k], 7.0f, 0.3f));
        }
    }

    // Pie
    auto foot = b.reduced (18.0f, 12.0f).removeFromBottom (18.0f);
    g.setFont (fonts.ui (13.0f));
    g.setColour (pal.dim);
    const int n = theory::countBits (v.activeMask);
    g.drawText (n == 1 ? String ("1 nota") : String (n) + " notas", foot, Justification::centredRight, false);
    g.drawText ("Clic para elegir tonalidad", foot, Justification::centredLeft, false);
}

void CircleView::mouseDown (const MouseEvent& e)
{
    int i; bool minor;
    if (hitTestRing (e.position, i, minor) && onKeyClicked)
    {
        const int pc = minor ? theory::pcOf (majorPcAt (i) + 9) : majorPcAt (i);
        onKeyClicked (pc, minor);
    }
}

void CircleView::mouseMove (const MouseEvent& e)
{
    int i = -1; bool minor = false;
    if (! hitTestRing (e.position, i, minor)) i = -1;
    if (i != hoverIndex || minor != hoverMinor)
    {
        hoverIndex = i;
        hoverMinor = minor;
        setMouseCursor (i >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
        repaint();
    }
}

void CircleView::mouseExit (const MouseEvent&)
{
    if (hoverIndex != -1) { hoverIndex = -1; repaint(); }
}

//==============================================================================
// Teclado
//==============================================================================
static bool isBlackPc (int pc) { return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }

static int whiteIndex (int note)
{
    int n = 0;
    for (int k = KeyboardView::lowNote; k < note; ++k)
        if (! isBlackPc (k % 12)) ++n;
    return n;
}

Rectangle<float> KeyboardView::keysArea() const
{
    auto b = getLocalBounds().toFloat().reduced (12.0f, 0.0f);
    const float beam = b.getHeight() * 0.26f;
    return b.withTrimmedTop (beam).withTrimmedBottom (24.0f);
}

KeyboardView::KeyGeom KeyboardView::keyRect (int note) const
{
    const auto a = keysArea();
    const float ww = a.getWidth() / 52.0f;
    if (! isBlackPc (note % 12))
        return { Rectangle<float> (a.getX() + (float) whiteIndex (note) * ww, a.getY(), ww, a.getHeight()), false };

    const float bw = ww * 0.62f;
    const float x = a.getX() + (float) (whiteIndex (note - 1) + 1) * ww - bw * 0.5f;
    return { Rectangle<float> (x, a.getY(), bw, a.getHeight() * 0.62f), true };
}

int KeyboardView::noteAt (Point<float> p, float& velocity) const
{
    const auto a = keysArea();
    if (! a.contains (p)) return -1;
    for (int n = lowNote; n <= highNote; ++n)
    {
        const auto k = keyRect (n);
        if (k.black && k.r.contains (p))
        {
            velocity = jlimit (0.2f, 1.0f, 0.3f + 0.7f * (p.y - k.r.getY()) / k.r.getHeight());
            return n;
        }
    }
    for (int n = lowNote; n <= highNote; ++n)
    {
        const auto k = keyRect (n);
        if (! k.black && k.r.contains (p))
        {
            velocity = jlimit (0.2f, 1.0f, 0.3f + 0.7f * (p.y - k.r.getY()) / k.r.getHeight());
            return n;
        }
    }
    return -1;
}

void KeyboardView::paint (Graphics& g)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;
    auto b = getLocalBounds().toFloat();
    draw::glassPanel (g, b, pal);

    const auto a = keysArea();
    const float beamTop = b.getY() + 10.0f;

    auto levelOf = [&] (int n)
    {
        const float d = v.down[(size_t) n] / 127.0f;
        const float s = v.sus[(size_t) n] / 127.0f * 0.55f;
        return jmax (d > 0 ? 0.55f + 0.45f * d : 0.0f, s > 0 ? 0.25f + 0.3f * s : 0.0f);
    };

    // Rayos de luz sobre las teclas encendidas
    for (int n = lowNote; n <= highNote; ++n)
    {
        const float lvl = levelOf (n);
        const float fl = v.flash[(size_t) n];
        if (lvl <= 0.0f && fl <= 0.01f) continue;
        const auto k = keyRect (n);
        const auto col = pal.noteColour (n);
        const float inten = jlimit (0.0f, 1.0f, lvl * 0.7f + fl * 0.6f);
        const float w = k.r.getWidth() * (k.black ? 1.3f : 0.9f);
        auto beam = Rectangle<float> (k.r.getCentreX() - w * 0.5f, beamTop, w, a.getY() - beamTop);
        ColourGradient bg (col.withAlpha (0.0f), beam.getX(), beam.getY(), col.withAlpha (0.75f * inten), beam.getX(), beam.getBottom(), false);
        g.setGradientFill (bg);
        g.fillRect (beam);
        // halo
        const auto c = Point<float> (k.r.getCentreX(), a.getY());
        const float hr = k.r.getWidth() * (1.6f + fl * 1.4f);
        ColourGradient halo (col.withAlpha (0.5f * inten), c.x, c.y, col.withAlpha (0.0f), c.x + hr, c.y, true);
        g.setGradientFill (halo);
        g.fillEllipse (c.x - hr, c.y - hr, hr * 2.0f, hr * 2.0f);
    }

    // Marco del teclado
    g.setColour (pal.dark ? Colour (0xff04050c) : Colour (0xff1b2033));
    g.fillRoundedRectangle (a.expanded (4.0f, 4.0f).withTrimmedTop (2.0f), 6.0f);

    const bool showScale = v.highlight;

    // Teclas blancas
    for (int n = lowNote; n <= highNote; ++n)
    {
        const auto k = keyRect (n);
        if (k.black) continue;
        auto r = k.r.reduced (0.6f, 0.0f);
        ColourGradient wg (pal.keyWhite, r.getX(), r.getY(), pal.keyWhiteShade, r.getX(), r.getBottom(), false);
        g.setGradientFill (wg);
        g.fillRoundedRectangle (r.withTrimmedTop (-4.0f), 3.0f);

        const int pc = n % 12;
        const bool inScale = showScale && (v.scaleMask & (1u << (unsigned) pc)) != 0;
        if (inScale)
        {
            g.setColour (pal.a.withAlpha (0.10f));
            g.fillRect (r.withTrimmedTop (r.getHeight() * 0.62f));
        }

        const float lvl = levelOf (n);
        if (lvl > 0.0f)
        {
            const auto col = pal.noteColour (n);
            ColourGradient lg (col.interpolatedWith (Colours::white, 0.35f).withAlpha (lvl), r.getX(), r.getY(),
                               col.withAlpha (lvl), r.getX(), r.getBottom(), false);
            g.setGradientFill (lg);
            g.fillRoundedRectangle (r.withTrimmedTop (-4.0f), 3.0f);
        }

        if (inScale)
        {
            const bool isRoot = pc == theory::pcOf (v.keyRoot);
            const float d = jmin (r.getWidth() * 0.42f, 9.0f);
            const auto dc = Point<float> (r.getCentreX(), r.getBottom() - d * 1.9f - (pc == 0 ? 14.0f : 0.0f));
            g.setColour ((lvl > 0 ? Colours::white : pal.a).withAlpha (isRoot ? 0.95f : 0.6f));
            if (isRoot) g.drawEllipse (dc.x - d * 0.6f, dc.y - d * 0.6f, d * 1.2f, d * 1.2f, 1.6f);
            g.fillEllipse (dc.x - d * 0.3f, dc.y - d * 0.3f, d * 0.6f, d * 0.6f);
        }

        if (pc == 0)
        {
            g.setColour (lvl > 0 ? Colour (0xff10131f) : pal.keyEdge);
            g.setFont (ctx.fonts.ui (jmin (12.0f, r.getWidth() * 0.55f), 1));
            g.drawText ((v.latin ? String ("Do") : String ("C")) + String (n / 12 - 1),
                        r.withTrimmedTop (r.getHeight() - 18.0f), Justification::centred, false);
        }
    }

    // Fieltro
    {
        auto felt = Rectangle<float> (a.getX(), a.getY() - 3.0f, a.getWidth(), 3.0f);
        ColourGradient fg (pal.a.withAlpha (0.8f), felt.getX(), felt.getY(), pal.b.withAlpha (0.8f), felt.getRight(), felt.getY(), false);
        g.setGradientFill (fg);
        g.fillRect (felt);
    }

    // Teclas negras
    for (int n = lowNote; n <= highNote; ++n)
    {
        const auto k = keyRect (n);
        if (! k.black) continue;
        auto r = k.r;
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (r.translated (1.2f, 1.5f), 2.5f);
        ColourGradient bg (pal.keyBlackTop, r.getX(), r.getY(), pal.keyBlack, r.getX(), r.getBottom(), false);
        g.setGradientFill (bg);
        g.fillRoundedRectangle (r, 2.5f);
        g.setColour (Colours::white.withAlpha (0.10f));
        g.fillRect (r.reduced (r.getWidth() * 0.18f, 0.0f).withTrimmedTop (r.getHeight() - 6.0f).withHeight (2.0f));

        const float lvl = levelOf (n);
        if (lvl > 0.0f)
        {
            const auto col = pal.noteColour (n);
            ColourGradient lg (col.withAlpha (lvl), r.getX(), r.getY(), col.interpolatedWith (Colours::black, 0.25f).withAlpha (lvl), r.getX(), r.getBottom(), false);
            g.setGradientFill (lg);
            g.fillRoundedRectangle (r, 2.5f);
        }

        const int pc = n % 12;
        if (showScale && (v.scaleMask & (1u << (unsigned) pc)) != 0)
        {
            const bool isRoot = pc == theory::pcOf (v.keyRoot);
            const float d = jmin (r.getWidth() * 0.5f, 8.0f);
            const auto dc = Point<float> (r.getCentreX(), r.getBottom() - d * 1.4f);
            g.setColour ((lvl > 0 ? Colours::white : pal.a.interpolatedWith (Colours::white, 0.3f)).withAlpha (isRoot ? 1.0f : 0.75f));
            if (isRoot) g.drawEllipse (dc.x - d * 0.6f, dc.y - d * 0.6f, d * 1.2f, d * 1.2f, 1.4f);
            g.fillEllipse (dc.x - d * 0.3f, dc.y - d * 0.3f, d * 0.6f, d * 0.6f);
        }
    }

    // Pie
    auto foot = Rectangle<float> (a.getX(), a.getBottom() + 4.0f, a.getWidth(), 18.0f);
    g.setFont (ctx.fonts.ui (12.5f));
    g.setColour (pal.dim);
    g.drawText (v.latin ? U8 ("La0 – Do8 · 88 teclas") : U8 ("A0 – C8 · 88 teclas"), foot, Justification::centredLeft, false);

    const auto pedalTxt = String ("Pedal");
    const auto pf = ctx.fonts.ui (12.5f);
    const float tw = textWidth (pf, pedalTxt);
    auto pr = foot.removeFromRight (tw + 20.0f);
    g.drawText (pedalTxt, pr.withTrimmedRight (18.0f), Justification::centredRight, false);
    g.setColour (v.pedal ? pal.a : pal.faint);
    g.fillEllipse (pr.getRight() - 11.0f, pr.getCentreY() - 4.5f, 9.0f, 9.0f);
}

void KeyboardView::mouseDown (const MouseEvent& e)
{
    float vel = 0.8f;
    const int n = noteAt (e.position, vel);
    if (n >= 0)
    {
        mouseNote = n;
        if (onNote) onNote (n, vel, true);
    }
}

void KeyboardView::mouseDrag (const MouseEvent& e)
{
    float vel = 0.8f;
    const int n = noteAt (e.position, vel);
    if (n != mouseNote)
    {
        if (mouseNote >= 0 && onNote) onNote (mouseNote, 0.0f, false);
        mouseNote = n;
        if (n >= 0 && onNote) onNote (n, vel, true);
    }
}

void KeyboardView::mouseUp (const MouseEvent&)
{
    if (mouseNote >= 0 && onNote) onNote (mouseNote, 0.0f, false);
    mouseNote = -1;
}

//==============================================================================
// Controles
//==============================================================================
void Switch::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat();
    auto pill = b.removeFromRight (46.0f).withSizeKeepingCentre (46.0f, 26.0f);

    g.setColour (pal.dim);
    g.setFont (ctx.fonts.ui (14.0f, 1));
    g.drawText (text, b.withTrimmedRight (10.0f), Justification::centredRight, true);

    if (on)
    {
        ColourGradient cg (pal.a, pill.getX(), pill.getY(), pal.b, pill.getRight(), pill.getBottom(), false);
        g.setGradientFill (cg);
    }
    else
        g.setColour (pal.dark ? Colours::white.withAlpha (0.12f) : Colour (0xff0b1a3a).withAlpha (0.14f));
    g.fillRoundedRectangle (pill, 13.0f);

    const float d = 20.0f;
    const float x = on ? pill.getRight() - d - 3.0f : pill.getX() + 3.0f;
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillEllipse (x, pill.getCentreY() - d * 0.5f + 1.0f, d, d);
    g.setColour (Colours::white);
    g.fillEllipse (x, pill.getCentreY() - d * 0.5f, d, d);
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

    const auto lf = ctx.fonts.ui (14.0f, 1);
    g.setFont (lf);
    g.setColour (pal.dim);
    const float lw = textWidth (lf, text) + 14.0f;
    g.drawText (text, b.removeFromLeft (lw), Justification::centredLeft, false);

    const float d = jmin (b.getHeight() - 6.0f, 44.0f);
    auto k = b.removeFromLeft (d).withSizeKeepingCentre (d, d);
    b.removeFromLeft (12.0f);

    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (15.0f, 1));
    g.drawText (valueText, b, Justification::centredLeft, false);

    const auto c = k.getCentre();
    const float r = d * 0.5f;
    const float a0 = -MathConstants<float>::pi * 0.75f, a1 = MathConstants<float>::pi * 0.75f;
    const float av = a0 + (a1 - a0) * value;

    Path track;
    track.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, a0, a1, true);
    g.setColour (pal.faint);
    g.strokePath (track, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

    Path arc;
    arc.addCentredArc (c.x, c.y, r - 2.0f, r - 2.0f, 0.0f, a0, av, true);
    ColourGradient ag (pal.a, k.getX(), k.getBottom(), pal.b, k.getRight(), k.getY(), false);
    g.setGradientFill (ag);
    g.strokePath (arc, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

    const float ir = r - 7.0f;
    ColourGradient body (pal.dark ? Colour (0xff2a3150) : Colours::white, c.x, c.y - ir,
                         pal.dark ? Colour (0xff121626) : Colour (0xffd7deec), c.x, c.y + ir, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - ir, c.y - ir, ir * 2.0f, ir * 2.0f);
    g.setColour (pal.panelEdge);
    g.drawEllipse (c.x - ir, c.y - ir, ir * 2.0f, ir * 2.0f, 1.0f);

    const auto tip = c + Point<float> (std::sin (av) * (ir - 4.0f), -std::cos (av) * (ir - 4.0f));
    g.setColour (pal.a);
    g.fillEllipse (tip.x - 2.5f, tip.y - 2.5f, 5.0f, 5.0f);
}

void Knob::mouseDown (const MouseEvent&)
{
    dragStart = value;
    if (onGestureStart) onGestureStart();
}

void Knob::mouseDrag (const MouseEvent& e)
{
    const float sens = e.mods.isShiftDown() ? 800.0f : 220.0f;
    const float nv = jlimit (0.0f, 1.0f, dragStart - (float) e.getDistanceFromDragStartY() / sens);
    if (onValue) onValue (nv);
}

void Knob::mouseUp (const MouseEvent&)
{
    if (onGestureEnd) onGestureEnd();
}

void Knob::mouseDoubleClick (const MouseEvent&)
{
    if (onReset) onReset();
}

void Knob::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    if (onGestureStart) onGestureStart();
    if (onValue) onValue (jlimit (0.0f, 1.0f, value + w.deltaY * 0.12f));
    if (onGestureEnd) onGestureEnd();
}

//------------------------------------------------------------------------------
void DropButton::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (pal.dark ? Colours::white.withAlpha (hover ? 0.10f : 0.06f) : Colours::white.withAlpha (hover ? 0.95f : 0.75f));
    g.fillRoundedRectangle (b, 10.0f);
    g.setColour (hover ? pal.a.withAlpha (0.6f) : pal.panelEdge);
    g.drawRoundedRectangle (b, 10.0f, 1.0f);

    auto inner = b.reduced (12.0f, 6.0f);
    auto chev = inner.removeFromRight (14.0f);
    g.setColour (pal.dim);
    g.setFont (ctx.fonts.ui (11.5f, 1).withExtraKerningFactor (0.05f));
    g.drawText (text, inner.removeFromTop (inner.getHeight() * 0.42f), Justification::bottomLeft, false);
    g.setColour (pal.text);
    g.setFont (fitFont (ctx.fonts, value, 15.5f, inner.getWidth(), 1));
    g.drawText (value, inner, Justification::centredLeft, false);

    Path p;
    const auto c = chev.getCentre();
    p.startNewSubPath (c.x - 4.5f, c.y - 2.0f);
    p.lineTo (c.x, c.y + 2.5f);
    p.lineTo (c.x + 4.5f, c.y - 2.0f);
    g.setColour (pal.dim);
    g.strokePath (p, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
}

void DropButton::mouseUp (const MouseEvent& e)
{
    if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
}

//------------------------------------------------------------------------------
void IconButton::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    const auto c = b.getCentre();
    const float s = jmin (b.getWidth(), b.getHeight());

    if (hover)
    {
        g.setColour (pal.text.withAlpha (0.08f));
        g.fillEllipse (b.withSizeKeepingCentre (s, s));
    }

    g.setColour (pal.text);
    switch (icon)
    {
        case Icon::sun:
        {
            const float r = s * 0.16f;
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            for (int i = 0; i < 8; ++i)
            {
                const float a = MathConstants<float>::pi * 0.25f * (float) i;
                const auto p0 = c + Point<float> (std::cos (a), std::sin (a)) * (s * 0.26f);
                const auto p1 = c + Point<float> (std::cos (a), std::sin (a)) * (s * 0.34f);
                g.drawLine ({ p0, p1 }, 1.8f);
            }
            break;
        }
        case Icon::moon:
        {
            const float r = s * 0.24f;
            Path m;
            m.addEllipse (c.x - r, c.y - r, r * 2, r * 2);
            Path cut;
            cut.addEllipse (c.x - r * 0.35f, c.y - r * 1.25f, r * 2, r * 2);
            m.setUsingNonZeroWinding (false);
            m.addPath (cut);
            Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (m);
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            break;
        }
        case Icon::menu:
        {
            const float w = s * 0.42f;
            for (int i = -1; i <= 1; ++i)
                g.fillRoundedRectangle (c.x - w * 0.5f, c.y + (float) i * s * 0.14f - 1.0f, w, 2.0f, 1.0f);
            break;
        }
        case Icon::palette:
        {
            const float r = s * 0.30f;
            if (ctx.vis.pal.rainbow)
            {
                for (int i = 0; i < 12; ++i)
                {
                    Path p;
                    const float a0 = MathConstants<float>::twoPi * (float) i / 12.0f;
                    p.addPieSegment (c.x - r, c.y - r, r * 2, r * 2, a0, a0 + MathConstants<float>::twoPi / 12.0f, 0.0f);
                    g.setColour (Colour::fromHSV ((float) i / 12.0f, 0.75f, 1.0f, 1.0f));
                    g.fillPath (p);
                }
            }
            else
            {
                ColourGradient cg (pal.a, c.x - r, c.y - r, pal.b, c.x + r, c.y + r, false);
                g.setGradientFill (cg);
                g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            }
            g.setColour (pal.text.withAlpha (0.5f));
            g.drawEllipse (c.x - r - 3, c.y - r - 3, r * 2 + 6, r * 2 + 6, 1.2f);
            break;
        }
    }
}

void IconButton::mouseUp (const MouseEvent& e)
{
    if (getLocalBounds().contains (e.getPosition()) && onClick) onClick();
}

//------------------------------------------------------------------------------
Rectangle<float> Segmented::segArea() const
{
    auto b = getLocalBounds().toFloat();
    const float lw = textWidth (ctx.fonts.ui (14.0f, 1), text) + 12.0f;
    return b.withTrimmedLeft (lw).withSizeKeepingCentre (b.getWidth() - lw, 30.0f);
}

void Segmented::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat();
    g.setColour (pal.dim);
    g.setFont (ctx.fonts.ui (14.0f, 1));
    g.drawText (text, b, Justification::centredLeft, false);

    auto s = segArea();
    g.setColour (pal.dark ? Colours::white.withAlpha (0.06f) : Colour (0xff0b1a3a).withAlpha (0.06f));
    g.fillRoundedRectangle (s, 15.0f);

    const float w = s.getWidth() / (float) options.size();
    for (int i = 0; i < options.size(); ++i)
    {
        auto r = Rectangle<float> (s.getX() + (float) i * w, s.getY(), w, s.getHeight()).reduced (2.0f);
        if (i == selected)
        {
            ColourGradient cg (pal.a.withAlpha (0.9f), r.getX(), r.getY(), pal.b.withAlpha (0.9f), r.getRight(), r.getBottom(), false);
            g.setGradientFill (cg);
            g.fillRoundedRectangle (r, 13.0f);
        }
        g.setColour (i == selected ? Colours::white : pal.dim);
        g.setFont (ctx.fonts.ui (13.5f, 1));
        g.drawText (options[i], r, Justification::centred, false);
    }
}

void Segmented::mouseUp (const MouseEvent& e)
{
    auto s = segArea();
    if (! s.contains (e.position)) return;
    const int i = jlimit (0, options.size() - 1, (int) ((e.position.x - s.getX()) / (s.getWidth() / (float) options.size())));
    if (i != selected)
    {
        selected = i;
        repaint();
        if (onChange) onChange (i);
    }
}

//==============================================================================
// Barra superior
//==============================================================================
TopBar::TopBar (UiContext& c)
    : keyButton (c, "Tonalidad"), scaleButton (c, "Escala"),
      modeButton (c, IconButton::Icon::sun), menuButton (c, IconButton::Icon::menu), ctx (c)
{
    addAndMakeVisible (keyButton);
    addAndMakeVisible (scaleButton);
    addAndMakeVisible (modeButton);
    addAndMakeVisible (menuButton);
}

void TopBar::setMidiStatus (bool connected, bool activity, const String& text, bool clickable)
{
    if (connected != midiConnected || activity != midiActivity || text != midiText || clickable != midiClickable)
    {
        midiConnected = connected;
        midiActivity = activity;
        midiText = text;
        midiClickable = clickable;
        setMouseCursor (MouseCursor::NormalCursor);
        repaint (midiArea().toNearestInt().expanded (4));
    }
}

Rectangle<float> TopBar::midiArea() const
{
    auto b = getLocalBounds().toFloat();
    const float left = 250.0f;
    const float right = (float) keyButton.getX() - 16.0f;
    return { left, b.getY() + 10.0f, jmax (120.0f, right - left), b.getHeight() - 20.0f };
}

void TopBar::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat();
    draw::glassPanel (g, b, pal, 18.0f);

    // Logo
    auto logo = b.reduced (20.0f, 10.0f).withWidth (220.0f);
    const auto lf = ctx.fonts.ui (30.0f, 2).withExtraKerningFactor (-0.01f);
    const float lw = textWidth (lf, "MidiSky");
    {
        ColourGradient lg (pal.a, logo.getX(), logo.getY(), pal.b, logo.getX() + lw, logo.getBottom(), false);
        g.setGradientFill (lg);
        g.setFont (lf);
        g.drawText ("MidiSky", logo.withHeight (34.0f).translated (0.0f, -1.0f), Justification::centredLeft, false);
        g.fillPath (draw::star ({ logo.getX() + lw + 8.0f, logo.getY() + 6.0f }, 6.0f, 0.28f));
    }
    g.setColour (pal.dim);
    g.setFont (ctx.fonts.ui (12.5f));
    g.drawText ("visualizador de escalas y acordes", logo.withTrimmedTop (32.0f), Justification::centredLeft, false);

    // MIDI
    auto m = midiArea();
    if (midiClickable)
    {
        g.setColour (pal.text.withAlpha (0.05f));
        g.fillRoundedRectangle (m.withSizeKeepingCentre (jmin (m.getWidth(), 280.0f), m.getHeight()), 10.0f);
    }
    auto mm = m.withSizeKeepingCentre (jmin (m.getWidth(), 280.0f), m.getHeight()).reduced (12.0f, 4.0f);
    g.setColour (pal.dim);
    g.setFont (ctx.fonts.ui (11.5f, 1).withExtraKerningFactor (0.08f));
    g.drawText ("MIDI", mm.removeFromTop (mm.getHeight() * 0.42f), Justification::bottomLeft, false);

    auto dotR = Rectangle<float> (mm.getX(), mm.getCentreY() - 5.0f, 10.0f, 10.0f);
    const auto on = Colour (0xff3ddc97);
    if (midiActivity)
    {
        ColourGradient glow (pal.a.withAlpha (0.9f), dotR.getCentreX(), dotR.getCentreY(), pal.a.withAlpha (0.0f), dotR.getCentreX() + 12.0f, dotR.getCentreY(), true);
        g.setGradientFill (glow);
        g.fillEllipse (dotR.expanded (8.0f));
    }
    g.setColour (midiActivity ? pal.a.interpolatedWith (Colours::white, 0.4f) : (midiConnected ? on : pal.faint));
    g.fillEllipse (dotR);

    g.setColour (pal.text);
    g.setFont (fitFont (ctx.fonts, midiText, 14.5f, mm.getWidth() - 40.0f, 1));
    g.drawText (midiText, mm.withTrimmedLeft (18.0f).withTrimmedRight (midiClickable ? 18.0f : 0.0f), Justification::centredLeft, true);

    if (midiClickable)
    {
        Path p;
        const auto c = Point<float> (mm.getRight() - 6.0f, mm.getCentreY());
        p.startNewSubPath (c.x - 4.5f, c.y - 2.0f);
        p.lineTo (c.x, c.y + 2.5f);
        p.lineTo (c.x + 4.5f, c.y - 2.0f);
        g.setColour (pal.dim);
        g.strokePath (p, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
    }
}

void TopBar::resized()
{
    auto b = getLocalBounds().reduced (14, 10);
    menuButton.setBounds (b.removeFromRight (40).withSizeKeepingCentre (40, 40));
    b.removeFromRight (4);
    modeButton.setBounds (b.removeFromRight (40).withSizeKeepingCentre (40, 40));
    b.removeFromRight (12);
    scaleButton.setBounds (b.removeFromRight (190));
    b.removeFromRight (8);
    keyButton.setBounds (b.removeFromRight (190));
}

void TopBar::mouseUp (const MouseEvent& e)
{
    if (midiClickable && midiArea().contains (e.position) && onMidiClicked)
        onMidiClicked();
}

//==============================================================================
// Tira de controles
//==============================================================================
ControlStrip::ControlStrip (UiContext& c)
    : highlight (c, "Resaltar escala"), pianoSound (c, "Sonido de piano"),
      skinButton (c, IconButton::Icon::palette), volume (c, "Volumen"),
      names (c, "Notas", { "C D E", "Do Re Mi" }), ctx (c)
{
    addAndMakeVisible (highlight);
    addAndMakeVisible (skinButton);
    addAndMakeVisible (pianoSound);
    addAndMakeVisible (volume);
    addAndMakeVisible (names);
}

void ControlStrip::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat();
    draw::glassPanel (g, b, pal, 16.0f);
    g.setColour (pal.faint);
    for (auto x : dividers)
        if (x > 0) g.fillRect (x, b.getY() + 14.0f, 1.0f, b.getHeight() - 28.0f);
}

void ControlStrip::resized()
{
    auto b = getLocalBounds().reduced (20, 6);
    const int w = b.getWidth();
    const int col = w / 4;

    auto c1 = b.removeFromLeft (col);
    skinButton.setBounds (c1.removeFromRight (40).withSizeKeepingCentre (40, 40));
    c1.removeFromRight (10);
    highlight.setBounds (c1.withTrimmedLeft (0));
    dividers[0] = (float) b.getX() + 10.0f;

    auto c2 = b.removeFromLeft (col).withTrimmedLeft (24);
    pianoSound.setBounds (c2.withTrimmedRight (16));
    dividers[1] = (float) b.getX() + 10.0f;

    auto c3 = b.removeFromLeft (col + col / 6).withTrimmedLeft (24);
    volume.setBounds (c3.withTrimmedRight (12));
    dividers[2] = (float) b.getX() + 6.0f;

    names.setBounds (b.withTrimmedLeft (22));
}

//==============================================================================
// Selector de skins
//==============================================================================
SkinPicker::SkinPicker (UiContext& c, int cur) : ctx (c), current (cur)
{
    setSize (4 * 92 + 24, 2 * 92 + 44);
}

Rectangle<float> SkinPicker::cell (int i) const
{
    return { 12.0f + (float) (i % 4) * 92.0f, 38.0f + (float) (i / 4) * 92.0f, 92.0f, 92.0f };
}

void SkinPicker::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (15.0f, 2));
    g.drawText ("Skins", Rectangle<float> (16.0f, 8.0f, 200.0f, 24.0f), Justification::centredLeft, false);

    const auto& list = skins();
    for (int i = 0; i < (int) list.size(); ++i)
    {
        auto r = cell (i).reduced (6.0f);
        if (i == hover)
        {
            g.setColour (pal.text.withAlpha (0.07f));
            g.fillRoundedRectangle (r, 10.0f);
        }
        auto sw = r.removeFromTop (r.getHeight() - 22.0f).withSizeKeepingCentre (44.0f, 44.0f);
        const auto& s = list[(size_t) i];
        if (s.rainbow)
        {
            for (int k = 0; k < 12; ++k)
            {
                Path p;
                const float a0 = MathConstants<float>::twoPi * (float) k / 12.0f;
                p.addPieSegment (sw, a0, a0 + MathConstants<float>::twoPi / 12.0f, 0.0f);
                g.setColour (Colour::fromHSV ((float) k / 12.0f, 0.75f, 1.0f, 1.0f));
                g.fillPath (p);
            }
        }
        else
        {
            auto ca = s.custom ? ctx.vis.pal.a : s.a;
            auto cb = s.custom ? ctx.vis.pal.b : s.b;
            if (s.custom && current != i) { ca = Colour (0xff4de1ff); cb = ca.withRotatedHue (0.1f); }
            ColourGradient cg (ca, sw.getX(), sw.getY(), cb, sw.getRight(), sw.getBottom(), false);
            g.setGradientFill (cg);
            g.fillEllipse (sw);
            if (s.custom)
            {
                g.setColour (Colours::white.withAlpha (0.9f));
                g.fillRect (sw.getCentreX() - 1.0f, sw.getCentreY() - 7.0f, 2.0f, 14.0f);
                g.fillRect (sw.getCentreX() - 7.0f, sw.getCentreY() - 1.0f, 14.0f, 2.0f);
            }
        }
        if (i == current)
        {
            g.setColour (pal.text);
            g.drawEllipse (sw.expanded (4.0f), 2.0f);
        }
        g.setColour (i == current ? pal.text : pal.dim);
        g.setFont (ctx.fonts.ui (12.5f, 1));
        g.drawText (U8 (s.name), r, Justification::centred, false);
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
            if (skins()[(size_t) i].custom)
                if (auto* box = findParentComponentOfClass<CallOutBox>())
                    box->dismiss();
            return;
        }
}
