#include "Sections.h"

using namespace juce;

static bool isBlackPc (int pc) { return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }

//==============================================================================
Section::Section (UiContext& c, String sectionId, String sectionTitle, Point<int> naturalSize)
    : id (std::move (sectionId)), title (std::move (sectionTitle)), natural (naturalSize), ctx (c),
      detachBtn (c, "windows", "Separar en su propia ventana")
{
    detachBtn.framed = false;
    detachBtn.onClick = [this] { if (onDetach) onDetach(); };
    addAndMakeVisible (detachBtn);
}

void Section::refreshControls()
{
    detachBtn.setVisible (! floating && ! ctx.vis.hideParams);
    resized();
    repaint();
}

void Section::resized()
{
    const int s = roundToInt (24.0f * k);
    const int pad = roundToInt ((floating ? 10.0f : 4.0f) * k);
    detachBtn.setBounds (getWidth() - s - pad, pad, s, s);
    detachBtn.setVisible (! floating && ! ctx.vis.hideParams);
    layoutContent (contentArea().toNearestInt());
}

Rectangle<float> Section::contentArea() const
{
    auto b = getLocalBounds().toFloat();
    if (floating && ! ctx.vis.transparent) return b.reduced (14.0f * k);
    if (floating) return b.reduced (6.0f * k);
    switch (panelKind())
    {
        case PanelKind::atril: return b.reduced (6.0f * k, 4.0f * k);
        case PanelKind::fall:  return b.reduced (22.0f * k, 8.0f * k);
        case PanelKind::keys:  return b.reduced (8.0f * k);
    }
    return b;
}

void Section::caption (Graphics& g, Rectangle<float>& area, const String& text)
{
    const auto& pal = ctx.vis.pal;
    auto row = area.removeFromTop (18.0f * k);
    g.setFont (ctx.fonts.label (pal.uiFont, 12.5f * k, 1));
    draw::shadowText (g, text, row.withTrimmedRight (28.0f * k), Justification::centredLeft,
                      transparentMode() ? Colours::white : pal.dim, transparentMode());
    area.removeFromTop (4.0f * k);
}

void Section::paint (Graphics& g)
{
    const auto& pal = ctx.vis.pal;
    auto b = getLocalBounds().toFloat();

    if (floating && ! ctx.vis.transparent)
    {
        draw::body (g, b, pal, ctx.tex);
        const auto fill = panelKind() == PanelKind::fall ? pal.fall : (panelKind() == PanelKind::keys ? pal.keysBlock : pal.atril);
        draw::panel (g, b.reduced (6.0f * k), fill, pal.railBorder, pal, k);
    }
    else if (! floating)
    {
        if (panelKind() == PanelKind::fall) draw::panel (g, b.reduced (0.5f), pal.vfd ? pal.fall : pal.fall, pal.railBorder, pal, k);
        if (panelKind() == PanelKind::keys) draw::panel (g, b.reduced (0.5f), pal.keysBlock, pal.railBorder, pal, k);
    }

    paintContent (g, contentArea());

    if (floating && ! ctx.vis.transparent)
        draw::themeOverlay (g, b, pal);
}

//==============================================================================
// Círculo de quintas
//==============================================================================
CircleSection::CircleSection (UiContext& c) : Section (c, "circulo", U8 ("Círculo de quintas"), { 330, 350 }) {}

static int circleIndexOfPc (int pc) { return (theory::pcOf (pc) * 7) % 12; }
static int majorPcAt (int i)        { return (i * 7) % 12; }

bool CircleSection::ringAt (Point<float> p, int& index, bool& minor) const
{
    const float dist = p.getDistanceFrom (centre);
    if (dist > radius || dist < radius * 0.40f) return false;
    float ang = std::atan2 (p.y - centre.y, p.x - centre.x) + MathConstants<float>::halfPi;
    if (ang < 0) ang += MathConstants<float>::twoPi;
    index = (int) std::floor ((ang + MathConstants<float>::pi / 12.0f) / (MathConstants<float>::twoPi / 12.0f)) % 12;
    minor = dist < radius * 0.68f;
    return true;
}

void CircleSection::paintContent (Graphics& g, Rectangle<float> area)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;
    const bool tm = transparentMode();
    caption (g, area, title);

    radius = jmax (20.0f, jmin (area.getWidth(), area.getHeight()) * 0.5f - 2.0f);
    centre = area.getCentre();
    const auto c = centre;
    const float rOut = radius, rMid = radius * 0.68f, rIn = radius * 0.40f;
    const float seg = MathConstants<float>::twoPi / 12.0f;

    if (tm)
    {
        g.setColour (Colours::black.withAlpha (0.30f));
        g.fillEllipse (c.x - rOut, c.y - rOut, rOut * 2.0f, rOut * 2.0f);
    }

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

    const auto textCol = tm ? Colours::white : pal.text;
    const auto dimCol  = tm ? Colours::white.withAlpha (0.75f) : pal.dim;

    for (int i = 0; i < 12; ++i)
    {
        const int dist = std::min ((i - region + 12) % 12, (region - i + 12) % 12);
        for (int ring = 0; ring < 2; ++ring)
        {
            const bool minorRing = ring == 1;
            auto p = minorRing ? ringSeg (i, rIn, rMid) : ringSeg (i, rMid, rOut);
            const bool isTonic = (i == tonicIndex && minorRing == tonicMinor);

            g.setColour (isTonic ? pal.accent : (dist <= 1 ? (minorRing ? pal.regionMinor : pal.region) : pal.ring));
            g.fillPath (p);
            if (i == hoverIndex && minorRing == hoverMinor)
            {
                g.setColour (textCol.withAlpha (0.12f));
                g.fillPath (p);
            }
            g.setColour (tm ? Colours::white.withAlpha (0.3f) : pal.ringLine);
            g.strokePath (p, PathStrokeType (pal.pixel ? 2.0f : 1.0f));

            const float ang = (float) i * seg - MathConstants<float>::halfPi;
            const float lr = minorRing ? (rIn + rMid) * 0.5f : (rMid + rOut) * 0.5f;
            const auto lp = c + Point<float> (std::cos (ang) * lr, std::sin (ang) * lr);
            const int pc = minorRing ? theory::pcOf (majorPcAt (i) + 9) : majorPcAt (i);
            const auto sigHere = theory::keySignature (pc, minorRing ? 1 : 0);
            auto name = theory::pcName (pc, sigHere, v.latin) + (minorRing ? "m" : "");
            const float fh = minorRing ? jmax (8.5f, radius * 0.085f) : jmax (10.0f, radius * 0.105f);
            g.setFont (minorRing ? ctx.fonts.label (pal.uiFont, fh, 0) : ctx.fonts.label (pal.uiFont, fh, isTonic ? 2 : 1));
            draw::shadowText (g, name, Rectangle<float> (lp.x - 40.0f, lp.y - fh * 0.62f, 80.0f, fh * 1.24f), Justification::centred,
                              isTonic ? pal.accentInk : (minorRing ? dimCol : textCol), tm && ! isTonic);
        }
    }

    // Constelación: las notas que suenan, unidas con líneas (aparecen y se van suavemente)
    std::vector<std::pair<int, float>> pts;   // índice en el círculo, brillo
    for (int pc = 0; pc < 12; ++pc)
        if (v.pcGlow[(size_t) pc] > 0.02f) pts.push_back ({ circleIndexOfPc (pc), v.pcGlow[(size_t) pc] });
    std::sort (pts.begin(), pts.end());
    const float sr = rMid + (rOut - rMid) * 0.82f;
    auto posOf = [&] (int i) { const float a = (float) i * seg - MathConstants<float>::halfPi; return c + Point<float> (std::cos (a), std::sin (a)) * sr; };

    if (pts.size() >= 2)
    {
        for (size_t a = 0; a < pts.size(); ++a)
        {
            const auto& p0 = pts[a];
            const auto& p1 = pts[(a + 1) % pts.size()];
            if (pts.size() == 2 && a == 1) break;
            g.setColour ((tm ? Colours::white : pal.constLine).withMultipliedAlpha (jmin (p0.second, p1.second)));
            g.drawLine ({ posOf (p0.first), posOf (p1.first) }, 1.4f);
        }
    }

    // Centro
    g.setColour (tm ? Colours::black.withAlpha (0.55f) : pal.center);
    g.fillEllipse (c.x - rIn, c.y - rIn, rIn * 2.0f, rIn * 2.0f);
    g.setColour (tm ? Colours::white.withAlpha (0.3f) : pal.ringLine);
    g.drawEllipse (c.x - rIn, c.y - rIn, rIn * 2.0f, rIn * 2.0f, 1.0f);

    const auto root = theory::pcName (v.keyRoot, v.sig, v.latin);
    const auto scName = String (CharPointer_UTF8 (sc.shortName));
    g.setFont (ctx.fonts.display (pal.displayFont, rIn * 0.48f));
    draw::shadowText (g, root, Rectangle<float> (c.x - rIn, c.y - rIn * 0.66f, rIn * 2.0f, rIn * 0.62f), Justification::centred, textCol, tm);
    g.setFont (draw::fit (ctx.fonts, pal.uiFont, scName, rIn * 0.21f, rIn * 1.7f, false));
    draw::shadowText (g, scName, Rectangle<float> (c.x - rIn, c.y - rIn * 0.02f, rIn * 2.0f, rIn * 0.3f), Justification::centred, dimCol, tm);
    const String sigTxt = v.sig.count == 0 ? U8 ("sin alteraciones")
                                           : String (v.sig.count) + (v.sig.flats ? (v.sig.count == 1 ? " bemol" : " bemoles")
                                                                                 : (v.sig.count == 1 ? " sostenido" : " sostenidos"));
    g.setFont (draw::fit (ctx.fonts, pal.uiFont, sigTxt, rIn * 0.18f, rIn * 1.6f, false));
    draw::shadowText (g, sigTxt, Rectangle<float> (c.x - rIn, c.y + rIn * 0.3f, rIn * 2.0f, rIn * 0.3f), Justification::centred, dimCol, tm);

    for (auto& p : pts)
    {
        const auto pos = posOf (p.first);
        const auto col = pal.noteColour (60 + majorPcAt (p.first));
        const float rr = 4.0f * k * (0.7f + 0.3f * p.second);
        ColourGradient glow (col.withAlpha (0.5f * p.second), pos.x, pos.y, col.withAlpha (0.0f), pos.x + rr * 3.2f, pos.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (pos.x - rr * 3.2f, pos.y - rr * 3.2f, rr * 6.4f, rr * 6.4f);
        g.setColour (col.withMultipliedAlpha (p.second));
        if (pal.pixel) g.fillRect (pos.x - rr, pos.y - rr, rr * 2.0f, rr * 2.0f);
        else           g.fillEllipse (pos.x - rr, pos.y - rr, rr * 2.0f, rr * 2.0f);
    }
}

void CircleSection::mouseDown (const MouseEvent& e)
{
    int i; bool minor;
    if (ringAt (e.position, i, minor) && onKeyClicked)
        onKeyClicked (minor ? theory::pcOf (majorPcAt (i) + 9) : majorPcAt (i), minor);
}

void CircleSection::mouseMove (const MouseEvent& e)
{
    int i = -1; bool minor = false;
    if (! ringAt (e.position, i, minor)) i = -1;
    if (i != hoverIndex || minor != hoverMinor)
    {
        hoverIndex = i; hoverMinor = minor;
        setMouseCursor (i >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
        repaint();
    }
}

void CircleSection::mouseExit (const MouseEvent&)
{
    if (hoverIndex != -1) { hoverIndex = -1; repaint(); }
}

//==============================================================================
// Partitura
//==============================================================================
ScoreSection::ScoreSection (UiContext& c) : Section (c, "partitura", "Pentagrama", { 500, 340 }) {}

static void drawGlyph (Graphics& g, const Font& f, juce_wchar cp, float x, float baseline)
{
    GlyphArrangement ga;
    ga.addLineOfText (f, String::charToString (cp), x, baseline);
    ga.draw (g);
}

void ScoreSection::paintContent (Graphics& g, Rectangle<float> area)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;
    const bool tm = transparentMode();

    auto paper = area.reduced (4.0f * k, 2.0f * k);
    Colour ink = pal.ink;
    if (tm)
    {
        g.setColour (Colours::black.withAlpha (0.32f));
        g.fillRoundedRectangle (paper, 8.0f * k);
        ink = Colours::white;
    }
    else
    {
        g.setColour (Colours::black.withAlpha (0.30f));
        g.fillRect (paper.translated (0.0f, 6.0f * k).reduced (4.0f * k, 0.0f));
        if (pal.paper != Tex::none)
        {
            g.setColour (pal.paperColour);
            g.fillRect (paper);
            draw::texture (g, ctx.tex.get (pal.paper), paper);
        }
        else
        {
            draw::panel (g, paper, pal.paperColour, pal.wellBorder, pal, k, 0.4f);
        }
    }

    // Título y armadura sobre el papel
    g.setColour (ink);
    g.setFont (pal.displayFont == FontKind::pixel ? ctx.fonts.pixel (12.0f * k) : ctx.fonts.serif (14.0f * k));
    g.drawText ("Pentagrama", paper.reduced (16.0f * k, 10.0f * k).removeFromTop (18.0f * k), Justification::centredLeft, false);

    String sigText;
    if (v.sig.count == 0)
        sigText = U8 ("Armadura: sin alteraciones");
    else
    {
        static const int sharpOrder[7] = { 3, 0, 4, 1, 5, 2, 6 };
        static const int flatOrder[7]  = { 6, 2, 5, 1, 4, 0, 3 };
        StringArray nm;
        for (int i = 0; i < v.sig.count; ++i)
            nm.add (theory::letterName (v.sig.flats ? flatOrder[i] : sharpOrder[i], v.latin) + (v.sig.flats ? "b" : "#"));
        sigText = "Armadura: " + String (v.sig.count) + (v.sig.flats ? (v.sig.count == 1 ? U8 (" bemol") : U8 (" bemoles"))
                                                                    : (v.sig.count == 1 ? U8 (" sostenido") : U8 (" sostenidos")))
                  + " (" + nm.joinIntoString (" ") + ")";
    }
    g.setColour (ink.withAlpha (0.8f));
    g.setFont (ctx.fonts.label (pal.uiFont, 11.5f * k, 0));
    g.drawText (sigText, paper.reduced (16.0f * k, 8.0f * k).removeFromBottom (16.0f * k), Justification::centredLeft, true);

    // Pentagrama doble
    auto staffArea = paper.reduced (14.0f * k, 0.0f).withTrimmedTop (30.0f * k).withTrimmedBottom (26.0f * k);
    const float sp = jmax (3.0f, jmin (staffArea.getHeight() / 20.0f, staffArea.getWidth() / 12.5f));
    const float midY = staffArea.getCentreY();
    const float gap = sp * 1.4f;
    auto yT = [&] (int d) { return midY - gap - (float) (d - 28) * sp * 0.5f; };
    auto yB = [&] (int d) { return midY + gap - (float) (d - 28) * sp * 0.5f; };

    const float lineW = jmax (1.0f, sp * 0.09f);
    const float left = staffArea.getX() + sp * 1.6f;
    const float right = staffArea.getRight() - sp * 0.3f;

    g.setColour (ink.withAlpha (0.75f));
    for (int d : { 30, 32, 34, 36, 38 }) g.fillRect (left, yT (d) - lineW * 0.5f, right - left, lineW);
    for (int d : { 18, 20, 22, 24, 26 }) g.fillRect (left, yB (d) - lineW * 0.5f, right - left, lineW);
    g.fillRect (left - lineW * 0.5f, yT (38), lineW * 1.4f, yB (18) - yT (38));
    g.fillRect (right - lineW, yT (38), lineW, yB (18) - yT (38));

    const auto mf = ctx.fonts.music (sp * 4.0f);
    {
        const float sysH = yB (18) - yT (38);
        const auto braceFont = ctx.fonts.music (sysH);
        const float bw = GlyphArrangement::getStringWidth (braceFont, String::charToString ((juce_wchar) 0xE000));
        g.setColour (ink.withAlpha (0.9f));
        drawGlyph (g, braceFont, 0xE000, left - bw - sp * 0.25f, yB (18));
    }
    g.setColour (ink);
    drawGlyph (g, mf, 0xE050, left + sp * 0.5f, yT (32));
    drawGlyph (g, mf, 0xE062, left + sp * 0.5f, yB (24));

    static const int trebleSharp[7] = { 38, 35, 39, 36, 33, 37, 34 };
    static const int trebleFlat[7]  = { 34, 37, 33, 36, 32, 35, 31 };
    static const int bassSharp[7]   = { 24, 21, 25, 22, 19, 23, 20 };
    static const int bassFlat[7]    = { 20, 23, 19, 22, 18, 21, 17 };
    float x = left + sp * 3.7f;
    const auto accGlyph = v.sig.flats ? (juce_wchar) 0xE260 : (juce_wchar) 0xE262;
    for (int i = 0; i < v.sig.count; ++i)
    {
        drawGlyph (g, mf, accGlyph, x, yT (v.sig.flats ? trebleFlat[i] : trebleSharp[i]));
        drawGlyph (g, mf, accGlyph, x, yB (v.sig.flats ? bassFlat[i] : bassSharp[i]));
        x += v.sig.flats ? sp * 1.0f : sp * 1.1f;
    }
    const float sigEnd = x;

    struct N { int midi, d; bool treble; int acc; bool showAcc; bool susOnly; bool shifted; int ottava; float xOff; int accCol; };
    std::vector<N> ns;
    for (auto m : v.active)
    {
        const auto s = theory::spell (m, v.sig);
        N n { m, s.diatonic(), m >= 60, s.acc, s.acc != v.sig.acc[(size_t) s.letter], v.down[(size_t) m] == 0, false, 0, 0.0f, 0 };
        if (n.treble && n.d > 45)   { n.d -= 7; n.ottava = 1; }
        if (! n.treble && n.d < 11) { n.d += 7; n.ottava = -1; }
        ns.push_back (n);
    }
    std::sort (ns.begin(), ns.end(), [] (const N& a, const N& b2) { return a.d < b2.d; });

    const float headW = GlyphArrangement::getStringWidth (mf, String::charToString ((juce_wchar) 0xE0A2));
    const float noteX = jmax (sigEnd + sp * 3.2f, (left + right) * 0.5f + sp * 0.8f);

    for (size_t i = 1; i < ns.size(); ++i)
        if (ns[i - 1].treble == ns[i].treble && ns[i].d - ns[i - 1].d <= 1 && ! ns[i - 1].shifted)
        {
            ns[i].shifted = true;
            ns[i].xOff = headW * 0.98f;
        }

    {
        std::vector<std::pair<int, int>> placed;
        for (int i = (int) ns.size() - 1; i >= 0; --i)
        {
            auto& n = ns[(size_t) i];
            if (! n.showAcc) continue;
            int col = 0;
            for (;;)
            {
                bool clash = false;
                for (auto& p : placed) if (p.second == col && std::abs (p.first - n.d) < 6) clash = true;
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

        g.setColour (ink.withAlpha (0.8f));
        auto ledger = [&] (int d) { g.fillRect (noteX - sp * 0.45f, ys (d) - lineW * 0.5f, headW + sp * 0.9f + (n.shifted ? headW : 0.0f), lineW); };
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

        const float alpha = (n.susOnly ? 0.45f : 1.0f) * jmax (0.35f, v.level[(size_t) n.midi]);
        const auto lightCol = pal.noteColour (n.midi);
        g.setColour (lightCol.withAlpha (0.30f * alpha));
        g.fillEllipse (nx + headW * 0.5f - sp, ny - sp, sp * 2.0f, sp * 2.0f);
        g.setColour ((tm ? Colours::white : pal.noteInk).withAlpha (alpha));
        drawGlyph (g, mf, 0xE0A2, nx, ny);

        if (n.showAcc)
        {
            const juce_wchar ag = n.acc > 0 ? 0xE262 : (n.acc < 0 ? 0xE260 : 0xE261);
            g.setColour (ink.withAlpha (alpha));
            drawGlyph (g, mf, ag, noteX - sp * 1.35f - (float) n.accCol * sp * 1.15f, ny);
        }
        if (n.ottava != 0)
        {
            g.setColour (ink.withAlpha (0.8f));
            g.setFont (ctx.fonts.label (pal.uiFont, sp * 1.1f, 1));
            const float ty = n.ottava > 0 ? ny - sp * 2.4f : ny + sp * 1.2f;
            g.drawText (n.ottava > 0 ? "8va" : "8vb", Rectangle<float> (nx - sp, ty, headW + sp * 2.0f, sp * 1.3f), Justification::centred);
        }
    }
}

//==============================================================================
// Acordes de la tonalidad (tablillas)
//==============================================================================
TabletsSection::TabletsSection (UiContext& c) : Section (c, "tablillas", "Acordes de la tonalidad", { 300, 350 }) {}

Rectangle<float> TabletsSection::tablet (int i) const
{
    const int n = 7;
    const float gapY = 6.0f * k;
    const float h = jmin (42.0f * k, (listArea.getHeight() - gapY * (float) (n - 1)) / (float) n);
    return { listArea.getX(), listArea.getY() + (float) i * (h + gapY), listArea.getWidth(), h };
}

void TabletsSection::paintContent (Graphics& g, Rectangle<float> area)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;
    const bool tm = transparentMode();
    caption (g, area, title);
    listArea = area;

    for (int i = 0; i < (int) v.diatonic.size() && i < 7; ++i)
    {
        const auto& d = v.diatonic[(size_t) i];
        auto r = tablet (i);
        const bool act = v.chord.valid && v.chord.root == d.rootPc && (d.mask & ~v.activeMask) == 0;
        auto fill = act ? pal.tabletOn : pal.tablet;
        if (tm) fill = act ? pal.accent.withAlpha (0.45f) : Colours::black.withAlpha (0.35f);
        if (i == hover) fill = fill.brighter (0.15f).withMultipliedAlpha (1.4f);
        draw::panel (g, r, fill, act ? pal.accent : (tm ? Colours::white.withAlpha (0.25f) : pal.tabletBorder), pal, k * 0.6f);

        auto inner = r.reduced (14.0f * k, 0.0f);
        g.setFont (pal.displayFont == FontKind::pixel ? ctx.fonts.pixel (r.getHeight() * 0.36f) : ctx.fonts.serif (r.getHeight() * 0.42f));
        draw::shadowText (g, d.roman, inner, Justification::centredLeft, tm ? Colours::white.withAlpha (0.8f) : pal.dim, tm);
        g.setFont (ctx.fonts.display (pal.displayFont, r.getHeight() * 0.48f));
        draw::shadowText (g, d.symbol, inner, Justification::centredRight, tm ? Colours::white : pal.text, tm);
    }
}

void TabletsSection::mouseDown (const MouseEvent& e)
{
    for (int i = 0; i < 7; ++i)
        if (tablet (i).contains (e.position)) { if (onPreview) onPreview (i); return; }
}

void TabletsSection::mouseMove (const MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < 7; ++i) if (tablet (i).contains (e.position)) h = i;
    if (h != hover)
    {
        hover = h;
        setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
        repaint();
    }
}

void TabletsSection::mouseExit (const MouseEvent&) { if (hover != -1) { hover = -1; repaint(); } }

//==============================================================================
// Acorde (la tapa del piano)
//==============================================================================
ChordSection::ChordSection (UiContext& c) : Section (c, "acorde", "Acorde", { 1100, 130 }) {}

void ChordSection::paintContent (Graphics& g, Rectangle<float> area)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;
    const bool tm = transparentMode();
    const int count = (int) v.active.size();

    String sub, leftTop, leftBig, leftSub;
    leftTop = "Grado en " + theory::keyTitle (v.keyRoot, v.scale, true);

    if (count == 0)
    {
        StringArray sn;
        for (auto s : theory::scales()[(size_t) v.scale].steps)
            sn.add (theory::pcName (v.keyRoot + s, v.sig, v.latin));
        sub = "Escala: " + sn.joinIntoString ("  ");
        leftBig = "";
        leftSub = U8 ("Toca una nota o un acorde");
    }
    else if (count == 1)
    {
        const int m = v.active.front();
        const double hz = 440.0 * std::pow (2.0, (m - 69) / 12.0);
        sub = theory::noteName (m, v.sig, true, false) + (m == 60 ? U8 (" central") : String()) + ", " + String (hz, 1) + " Hz";
        const auto deg = theory::noteDegree (theory::pcOf (m), v.keyRoot, v.scale);
        leftBig = deg.upToFirstOccurrenceOf (U8 (" ·"), false, false);
        leftSub = deg.fromFirstOccurrenceOf (U8 ("· "), false, false);
    }
    else if (v.chord.valid && (count >= 3 || v.chord.suffix == "5"))
    {
        sub = theory::pcName (v.chord.root, v.sig, true) + " " + v.chord.quality;
        if (v.chord.inversion != "estado fundamental") sub << ", " << v.chord.inversion;
        leftBig = v.degree.roman;
        leftSub = v.degree.function.replace (U8 (" · "), U8 (": "));
    }
    else if (count == 2)
    {
        const int iv = v.active[1] - v.active[0];
        sub = theory::noteName (v.active[0], v.sig, v.latin) + " a " + theory::noteName (v.active[1], v.sig, v.latin)
              + ", " + String (iv) + (iv == 1 ? " semitono" : " semitonos");
        const int ivc = iv % 12;
        const bool consonant = ivc == 0 || ivc == 3 || ivc == 4 || ivc == 5 || ivc == 7 || ivc == 8 || ivc == 9;
        leftBig = "Intervalo";
        leftSub = consonant ? U8 ("Consonante: estable") : U8 ("Disonante: con tensión");
    }
    else
    {
        sub = String (count) + U8 (" notas sin nombre de acorde común");
        leftBig = "";
        leftSub = U8 ("Prueba quitando una nota");
    }

    const bool wide = area.getWidth() > 720.0f * k;
    auto left = wide ? area.removeFromLeft (230.0f * k) : Rectangle<float>();
    auto right = wide ? area.removeFromRight (250.0f * k) : Rectangle<float>();

    const auto textCol = tm ? Colours::white : pal.text;
    const auto dimCol  = tm ? Colours::white.withAlpha (0.8f) : pal.dim;

    if (wide)
    {
        auto l = left;
        g.setFont (ctx.fonts.label (pal.uiFont, 11.5f * k, 1));
        draw::shadowText (g, leftTop, l.removeFromTop (l.getHeight() * 0.28f), Justification::bottomLeft, dimCol, tm);
        g.setFont (pal.displayFont == FontKind::pixel ? ctx.fonts.pixel (22.0f * k) : ctx.fonts.serif (28.0f * k, false));
        draw::shadowText (g, leftBig, l.removeFromTop (l.getHeight() * 0.55f), Justification::centredLeft, textCol, tm);
        g.setFont (ctx.fonts.label (pal.uiFont, 13.0f * k, 0));
        draw::shadowText (g, leftSub, l, Justification::topLeft, dimCol, tm);

        // Notas que suenan
        auto r = right.reduced (0.0f, 6.0f * k);
        const auto pf = ctx.fonts.label (pal.uiFont, 12.5f * k, 2);
        float x = r.getRight();
        float y = r.getY() + 4.0f * k;
        const float th = 24.0f * k;
        for (int i = (int) v.active.size() - 1; i >= 0; --i)
        {
            const int m = v.active[(size_t) i];
            const auto nm = theory::noteName (m, v.sig, v.latin);
            const float w = draw::textWidth (pf, nm) + 24.0f * k;
            if (x - w < r.getX()) { x = r.getRight(); y += th + 6.0f * k; }
            if (y + th > r.getBottom()) break;
            auto tr = Rectangle<float> (x - w, y, w, th);
            draw::panel (g, tr, tm ? Colours::black.withAlpha (0.4f) : pal.well, tm ? Colours::white.withAlpha (0.25f) : pal.wellBorder, pal, k * 0.4f);
            g.setColour (pal.noteColour (m).withMultipliedAlpha (jmax (0.4f, v.level[(size_t) m])));
            g.fillRect (tr.getX() + 7.0f * k, tr.getCentreY() - 3.5f * k, 7.0f * k, 7.0f * k);
            g.setColour (textCol);
            g.setFont (pf);
            g.drawText (nm, tr.withTrimmedLeft (16.0f * k), Justification::centred, false);
            x -= w + 6.0f * k;
        }
    }

    // Centro: el acorde grande (con transición suave) y la descripción
    auto centre = area;
    const float subH = 20.0f * k;
    auto subArea = centre.removeFromBottom (subH + 4.0f * k);
    float sealW = 0.0f;
    if (pal.seal && ! tm) sealW = 46.0f * k;

    if (pal.vfd && ! tm)
    {
        auto screen = centre.withSizeKeepingCentre (jmin (centre.getWidth(), 520.0f * k), centre.getHeight());
        draw::panel (g, screen, Colour (0xff0d0904), Colour (0xff2a1d0d), pal, k * 0.5f);
    }

    const float bigMax = jmin (centre.getHeight() * 0.92f, 74.0f * k);
    auto drawBig = [&] (const String& text, float alpha, float yOff)
    {
        if (text.isEmpty() || alpha <= 0.01f) return;
        const auto f = draw::fit (ctx.fonts, pal.displayFont, text, bigMax, centre.getWidth() - sealW - 10.0f * k, true);
        const float tw = draw::textWidth (f, text);
        auto r = centre.translated (0.0f, yOff);
        if (sealW > 0.0f)
        {
            auto seal = Rectangle<float> (r.getCentreX() - (tw + sealW) * 0.5f, r.getCentreY() - 20.0f * k, 40.0f * k, 40.0f * k);
            g.setColour (Colour (0xffa8341f).withAlpha (alpha));
            g.fillRoundedRectangle (seal, 4.0f * k);
            g.setColour (Colour (0xfffbf3e6).withAlpha (alpha));
            g.setFont (ctx.fonts.serif (15.0f * k, true));
            g.drawText (theory::pcName (v.keyRoot, v.sig, true), seal, Justification::centred, false);
            r = r.withTrimmedLeft (sealW);
        }
        g.setFont (f);
        if (pal.vfd && ! tm)
        {
            g.setColour (pal.chord.withAlpha (0.25f * alpha));
            g.drawText (text, r.translated (0.0f, 1.0f).expanded (1.0f), Justification::centred, false);
        }
        draw::shadowText (g, text, r, Justification::centred, (tm ? Colours::white : pal.chord).withAlpha (alpha), tm);
    };

    const float f = v.bigFade;
    const float ease = 1.0f - (1.0f - f) * (1.0f - f);
    drawBig (v.prevBigText, 1.0f - ease, -6.0f * k * ease);
    drawBig (v.bigText, ease, 6.0f * k * (1.0f - ease));

    g.setFont (draw::fit (ctx.fonts, pal.uiFont, sub, 14.0f * k, subArea.getWidth(), false));
    draw::shadowText (g, sub, subArea, Justification::centredTop, pal.vfd && ! tm ? pal.dim : dimCol, tm);
}

//==============================================================================
// Teclado
//==============================================================================
KeyboardSection::KeyboardSection (UiContext& c)
    : Section (c, "teclado", "Teclado", { 1164, 180 }),
      pianoSw (c), scaleSw (c), volume (c), names (c, { "C", "Do" })
{
    for (Component* comp : std::initializer_list<Component*> { &pianoSw, &scaleSw, &volume, &names })
        addAndMakeVisible (comp);
}

bool KeyboardSection::showCheeks() const
{
    return ! ctx.vis.hideParams && ! transparentMode();
}

void KeyboardSection::layoutContent (Rectangle<int> areaI)
{
    auto area = areaI.toFloat();
    const bool cheeks = showCheeks();
    for (Component* comp : std::initializer_list<Component*> { &pianoSw, &scaleSw, &volume, &names })
        comp->setVisible (cheeks);

    if (cheeks)
    {
        const float cw = 98.0f * k;
        leftCheek = area.removeFromLeft (cw);
        rightCheek = area.removeFromRight (cw);
        area.reduce (8.0f * k, 0.0f);

        auto sw = [&] (Rectangle<float> ch, float y) { return Rectangle<float> (ch.getCentreX() - 21.0f * k, ch.getY() + y, 42.0f * k, 22.0f * k).toNearestInt(); };
        pianoSw.setBounds (sw (leftCheek, 24.0f * k));
        volume.setBounds (Rectangle<float> (leftCheek.getCentreX() - 20.0f * k, leftCheek.getY() + 76.0f * k, 40.0f * k, 40.0f * k).toNearestInt());
        scaleSw.setBounds (sw (rightCheek, 24.0f * k));
        names.setBounds (Rectangle<float> (rightCheek.getCentreX() - 34.0f * k, rightCheek.getY() + 82.0f * k, 68.0f * k, 26.0f * k).toNearestInt());
    }
    else
    {
        leftCheek = rightCheek = {};
    }
    keysArea = area.withTrimmedTop (5.0f * k).withTrimmedBottom (2.0f * k);
}

KeyboardSection::KeyGeom KeyboardSection::keyRect (int note) const
{
    const float ww = keysArea.getWidth() / 52.0f;
    auto whiteIndex = [] (int n) { int c = 0; for (int x = lowNote; x < n; ++x) if (! isBlackPc (x % 12)) ++c; return c; };
    if (! isBlackPc (note % 12))
        return { Rectangle<float> (keysArea.getX() + (float) whiteIndex (note) * ww, keysArea.getY(), ww, keysArea.getHeight()), false };
    const float bw = ww * 0.62f;
    const float x = keysArea.getX() + (float) (whiteIndex (note - 1) + 1) * ww - bw * 0.5f;
    return { Rectangle<float> (x, keysArea.getY(), bw, keysArea.getHeight() * 0.62f), true };
}

int KeyboardSection::noteAt (Point<float> p, float& velocity) const
{
    if (! keysArea.contains (p)) return -1;
    for (int pass = 0; pass < 2; ++pass)
        for (int n = lowNote; n <= highNote; ++n)
        {
            const auto kg = keyRect (n);
            if (kg.black == (pass == 0) && kg.r.contains (p))
            {
                velocity = jlimit (0.2f, 1.0f, 0.3f + 0.7f * (p.y - kg.r.getY()) / kg.r.getHeight());
                return n;
            }
        }
    return -1;
}

void KeyboardSection::paintContent (Graphics& g, Rectangle<float>)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;

    // Laterales (cheek blocks) con sus etiquetas
    if (showCheeks())
    {
        for (auto ch : { leftCheek, rightCheek })
        {
            if (pal.cheekTex != Tex::none)
            {
                Graphics::ScopedSaveState ss (g);
                g.reduceClipRegion (draw::panelPath (ch, pal, k * 0.7f));
                draw::texture (g, ctx.tex.get (pal.cheekTex), ch);
                g.setColour (Colours::black.withAlpha (0.15f));
                g.fillRect (ch);
            }
            else
            {
                draw::panel (g, ch, pal.cheek, pal.wellBorder, pal, k * 0.7f);
            }
        }
        const auto labCol = pal.cheekTex != Tex::none ? Colour (0xfff1e4cc) : pal.dim;
        g.setColour (labCol);
        g.setFont (ctx.fonts.label (pal.uiFont, 11.0f * k, 1));
        g.drawText ("Piano", leftCheek.withHeight (22.0f * k), Justification::centredBottom, false);
        g.drawText ("Volumen", leftCheek.withTrimmedTop (52.0f * k).withHeight (20.0f * k), Justification::centred, false);
        g.drawText (volume.valueText, leftCheek.withTrimmedTop (118.0f * k).withHeight (20.0f * k), Justification::centred, false);
        g.drawText ("Escala", rightCheek.withHeight (22.0f * k), Justification::centredBottom, false);
        g.drawText ("Notas", rightCheek.withTrimmedTop (56.0f * k).withHeight (20.0f * k), Justification::centred, false);
    }

    // Cama del teclado y fieltro
    g.setColour (pal.keyBed);
    g.fillRect (keysArea.expanded (3.0f * k, 0.0f).withTrimmedTop (-5.0f * k));
    g.setColour (pal.felt);
    g.fillRect (keysArea.getX(), keysArea.getY() - 5.0f * k, keysArea.getWidth(), 5.0f * k);

    auto lit = [&] (int n) { return v.level[(size_t) n]; };

    for (int n = lowNote; n <= highNote; ++n)
    {
        const auto kg = keyRect (n);
        if (kg.black) continue;
        auto r = kg.r.reduced (0.6f, 0.0f);
        const float rad = pal.pixel ? 0.0f : 2.0f;
        g.setColour (pal.keyWhite);
        g.fillRoundedRectangle (r, rad);
        g.setColour (pal.keyShade.withAlpha (0.55f));
        g.fillRect (r.withTrimmedTop (r.getHeight() - 14.0f * k));

        const float l = lit (n);
        if (l > 0.003f)
        {
            const float susOnly = (v.down[(size_t) n] == 0 && v.sus[(size_t) n] > 0) ? 0.55f : 1.0f;
            g.setColour (pal.noteColour (n).withAlpha (jmin (1.0f, l * susOnly)));
            g.fillRoundedRectangle (r, rad);
            g.setColour (Colours::white.withAlpha (0.4f * l));
            g.fillRect (r.withHeight (2.0f));
        }

        const int pc = n % 12;
        if (v.highlight && (v.scaleMask & (1u << (unsigned) pc)) != 0)
        {
            const bool isRoot = pc == theory::pcOf (v.keyRoot);
            const float d = jmin (r.getWidth() * 0.34f, 7.0f * k);
            const auto dc = Point<float> (r.getCentreX(), r.getBottom() - d * 2.4f - (pc == 0 ? 13.0f * k : 0.0f));
            g.setColour (l > 0.3f ? Colours::black.withAlpha (0.5f) : pal.markOn);
            if (isRoot || pal.pixel) g.fillRect (dc.x - d * 0.5f, dc.y - d * 0.5f, d, d);
            else                     g.fillEllipse (dc.x - d * 0.32f, dc.y - d * 0.32f, d * 0.64f, d * 0.64f);
        }
        if (pc == 0 && r.getWidth() > 9.0f)
        {
            g.setColour (l > 0.3f ? Colours::black.withAlpha (0.65f) : pal.keyLabel);
            g.setFont (ctx.fonts.label (pal.uiFont, jmin (10.5f * k, r.getWidth() * 0.6f), 1));
            g.drawText ((v.latin ? String ("Do") : String ("C")) + String (n / 12 - 1), r.withTrimmedTop (r.getHeight() - 15.0f * k), Justification::centred, false);
        }
    }

    for (int n = lowNote; n <= highNote; ++n)
    {
        const auto kg = keyRect (n);
        if (! kg.black) continue;
        auto r = kg.r;
        const float rad = pal.pixel ? 0.0f : 2.0f;
        g.setColour (Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (r.translated (1.0f, 1.5f), rad);
        g.setColour (pal.keyBlack);
        g.fillRoundedRectangle (r, rad);
        g.setColour (Colours::white.withAlpha (0.09f));
        g.fillRect (r.reduced (r.getWidth() * 0.2f, 0.0f).withTrimmedTop (r.getHeight() - 7.0f * k).withHeight (2.0f));

        const float l = lit (n);
        if (l > 0.003f)
        {
            g.setColour (pal.noteColour (n).withMultipliedBrightness (0.88f).withAlpha (jmin (1.0f, l)));
            g.fillRoundedRectangle (r.reduced (1.0f, 0.0f).withTrimmedBottom (1.0f), rad);
        }
        const int pc = n % 12;
        if (v.highlight && (v.scaleMask & (1u << (unsigned) pc)) != 0)
        {
            const bool isRoot = pc == theory::pcOf (v.keyRoot);
            const float d = jmin (r.getWidth() * 0.42f, 6.5f * k);
            const auto dc = Point<float> (r.getCentreX(), r.getBottom() - d * 1.7f);
            g.setColour (l > 0.3f ? Colours::black.withAlpha (0.55f) : pal.markOn.brighter (0.4f));
            if (isRoot || pal.pixel) g.fillRect (dc.x - d * 0.5f, dc.y - d * 0.5f, d, d);
            else                     g.fillEllipse (dc.x - d * 0.32f, dc.y - d * 0.32f, d * 0.64f, d * 0.64f);
        }
    }
}

void KeyboardSection::mouseDown (const MouseEvent& e)
{
    float vel = 0.8f;
    const int n = noteAt (e.position, vel);
    if (n >= 0) { mouseNote = n; if (onNote) onNote (n, vel, true); }
}

void KeyboardSection::mouseDrag (const MouseEvent& e)
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

void KeyboardSection::mouseUp (const MouseEvent&)
{
    if (mouseNote >= 0 && onNote) onNote (mouseNote, 0.0f, false);
    mouseNote = -1;
}
