#include "Views.h"

using namespace juce;

//==============================================================================
// Ayudas de dibujo
//==============================================================================
namespace draw
{
    void plate (Graphics& g, Rectangle<float> r, const Palette& pal, float radius)
    {
        g.setColour (pal.plate);
        g.fillRoundedRectangle (r, radius);
        g.setColour (pal.plateHighlight);
        g.fillRect (r.getX() + radius, r.getY() + 1.0f, r.getWidth() - radius * 2.0f, 1.0f);
        g.setColour (pal.plateEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    }

    void well (Graphics& g, Rectangle<float> r, const Palette& pal, float radius)
    {
        g.setColour (pal.well);
        g.fillRoundedRectangle (r, radius);
        {
            Graphics::ScopedSaveState ss (g);
            Path clip;
            clip.addRoundedRectangle (r, radius);
            g.reduceClipRegion (clip);
            ColourGradient sh (Colours::black.withAlpha (pal.dark ? 0.45f : 0.12f), r.getX(), r.getY(),
                               Colours::black.withAlpha (0.0f), r.getX(), r.getY() + 7.0f, false);
            g.setGradientFill (sh);
            g.fillRect (r.withHeight (7.0f));
        }
        g.setColour (pal.wellEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
        g.setColour (pal.plateHighlight);
        g.fillRect (r.getX() + radius, r.getBottom(), r.getWidth() - radius * 2.0f, 1.0f);
    }

    void screen (Graphics& g, Rectangle<float> r, const Palette& pal, float radius)
    {
        g.setColour (pal.screen);
        g.fillRoundedRectangle (r, radius);
        {
            Graphics::ScopedSaveState ss (g);
            Path clip;
            clip.addRoundedRectangle (r, radius);
            g.reduceClipRegion (clip);
            // viñeta y reflejo del vidrio
            const auto c = r.getCentre();
            ColourGradient vig (Colours::transparentBlack, c.x, c.y, Colours::black.withAlpha (0.55f), c.x + r.getWidth() * 0.62f, c.y, true);
            g.setGradientFill (vig);
            g.fillRect (r);
            ColourGradient ref (Colours::white.withAlpha (0.035f), r.getX(), r.getY(), Colours::white.withAlpha (0.0f), r.getX(), r.getY() + r.getHeight() * 0.5f, false);
            g.setGradientFill (ref);
            g.fillRect (r);
        }
        g.setColour (pal.screenEdge);
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.5f);
        g.setColour (pal.plateHighlight);
        g.fillRect (r.getX() + radius, r.getBottom() + 0.5f, r.getWidth() - radius * 2.0f, 1.0f);
    }

    void caption (Graphics& g, Rectangle<float> area, const String& text, Colour colour, const Fonts& fonts)
    {
        g.setColour (colour);
        g.setFont (fonts.ui (13.5f, 1));
        g.drawText (text, area, Justification::centredLeft, false);
    }

    void stars (Graphics& g, Rectangle<float> r, int count, int seed, Colour tint)
    {
        Random rng (seed);
        for (int i = 0; i < count; ++i)
        {
            const float x = r.getX() + rng.nextFloat() * r.getWidth();
            const float y = r.getY() + rng.nextFloat() * r.getHeight();
            const float s = 0.7f + rng.nextFloat() * rng.nextFloat() * 1.3f;
            g.setColour (tint.withMultipliedAlpha (0.25f + 0.75f * rng.nextFloat()));
            g.fillEllipse (x - s * 0.5f, y - s * 0.5f, s, s);
        }
    }

    void led (Graphics& g, Point<float> c, float r, Colour col, bool on)
    {
        if (on)
        {
            ColourGradient glow (col.withAlpha (0.45f), c.x, c.y, col.withAlpha (0.0f), c.x + r * 3.2f, c.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (c.x - r * 3.2f, c.y - r * 3.2f, r * 6.4f, r * 6.4f);
            g.setColour (col);
        }
        else
        {
            g.setColour (Colours::black.withAlpha (0.35f));
        }
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (Colours::white.withAlpha (on ? 0.55f : 0.12f));
        g.fillEllipse (c.x - r * 0.45f, c.y - r * 0.6f, r * 0.7f, r * 0.5f);
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
    draw::plate (g, b, pal);
    draw::caption (g, b.reduced (16.0f, 10.0f).removeFromTop (20.0f), "Pentagrama", pal.dim, ctx.fonts);
    auto paper = b.reduced (12.0f).withTrimmedTop (28.0f).withTrimmedBottom (26.0f);
    draw::well (g, paper, pal);

    auto area = paper.reduced (8.0f, 0.0f).withTrimmedTop (10.0f).withTrimmedBottom (10.0f);
    const float sp = jmin (area.getHeight() / 20.0f, area.getWidth() / 12.5f);
    const float midY = area.getCentreY();
    const float gap = sp * 1.4f; // separación extra entre clave de sol y clave de fa
    auto yT = [&] (int d) { return midY - gap - (float) (d - 28) * sp * 0.5f; };
    auto yB = [&] (int d) { return midY + gap - (float) (d - 28) * sp * 0.5f; };

    const float lineW = jmax (1.0f, sp * 0.09f);
    const float left = area.getX() + sp * 1.4f;
    const float right = area.getRight() - sp * 0.4f;
    const auto inkCol = pal.ink;

    g.setColour (inkCol.withAlpha (0.55f));
    for (int d : { 30, 32, 34, 36, 38 })
        g.fillRect (left, yT (d) - lineW * 0.5f, right - left, lineW);
    for (int d : { 18, 20, 22, 24, 26 })
        g.fillRect (left, yB (d) - lineW * 0.5f, right - left, lineW);
    g.fillRect (left - lineW * 0.5f, yT (38), lineW * 1.4f, yB (18) - yT (38));
    g.fillRect (right - lineW, yT (38), lineW, yB (18) - yT (38));

    const auto mf = ctx.fonts.music (sp * 4.0f);

    // Llave del sistema
    {
        const float sysH = yB (18) - yT (38);
        const auto braceFont = ctx.fonts.music (sysH);
        const float bw = glyphWidth (braceFont, 0xE000);
        g.setColour (inkCol.withAlpha (0.85f));
        drawGlyphF (g, braceFont, 0xE000, left - bw - sp * 0.25f, yB (18));
    }

    g.setColour (inkCol);
    drawGlyphF (g, mf, 0xE050, left + sp * 0.5f, yT (32)); // clave de sol
    drawGlyphF (g, mf, 0xE062, left + sp * 0.5f, yB (24)); // clave de fa

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
        drawGlyphF (g, mf, accGlyph, x, yT (v.sig.flats ? trebleFlat[i] : trebleSharp[i]));
        drawGlyphF (g, mf, accGlyph, x, yB (v.sig.flats ? bassFlat[i] : bassSharp[i]));
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
        auto ys = [&] (int d) { return n.treble ? yT (d) : yB (d); };
        const float ny = ys (n.d);

        // Líneas adicionales
        g.setColour (inkCol.withAlpha (0.7f));
        auto ledger = [&] (int d)
        {
            g.fillRect (noteX - sp * 0.45f, ys (d) - lineW * 0.5f, headW + sp * 0.9f + (n.shifted ? headW : 0.0f), lineW);
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
        if (! pal.dark) col = col.withMultipliedBrightness (0.72f).withMultipliedSaturation (1.25f);
        const float alpha = n.sustainedOnly ? 0.45f : 1.0f;

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
    g.drawText (sigText, b.reduced (16.0f, 6.0f).removeFromBottom (20.0f), Justification::centredLeft, true);
}

//==============================================================================
// Pantalla central
//==============================================================================
Rectangle<float> ChordScreen::chipArea() const
{
    auto b = screenArea().reduced (12.0f);
    const float h = jlimit (48.0f, 66.0f, b.getHeight() * 0.19f);
    return b.removeFromBottom (h);
}

Rectangle<float> ChordScreen::screenArea() const
{
    return getLocalBounds().toFloat().reduced (12.0f).withTrimmedTop (28.0f);
}

Rectangle<float> ChordScreen::chipBounds (int i, int count) const
{
    auto a = chipArea();
    const float gap = 6.0f;
    const float w = (a.getWidth() - gap * (float) (count - 1)) / (float) jmax (1, count);
    return { a.getX() + (float) i * (w + gap), a.getY(), w, a.getHeight() };
}

void ChordScreen::paint (Graphics& g)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;
    auto& fonts = ctx.fonts;
    auto outer = getLocalBounds().toFloat();
    draw::plate (g, outer, pal);
    draw::caption (g, outer.reduced (16.0f, 10.0f).removeFromTop (20.0f), "Acorde", pal.dim, fonts);

    {
        auto keyText = theory::keyTitle (v.keyRoot, v.scale, v.latin);
        if (v.autoKey) keyText = (v.autoHasData ? U8 ("Auto: ") : U8 ("Auto (esperando notas): ")) + keyText;
        g.setFont (fonts.ui (13.0f));
        g.setColour (pal.dim);
        g.drawText (keyText, outer.reduced (16.0f, 10.0f).removeFromTop (20.0f), Justification::centredRight, true);
    }

    auto b = screenArea();
    draw::screen (g, b, pal);
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (b.reduced (2.0f).toNearestInt());
        draw::stars (g, b, 46, 7, pal.screenFaint);
        if (! v.active.empty())
        {
            const auto c = b.getCentre().translated (0.0f, -b.getHeight() * 0.12f);
            ColourGradient glow (pal.accent.withAlpha (0.10f), c.x, c.y, pal.accent.withAlpha (0.0f), c.x + b.getWidth() * 0.4f, c.y, true);
            g.setGradientFill (glow);
            g.fillRect (b);
        }
    }

    // Área de contenido
    auto content = b.reduced (20.0f, 0.0f).withTrimmedTop (14.0f);
    content.setBottom (chipArea().getY() - 24.0f);

    const int count = (int) v.active.size();
    String big, sub, chip;
    int bigWeight = 2;

    if (count == 0)
    {
        big = theory::keyTitle (v.keyRoot, v.scale, v.latin);
        StringArray sn;
        for (auto s2 : theory::scales()[(size_t) v.scale].steps)
            sn.add (theory::pcName (v.keyRoot + s2, v.sig, v.latin));
        sub = "Escala: " + sn.joinIntoString ("  ");
        chip = U8 ("Toca una nota o un acorde");
        bigWeight = 1;
    }
    else if (count == 1)
    {
        const int m = v.active.front();
        big = theory::noteName (m, v.sig, v.latin, true);
        const double hz = 440.0 * std::pow (2.0, (m - 69) / 12.0);
        sub = theory::noteName (m, v.sig, true, false) + (m == 60 ? U8 (" central") : String()) + U8 (", ") + String (hz, 1) + " Hz";
        chip = theory::noteDegree (theory::pcOf (m), v.keyRoot, v.scale);
    }
    else if (v.chord.valid && (count >= 3 || v.chord.suffix == "5"))
    {
        big = v.chord.symbol;
        sub = theory::pcName (v.chord.root, v.sig, true) + " " + v.chord.quality;
        if (v.chord.inversion != "estado fundamental")
            sub << ", " << v.chord.inversion;
        chip = v.degree.roman + U8 ("   ") + v.degree.function.replace (U8 (" · "), U8 (": "));
    }
    else if (count == 2)
    {
        const int iv = v.active[1] - v.active[0];
        big = theory::intervalName (iv);
        sub = theory::noteName (v.active[0], v.sig, v.latin) + U8 (" a ") + theory::noteName (v.active[1], v.sig, v.latin)
              + ", " + String (iv) + (iv == 1 ? " semitono" : " semitonos");
        const int ivc = iv % 12;
        const bool consonant = ivc == 0 || ivc == 3 || ivc == 4 || ivc == 5 || ivc == 7 || ivc == 8 || ivc == 9;
        chip = consonant ? U8 ("Consonante: suena estable") : U8 ("Disonante: suena con tensión");
    }
    else
    {
        StringArray sn;
        for (int pc = 0; pc < 12; ++pc)
            if (v.activeMask & (1u << (unsigned) pc)) sn.add (theory::pcName (pc, v.sig, v.latin));
        big = sn.joinIntoString (" ");
        sub = String (count) + U8 (" notas sin nombre de acorde común");
        chip = U8 ("Prueba quitando una nota");
        bigWeight = 1;
    }

    const float bigMaxH = jlimit (28.0f, 120.0f, content.getHeight() * 0.34f);
    const auto bigFont = fitFont (fonts, big, bigMaxH, content.getWidth() * 0.94f, bigWeight);
    const float bigH = bigFont.getHeight();
    const float blockH = bigH + 30.0f + 34.0f + 40.0f;
    float y = content.getY() + jmax (0.0f, (content.getHeight() - blockH) * 0.45f);

    g.setColour (count > 0 ? pal.screenText : pal.screenText.withAlpha (0.8f));
    g.setFont (bigFont);
    g.drawText (big, Rectangle<float> (content.getX(), y, content.getWidth(), bigH), Justification::centred, false);
    y += bigH + 4.0f;

    g.setColour (pal.screenDim);
    g.setFont (fitFont (fonts, sub, 16.0f, content.getWidth() * 0.96f, 0));
    g.drawText (sub, Rectangle<float> (content.getX(), y, content.getWidth(), 24.0f), Justification::centred, false);
    y += 32.0f;

    if (chip.isNotEmpty())
    {
        const auto cf = fonts.ui (13.5f, 1);
        const float cw = textWidth (cf, chip) + 24.0f;
        auto cr = Rectangle<float> (content.getCentreX() - cw * 0.5f, y, cw, 26.0f);
        g.setColour (count > 0 ? pal.screenDim : pal.screenFaint);
        g.drawRoundedRectangle (cr.reduced (0.5f), 4.0f, 1.0f);
        g.setColour (count > 0 ? pal.screenText : pal.screenDim);
        g.setFont (cf);
        g.drawText (chip, cr, Justification::centred, false);
        y += 36.0f;
    }

    // Etiquetas con las notas tocadas
    if (count > 0)
    {
        const auto pf = fonts.ui (13.0f, 1);
        std::vector<std::pair<String, Colour>> tags;
        for (auto m : v.active)
            tags.push_back ({ theory::noteName (m, v.sig, v.latin), pal.noteColour (m) });
        if (tags.size() > 10) tags.resize (10);

        float total = 0.0f;
        for (auto& t : tags) total += textWidth (pf, t.first) + 26.0f;
        total += 6.0f * (float) (tags.size() - 1);
        float px = content.getCentreX() - total * 0.5f;
        for (auto& t : tags)
        {
            const float w = textWidth (pf, t.first) + 26.0f;
            auto r = Rectangle<float> (px, y, w, 22.0f);
            g.setColour (pal.screenFaint.withMultipliedAlpha (0.6f));
            g.fillRoundedRectangle (r, 3.0f);
            g.setColour (t.second);
            g.fillRect (r.getX() + 7.0f, r.getCentreY() - 3.5f, 7.0f, 7.0f);
            g.setColour (pal.screenText);
            g.setFont (pf);
            g.drawText (t.first, r.withTrimmedLeft (17.0f).withTrimmedRight (4.0f), Justification::centred, false);
            px += w + 6.0f;
        }
    }

    // Acordes de la tonalidad: una fila de botones
    auto ca = chipArea();
    g.setColour (pal.screenDim);
    g.setFont (fonts.ui (12.0f));
    g.drawText (U8 ("Acordes de la tonalidad"), ca.withHeight (16.0f).translated (2.0f, -18.0f), Justification::centredLeft, false);

    const int nChips = (int) v.diatonic.size();
    for (int i = 0; i < nChips; ++i)
    {
        const auto& d = v.diatonic[(size_t) i];
        auto r = chipBounds (i, nChips);
        const bool activeChip = v.chord.valid && v.chord.root == d.rootPc && (d.mask & ~v.activeMask) == 0;

        if (activeChip)
        {
            g.setColour (pal.accent.withAlpha (0.22f));
            g.fillRoundedRectangle (r, 5.0f);
            g.setColour (pal.accent);
            g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.2f);
        }
        else
        {
            g.setColour (pal.screenFaint.withMultipliedAlpha (i == hoverChip ? 1.2f : 0.55f));
            g.fillRoundedRectangle (r, 5.0f);
        }

        g.setColour (activeChip ? pal.screenText : pal.screenDim);
        g.setFont (fonts.ui (jmin (12.5f, r.getHeight() * 0.28f), 1));
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
    draw::plate (g, b, pal);
    draw::caption (g, b.reduced (16.0f, 10.0f).removeFromTop (20.0f), U8 ("Círculo de quintas"), pal.dim, fonts);

    Point<float> c; float r;
    geometry (c, r);
    const float rOut = r, rMid = r * 0.68f, rIn = r * 0.40f;
    const float seg = MathConstants<float>::twoPi / 12.0f;

    // Base del disco
    g.setColour (pal.well);
    g.fillEllipse (c.x - rOut, c.y - rOut, rOut * 2.0f, rOut * 2.0f);
    g.setColour (pal.wellEdge);
    g.drawEllipse (c.x - rOut, c.y - rOut, rOut * 2.0f, rOut * 2.0f, 1.0f);

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
                g.setColour (pal.accent);
                g.fillPath (p);
            }
            else if (dist <= 1)
            {
                g.setColour (pal.accent.withAlpha (minorRing ? 0.10f : 0.16f));
                g.fillPath (p);
            }

            if (i == hoverIndex && minorRing == hoverMinor)
            {
                g.setColour (pal.text.withAlpha (0.10f));
                g.fillPath (p);
            }

            g.setColour (pal.faint);
            g.strokePath (p, PathStrokeType (1.0f));

            const float ang = (float) i * seg - MathConstants<float>::halfPi;
            const float lr = minorRing ? (rIn + rMid) * 0.5f : (rMid + rOut) * 0.5f;
            const auto lp = c + Point<float> (std::cos (ang) * lr, std::sin (ang) * lr);
            const int pc = minorRing ? theory::pcOf (majorPcAt (i) + 9) : majorPcAt (i);
            const auto sigHere = theory::keySignature (pc, minorRing ? 1 : 0);
            auto name = theory::pcName (pc, sigHere, v.latin) + (minorRing ? "m" : "");
            const float fh = minorRing ? jmax (10.0f, r * 0.085f) : jmax (12.0f, r * 0.11f);
            g.setFont (fonts.ui (fh, isTonic ? 2 : 1));
            g.setColour (isTonic ? pal.onAccent : (minorRing ? pal.dim : pal.text));
            g.drawText (name, Rectangle<float> (lp.x - 40.0f, lp.y - fh * 0.6f, 80.0f, fh * 1.2f), Justification::centred, false);
        }
    }

    // Constelación: las notas que suenan, unidas con líneas
    std::vector<int> idx;
    for (int pc = 0; pc < 12; ++pc)
        if (v.activeMask & (1u << (unsigned) pc)) idx.push_back (circleIndexOfPc (pc));
    std::sort (idx.begin(), idx.end());

    std::vector<Point<float>> pts;
    {
        const float sr = rMid + (rOut - rMid) * 0.82f;
        for (auto i : idx)
        {
            const float ang = (float) i * seg - MathConstants<float>::halfPi;
            pts.push_back (c + Point<float> (std::cos (ang) * sr, std::sin (ang) * sr));
        }
    }

    if (pts.size() >= 2)
    {
        Path poly;
        poly.startNewSubPath (pts[0]);
        for (size_t k = 1; k < pts.size(); ++k) poly.lineTo (pts[k]);
        if (pts.size() >= 3) poly.closeSubPath();
        g.setColour (pal.text.withAlpha (pal.dark ? 0.75f : 0.65f));
        g.strokePath (poly, PathStrokeType (1.4f));
    }

    // Centro: tonalidad y armadura
    {
        g.setColour (pal.plate);
        g.fillEllipse (c.x - rIn, c.y - rIn, rIn * 2.0f, rIn * 2.0f);
        g.setColour (pal.wellEdge);
        g.drawEllipse (c.x - rIn, c.y - rIn, rIn * 2.0f, rIn * 2.0f, 1.0f);

        const auto root = theory::pcName (v.keyRoot, v.sig, v.latin);
        const auto scName = String (CharPointer_UTF8 (sc.shortName));
        g.setColour (pal.text);
        g.setFont (fonts.ui (rIn * 0.46f, 2));
        g.drawText (root, Rectangle<float> (c.x - rIn, c.y - rIn * 0.62f, rIn * 2.0f, rIn * 0.58f), Justification::centred, false);
        g.setColour (pal.dim);
        g.setFont (fitFont (fonts, scName, rIn * 0.22f, rIn * 1.7f, 0));
        g.drawText (scName, Rectangle<float> (c.x - rIn, c.y - rIn * 0.02f, rIn * 2.0f, rIn * 0.3f), Justification::centred, false);
        const String sigTxt = v.sig.count == 0 ? U8 ("sin alteraciones")
                                               : String (v.sig.count) + (v.sig.flats ? (v.sig.count == 1 ? " bemol" : " bemoles")
                                                                                     : (v.sig.count == 1 ? " sostenido" : " sostenidos"));
        g.setFont (fitFont (fonts, sigTxt, rIn * 0.19f, rIn * 1.6f, 1));
        g.drawText (sigTxt, Rectangle<float> (c.x - rIn, c.y + rIn * 0.3f, rIn * 2.0f, rIn * 0.3f), Justification::centred, false);
    }

    for (size_t k = 0; k < pts.size(); ++k)
        draw::led (g, pts[k], 4.5f, pal.noteColour (60 + majorPcAt (idx[k])), true);

    // Pie
    auto foot = b.reduced (16.0f, 8.0f).removeFromBottom (18.0f);
    g.setFont (fonts.ui (12.5f));
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
    auto b = getLocalBounds().toFloat().reduced (14.0f, 0.0f);
    const float beam = jlimit (18.0f, 60.0f, b.getHeight() * 0.18f);
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
    draw::plate (g, b, pal);

    const auto a = keysArea();
    const float beamTop = b.getY() + 8.0f;

    auto levelOf = [&] (int n)
    {
        const float d = v.down[(size_t) n] / 127.0f;
        const float s = v.sus[(size_t) n] / 127.0f;
        return jmax (d > 0 ? 0.6f + 0.4f * d : 0.0f, s > 0 ? 0.3f + 0.2f * s : 0.0f);
    };

    // Luz que sale de las teclas encendidas
    for (int n = lowNote; n <= highNote; ++n)
    {
        const float lvl = levelOf (n);
        const float fl = v.flash[(size_t) n];
        if (lvl <= 0.0f && fl <= 0.01f) continue;
        const auto k = keyRect (n);
        const auto col = pal.noteColour (n);
        const float inten = jlimit (0.0f, 1.0f, lvl * 0.55f + fl * 0.5f);
        const float w = k.r.getWidth() * (k.black ? 1.0f : 0.7f);
        auto beam = Rectangle<float> (k.r.getCentreX() - w * 0.5f, beamTop, w, a.getY() - beamTop - 4.0f);
        ColourGradient bg (col.withAlpha (0.0f), beam.getX(), beam.getY(), col.withAlpha (0.55f * inten), beam.getX(), beam.getBottom(), false);
        g.setGradientFill (bg);
        g.fillRect (beam);
    }

    // Cama del teclado
    g.setColour (pal.dark ? Colour (0xff050506) : Colour (0xff1d1f24));
    g.fillRoundedRectangle (a.expanded (4.0f, 4.0f).withTrimmedTop (-2.0f), 4.0f);

    const bool showScale = v.highlight;

    // Teclas blancas
    for (int n = lowNote; n <= highNote; ++n)
    {
        const auto k = keyRect (n);
        if (k.black) continue;
        auto r = k.r.reduced (0.6f, 0.0f);
        ColourGradient wg (pal.keyWhite, r.getX(), r.getY(), pal.keyWhiteShade, r.getX(), r.getBottom(), false);
        g.setGradientFill (wg);
        g.fillRoundedRectangle (r.withTrimmedTop (-4.0f), 2.5f);

        const int pc = n % 12;
        const bool inScale = showScale && (v.scaleMask & (1u << (unsigned) pc)) != 0;
        const float lvl = levelOf (n);
        if (lvl > 0.0f)
        {
            const auto col = pal.noteColour (n);
            g.setColour (col.withAlpha (0.35f + 0.65f * lvl));
            g.fillRoundedRectangle (r.withTrimmedTop (-4.0f), 2.5f);
            g.setColour (Colours::white.withAlpha (0.35f));
            g.fillRect (r.withHeight (2.0f));
        }

        if (inScale)
        {
            const bool isRoot = pc == theory::pcOf (v.keyRoot);
            const float d = jmin (r.getWidth() * 0.36f, 7.0f);
            const auto dc = Point<float> (r.getCentreX(), r.getBottom() - d * 2.2f - (pc == 0 ? 14.0f : 0.0f));
            const auto mark = lvl > 0 ? Colours::black.withAlpha (0.55f) : (pal.rainbow ? pal.noteColour (n).withMultipliedBrightness (0.75f) : pal.accent.withMultipliedBrightness (pal.dark ? 0.9f : 0.75f));
            g.setColour (mark);
            if (isRoot) g.fillRect (dc.x - d * 0.5f, dc.y - d * 0.5f, d, d);
            else        g.fillEllipse (dc.x - d * 0.32f, dc.y - d * 0.32f, d * 0.64f, d * 0.64f);
        }

        if (pc == 0)
        {
            g.setColour (lvl > 0 ? Colours::black.withAlpha (0.7f) : pal.keyEdge);
            g.setFont (ctx.fonts.ui (jmin (11.5f, r.getWidth() * 0.55f), 1));
            g.drawText ((v.latin ? String ("Do") : String ("C")) + String (n / 12 - 1),
                        r.withTrimmedTop (r.getHeight() - 17.0f), Justification::centred, false);
        }
    }

    // Fieltro
    g.setColour (pal.felt);
    g.fillRect (Rectangle<float> (a.getX(), a.getY() - 3.0f, a.getWidth(), 3.0f));

    // Teclas negras
    for (int n = lowNote; n <= highNote; ++n)
    {
        const auto k = keyRect (n);
        if (! k.black) continue;
        auto r = k.r;
        g.setColour (Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (r.translated (1.0f, 1.5f), 2.0f);
        ColourGradient bg (pal.keyBlackTop, r.getX(), r.getY(), pal.keyBlack, r.getX(), r.getY() + r.getHeight() * 0.85f, false);
        g.setGradientFill (bg);
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (Colours::white.withAlpha (0.08f));
        g.fillRect (r.reduced (r.getWidth() * 0.2f, 0.0f).withTrimmedTop (r.getHeight() - 7.0f).withHeight (2.0f));

        const float lvl = levelOf (n);
        if (lvl > 0.0f)
        {
            const auto col = pal.noteColour (n);
            g.setColour (col.withMultipliedBrightness (0.85f).withAlpha (0.35f + 0.65f * lvl));
            g.fillRoundedRectangle (r.reduced (1.0f, 0.0f).withTrimmedBottom (1.0f), 2.0f);
        }

        const int pc = n % 12;
        if (showScale && (v.scaleMask & (1u << (unsigned) pc)) != 0)
        {
            const bool isRoot = pc == theory::pcOf (v.keyRoot);
            const float d = jmin (r.getWidth() * 0.45f, 6.5f);
            const auto dc = Point<float> (r.getCentreX(), r.getBottom() - d * 1.6f);
            const auto mark = lvl > 0 ? Colours::black.withAlpha (0.6f) : (pal.rainbow ? pal.noteColour (n) : pal.accent);
            g.setColour (mark);
            if (isRoot) g.fillRect (dc.x - d * 0.5f, dc.y - d * 0.5f, d, d);
            else        g.fillEllipse (dc.x - d * 0.32f, dc.y - d * 0.32f, d * 0.64f, d * 0.64f);
        }
    }

    // Pie
    auto foot = Rectangle<float> (a.getX(), a.getBottom() + 5.0f, a.getWidth(), 17.0f);
    g.setFont (ctx.fonts.ui (12.0f));
    g.setColour (pal.dim);
    g.drawText (v.latin ? U8 ("La0 a Do8, 88 teclas") : U8 ("A0 a C8, 88 teclas"), foot, Justification::centredLeft, false);
    if (v.highlight)
        g.drawText (U8 ("Cuadro = tónica, punto = nota de la escala"), foot, Justification::centred, false);

    const auto pedalTxt = String ("Pedal");
    const float tw = textWidth (ctx.fonts.ui (12.0f), pedalTxt);
    auto pr = foot.removeFromRight (tw + 18.0f);
    g.drawText (pedalTxt, pr.withTrimmedRight (16.0f), Justification::centredRight, false);
    draw::led (g, { pr.getRight() - 5.0f, pr.getCentreY() }, 3.5f, pal.accent, v.pedal);
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
    auto track = b.removeFromRight (44.0f).withSizeKeepingCentre (44.0f, 22.0f);

    g.setColour (pal.text.withAlpha (0.8f));
    g.setFont (ctx.fonts.ui (14.0f, 1));
    g.drawText (text, b.withTrimmedRight (12.0f), Justification::centredRight, true);

    draw::well (g, track, pal, 11.0f);
    if (on)
    {
        g.setColour (pal.accent);
        g.fillRoundedRectangle (track.reduced (2.0f), 9.0f);
    }

    const float d = 18.0f;
    const float x = on ? track.getRight() - d - 2.0f : track.getX() + 2.0f;
    auto knob = Rectangle<float> (x, track.getCentreY() - d * 0.5f, d, d);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillEllipse (knob.translated (0.0f, 1.0f));
    g.setColour (pal.dark ? Colour (0xffdfe1e5) : Colours::white);
    g.fillEllipse (knob);
    g.setColour (Colours::black.withAlpha (0.15f));
    g.drawEllipse (knob.reduced (0.5f), 1.0f);
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
    g.setColour (pal.text.withAlpha (0.8f));
    const float lw = textWidth (lf, text) + 14.0f;
    g.drawText (text, b.removeFromLeft (lw), Justification::centredLeft, false);

    const float d = jmin (b.getHeight() - 4.0f, 46.0f);
    auto k = b.removeFromLeft (d).withSizeKeepingCentre (d, d);
    b.removeFromLeft (12.0f);

    auto vr = b.withSizeKeepingCentre (jmin (b.getWidth(), 76.0f), 26.0f).withX (b.getX());
    draw::well (g, vr, pal, 4.0f);
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (14.0f, 1));
    g.drawText (valueText, vr, Justification::centred, false);

    const auto c = k.getCentre();
    const float r = d * 0.5f;
    const float a0 = -MathConstants<float>::pi * 0.75f, a1 = MathConstants<float>::pi * 0.75f;
    const float av = a0 + (a1 - a0) * value;

    // Marcas de escala alrededor de la perilla
    for (int i = 0; i <= 10; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 10.0f;
        const bool lit = (float) i / 10.0f <= value + 0.001f;
        const auto p0 = c + Point<float> (std::sin (a), -std::cos (a)) * (r - 1.0f);
        const auto p1 = c + Point<float> (std::sin (a), -std::cos (a)) * (r - (i % 5 == 0 ? 6.0f : 4.0f));
        g.setColour (lit ? pal.accent : pal.faint);
        g.drawLine ({ p0, p1 }, 1.4f);
    }

    const float ir = r - 8.0f;
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillEllipse (c.x - ir, c.y - ir + 1.5f, ir * 2.0f, ir * 2.0f);
    ColourGradient body (pal.dark ? Colour (0xff3a3d44) : Colours::white, c.x, c.y - ir,
                         pal.dark ? Colour (0xff1e2025) : Colour (0xffcfd3d9), c.x, c.y + ir, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - ir, c.y - ir, ir * 2.0f, ir * 2.0f);
    g.setColour (pal.plateEdge);
    g.drawEllipse (c.x - ir, c.y - ir, ir * 2.0f, ir * 2.0f, 1.0f);

    const auto p0 = c + Point<float> (std::sin (av), -std::cos (av)) * (ir * 0.25f);
    const auto p1 = c + Point<float> (std::sin (av), -std::cos (av)) * (ir - 2.0f);
    g.setColour (pal.dark ? Colours::white : pal.text);
    g.drawLine ({ p0, p1 }, 2.0f);
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
    draw::well (g, b, pal, 6.0f);
    if (hover)
    {
        g.setColour (pal.text.withAlpha (0.05f));
        g.fillRoundedRectangle (b, 6.0f);
    }

    auto inner = b.reduced (12.0f, 6.0f);
    auto chev = inner.removeFromRight (14.0f);
    g.setColour (pal.dim);
    g.setFont (ctx.fonts.ui (11.5f, 1));
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
            const float r = s * 0.28f;
            if (ctx.vis.pal.rainbow)
            {
                for (int i = 0; i < 12; ++i)
                {
                    Path p;
                    const float a0 = MathConstants<float>::twoPi * (float) i / 12.0f;
                    p.addPieSegment (c.x - r, c.y - r, r * 2, r * 2, a0, a0 + MathConstants<float>::twoPi / 12.0f, 0.0f);
                    g.setColour (Colour::fromHSV ((float) ((i * 7) % 12) / 12.0f, 0.62f, 1.0f, 1.0f));
                    g.fillPath (p);
                }
            }
            else
            {
                g.setColour (pal.accent);
                g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            }
            g.setColour (pal.plateEdge);
            g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.0f);
            g.setColour (pal.dim);
            g.drawEllipse (c.x - r - 3.5f, c.y - r - 3.5f, r * 2 + 7.0f, r * 2 + 7.0f, 1.0f);
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
    return b.withTrimmedLeft (lw).withSizeKeepingCentre (b.getWidth() - lw, 32.0f);
}

void Segmented::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat();
    g.setColour (pal.text.withAlpha (0.8f));
    g.setFont (ctx.fonts.ui (14.0f, 1));
    g.drawText (text, b, Justification::centredLeft, false);

    auto sArea = segArea();
    draw::well (g, sArea, pal, 6.0f);

    const float w = sArea.getWidth() / (float) options.size();
    for (int i = 0; i < options.size(); ++i)
    {
        auto r = Rectangle<float> (sArea.getX() + (float) i * w, sArea.getY(), w, sArea.getHeight()).reduced (3.0f);
        if (i == selected)
        {
            draw::plate (g, r, pal, 4.0f);
            draw::led (g, { r.getX() + 9.0f, r.getCentreY() }, 2.6f, pal.accent, true);
        }
        g.setColour (i == selected ? pal.text : pal.dim);
        g.setFont (ctx.fonts.ui (13.5f, 1));
        g.drawText (options[i], r.withTrimmedLeft (i == selected ? 10.0f : 0.0f), Justification::centred, false);
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
    draw::plate (g, b, pal);

    // Marca: una pequeña constelación (tres estrellas unidas) y el nombre
    auto logo = b.reduced (18.0f, 10.0f).withWidth (230.0f);
    {
        const float x0 = logo.getX() + 2.0f, y0 = logo.getY() + 6.0f;
        const Point<float> st[4] = { { x0, y0 + 16.0f }, { x0 + 9.0f, y0 + 4.0f }, { x0 + 19.0f, y0 + 10.0f }, { x0 + 26.0f, y0 } };
        g.setColour (pal.dim);
        for (int i = 0; i < 3; ++i) g.drawLine ({ st[i], st[i + 1] }, 1.0f);
        for (int i = 0; i < 4; ++i)
        {
            const float rr = i == 3 ? 2.8f : 2.0f;
            g.setColour (i == 3 ? pal.accent : pal.text);
            g.fillEllipse (st[i].x - rr, st[i].y - rr, rr * 2.0f, rr * 2.0f);
        }
    }
    auto word = logo.withTrimmedLeft (38.0f);
    g.setColour (pal.text);
    g.setFont (ctx.fonts.ui (27.0f, 2).withExtraKerningFactor (-0.01f));
    g.drawText ("MidiSky", word.withHeight (30.0f), Justification::centredLeft, false);
    g.setColour (pal.dim);
    g.setFont (ctx.fonts.ui (12.5f));
    g.drawText ("escalas, acordes y notas", word.withTrimmedTop (30.0f), Justification::centredLeft, false);

    // MIDI
    auto m = midiArea();
    auto box = m.withSizeKeepingCentre (jmin (m.getWidth(), 290.0f), m.getHeight());
    draw::well (g, box, pal, 6.0f);
    auto mm = box.reduced (12.0f, 5.0f);
    g.setColour (pal.dim);
    g.setFont (ctx.fonts.ui (11.5f, 1));
    g.drawText ("MIDI", mm.removeFromTop (mm.getHeight() * 0.42f), Justification::bottomLeft, false);

    const auto ledC = Point<float> (mm.getX() + 4.0f, mm.getCentreY());
    if (midiActivity)        draw::led (g, ledC, 4.0f, pal.accent, true);
    else if (midiConnected)  draw::led (g, ledC, 4.0f, Colour (0xff42d67e), true);
    else                     draw::led (g, ledC, 4.0f, pal.dim, false);

    g.setColour (pal.text);
    g.setFont (fitFont (ctx.fonts, midiText, 14.5f, mm.getWidth() - 40.0f, 1));
    g.drawText (midiText, mm.withTrimmedLeft (16.0f).withTrimmedRight (midiClickable ? 18.0f : 0.0f), Justification::centredLeft, true);

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
    draw::plate (g, b, pal);
    for (auto x : dividers)
        if (x > 0)
        {
            g.setColour (pal.plateEdge.withMultipliedAlpha (0.8f));
            g.fillRect (x, b.getY() + 12.0f, 1.0f, b.getHeight() - 24.0f);
            g.setColour (pal.plateHighlight);
            g.fillRect (x + 1.0f, b.getY() + 12.0f, 1.0f, b.getHeight() - 24.0f);
        }
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
    setSize (4 * 92 + 24, 3 * 92 + 44);
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
    g.drawText ("Color de la luz", Rectangle<float> (16.0f, 8.0f, 240.0f, 24.0f), Justification::centredLeft, false);

    const auto& list = skins();
    for (int i = 0; i < (int) list.size(); ++i)
    {
        auto r = cell (i).reduced (5.0f);
        if (i == hover)
        {
            g.setColour (pal.text.withAlpha (0.06f));
            g.fillRoundedRectangle (r, 6.0f);
        }
        auto sw = r.removeFromTop (r.getHeight() - 22.0f).withSizeKeepingCentre (40.0f, 40.0f);
        const auto& s = list[(size_t) i];
        if (s.rainbow)
        {
            for (int k = 0; k < 12; ++k)
            {
                Path p;
                const float a0 = MathConstants<float>::twoPi * (float) k / 12.0f;
                p.addPieSegment (sw, a0, a0 + MathConstants<float>::twoPi / 12.0f, 0.0f);
                g.setColour (Colour::fromHSV ((float) ((k * 7) % 12) / 12.0f, 0.62f, 1.0f, 1.0f));
                g.fillPath (p);
            }
        }
        else
        {
            g.setColour (s.custom ? (i == current ? pal.accent : Colour (0xff4de1ff)) : s.light);
            g.fillEllipse (sw);
            if (s.custom)
            {
                g.setColour (Colours::black.withAlpha (0.6f));
                g.fillRect (sw.getCentreX() - 1.0f, sw.getCentreY() - 7.0f, 2.0f, 14.0f);
                g.fillRect (sw.getCentreX() - 7.0f, sw.getCentreY() - 1.0f, 14.0f, 2.0f);
            }
        }
        g.setColour (pal.plateEdge);
        g.drawEllipse (sw, 1.0f);
        if (i == current)
        {
            g.setColour (pal.text);
            g.drawEllipse (sw.expanded (4.0f), 1.5f);
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
