#include "Sections.h"
#include "Pieces.h"

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
      detachBtn (c, "detach", "Separar en su propia ventana"),
      tuneBtn (c, "tune", U8 ("Ajustar solo esta pieza: diseño, forma, letras y colores"))
{
    detachBtn.framed = false;
    detachBtn.onClick = [this] { if (onDetach) onDetach(); };
    addAndMakeVisible (detachBtn);
    tuneBtn.framed = false;
    tuneBtn.onClick = [this] { if (onTune) onTune(); };
    addAndMakeVisible (tuneBtn);
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
    const bool controls = ! floating && ! ctx.vis.hideParams;
    detachBtn.setVisible (wantsDetachButton() && controls);
    tuneBtn.lightOnDark = detachBtn.lightOnDark;
    tuneBtn.setBounds (detachBtn.isVisible() ? detachBtn.getX() - s : getWidth() - s - pad, pad, s, s);
    tuneBtn.setVisible (wantsTuneButton() && controls);
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
    const auto st = pieces::staffLook (pal);
    Colour ink = st.ink;
    Colour dimInk = st.dim;
    if (tm)
    {
        g.setColour (Colours::black.withAlpha (0.32f));
        g.fillRoundedRectangle (paper, 18.0f * k);
        ink = Colours::white;
        dimInk = Colours::white.withAlpha (0.85f);
    }
    else if (! st.none)
    {
        static const int frameKind[4] = { 0, 1, 5, 4 };
        const int fk = frameKind[jlimit (0, 3, pal.pieces.stfShape)];
        const auto frame = draw::shapePath (paper, fk, 18.0f * k);
        if (st.card)
        {
            Skin p2 = pal;
            p2.card = st.paper;
            if (fk == 0) draw::card (g, paper, p2, 18.0f * k);
            else         draw::cardShape (g, frame, p2);
        }
        else if (fk == 0)
            draw::recess (g, paper, pal, 18.0f * k, st.paper);
        else
            draw::recessShape (g, frame, pal, st.paper);
        if (st.texture != pieces::texNone)
            pieces::texture (g, draw::shapePath (paper.reduced (1.0f), fk, 18.0f * k), st.texture, st.paper.getPerceivedBrightness() < 0.5f, 1.0f);
    }
    caption (g, paper, "Pentagrama", tm ? Colours::white : (st.none ? pal.text : ink));
    const float staffGlow = tm ? 0.0f : st.glow;

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
    g.setColour (st.none && ! tm ? pal.dim : dimInk);
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

    if (staffGlow > 0.0f)
    {
        // Neón: las líneas brillan
        for (float w : { lineW * 7.0f, lineW * 3.5f })
        {
            g.setColour (ink.withAlpha (0.10f * staffGlow));
            for (int d : { 30, 32, 34, 36, 38 }) g.fillRoundedRectangle (left, yT (d) - w * 0.5f, right - left, w, w * 0.5f);
            for (int d : { 18, 20, 22, 24, 26 }) g.fillRoundedRectangle (left, yB (d) - w * 0.5f, right - left, w, w * 0.5f);
        }
    }
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
        // Sin sombras: la nota limpia. Con arcoíris, del mismo color que su tecla
        const auto headCol = pal.rainbow && ! tm ? pal.noteColour (n.midi) : ink;
        g.setColour (headCol.withAlpha (alpha));
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
    const int distinct = theory::countBits (v.activeMask);

    String sub, leftTop, leftBig, leftSub;
    leftTop = "Grado en " + theory::keyTitle (v.keyRoot, v.scale, true);

    if (count == 0)
    {
        if (pal.pieces.scrIdle == 1)
        {
            StringArray sn;
            for (auto s : theory::scales()[(size_t) v.scale].steps)
                sn.add (theory::pcName (v.keyRoot + s, v.sig, v.latin));
            sub = "Escala: " + sn.joinIntoString ("  ");
            leftSub = U8 ("Toca una nota o un acorde");
        }
        else leftTop = {};   // vacía hasta que toques algo
        leftBig = "";
    }
    else if (distinct == 1)
    {
        const int m = v.active.front();
        const double hz = 440.0 * std::pow (2.0, (m - 69) / 12.0);
        if (count == 1)
            sub = theory::noteName (m, v.sig, true, false) + (m == 60 ? U8 (" central") : String()) + ", " + String (hz, 1) + " Hz";
        else
            sub = theory::pcName (m % 12, v.sig, true) + U8 (" en ") + String (count) + U8 (" octavas");
        const auto deg = theory::noteDegree (theory::pcOf (m), v.keyRoot, v.scale);
        leftBig = deg.upToFirstOccurrenceOf (U8 (" ·"), false, false);
        leftSub = deg.fromFirstOccurrenceOf (U8 ("· "), false, false);
    }
    else if (v.chord.valid && (distinct >= 3 || v.chord.suffix == "5"))
    {
        sub = theory::pcName (v.chord.root, v.sig, true) + " " + v.chord.quality;
        if (v.chord.inversion != "estado fundamental") sub << ", " << v.chord.inversion;
        leftBig = v.degree.roman;
        leftSub = v.degree.function.replace (U8 (" · "), U8 (": "));
    }
    else if (distinct == 2)
    {
        const int iv = theory::firstInterval (v.active);
        sub = theory::noteName (v.active[0], v.sig, v.latin) + " a " + theory::noteName (v.active[0] + iv, v.sig, v.latin)
              + ", " + String (iv) + (iv == 1 ? " semitono" : " semitonos");
        const int ivc = iv % 12;
        const bool consonant = ivc == 0 || ivc == 3 || ivc == 4 || ivc == 5 || ivc == 7 || ivc == 8 || ivc == 9;
        leftBig = "Intervalo";
        leftSub = consonant ? U8 ("Consonante: estable") : U8 ("Disonante: con tensión");
    }
    else
    {
        sub = String (distinct) + U8 (" notas sin nombre de acorde común");
        leftBig = "";
        leftSub = U8 ("Prueba quitando una nota");
    }

    const auto sl = pieces::screenLook (pal);
    const int fontId = sl.font;
    const int scrShape = tm ? 0 : pal.pieces.scrShape;
    auto bigFont = [&] (float h) { return fontId == 0 ? (pal.pixelScreen ? ctx.fonts.pixel (h) : ctx.fonts.ui (h, 2)) : ctx.fonts.display (fontId, pal.pixelScreen, h); };
    Rectangle<float> scr;
    if (tm)
    {
        scr = area.reduced (4.0f * k);
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (scr, 16.0f * k);
    }
    else
    {
        Skin p2 = pal;
        p2.screen = sl.bg;
        draw::screen (g, area, p2, k, scrShape);
        scr = area.reduced (8.0f * k);
        if (sl.texture != pieces::texNone)
            pieces::texture (g, draw::shapePath (scr, scrShape, 14.0f * k), sl.texture, ! sl.light, 1.0f);
    }
    const auto textCol = tm ? Colours::white : sl.text;
    const auto dimCol  = tm ? Colours::white.withAlpha (0.8f) : sl.dim;
    // Con forma de pastilla u óvalo, lo de adentro se mete para no salirse de la orilla
    auto in = scrShape == 3 ? scr.reduced (scr.getWidth() * 0.13f, scr.getHeight() * 0.12f)
            : scrShape == 2 ? scr.reduced (jmin (scr.getHeight() * 0.32f, scr.getWidth() * 0.2f), 14.0f * k)
                            : scr.reduced (18.0f * k, 14.0f * k);
    const float glowAmt = tm ? 0.0f : sl.glow;

    // Esquina: el grado
    {
        auto l = in;
        g.setFont (ctx.fonts.ui (15.5f * k, 1));
        draw::shadowText (g, leftTop, l.removeFromTop (20.0f * k), Justification::centredLeft, dimCol, tm);
        const auto lf = fontId == 0 ? (pal.pixelScreen ? ctx.fonts.pixel (32.0f * k) : ctx.fonts.ui (30.0f * k, 2)) : bigFont (28.0f * k);
        const auto lt = Fonts::displayText (fontId, leftBig);
        auto lr = l.removeFromTop (38.0f * k);
        if (glowAmt > 0.0f) glowSmall.draw (g, lt, lf, lr, Justification::centredLeft, textCol, glowAmt);
        g.setFont (lf);
        draw::shadowText (g, lt, lr, Justification::centredLeft, textCol, tm);
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
            g.fillRect (in.getRight() - (detachBtn.isVisible() ? 30.0f * k : 0.0f) - (tuneBtn.isVisible() ? 30.0f * k : 0.0f) - (float) (4 - i) * 16.0f * k, in.getY() + 4.0f * k, 11.0f * k, 11.0f * k);
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
            const auto col = pal.rainbow || ! pal.pixelScreen ? pal.noteColour (m) : pal.noteTag (i);
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
        Font f = bigFont (maxH);
        float h = maxH;
        while (h > 12.0f && draw::textWidth (f, text) > box.getWidth() - sidePad)
        {
            h *= 0.92f;
            f = bigFont (h);
        }
        return f;
    };
    const auto bigNow = Fonts::displayText (fontId, v.bigText), bigPrev = Fonts::displayText (fontId, v.prevBigText);
    bool dropBelow = false;
    {
        const auto f0 = fitBig (bigNow, centre, bigMax, 150.0f * k);
        const auto box = centre.translated (60.0f * k, 0.0f).withTrimmedRight (60.0f * k);
        const float tw = draw::textWidth (f0, bigNow);
        const float textLeft = box.getCentreX() - tw * 0.5f;
        const float textTop = box.getCentreY() - f0.getAscent() * 0.75f;
        dropBelow = textLeft < leftRight && textTop < cornerBottom;
    }
    auto bigBox = dropBelow ? in.withTop (cornerBottom) : centre.translated (60.0f * k, 0.0f).withTrimmedRight (60.0f * k);
    const float bigH = dropBelow ? jmin (bigBox.getHeight() * 1.05f, bigMax) : bigMax;
    const float f = v.bigFade;
    const float ease = 1.0f - (1.0f - f) * (1.0f - f);
    const int motion = pal.pieces.anim ? pal.pieces.scrMotion : 4;
    auto drawBig = [&] (const String& text, float alpha, float yOff, float scale, int chars)
    {
        if (text.isEmpty() || alpha <= 0.01f) return;
        const auto fnt = fitBig (text, dropBelow ? bigBox : centre, bigH, dropBelow ? 20.0f * k : 150.0f * k);
        auto r = bigBox.translated (0.0f, yOff);
        String shown = text;
        if (chars >= 0)
        {
            // Escribir: aparecen las letras una a una (el resto ocupa su lugar sin verse)
            shown = text.substring (0, chars);
            const float full = draw::textWidth (fnt, text), part = draw::textWidth (fnt, shown);
            r = r.withX (r.getCentreX() - full * 0.5f).withWidth (part + 2.0f);
        }
        if (glowAmt > 0.0f)
        {
            // Las luces laten un poco al cambiar de acorde
            const float pulse = 1.0f + 0.7f * (1.0f - ease) * (chars >= 0 ? 0.0f : 1.0f);
            glowBig.draw (g, shown, fnt, r, chars >= 0 ? Justification::centredLeft : Justification::centred,
                          textCol, glowAmt * alpha * pulse, scale);
        }
        g.setFont (fnt.withHeight (fnt.getHeight() * scale));
        const auto just = chars >= 0 ? Justification::centredLeft : Justification::centred;
        if (! tm && ! sl.light)
        {
            g.setColour (textCol.withAlpha (0.12f * alpha));
            g.drawText (shown, r.translated (0.0f, 2.0f), just, false);
        }
        draw::shadowText (g, shown, r, just, textCol.withAlpha (alpha), tm);
    };
    switch (motion)
    {
        case 1:   // fundido
            drawBig (bigPrev, 1.0f - ease, 0.0f, 1.0f, -1);
            drawBig (bigNow, ease, 0.0f, 1.0f, -1);
            break;
        case 2:   // zoom
            drawBig (bigPrev, 1.0f - ease, 0.0f, 1.0f - 0.2f * ease, -1);
            drawBig (bigNow, jmin (1.0f, ease * 1.4f), 0.0f, 1.0f + 0.22f * (1.0f - ease), -1);
            break;
        case 3:   // escribir
            drawBig (bigNow, 1.0f, 0.0f, 1.0f, f >= 1.0f ? -1 : jmin (bigNow.length(), (int) std::ceil (f * (float) bigNow.length())));
            break;
        case 4:   // quieto
            drawBig (bigNow, 1.0f, 0.0f, 1.0f, -1);
            break;
        default:  // deslizar
            drawBig (bigPrev, 1.0f - ease, -10.0f * k * ease, 1.0f - 0.06f * ease, -1);
            drawBig (bigNow, ease, 10.0f * k * (1.0f - ease), 0.94f + 0.06f * ease, -1);
            break;
    }

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

void DegreesSection::layoutContent (Rectangle<int> b)
{
    listArea = b.toFloat().reduced (1.0f, 6.0f * k);
    // A la derecha: el botoncito de ajustes y la octava de los acordes (▲ número ▼)
    if (tuneBtn.isVisible())
    {
        auto t = listArea.removeFromRight (26.0f * k);
        tuneBtn.setBounds (t.withSizeKeepingCentre (26.0f * k, 26.0f * k).toNearestInt());
        listArea.removeFromRight (4.0f * k);
    }
    octArea = listArea.removeFromRight (40.0f * k);
    listArea.removeFromRight (8.0f * k);
}

void DegreesSection::paintContent (Graphics& g, Rectangle<float> area)
{
    auto& v = ctx.vis;
    const auto& pal = v.pal;
    const bool tm = transparentMode();
    if (listArea.isEmpty()) layoutContent (area.toNearestInt());
    {
        auto o = octArea;
        if (tm)
        {
            g.setColour (Colours::black.withAlpha (0.35f));
            g.fillRoundedRectangle (o, 12.0f * k);
        }
        else draw::recess (g, o, pal, 12.0f * k, pal.night ? pal.plate.darker (0.2f) : pal.plate);
        const auto col = tm ? Colours::white : pal.text;
        auto up = o.removeFromTop (o.getHeight() * 0.32f);
        auto down = o.removeFromBottom (o.getHeight() * 0.47f);
        {
            Graphics::ScopedSaveState ss (g);
            auto ur = up.withSizeKeepingCentre (16.0f * k, 16.0f * k).translated (0.0f, 3.0f * k);
            g.addTransform (AffineTransform::rotation (MathConstants<float>::pi, ur.getCentreX(), ur.getCentreY()));
            draw::icon (g, "chev", ur, col.withAlpha (octPressed == 1 ? 1.0f : 0.65f), 2.0f);
        }
        g.setColour (col);
        g.setFont (ctx.fonts.ui (17.0f * k, 2));
        g.drawText (String (octave), o.withTrimmedTop (-4.0f * k), Justification::centred, false);
        g.setColour (col.withAlpha (0.55f));
        g.setFont (ctx.fonts.ui (11.0f * k, 1));
        g.drawText ("oct", down.removeFromTop (down.getHeight() * 0.4f).translated (0.0f, -6.0f * k), Justification::centred, false);
        draw::icon (g, "chev", down.withSizeKeepingCentre (16.0f * k, 16.0f * k).translated (0.0f, -2.0f * k), col.withAlpha (octPressed == -1 ? 1.0f : 0.65f), 2.0f);
    }

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
            pieces::degreeTile (g, ctx, r, i, d.roman, d.symbol, glow, press[(size_t) i], i == hover ? 1.0f : 0.0f, k, &glowCache);
            continue;
        }
        auto inner = r.withTrimmedBottom (2.5f).translated (0.0f, 2.5f * press[(size_t) i]);
        const auto textCol = tm ? Colours::white : pal.btnText.interpolatedWith (pal.accentInk, glow);
        const auto dimCol = tm ? Colours::white.withAlpha (0.8f) : pal.btnText.withAlpha (0.55f).interpolatedWith (pal.accentInk, glow);
        if (pal.pieces.degLabels == 0)
        {
            g.setColour (textCol);
            g.setFont (ctx.fonts.ui (22.0f * k, 2));
            g.drawText (d.roman, inner, Justification::centred, false);
            continue;
        }
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
    if (e.mods.isPopupMenu()) { if (onSettings) onSettings(); return; }
    if (octArea.contains (e.position))
    {
        // Mitad de arriba sube una octava, mitad de abajo baja
        octPressed = e.position.y < octArea.getCentreY() ? 1 : -1;
        if (onOctave) onOctave (octPressed);
        repaint();
        return;
    }
    for (int i = 0; i < 7; ++i)
        if (tile (i).contains (e.position))
        {
            pressed = i;
            animate();
            const auto t = tile (i);
            const float vel = jlimit (0.2f, 1.0f, 0.3f + 0.7f * (e.position.y - t.getY()) / jmax (1.0f, t.getHeight()));
            if (onPreview) onPreview (i, vel);
            return;
        }
}

void DegreesSection::mouseUp (const MouseEvent&)
{
    if (octPressed != 0) { octPressed = 0; repaint(); }
    if (pressed >= 0 && onRelease) onRelease();
    pressed = -1;
    animate();
}

void DegreesSection::mouseMove (const MouseEvent& e)
{
    int h = -1;
    for (int i = 0; i < 7; ++i) if (tile (i).contains (e.position)) h = i;
    if (octArea.contains (e.position)) { setMouseCursor (MouseCursor::PointingHandCursor); if (hover != -1) { hover = -1; repaint(); } return; }
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
    const int cskin = tm ? 0 : pal.pieces.cirSkin;
    const auto acc = pieces::circleColour (pal);
    const auto accInk = acc.getPerceivedBrightness() > 0.72f ? Colour (0xff1a1a1a) : Colours::white;
    const auto regionCol = pal.pieces.cirColour.isTransparent() ? pal.region : acc.withAlpha (pal.night ? 0.16f : 0.12f);
    auto plate = area.reduced (2.0f * k).withTrimmedBottom (4.0f * k);
    if (tm)
    {
        g.setColour (Colours::black.withAlpha (0.28f));
        g.fillRoundedRectangle (plate, 18.0f * k);
    }
    else
    {
        const int ps = pal.pieces.cirShape;
        const auto plateFill = cskin == 4 ? Colour (0xff0b0818) : pal.plate;
        if (ps == 0)      draw::recess (g, plate, pal, 18.0f * k, plateFill);
        else if (ps != 3) draw::recessShape (g, draw::shapePath (plate, ps == 1 ? 1 : 5, 18.0f * k), pal, plateFill);
    }
    caption (g, plate, title, tm || cskin == 4 ? Colours::white : pal.text);

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
    const bool darkDisc = cskin == 3 || cskin == 4;
    const bool pastelDark = cskin == 2 && pal.night;
    const auto textCol = tm || darkDisc || pastelDark ? Colours::white : (cskin == 2 ? Colour (0xff1d1d1d) : pal.text);
    const auto dimCol  = tm || darkDisc || pastelDark ? Colours::white.withAlpha (0.75f) : (cskin == 2 ? Colour (0xff1d1d1d).withAlpha (0.7f) : pal.dim);

    Path disc;
    disc.addEllipse (c.x - rOut, c.y - rOut, rOut * 2.0f, rOut * 2.0f);
    if (cskin == 3)
    {
        // Vinilo: disco negro con surcos
        g.setColour (Colours::black.withAlpha (0.3f));
        g.fillEllipse (c.x - rOut - 2.0f, c.y - rOut + 1.0f, rOut * 2.0f + 4.0f, rOut * 2.0f + 4.0f);
        g.setColour (Colour (0xff121214));
        g.fillPath (disc);
    }

    for (int i = 0; i < 12; ++i)
    {
        const int dist = std::min ((i - region + 12) % 12, (region - i + 12) % 12);
        for (int ring = 0; ring < 2; ++ring)
        {
            const bool minorRing = ring == 1;
            auto p = minorRing ? ringSeg (i, rIn, rMid) : ringSeg (i, rMid, rOut);
            const bool isTonic = (i == tonicIndex && minorRing == tonicMinor);
            Colour fill;
            if (isTonic) fill = acc;
            else if (cskin == 1) fill = dist <= 1 ? regionCol.withMultipliedAlpha (0.7f) : Colours::transparentBlack;
            else if (cskin == 2)
            {
                const float hue = (float) i / 12.0f;
                fill = Colour::fromHSV (hue, minorRing ? 0.30f : 0.42f, pal.night ? (minorRing ? 0.42f : 0.5f) : (minorRing ? 0.97f : 0.93f), 1.0f);
                if (dist > 1) fill = fill.withMultipliedSaturation (0.45f).withMultipliedAlpha (0.85f);
            }
            else if (cskin == 3) fill = dist <= 1 ? acc.withAlpha (0.20f) : Colours::transparentBlack;
            else if (cskin == 4) fill = dist <= 1 ? acc.withAlpha (0.12f) : Colours::transparentBlack;
            else fill = dist <= 1 ? regionCol : (tm ? Colours::black.withAlpha (0.2f) : pal.ring);
            g.setColour (fill);
            g.fillPath (p);
            if (cskin == 4 && isTonic)
            {
                g.setColour (acc.withAlpha (0.25f));
                g.strokePath (p, PathStrokeType (7.0f * k));
            }
            if (i == hoverIndex && minorRing == hoverMinor)
            {
                g.setColour (textCol.withAlpha (0.12f));
                g.fillPath (p);
            }
            if (cskin == 4)
            {
                g.setColour (acc.withAlpha (dist <= 1 ? 0.55f : 0.28f));
                g.strokePath (p, PathStrokeType (1.1f));
            }
            else if (cskin == 3)
            {
                g.setColour (Colours::white.withAlpha (0.07f));
                g.strokePath (p, PathStrokeType (1.0f));
            }
            else if (cskin == 1)
            {
                g.setColour (pal.text.withAlpha (dist <= 1 ? 0.35f : 0.18f));
                g.strokePath (p, PathStrokeType (1.0f));
            }
            else
            {
                g.setColour (tm ? Colours::white.withAlpha (0.3f) : (cskin == 2 ? Colours::black.withAlpha (0.12f) : pal.plateLine));
                g.strokePath (p, PathStrokeType (1.0f));
            }

            const float ang = (float) i * seg - MathConstants<float>::halfPi;
            const float lr = minorRing ? (rIn + rMid) * 0.5f : (rMid + rOut) * 0.5f;
            const auto lp = c + Point<float> (std::cos (ang) * lr, std::sin (ang) * lr);
            const int pc = minorRing ? theory::pcOf (majorPcAt (i) + 9) : majorPcAt (i);
            const auto sigHere = theory::keySignature (pc, minorRing ? 1 : 0);
            auto name = theory::pcName (pc, sigHere, v.latin) + (minorRing ? "m" : "");
            const float fh = minorRing ? jmax (10.5f, radius * 0.098f) : jmax (12.0f, radius * 0.115f);
            g.setFont (ctx.fonts.ui (fh, minorRing ? 1 : 2));
            draw::shadowText (g, name, Rectangle<float> (lp.x - 40.0f, lp.y - fh * 0.62f, 80.0f, fh * 1.24f), Justification::centred,
                              isTonic ? accInk : (minorRing ? dimCol : textCol), tm && ! isTonic);
        }
    }

    // Textura del círculo (surcos, metal, grano o puntos)
    if (! tm)
    {
        const int tex = pieces::circleTexture (pal);
        if (tex != pieces::texNone)
            pieces::texture (g, disc, tex, darkDisc || pal.night, cskin == 3 ? 1.0f : 0.8f, c, rOut);
    }

    // --- Constelación: líneas entre las notas que suenan, se dibujan solas y una luz las recorre
    const double t = v.time;
    const auto& pcs = pal.pieces;
    const int route = tm ? 0 : pcs.cirRoute;
    const float lw = tm ? 1.0f : pcs.cirLineW;
    const float knobR = rIn * (tm ? 1.0f : pcs.cirKnob);
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
    if (route == 0) buildLoop (idx, rn, knobR, loop);
    else            buildStraight (idx, rn, loop);
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
    const auto lineCol = tm ? Colours::white : acc;
    const bool flow = pcs.cirFlow && pcs.anim;
    animating = false;

    // Qué tan visible es un punto de la línea (las que "se difuminan" se apagan al acercarse al centro)
    auto fadeAt = [&] (Point<float> p)
    {
        if (route != 2) return 1.0f;
        const float d = p.getDistanceFrom (c) / jmax (1.0f, rn);
        const float f = jlimit (0.0f, 1.0f, (d - 0.12f) / 0.55f);
        return f * f * (3.0f - 2.0f * f);
    };

    auto drawLines = [&]
    {
        if (path->size() < 2 || lineAlpha <= 0.01f) return;
        std::vector<float> cum (path->size(), 0.0f);
        for (size_t i = 1; i < path->size(); ++i) cum[i] = cum[i - 1] + (*path)[i].getDistanceFrom ((*path)[i - 1]);
        const float total = cum.back();
        const float limit = loop.empty() ? total : total * drawEase;
        const float breathe = flow ? 0.5f + 0.5f * (float) std::sin (t * 2.4) : 0.5f;
        if (route == 2)
        {
            // Se difuminan: tramo por tramo, cada uno con su transparencia
            for (size_t i = 1; i < path->size() && cum[i - 1] < limit; ++i)
            {
                auto a = (*path)[i - 1], b = (*path)[i];
                if (cum[i] > limit) b = a + (b - a) * ((limit - cum[i - 1]) / jmax (0.001f, cum[i] - cum[i - 1]));
                const float fa = fadeAt ((a + b) * 0.5f);
                if (fa < 0.01f) continue;
                g.setColour (lineCol.withAlpha ((0.10f + 0.08f * breathe) * lineAlpha * fa));
                g.drawLine ({ a, b }, 8.0f * k * lw);
                g.setColour (lineCol.withAlpha (0.95f * lineAlpha * fa));
                g.drawLine ({ a, b }, 2.2f * k * lw);
            }
        }
        else
        {
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
            g.setColour (lineCol.withAlpha ((0.10f + 0.08f * breathe) * lineAlpha));
            g.strokePath (line, PathStrokeType (8.0f * k * lw, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (lineCol.withAlpha (0.95f * lineAlpha));
            g.strokePath (line, PathStrokeType (2.2f * k * lw, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // Luz que recorre las líneas (cometa)
        if (flow && ! loop.empty() && drawOn >= 1.0f && total > 1.0f)
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
                const float a = (1.0f - (float) j / 10.0f) * fadeAt (p);
                const float rr = (3.4f - 0.2f * (float) j) * k * jmax (0.8f, lw);
                g.setColour (Colours::white.withAlpha (0.85f * a));
                g.fillEllipse (p.x - rr * 0.5f, p.y - rr * 0.5f, rr, rr);
            }
            // Destellos que viajan en sentido contrario, más suaves
            const auto q = pointAt (total - head * 0.6f);
            g.setColour (lineCol.brighter (0.6f).withAlpha (0.6f * fadeAt (q)));
            g.fillEllipse (q.x - 2.0f * k, q.y - 2.0f * k, 4.0f * k, 4.0f * k);
        }
        animating = true;
    };

    // Por debajo o rodeando: antes del botón del centro. Encima o difuminadas: después.
    if (route == 0 || route == 1) drawLines();

    // --- El botón del centro: apunta a la tonalidad
    {
        Colour cap = tm ? Colours::black.withAlpha (0.6f) : (cskin == 3 ? acc.darker (0.25f) : (cskin == 4 ? Colour (0xff160f30) : pal.caps[0]));
        if (! tm) draw::knobBody (g, c, knobR, pal, cap);
        else { g.setColour (cap); g.fillEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f); }
        if (cskin == 4 && ! tm)
        {
            g.setColour (acc.withAlpha (0.45f));
            g.drawEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f, 1.5f);
        }
        const float a = v.knobAngle;
        const auto i0 = c + Point<float> (std::sin (a), -std::cos (a)) * (knobR * 0.80f);
        const auto i1 = c + Point<float> (std::sin (a), -std::cos (a)) * (knobR * 0.96f);
        g.setColour (cskin == 3 ? Colours::white : acc);
        g.drawLine ({ i0, i1 }, jmax (2.0f, knobR * 0.08f));

        const auto root = theory::pcName (v.keyRoot, v.sig, v.latin);
        const auto scName = String (CharPointer_UTF8 (sc.shortName));
        const auto capInk = cap.getPerceivedBrightness() > 0.7f ? Colour (0xff1a1a1a) : Colours::white;
        g.setColour (capInk);
        g.setFont (ctx.fonts.ui (knobR * 0.5f, 2));
        g.drawText (root, Rectangle<float> (c.x - knobR, c.y - knobR * 0.62f, knobR * 2.0f, knobR * 0.66f), Justification::centred, false);
        if (knobR > 26.0f)   // con el centro chiquito solo cabe la tónica
        {
            const String sigTxt = v.sig.count == 0 ? U8 ("sin alteraciones")
                                                   : String (v.sig.count) + (v.sig.flats ? (v.sig.count == 1 ? " bemol" : " bemoles")
                                                                                         : (v.sig.count == 1 ? " sostenido" : " sostenidos"));
            g.setColour (capInk.withAlpha (0.85f));
            g.setFont (draw::fitUi (ctx.fonts, scName, knobR * 0.23f, knobR * 1.5f, 1));
            g.drawText (scName, Rectangle<float> (c.x - knobR, c.y + knobR * 0.06f, knobR * 2.0f, knobR * 0.26f), Justification::centred, false);
            g.setFont (draw::fitUi (ctx.fonts, sigTxt, knobR * 0.2f, knobR * 1.4f, 0));
            g.drawText (sigTxt, Rectangle<float> (c.x - knobR, c.y + knobR * 0.32f, knobR * 2.0f, knobR * 0.24f), Justification::centred, false);
        }
        else
        {
            g.setColour (capInk.withAlpha (0.85f));
            g.setFont (draw::fitUi (ctx.fonts, scName, knobR * 0.3f, knobR * 1.6f, 1));
            g.drawText (scName, Rectangle<float> (c.x - knobR, c.y + knobR * 0.08f, knobR * 2.0f, knobR * 0.36f), Justification::centred, false);
        }
    }

    if (route == 2 || route == 3) drawLines();

    // --- Nodos: puntitos de color que laten
    const int nodes = tm ? 0 : pcs.cirNodes;
    int ni = 0;
    for (int pc = 0; pc < 12; ++pc)
    {
        const float gl = v.pcGlow[(size_t) pc];
        if (gl < 0.02f) continue;
        const int i = circleIndexOfPc (pc);
        const float a = (float) i * seg - MathConstants<float>::halfPi;
        const auto pos = c + Point<float> (std::cos (a), std::sin (a)) * rn;
        const auto col = pal.noteColour (pc);   // del mismo color que las teclas (y arcoíris si está prendido)
        ++ni;
        animating = true;
        if (nodes == 2) continue;
        const float pulse = pcs.anim ? 1.0f + 0.14f * (float) std::sin (t * 4.0 + (double) i) : 1.0f;
        const float rr = 6.0f * k * (0.6f + 0.4f * gl) * pulse * jmax (0.8f, std::sqrt (lw));
        if (nodes == 1)
        {
            // Anillos: solo el contorno, sin brillo
            g.setColour (col.withMultipliedAlpha (gl));
            g.drawEllipse (pos.x - rr * 1.2f, pos.y - rr * 1.2f, rr * 2.4f, rr * 2.4f, 2.2f * k);
            g.setColour (pal.plate.withMultipliedAlpha (gl));
            g.fillEllipse (pos.x - rr * 0.6f, pos.y - rr * 0.6f, rr * 1.2f, rr * 1.2f);
            continue;
        }
        ColourGradient glow (col.withAlpha (0.45f * gl), pos.x, pos.y, col.withAlpha (0.0f), pos.x + rr * 3.2f, pos.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (pos.x - rr * 3.2f, pos.y - rr * 3.2f, rr * 6.4f, rr * 6.4f);
        g.setColour (col.withMultipliedAlpha (gl));
        g.fillEllipse (pos.x - rr, pos.y - rr, rr * 2.0f, rr * 2.0f);
        g.setColour (Colours::white.withAlpha (0.9f * gl));
        g.drawEllipse (pos.x - rr, pos.y - rr, rr * 2.0f, rr * 2.0f, 1.5f * k);
    }
    (void) ni;

    // Abajo a un lado: nombres de las notas en inglés o español (para todo MidiCat)
    langArea = {};
    if (! tm && ! v.hideParams)
    {
        langArea = Rectangle<float> (plate.getRight() - 14.0f * k - 70.0f * k, plate.getBottom() - 12.0f * k - 24.0f * k, 70.0f * k, 24.0f * k);
        const float rr = langArea.getHeight() * 0.5f;
        const bool dark = darkDisc || (cskin == 4) || pal.night;
        g.setColour (dark ? Colours::white.withAlpha (0.08f) : Colours::black.withAlpha (0.06f));
        g.fillRoundedRectangle (langArea, rr);
        g.setColour ((dark ? Colours::white : pal.text).withAlpha (hoverLang ? 0.35f : 0.18f));
        g.drawRoundedRectangle (langArea.reduced (0.5f), rr, 1.0f);
        auto half = langArea.reduced (2.0f * k);
        auto en = half.removeFromLeft (half.getWidth() * 0.5f);
        auto es = half;
        auto& on = v.latin ? es : en;
        g.setColour (acc);
        g.fillRoundedRectangle (on, on.getHeight() * 0.5f);
        g.setFont (ctx.fonts.ui (12.0f * k, 2));
        const auto off = dark && cskin != 0 ? Colours::white.withAlpha (0.7f) : (pal.night ? Colours::white.withAlpha (0.7f) : pal.text.withAlpha (0.6f));
        g.setColour (v.latin ? off : accInk);
        g.drawText ("EN", en, Justification::centred, false);
        g.setColour (v.latin ? accInk : off);
        g.drawText ("ES", es, Justification::centred, false);
    }
}

String CircleSection::getTooltip()
{
    return hoverLang ? U8 ("Nombres de las notas: EN = C D E (inglés), ES = Do Re Mi (español)") : String();
}

// Líneas rectas entre las notas (cruzan el centro); con puntos intermedios para que la luz las recorra
void CircleSection::buildStraight (const std::vector<int>& idx, float rn, std::vector<Point<float>>& out) const
{
    out.clear();
    if (idx.size() < 2) return;
    const float seg = MathConstants<float>::twoPi / 12.0f;
    auto pt = [&] (int i) { const float a = (float) i * seg - MathConstants<float>::halfPi; return centre + Point<float> (std::cos (a), std::sin (a)) * rn; };
    const size_t n = idx.size();
    const size_t segs = n == 2 ? 1 : n;
    for (size_t s = 0; s < segs; ++s)
    {
        const auto p0 = pt (idx[s]), p1 = pt (idx[(s + 1) % n]);
        const int steps = 24;
        for (int i = (s == 0 ? 0 : 1); i <= steps; ++i)
            out.push_back (p0 + (p1 - p0) * ((float) i / (float) steps));
    }
}

void CircleSection::mouseDown (const MouseEvent& e)
{
    if (langArea.contains (e.position))
    {
        if (onLatin) onLatin (! ctx.vis.latin);
        return;
    }
    int i; bool minor;
    if (ringAt (e.position, i, minor) && onKeyClicked)
        onKeyClicked (minor ? theory::pcOf (majorPcAt (i) + 9) : majorPcAt (i), minor);
}

void CircleSection::mouseMove (const MouseEvent& e)
{
    const bool lang = langArea.contains (e.position);
    int i = -1; bool minor = false;
    if (lang || ! ringAt (e.position, i, minor)) i = -1;
    if (i != hoverIndex || minor != hoverMinor || lang != hoverLang)
    {
        hoverIndex = i; hoverMinor = minor; hoverLang = lang;
        setMouseCursor (i >= 0 || lang ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
        repaint();
    }
}

void CircleSection::mouseExit (const MouseEvent&)
{
    if (hoverIndex != -1 || hoverLang) { hoverIndex = -1; hoverLang = false; repaint(); }
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
      uiColourBtn (c, "palette", U8 ("Personalizar: estilo, temas, colores y movimiento (cada pieza tiene su propio botoncito)"), "Interfaz"),
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
        // Teclas, colores, formas y movimiento: todo en un solo botón
        keysColourBtn.setVisible (false);
        uiColourBtn.setBounds (Rectangle<float> (L.getX(), L.getBottom() - 40.0f * k, L.getWidth(), 40.0f * k).toNearestInt());

        auto R = rightCheek;
        const bool det = detachBtn.isVisible();
        const bool tune = tuneBtn.isVisible();
        const float icons = (det ? 34.0f * k : 0.0f) + (tune ? 34.0f * k : 0.0f);
        pedalBtn.setBounds (Rectangle<float> (R.getX(), R.getY(), R.getWidth() - (icons > 0.0f ? icons + 6.0f * k : 0.0f), 48.0f * k).toNearestInt());
        if (det) detachBtn.setBounds (Rectangle<float> (R.getRight() - 32.0f * k, R.getY() + 9.0f * k, 32.0f * k, 30.0f * k).toNearestInt());
        if (tune) tuneBtn.setBounds (Rectangle<float> (R.getRight() - (det ? 66.0f : 32.0f) * k, R.getY() + 9.0f * k, 32.0f * k, 30.0f * k).toNearestInt());
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
        tuneBtn.setVisible (false);
    }
}

KeyboardSection::KeyGeom KeyboardSection::geomFor (int note, Rectangle<float> ka, float viewWhite, float viewWhites, int st)
{
    const float ww = ka.getWidth() / jmax (1.0f, viewWhites);
    const float x0 = ka.getX();
    if (! isBlackPc (note % 12))
    {
        const float x = x0 + ((float) whiteIndexOf (note) - viewWhite) * ww;
        if (st == keysPastilla) return { Rectangle<float> (x, ka.getY() + ka.getHeight() * 0.33f, ww, ka.getHeight() * 0.67f), false };
        if (st == keysBotones)  return { Rectangle<float> (x, ka.getY() + ka.getHeight() * 0.52f, ww, ka.getHeight() * 0.48f), false };
        return { Rectangle<float> (x, ka.getY(), ww, ka.getHeight()), false };
    }
    const float cx = x0 + ((float) whiteIndexOf (note - 1) + 1.0f - viewWhite) * ww;
    if (st == keysPastilla)
    {
        const float d = jmin (ww * 0.74f, ka.getHeight() * 0.28f);
        return { Rectangle<float> (cx - d * 0.5f, ka.getY() + ka.getHeight() * 0.15f - d * 0.5f, d, d), true };
    }
    if (st == keysBotones) return { Rectangle<float> (cx - ww * 0.5f, ka.getY(), ww, ka.getHeight() * 0.48f), true };
    const float bw = ww * (st == keysPlana ? 0.6f : 0.62f);
    return { Rectangle<float> (cx - bw * 0.5f, ka.getY(), bw, ka.getHeight() * (st == keysPlana ? 0.58f : 0.6f)), true };
}

KeyboardSection::KeyGeom KeyboardSection::keyRect (int note) const
{
    return geomFor (note, keysArea, ctx.vis.viewWhite, ctx.vis.viewWhites, ctx.vis.pal.keyStyle);
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
    KeyPaint kp;
    kp.level = v.level[(size_t) n];
    kp.susOnly = (v.down[(size_t) n] == 0 && v.sus[(size_t) n] > 0) ? 0.55f : 1.0f;
    kp.inScale = v.highlight && (v.scaleMask & (1u << (unsigned) (n % 12))) != 0;
    kp.isRoot = n % 12 == theory::pcOf (v.keyRoot);
    kp.latin = v.latin;
    paintKey (g, ctx.fonts, v.pal, n, kg, k, kp);
}

void KeyboardSection::paintKey (Graphics& g, const Fonts& fonts, const Skin& pal, int n, const KeyGeom& kg, float k, const KeyPaint& kp)
{
    const int st = pal.keyStyle;
    const float l = kp.level;
    const float susOnly = kp.susOnly;
    const float pressDepth = jmin (1.0f, l * 1.4f) * 2.0f * k;
    const auto litCol = pal.noteColour (n);
    const int pc = n % 12;
    const bool inScale = kp.inScale;
    const bool isRoot = kp.isRoot;
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
            g.setFont (draw::fitUi (fonts, "Do8", 13.5f * k, ww - 4.0f, 2));
            g.drawText ((kp.latin ? String ("Do") : String ("C")) + String (n / 12 - 1), face.withTrimmedTop (face.getHeight() - 22.0f * k), Justification::centred, false);
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

void KeyboardSection::paintPreview (Graphics& g, UiContext& ctx, const Skin& pal, Rectangle<float> area, float k)
{
    // Una octava (Do a Do) con Do mayor sonando y las marcas de la escala, igual que el teclado
    draw::recess (g, area, pal, 14.0f * k, pal.body2.interpolatedWith (pal.keyBed, 0.5f));
    const auto ka = area.reduced (14.0f * k, 12.0f * k);
    const int first = 60, last = 72;
    const float whites = 8.0f;
    const float viewWhite = (float) whiteIndexOf (first);
    for (int pass = 0; pass < 2; ++pass)
        for (int n = first; n <= last; ++n)
        {
            const auto kg = geomFor (n, ka, viewWhite, whites, pal.keyStyle);
            if (kg.black != (pass == 1)) continue;
            KeyPaint kp;
            kp.level = (n == 60 || n == 64 || n == 67) ? 1.0f : 0.0f;
            kp.inScale = ! isBlackPc (n % 12);
            kp.isRoot = n % 12 == 0;
            kp.latin = ctx.vis.latin;
            paintKey (g, ctx.fonts, pal, n, kg, k, kp);
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
