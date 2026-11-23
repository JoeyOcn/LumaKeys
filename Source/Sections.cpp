#include "Sections.h"

using namespace juce;

static bool isBlackPc (int pc) { return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }

// Posición de una tecla contando solo blancas (las negras toman la blanca de abajo)
static int whiteIndexOf (int note)
{
    static const int inOct[12] = { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 };
    return (note / 12) * 7 + inOct[note % 12];
}

//==============================================================================
Section::Section (UiContext& c, String sectionId, String sectionTitle, Point<int> naturalSize)
    : id (std::move (sectionId)), title (std::move (sectionTitle)), natural (naturalSize), ctx (c),
      detachBtn (c, "detach", "Separar en su propia ventana")
{
    detachBtn.framed = false;
    detachBtn.onClick = [this] { if (onDetach) onDetach(); };
    addAndMakeVisible (detachBtn);
}

void Section::refreshControls()
{
    resized();
    repaint();
}

void Section::resized()
{
    const int s = roundToInt (30.0f * k);
    const int pad = roundToInt (10.0f * k);
    detachBtn.setBounds (getWidth() - s - pad, pad, s, s);
    detachBtn.setVisible (wantsDetachButton() && ! floating && ! ctx.vis.hideParams);
    layoutContent (getLocalBounds());
}

void Section::caption (Graphics& g, Rectangle<float> area, const String& text, Colour colour)
{
    g.setFont (ctx.fonts.ui (15.0f * k, 2));
    draw::shadowText (g, text, Rectangle<float> (area.getX() + 18.0f * k, area.getY() + 12.0f * k, area.getWidth() - 70.0f * k, 22.0f * k),
                      Justification::centredLeft, colour, transparentMode());
}

void Section::paint (Graphics& g)
{
    paintContent (g, getLocalBounds().toFloat());
}

//==============================================================================
// Pentagrama
//==============================================================================
ScoreSection::ScoreSection (UiContext& c) : Section (c, "partitura", "Pentagrama", { 350, 388 }) {}

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
    auto paper = area.reduced (2.0f * k, 2.0f * k).withTrimmedBottom (4.0f * k);
    Colour ink = pal.ink;
    if (tm)
    {
        g.setColour (Colours::black.withAlpha (0.32f));
        g.fillRoundedRectangle (paper, 18.0f * k);
        ink = Colours::white;
    }
    else
    {
        draw::card (g, paper, pal, 18.0f * k);
    }
    caption (g, paper, "Pentagrama", tm ? Colours::white : pal.text);

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
    g.setColour (tm ? Colours::white.withAlpha (0.85f) : pal.dim);
    g.setFont (draw::fitUi (ctx.fonts, sigText, 15.0f * k, paper.getWidth() - 36.0f * k, 1));
    g.drawText (sigText, paper.reduced (18.0f * k, 12.0f * k).removeFromBottom (18.0f * k), Justification::centredLeft, true);

    // Pentagrama doble
    auto staffArea = paper.reduced (18.0f * k, 0.0f).withTrimmedTop (48.0f * k).withTrimmedBottom (40.0f * k);
    const float sp = jmax (3.0f, jmin (staffArea.getHeight() / 15.5f, staffArea.getWidth() / 13.0f));
    const float midY = staffArea.getCentreY();
    const float gap = sp * 1.5f;
    auto yT = [&] (int d) { return midY - gap - (float) (d - 28) * sp * 0.5f; };
    auto yB = [&] (int d) { return midY + gap - (float) (d - 28) * sp * 0.5f; };

    const float lineW = jmax (1.0f, sp * 0.09f);
    const float left = staffArea.getX() + sp * 1.4f;
    const float right = staffArea.getRight() - sp * 0.2f;

    g.setColour (ink.withAlpha (0.8f));
    for (int d : { 30, 32, 34, 36, 38 }) g.fillRect (left, yT (d) - lineW * 0.5f, right - left, lineW);
    for (int d : { 18, 20, 22, 24, 26 }) g.fillRect (left, yB (d) - lineW * 0.5f, right - left, lineW);
    g.fillRect (left - lineW * 0.5f, yT (38), lineW * 1.4f, yB (18) - yT (38));
    g.fillRect (right - lineW, yT (38), lineW, yB (18) - yT (38));

    const auto mf = ctx.fonts.music (sp * 4.0f);
    {
        const float sysH = yB (18) - yT (38);
        const auto braceFont = ctx.fonts.music (sysH);
        const float bw = GlyphArrangement::getStringWidth (braceFont, String::charToString ((juce_wchar) 0xE000));
        Graphics::ScopedSaveState ss (g);
        g.setColour (ink.withAlpha (0.9f));
        const float sx = 0.45f;
        g.addTransform (AffineTransform::scale (sx, 1.0f, left - sp * 0.3f, yB (18)));
        drawGlyph (g, braceFont, 0xE000, left - sp * 0.3f - bw, yB (18));
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
    const float noteX = jmax (sigEnd + sp * 3.2f, (left + right) * 0.5f + sp * 1.2f);

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
        g.setColour (lightCol.withAlpha (0.28f * alpha));
        g.fillEllipse (nx + headW * 0.5f - sp * 1.05f, ny - sp * 1.05f, sp * 2.1f, sp * 2.1f);
        g.setColour (ink.withAlpha (alpha));
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
            g.setFont (ctx.fonts.ui (sp * 1.15f, 1));
            const float ty = n.ottava > 0 ? ny - sp * 2.4f : ny + sp * 1.2f;
            g.drawText (n.ottava > 0 ? "8va" : "8vb", Rectangle<float> (nx - sp, ty, headW + sp * 2.0f, sp * 1.3f), Justification::centred);
        }
    }
}

//==============================================================================
// Pantalla
//==============================================================================
ScreenSection::ScreenSection (UiContext& c) : Section (c, "acorde", "Pantalla", { 424, 272 })
{
    detachBtn.lightOnDark = true;
}

void ScreenSection::paintContent (Graphics& g, Rectangle<float> area)
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

    Rectangle<float> scr;
    if (tm)
    {
        scr = area.reduced (4.0f * k);
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (scr, 16.0f * k);
    }
    else
    {
        draw::screen (g, area, pal, k);
        scr = area.reduced (8.0f * k);
    }
    const auto textCol = tm ? Colours::white : pal.scrText;
    const auto dimCol  = tm ? Colours::white.withAlpha (0.8f) : pal.scrDim;
    auto in = scr.reduced (18.0f * k, 14.0f * k);

    // Esquina: el grado
    {
        auto l = in;
        g.setFont (ctx.fonts.ui (15.5f * k, 1));
        draw::shadowText (g, leftTop, l.removeFromTop (20.0f * k), Justification::centredLeft, dimCol, tm);
        g.setFont (pal.pixelScreen ? ctx.fonts.pixel (32.0f * k) : ctx.fonts.ui (30.0f * k, 2));
        draw::shadowText (g, leftBig, l.removeFromTop (38.0f * k), Justification::centredLeft, textCol, tm);
        g.setFont (ctx.fonts.ui (15.0f * k, 1));
        draw::shadowText (g, leftSub, l.removeFromTop (20.0f * k), Justification::centredLeft, dimCol, tm);
    }

    // Lucecitas de estado (estilo Sampler)
    if (pal.pixelScreen && ! tm)
    {
        const Colour dots[4] = { Colour (0xff18c27a), Colour (0xff2e6bff), Colour (0xffffd400), pal.accent };
        for (int i = 0; i < 4; ++i)
        {
            const bool on = i < (int) v.active.size();
            g.setColour (dots[i].withAlpha (on ? 1.0f : 0.25f));
            g.fillRect (in.getRight() - (detachBtn.isVisible() ? 30.0f * k : 0.0f) - (float) (4 - i) * 16.0f * k, in.getY() + 4.0f * k, 11.0f * k, 11.0f * k);
        }
    }

    // Notas que suenan (abajo)
    auto tags = in.removeFromBottom (34.0f * k);
    {
        const auto pf = ctx.fonts.ui (15.0f * k, 2);
        float x = tags.getX();
        for (int i = 0; i < (int) v.active.size(); ++i)
        {
            const int m = v.active[(size_t) i];
            const auto nm = theory::noteName (m, v.sig, v.latin);
            const float w = draw::textWidth (pf, nm) + 28.0f * k;
            if (x + w > tags.getRight()) break;
            auto tr = Rectangle<float> (x, tags.getY() + 2.0f * k, w, 30.0f * k);
            const auto col = pal.rainbow ? pal.noteColour (m) : pal.noteTag (i);
            const float lv = jmax (0.45f, v.level[(size_t) m]);
            g.setColour (col.withAlpha (0.16f * lv));
            g.fillRoundedRectangle (tr, tr.getHeight() * 0.5f);
            g.setColour (col.withAlpha (lv));
            g.drawRoundedRectangle (tr.reduced (0.75f), tr.getHeight() * 0.5f, 1.6f);
            g.setColour (textCol);
            g.setFont (pf);
            g.drawText (nm, tr, Justification::centred, false);
            x += w + 9.0f * k;
        }
    }

    // Centro: el acorde grande (cambia con un desliz suave) y la descripción
    auto subArea = in.removeFromBottom (26.0f * k);
    auto centre = in.withTrimmedTop (20.0f * k);
    const float bigMax = jmin (centre.getHeight() * 0.95f, (pal.pixelScreen ? 120.0f : 100.0f) * k);
    // Lo que ocupa el texto de la esquina (para que el acorde grande nunca le pase encima)
    const float leftRight = in.getX() + jmax (draw::textWidth (ctx.fonts.ui (15.5f * k, 1), leftTop),
                                              draw::textWidth (ctx.fonts.ui (15.0f * k, 1), leftSub)) + 14.0f * k;
    const float cornerBottom = in.getY() + 80.0f * k;
    auto fitBig = [&] (const String& text, Rectangle<float> box, float maxH, float sidePad)
    {
        Font f = pal.pixelScreen ? ctx.fonts.pixel (maxH) : ctx.fonts.ui (maxH, 2);
        float h = maxH;
        while (h > 12.0f && draw::textWidth (f, text) > box.getWidth() - sidePad)
        {
            h *= 0.92f;
            f = pal.pixelScreen ? ctx.fonts.pixel (h) : ctx.fonts.ui (h, 2);
        }
        return f;
    };
    bool dropBelow = false;
    {
        const auto f0 = fitBig (v.bigText, centre, bigMax, 150.0f * k);
        const auto box = centre.translated (60.0f * k, 0.0f).withTrimmedRight (60.0f * k);
        const float tw = draw::textWidth (f0, v.bigText);
        const float textLeft = box.getCentreX() - tw * 0.5f;
        const float textTop = box.getCentreY() - f0.getAscent() * 0.75f;
        dropBelow = textLeft < leftRight && textTop < cornerBottom;
    }
    auto bigBox = dropBelow ? in.withTop (cornerBottom) : centre.translated (60.0f * k, 0.0f).withTrimmedRight (60.0f * k);
    const float bigH = dropBelow ? jmin (bigBox.getHeight() * 1.05f, bigMax) : bigMax;
    auto drawBig = [&] (const String& text, float alpha, float yOff, float scale)
    {
        if (text.isEmpty() || alpha <= 0.01f) return;
        const auto f = fitBig (text, dropBelow ? bigBox : centre, bigH, dropBelow ? 20.0f * k : 150.0f * k);
        g.setFont (f.withHeight (f.getHeight() * scale));
        auto r = bigBox.translated (0.0f, yOff);
        if (! tm)
        {
            g.setColour (textCol.withAlpha (0.12f * alpha));
            g.drawText (text, r.translated (0.0f, 2.0f), Justification::centred, false);
        }
        draw::shadowText (g, text, r, Justification::centred, textCol.withAlpha (alpha), tm);
    };
    const float f = v.bigFade;
    const float ease = 1.0f - (1.0f - f) * (1.0f - f);
    drawBig (v.prevBigText, 1.0f - ease, -10.0f * k * ease, 1.0f - 0.06f * ease);
    drawBig (v.bigText, ease, 10.0f * k * (1.0f - ease), 0.94f + 0.06f * ease);

    g.setFont (draw::fitUi (ctx.fonts, sub, 15.5f * k, subArea.getWidth(), 1));
    draw::shadowText (g, sub, subArea, Justification::centred, dimCol, tm);
}

//==============================================================================
// Grados
//==============================================================================
DegreesSection::DegreesSection (UiContext& c) : Section (c, "tablillas", "Grados", { 424, 104 }) {}

Rectangle<float> DegreesSection::tile (int i) const
{
    const float gap = 8.0f * k;
    const float w = (listArea.getWidth() - gap * 6.0f) / 7.0f;
    return { listArea.getX() + (float) i * (w + gap), listArea.getY(), w, listArea.getHeight() };
}

bool DegreesSection::stepAnimation (float dt)
{
    bool moving = false;
    for (int i = 0; i < 7; ++i)
    {
        const float t = (i == pressed) ? 1.0f : 0.0f;
        press[(size_t) i] = approach (press[(size_t) i], t, 0.5f, dt);
        moving = moving || press[(size_t) i] != t;
    }
    repaint();
    return moving;
}

void DegreesSection::paintContent (Graphics& g, Rectangle<float> area)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;
    const bool tm = transparentMode();
    listArea = area.reduced (1.0f, 6.0f * k);

    for (int i = 0; i < (int) v.diatonic.size() && i < 7; ++i)
    {
        const auto& d = v.diatonic[(size_t) i];
        auto r = tile (i);
        const float glow = v.degreeGlow[(size_t) i];
        if (tm)
        {
            g.setColour (Colours::black.withAlpha (0.35f).interpolatedWith (pal.accent.withAlpha (0.8f), glow));
            g.fillRoundedRectangle (r, 12.0f * k);
        }
        else
        {
            draw::button (g, r, pal, 12.0f * k, press[(size_t) i], false, i == hover ? 1.0f : 0.0f);
            if (glow > 0.01f)
            {
                auto face = r.withTrimmedBottom (2.5f).translated (0.0f, 2.5f * press[(size_t) i]);
                g.setColour (pal.accent.withAlpha (glow));
                g.fillRoundedRectangle (face, 12.0f * k);
                g.setColour (pal.accent.withAlpha (0.35f * glow));
                g.drawRoundedRectangle (face.expanded (2.0f), 13.0f * k, 2.0f);
            }
        }
        auto inner = r.withTrimmedBottom (2.5f).translated (0.0f, 2.5f * press[(size_t) i]);
        const auto textCol = tm ? Colours::white : pal.btnText.interpolatedWith (pal.accentInk, glow);
        const auto dimCol = tm ? Colours::white.withAlpha (0.8f) : pal.btnText.withAlpha (0.55f).interpolatedWith (pal.accentInk, glow);
        g.setColour (dimCol);
        g.setFont (ctx.fonts.ui (15.5f * k, 1));
        g.drawText (d.roman, inner.withHeight (inner.getHeight() * 0.42f).translated (0.0f, 6.0f * k), Justification::centred, false);
        g.setColour (textCol);
        g.setFont (draw::fitUi (ctx.fonts, d.symbol, 20.0f * k, inner.getWidth() - 8.0f, 2));
        g.drawText (d.symbol, inner.withTrimmedTop (inner.getHeight() * 0.42f).withTrimmedBottom (6.0f * k), Justification::centred, false);
    }
}

void DegreesSection::mouseDown (const MouseEvent& e)
{
    for (int i = 0; i < 7; ++i)
        if (tile (i).contains (e.position))
        {
            pressed = i;
            animate();
            if (onPreview) onPreview (i);
            return;
        }
}

void DegreesSection::mouseUp (const MouseEvent&)
{
    pressed = -1;
    animate();
}

void DegreesSection::mouseMove (const MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < 7; ++i) if (tile (i).contains (e.position)) h = i;
    if (h != hover)
    {
        hover = h;
        setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
        repaint();
    }
}

void DegreesSection::mouseExit (const MouseEvent&) { if (hover != -1) { hover = -1; repaint(); } }

//==============================================================================
// Círculo de quintas
//==============================================================================
CircleSection::CircleSection (UiContext& c) : Section (c, "circulo", U8 ("Círculo de quintas"), { 350, 388 }) {}

static int circleIndexOfPc (int pc) { return (theory::pcOf (pc) * 7) % 12; }
static int majorPcAt (int i)        { return (i * 7) % 12; }

bool CircleSection::ringAt (Point<float> p, int& index, bool& minor) const
{
    const float dist = p.getDistanceFrom (centre);
    if (dist > radius || dist < radius * 0.45f) return false;
    float ang = std::atan2 (p.y - centre.y, p.x - centre.x) + MathConstants<float>::halfPi;
    if (ang < 0) ang += MathConstants<float>::twoPi;
    index = (int) std::floor ((ang + MathConstants<float>::pi / 12.0f) / (MathConstants<float>::twoPi / 12.0f)) % 12;
    minor = dist < radius * 0.70f;
    return true;
}

// Une las notas en orden del círculo. Si una línea pasaría por la perilla, se curva por fuera.
void CircleSection::buildLoop (const std::vector<int>& idx, float rn, float knobR, std::vector<Point<float>>& out) const
{
    out.clear();
    if (idx.size() < 2) return;
    const float seg = MathConstants<float>::twoPi / 12.0f;
    auto pt = [&] (float a, float r) { return centre + Point<float> (std::cos (a), std::sin (a)) * r; };
    const size_t n = idx.size();
    const size_t segs = n == 2 ? 1 : n;
    for (size_t s = 0; s < segs; ++s)
    {
        const float A = (float) idx[s] * seg - MathConstants<float>::halfPi;
        const float B = (float) idx[(s + 1) % n] * seg - MathConstants<float>::halfPi;
        float d = std::fmod (B - A + MathConstants<float>::twoPi * 2.0f, MathConstants<float>::twoPi);
        if (d > MathConstants<float>::pi) d -= MathConstants<float>::twoPi;
        if (std::abs (std::abs (d) - MathConstants<float>::pi) < 0.01f) d = MathConstants<float>::pi;   // opuestos: rodear por un lado fijo
        const float mid = A + d * 0.5f;
        const float straight = rn * std::cos (std::abs (d) * 0.5f);
        const float minR = knobR * 1.3f;
        const float cr = straight >= minR ? straight : 2.0f * minR - straight;
        const auto p0 = pt (A, rn), p1 = pt (B, rn), cp = pt (mid, cr);
        const int steps = 28;
        for (int i = (s == 0 ? 0 : 1); i <= steps; ++i)
        {
            const float t = (float) i / (float) steps;
            const float u = 1.0f - t;
            out.push_back (p0 * (u * u) + cp * (2.0f * u * t) + p1 * (t * t));
        }
    }
}

void CircleSection::paintContent (Graphics& g, Rectangle<float> area)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;
    const bool tm = transparentMode();
    auto plate = area.reduced (2.0f * k).withTrimmedBottom (4.0f * k);
    if (tm)
    {
        g.setColour (Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (plate, 18.0f * k);
    }
    else
    {
        draw::recess (g, plate, pal, 18.0f * k, pal.plate);
    }
    caption (g, plate, title, tm ? Colours::white : pal.text);

    auto ringArea = plate.reduced (14.0f * k).withTrimmedTop (34.0f * k);
    radius = jmax (20.0f, jmin (ringArea.getWidth(), ringArea.getHeight()) * 0.5f - 2.0f);
    centre = ringArea.getCentre();
    const auto c = centre;
    const float rOut = radius, rMid = radius * 0.70f, rIn = radius * 0.45f;
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
            g.setColour (isTonic ? pal.accent : (dist <= 1 ? pal.region : (tm ? Colours::black.withAlpha (0.2f) : pal.ring)));
            g.fillPath (p);
            if (i == hoverIndex && minorRing == hoverMinor)
            {
                g.setColour (textCol.withAlpha (0.12f));
                g.fillPath (p);
            }
            g.setColour (tm ? Colours::white.withAlpha (0.3f) : pal.plateLine);
            g.strokePath (p, PathStrokeType (1.0f));

            const float ang = (float) i * seg - MathConstants<float>::halfPi;
            const float lr = minorRing ? (rIn + rMid) * 0.5f : (rMid + rOut) * 0.5f;
            const auto lp = c + Point<float> (std::cos (ang) * lr, std::sin (ang) * lr);
            const int pc = minorRing ? theory::pcOf (majorPcAt (i) + 9) : majorPcAt (i);
            const auto sigHere = theory::keySignature (pc, minorRing ? 1 : 0);
            auto name = theory::pcName (pc, sigHere, v.latin) + (minorRing ? "m" : "");
            const float fh = minorRing ? jmax (10.5f, radius * 0.098f) : jmax (12.0f, radius * 0.115f);
            g.setFont (ctx.fonts.ui (fh, minorRing ? 1 : 2));
            draw::shadowText (g, name, Rectangle<float> (lp.x - 40.0f, lp.y - fh * 0.62f, 80.0f, fh * 1.24f), Justification::centred,
                              isTonic ? pal.accentInk : (minorRing ? dimCol : textCol), tm && ! isTonic);
        }
    }

    // --- Constelación: líneas que rodean la perilla, se dibujan solas y una luz las recorre
    const double t = v.time;
    std::vector<int> idx;
    for (int pc = 0; pc < 12; ++pc)
        if (v.activeMask & (1u << (unsigned) pc)) idx.push_back (circleIndexOfPc (pc));
    std::sort (idx.begin(), idx.end());
    uint16_t circMask = 0;
    for (auto i : idx) circMask = (uint16_t) (circMask | (1u << (unsigned) i));

    const float rn = rOut * 0.955f;
    if (circMask != shownMask)
    {
        if (circMask == 0) releasedAt = t;
        else               changedAt = t;
        shownMask = circMask;
    }
    std::vector<Point<float>> loop;
    buildLoop (idx, rn, rIn, loop);
    if (! loop.empty()) lastLoop = loop;

    float lineAlpha = 1.0f;
    const auto* path = &loop;
    if (loop.empty())
    {
        lineAlpha = jmax (0.0f, 1.0f - (float) (t - releasedAt) / 0.45f);
        path = &lastLoop;
    }
    const float drawOn = jlimit (0.0f, 1.0f, (float) (t - changedAt) / 0.38f);
    const float drawEase = 1.0f - (1.0f - drawOn) * (1.0f - drawOn);
    const auto lineCol = tm ? Colours::white : pal.accent;
    animating = false;

    if (path->size() >= 2 && lineAlpha > 0.01f)
    {
        std::vector<float> cum (path->size(), 0.0f);
        for (size_t i = 1; i < path->size(); ++i) cum[i] = cum[i - 1] + (*path)[i].getDistanceFrom ((*path)[i - 1]);
        const float total = cum.back();
        const float limit = loop.empty() ? total : total * drawEase;
        Path line;
        line.startNewSubPath ((*path)[0]);
        for (size_t i = 1; i < path->size() && cum[i - 1] < limit; ++i)
        {
            if (cum[i] <= limit) line.lineTo ((*path)[i]);
            else
            {
                const float f = (limit - cum[i - 1]) / jmax (0.001f, cum[i] - cum[i - 1]);
                line.lineTo ((*path)[i - 1] + ((*path)[i] - (*path)[i - 1]) * f);
            }
        }
        // Brillo que respira
        const float breathe = 0.5f + 0.5f * (float) std::sin (t * 2.4);
        g.setColour (lineCol.withAlpha ((0.10f + 0.08f * breathe) * lineAlpha));
        g.strokePath (line, PathStrokeType (8.0f * k, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (lineCol.withAlpha (0.95f * lineAlpha));
        g.strokePath (line, PathStrokeType (2.2f * k, PathStrokeType::curved, PathStrokeType::rounded));

        // Luz que recorre las líneas (cometa)
        if (! loop.empty() && drawOn >= 1.0f && total > 1.0f)
        {
            auto pointAt = [&] (float d)
            {
                d = std::fmod (d + total * 4.0f, total);
                const auto it = std::upper_bound (cum.begin(), cum.end(), d);
                const size_t i = (size_t) jlimit (1, (int) cum.size() - 1, (int) (it - cum.begin()));
                const float f = (d - cum[i - 1]) / jmax (0.001f, cum[i] - cum[i - 1]);
                return (*path)[i - 1] + ((*path)[i] - (*path)[i - 1]) * f;
            };
            const float head = (float) std::fmod (t * 160.0 * k, (double) total);
            for (int j = 0; j < 10; ++j)
            {
                const auto p = pointAt (head - (float) j * 5.0f * k);
                const float a = (1.0f - (float) j / 10.0f);
                const float rr = (3.4f - 0.2f * (float) j) * k;
                g.setColour (Colours::white.withAlpha (0.85f * a));
                g.fillEllipse (p.x - rr * 0.5f, p.y - rr * 0.5f, rr, rr);
            }
            // Destellos que viajan en sentido contrario, más suaves
            const auto q = pointAt (total - head * 0.6f);
            g.setColour (lineCol.brighter (0.6f).withAlpha (0.6f));
            g.fillEllipse (q.x - 2.0f * k, q.y - 2.0f * k, 4.0f * k, 4.0f * k);
        }
        animating = true;
    }

    // --- La perilla del centro: apunta a la tonalidad
    {
        Colour cap = tm ? Colours::black.withAlpha (0.6f) : pal.caps[0];
        if (! tm) draw::knobBody (g, c, rIn, pal, cap);
        else { g.setColour (cap); g.fillEllipse (c.x - rIn, c.y - rIn, rIn * 2.0f, rIn * 2.0f); }
        const float a = v.knobAngle;
        const auto i0 = c + Point<float> (std::sin (a), -std::cos (a)) * (rIn * 0.80f);
        const auto i1 = c + Point<float> (std::sin (a), -std::cos (a)) * (rIn * 0.96f);
        g.setColour (pal.accent);
        g.drawLine ({ i0, i1 }, jmax (3.0f, rIn * 0.08f));

        const auto root = theory::pcName (v.keyRoot, v.sig, v.latin);
        const auto scName = String (CharPointer_UTF8 (sc.shortName));
        g.setColour (Colours::white);
        g.setFont (ctx.fonts.ui (rIn * 0.5f, 2));
        g.drawText (root, Rectangle<float> (c.x - rIn, c.y - rIn * 0.62f, rIn * 2.0f, rIn * 0.66f), Justification::centred, false);
        const String sigTxt = v.sig.count == 0 ? U8 ("sin alteraciones")
                                               : String (v.sig.count) + (v.sig.flats ? (v.sig.count == 1 ? " bemol" : " bemoles")
                                                                                     : (v.sig.count == 1 ? " sostenido" : " sostenidos"));
        g.setColour (Colours::white.withAlpha (0.85f));
        g.setFont (draw::fitUi (ctx.fonts, scName, rIn * 0.23f, rIn * 1.5f, 1));
        g.drawText (scName, Rectangle<float> (c.x - rIn, c.y + rIn * 0.06f, rIn * 2.0f, rIn * 0.26f), Justification::centred, false);
        g.setFont (draw::fitUi (ctx.fonts, sigTxt, rIn * 0.2f, rIn * 1.4f, 0));
        g.drawText (sigTxt, Rectangle<float> (c.x - rIn, c.y + rIn * 0.32f, rIn * 2.0f, rIn * 0.24f), Justification::centred, false);
    }

    // --- Nodos: puntitos de color que laten
    int ni = 0;
    for (int pc = 0; pc < 12; ++pc)
    {
        const float gl = v.pcGlow[(size_t) pc];
        if (gl < 0.02f) continue;
        const int i = circleIndexOfPc (pc);
        const float a = (float) i * seg - MathConstants<float>::halfPi;
        const auto pos = c + Point<float> (std::cos (a), std::sin (a)) * rn;
        const auto col = pal.rainbow ? pal.noteColour (pc) : pal.noteTag (ni++);
        const float pulse = 1.0f + 0.14f * (float) std::sin (t * 4.0 + (double) i);
        const float rr = 6.0f * k * (0.6f + 0.4f * gl) * pulse;
        ColourGradient glow (col.withAlpha (0.45f * gl), pos.x, pos.y, col.withAlpha (0.0f), pos.x + rr * 3.2f, pos.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (pos.x - rr * 3.2f, pos.y - rr * 3.2f, rr * 6.4f, rr * 6.4f);
        g.setColour (col.withMultipliedAlpha (gl));
        g.fillEllipse (pos.x - rr, pos.y - rr, rr * 2.0f, rr * 2.0f);
        g.setColour (Colours::white.withAlpha (0.9f * gl));
        g.drawEllipse (pos.x - rr, pos.y - rr, rr * 2.0f, rr * 2.0f, 1.5f * k);
        animating = true;
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
// Teclado
//==============================================================================
KeyboardSection::KeyboardSection (UiContext& c)
    : Section (c, "teclado", "Teclado", { 1156, 234 }),
      pianoSw (c), scaleSw (c), volume (c),
      soundBtn (c, "Sonido"),
      pedalBtn (c, {}, U8 ("Sostiene las notas como el pedal de un piano (sin pedal físico)"), "Pedal"),
      keysColourBtn (c, "keys", "Color y forma de las teclas", "Teclas"),
      uiColourBtn (c, "palette", "Color de la interfaz", "Interfaz"),
      leftBtn (c, "left", U8 ("Bajar una octava")), rightBtn (c, "right", U8 ("Subir una octava")),
      oct2 (c, {}, "2 octavas", "2"), oct3 (c, {}, "3 octavas", "3"), oct4 (c, {}, "4 octavas", "4")
{
    for (Component* comp : std::initializer_list<Component*> { &pianoSw, &scaleSw, &volume, &soundBtn, &pedalBtn, &keysColourBtn, &uiColourBtn, &leftBtn, &rightBtn, &oct2, &oct3, &oct4 })
        addAndMakeVisible (comp);
    leftBtn.onClick  = [this] { if (onShift) onShift (-1); };
    rightBtn.onClick = [this] { if (onShift) onShift (1); };
    oct2.onClick = [this] { if (onOctaves) onOctaves (2); };
    oct3.onClick = [this] { if (onOctaves) onOctaves (3); };
    oct4.onClick = [this] { if (onOctaves) onOctaves (4); };
    volume.capIndex = 1;
}

void KeyboardSection::setRange (int s, int o)
{
    startC = s;
    octaves = o;
    oct2.setActive (o == 2);
    oct3.setActive (o == 3);
    oct4.setActive (o == 4);
    repaint();
}

bool KeyboardSection::showCheeks() const
{
    return ! ctx.vis.hideParams && ! transparentMode();
}

void KeyboardSection::layoutContent (Rectangle<int> areaI)
{
    auto area = areaI.toFloat().reduced (2.0f * k).withTrimmedBottom (4.0f * k);
    const bool cheeks = showCheeks();
    for (Component* comp : std::initializer_list<Component*> { &pianoSw, &scaleSw, &volume, &soundBtn, &pedalBtn, &keysColourBtn, &uiColourBtn, &leftBtn, &rightBtn, &oct2, &oct3, &oct4 })
        comp->setVisible (cheeks);

    if (cheeks)
    {
        auto in = area.reduced (18.0f * k, 16.0f * k);
        leftCheek = in.removeFromLeft (172.0f * k);
        rightCheek = in.removeFromRight (172.0f * k);
        in.reduce (14.0f * k, 0.0f);
        keysArea = in;

        auto L = leftCheek;
        soundBtn.k = k;
        soundBtn.setBounds (Rectangle<float> (L.getX(), L.getY(), L.getWidth(), 48.0f * k).toNearestInt());
        pianoSw.setBounds (Rectangle<float> (L.getX(), L.getY() + 82.0f * k, 54.0f * k, 30.0f * k).toNearestInt());
        volume.setBounds (Rectangle<float> (L.getRight() - 70.0f * k, L.getY() + 58.0f * k, 68.0f * k, 68.0f * k).toNearestInt());
        const float half = (L.getWidth() - 8.0f * k) * 0.5f;
        keysColourBtn.setBounds (Rectangle<float> (L.getX(), L.getBottom() - 40.0f * k, half, 40.0f * k).toNearestInt());
        uiColourBtn.setBounds (Rectangle<float> (L.getRight() - half, L.getBottom() - 40.0f * k, half, 40.0f * k).toNearestInt());

        auto R = rightCheek;
        const bool det = detachBtn.isVisible();
        pedalBtn.setBounds (Rectangle<float> (R.getX(), R.getY(), R.getWidth() - (det ? 40.0f * k : 0.0f), 48.0f * k).toNearestInt());
        if (det) detachBtn.setBounds (Rectangle<float> (R.getRight() - 32.0f * k, R.getY() + 9.0f * k, 32.0f * k, 30.0f * k).toNearestInt());
        scaleSw.setBounds (Rectangle<float> (R.getRight() - 56.0f * k, R.getY() + 60.0f * k, 54.0f * k, 30.0f * k).toNearestInt());
        auto nav = Rectangle<float> (R.getX(), R.getY() + 102.0f * k, R.getWidth(), 40.0f * k);
        leftBtn.setBounds (nav.removeFromLeft (40.0f * k).toNearestInt());
        rightBtn.setBounds (nav.removeFromRight (40.0f * k).toNearestInt());
        rangeDisplay = nav.reduced (4.0f * k, 2.0f * k);
        auto counts = Rectangle<float> (R.getX(), R.getBottom() - 36.0f * k, R.getWidth(), 36.0f * k);
        const float bw = (counts.getWidth() - 12.0f * k) / 3.0f;
        oct2.setBounds (counts.removeFromLeft (bw).toNearestInt()); counts.removeFromLeft (6.0f * k);
        oct3.setBounds (counts.removeFromLeft (bw).toNearestInt()); counts.removeFromLeft (6.0f * k);
        oct4.setBounds (counts.toNearestInt());
    }
    else
    {
        leftCheek = rightCheek = {};
        keysArea = area.reduced (14.0f * k, 14.0f * k);
    }
}

KeyboardSection::KeyGeom KeyboardSection::keyRect (int note) const
{
    const auto& v = ctx.vis;
    const float ww = keysArea.getWidth() / jmax (1.0f, v.viewWhites);
    const int st = ctx.vis.pal.keyStyle;
    const float x0 = keysArea.getX();
    const auto ka = keysArea;
    if (! isBlackPc (note % 12))
    {
        const float x = x0 + ((float) whiteIndexOf (note) - v.viewWhite) * ww;
        if (st == keysPastilla) return { Rectangle<float> (x, ka.getY() + ka.getHeight() * 0.33f, ww, ka.getHeight() * 0.67f), false };
        if (st == keysBotones)  return { Rectangle<float> (x, ka.getY() + ka.getHeight() * 0.52f, ww, ka.getHeight() * 0.48f), false };
        return { Rectangle<float> (x, ka.getY(), ww, ka.getHeight()), false };
    }
    const float cx = x0 + ((float) whiteIndexOf (note - 1) + 1.0f - v.viewWhite) * ww;
    if (st == keysPastilla)
    {
        const float d = jmin (ww * 0.74f, ka.getHeight() * 0.28f);
        return { Rectangle<float> (cx - d * 0.5f, ka.getY() + ka.getHeight() * 0.15f - d * 0.5f, d, d), true };
    }
    if (st == keysBotones) return { Rectangle<float> (cx - ww * 0.5f, ka.getY(), ww, ka.getHeight() * 0.48f), true };
    const float bw = ww * (st == keysPlana ? 0.6f : 0.62f);
    return { Rectangle<float> (cx - bw * 0.5f, ka.getY(), bw, ka.getHeight() * (st == keysPlana ? 0.58f : 0.6f)), true };
}

int KeyboardSection::noteAt (Point<float> p, float& velocity) const
{
    if (! keysArea.contains (p)) return -1;
    const int lo = jmax (21, startC - 12), hi = jmin (108, lastNote() + 12);
    for (int pass = 0; pass < 2; ++pass)
        for (int n = lo; n <= hi; ++n)
        {
            const auto kg = keyRect (n);
            if (kg.black == (pass == 0) && kg.r.contains (p) && ! (kg.black && (kg.r.getCentreX() < keysArea.getX() + 2.0f || kg.r.getCentreX() > keysArea.getRight() - 2.0f)))
            {
                velocity = jlimit (0.2f, 1.0f, 0.3f + 0.7f * (p.y - kg.r.getY()) / jmax (1.0f, kg.r.getHeight()));
                return n;
            }
        }
    return -1;
}

void KeyboardSection::drawKey (Graphics& g, int n, const KeyGeom& kg) const
{
    const auto& v = ctx.vis;
    const auto& pal = v.pal;
    const int st = pal.keyStyle;
    const float l = v.level[(size_t) n];
    const float susOnly = (v.down[(size_t) n] == 0 && v.sus[(size_t) n] > 0) ? 0.55f : 1.0f;
    const float pressDepth = jmin (1.0f, l * 1.4f) * 2.0f * k;
    const auto litCol = pal.noteColour (n);
    const int pc = n % 12;
    const bool inScale = v.highlight && (v.scaleMask & (1u << (unsigned) pc)) != 0;
    const bool isRoot = pc == theory::pcOf (v.keyRoot);
    const float ww = kg.r.getWidth();

    auto r = kg.r;
    if (! kg.black)
    {
        const float gap = (st == keysClasica) ? 1.0f : jmax (2.0f, 5.0f * k);
        r = r.reduced (gap * 0.5f, 0.0f);
        const float rad = st == keysPastilla ? r.getWidth() * 0.5f : (st == keysBotones ? 8.0f * k : (st == keysPlana ? 9.0f * k : 5.0f * k));
        auto face = r.translated (0.0f, pressDepth);
        if (st != keysClasica)
        {
            g.setColour (Colours::black.withAlpha ((pal.night ? 0.4f : 0.16f) * (1.0f - 0.5f * l)));
            g.fillRoundedRectangle (r.translated (0.0f, 3.0f * k), rad);
        }
        g.setColour (pal.keyW);
        g.fillRoundedRectangle (face, rad);
        if (l > 0.003f)
        {
            g.setColour (litCol.withAlpha (jmin (1.0f, l * susOnly)));
            g.fillRoundedRectangle (face, rad);
        }
        g.setColour (pal.keyWEdge.withAlpha (0.9f));
        g.drawRoundedRectangle (face.reduced (0.5f), rad, 1.0f);
        if (st == keysClasica)
        {
            g.setColour (Colours::black.withAlpha (0.06f));
            g.fillRect (face.withTrimmedTop (face.getHeight() - 12.0f * k));
        }
        // Marca de la escala
        if (inScale)
        {
            const float d = jmin (ww * 0.3f, 8.0f * k);
            const auto dc = Point<float> (face.getCentreX(), face.getBottom() - d * 2.4f - (pc == 0 ? 18.0f * k : 0.0f));
            g.setColour (l > 0.3f ? Colours::white.withAlpha (0.85f) : pal.accent.withAlpha (0.85f));
            if (isRoot) g.fillRoundedRectangle (dc.x - d * 0.5f, dc.y - d * 0.5f, d, d, d * 0.2f);
            else        g.fillEllipse (dc.x - d * 0.3f, dc.y - d * 0.3f, d * 0.6f, d * 0.6f);
        }
        if (pc == 0 && ww > 14.0f)
        {
            const bool darkKey = pal.keyW.getPerceivedBrightness() < 0.5f;
            g.setColour (l > 0.3f ? pal.accentInk : (darkKey ? Colours::white.withAlpha (0.75f) : pal.dim));
            g.setFont (draw::fitUi (ctx.fonts, "Do8", 13.5f * k, ww - 4.0f, 2));
            g.drawText ((v.latin ? String ("Do") : String ("C")) + String (n / 12 - 1), face.withTrimmedTop (face.getHeight() - 22.0f * k), Justification::centred, false);
        }
        return;
    }

    // Negras
    const float rad = st == keysPastilla ? r.getWidth() * 0.5f : (st == keysBotones ? 8.0f * k : (st == keysPlana ? 7.0f * k : 3.0f * k));
    if (st == keysBotones) r = r.reduced (jmax (2.0f, 3.0f * k), 0.0f);
    auto face = r.translated (0.0f, pressDepth * 0.7f);
    g.setColour (Colours::black.withAlpha (pal.night ? 0.5f : 0.25f));
    g.fillRoundedRectangle (r.translated (0.0f, 3.0f * k), rad);
    g.setColour (pal.keyB);
    g.fillRoundedRectangle (face, rad);
    if (l > 0.003f)
    {
        g.setColour (litCol.withAlpha (jmin (1.0f, l * susOnly)));
        g.fillRoundedRectangle (face, rad);
    }
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (face.reduced (0.5f), rad, 1.0f);
    if (st == keysClasica)
    {
        g.setColour (Colours::white.withAlpha (0.12f));
        g.fillRoundedRectangle (face.reduced (3.0f * k, 0.0f).withTrimmedTop (face.getHeight() - 10.0f * k).withHeight (6.0f * k), 2.0f);
    }
    if (inScale)
    {
        const float d = jmin (face.getWidth() * 0.34f, 7.0f * k);
        const auto dc = Point<float> (face.getCentreX(), st == keysPastilla ? face.getCentreY() : face.getBottom() - d * 1.8f);
        g.setColour (l > 0.3f ? Colours::white.withAlpha (0.85f) : pal.accent.brighter (0.3f));
        if (isRoot) g.fillRoundedRectangle (dc.x - d * 0.5f, dc.y - d * 0.5f, d, d, d * 0.2f);
        else        g.fillEllipse (dc.x - d * 0.3f, dc.y - d * 0.3f, d * 0.6f, d * 0.6f);
    }
}

void KeyboardSection::paintContent (Graphics& g, Rectangle<float> area)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;
    const bool tm = transparentMode();
    auto bed = area.reduced (2.0f * k).withTrimmedBottom (4.0f * k);
    if (! tm) draw::recess (g, bed, pal, 20.0f * k, pal.body2.interpolatedWith (pal.keyBed, 0.5f));

    if (showCheeks())
    {
        g.setColour (pal.text);
        g.setFont (ctx.fonts.ui (14.5f * k, 2));
        g.drawText ("Piano", Rectangle<float> (leftCheek.getX(), leftCheek.getY() + 60.0f * k, 80.0f * k, 20.0f * k), Justification::centredLeft, false);
        g.drawText ("Escala", Rectangle<float> (rightCheek.getX(), rightCheek.getY() + 64.0f * k, 90.0f * k, 22.0f * k), Justification::centredLeft, false);
        g.setColour (pal.dim);
        g.setFont (ctx.fonts.ui (14.5f * k, 1));
        g.drawText (volume.valueText, Rectangle<float> (leftCheek.getRight() - 86.0f * k, leftCheek.getY() + 127.0f * k, 100.0f * k, 18.0f * k), Justification::centred, false);

        // Rango que se ve
        draw::recess (g, rangeDisplay, pal, 8.0f * k, pal.night ? pal.plate.darker (0.2f) : pal.plate);
        const auto nm = [&] (int n) { return (v.latin ? String ("Do") : String ("C")) + String (n / 12 - 1); };
        const String rangeTxt = nm (startC) + U8 ("–") + nm (lastNote());
        g.setColour (pal.text);
        g.setFont (draw::fitUi (ctx.fonts, rangeTxt, 14.5f * k, rangeDisplay.getWidth() - 4.0f, 2));
        g.drawText (rangeTxt, rangeDisplay, Justification::centred, false);
    }

    // Teclas (se deslizan cuando cambias de octava)
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (keysArea.expanded (2.0f, 8.0f * k).toNearestInt());
        const int lo = jmax (12, (int) std::floor (v.viewWhite / 7.0f) * 12 - 12);
        const int hi = jmin (120, lo + 12 * 7);
        for (int n = lo; n <= hi; ++n)
        {
            const auto kg = keyRect (n);
            if (kg.black || kg.r.getRight() < keysArea.getX() - 2.0f || kg.r.getX() > keysArea.getRight() + 2.0f) continue;
            drawKey (g, n, kg);
        }
        for (int n = lo; n <= hi; ++n)
        {
            const auto kg = keyRect (n);
            if (! kg.black || kg.r.getCentreX() < keysArea.getX() + 2.0f || kg.r.getCentreX() > keysArea.getRight() - 2.0f) continue;
            drawKey (g, n, kg);
        }
    }

    // Avisos de notas que suenan fuera de lo que se ve
    bool below = false, above = false;
    for (auto n : v.active) { if (n < startC) below = true; if (n > lastNote()) above = true; }
    auto arrow = [&] (bool left)
    {
        const float s = 12.0f * k;
        const float x = left ? keysArea.getX() - s * 0.9f : keysArea.getRight() + s * 0.9f;
        const float y = keysArea.getCentreY();
        Path p;
        if (left) p.addTriangle (x - s * 0.5f, y, x + s * 0.4f, y - s * 0.6f, x + s * 0.4f, y + s * 0.6f);
        else      p.addTriangle (x + s * 0.5f, y, x - s * 0.4f, y - s * 0.6f, x - s * 0.4f, y + s * 0.6f);
        g.setColour (pal.accent);
        g.fillPath (p);
    };
    if (below) arrow (true);
    if (above) arrow (false);

    // Puntitos de color en los botones de color
    keysColourBtn.swatch = pal.rainbow ? Colour::fromHSV ((float) std::fmod (v.time * 0.2, 1.0), 0.6f, 1.0f, 1.0f) : pal.lit;
    uiColourBtn.swatch = pal.accent;
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
