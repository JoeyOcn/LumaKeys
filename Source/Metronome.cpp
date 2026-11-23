#include "Metronome.h"
#include "Style.h"

using namespace juce;

StringArray metronomeSoundNames()
{
    return { U8 ("Clásico"), "Clic", "Madera", "Beep", "Aro", "Cencerro" };
}

void Metronome::prepare (double sampleRate)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;
    playing = nullptr;
    playPos = 0;
    gain.reset (sr, 0.03);
    buildSounds();
}

void Metronome::buildSounds()
{
    sounds.clear();
    Random rng (1234);
    const float twoPi = MathConstants<float>::twoPi;

    auto make = [&] (float seconds, float peak, std::function<float (float, float)> fn)
    {
        std::vector<float> v ((size_t) jmax (16, (int) (seconds * (float) sr)));
        float hp = 0.0f, prevNoise = 0.0f;
        for (size_t i = 0; i < v.size(); ++i)
        {
            const float t = (float) i / (float) sr;
            const float white = rng.nextFloat() * 2.0f - 1.0f;
            hp = 0.6f * (hp + white - prevNoise);   // ruido sin graves (más fino)
            prevNoise = white;
            const float attack = jmin (1.0f, t / 0.0007f);
            v[i] = fn (t, hp) * attack;
        }
        float mx = 0.0f;
        for (auto x : v) mx = jmax (mx, std::abs (x));
        if (mx > 0.0f) for (auto& x : v) x *= peak / mx;
        // final suave para que no truene
        const int fade = jmin ((int) v.size() / 4, (int) (0.004 * sr));
        for (int i = 0; i < fade; ++i) v[v.size() - 1 - (size_t) i] *= (float) i / (float) fade;
        return v;
    };

    for (int s = 0; s < 6; ++s)
    {
        std::array<std::vector<float>, 2> pair;
        for (int a = 0; a < 2; ++a)
        {
            const bool acc = a == 1;
            const float peak = acc ? 0.5f : 0.34f;
            switch (s)
            {
                case 0: // Clásico: tic agudo y corto
                {
                    const float f = acc ? 1568.0f : 1047.0f;
                    pair[(size_t) a] = make (0.09f, peak, [=] (float t, float n)
                        { return std::sin (twoPi * f * t) * std::exp (-t / 0.018f) + 0.18f * n * std::exp (-t / 0.0018f); });
                    break;
                }
                case 1: // Clic: chasquido seco
                    pair[(size_t) a] = make (0.03f, peak, [=] (float t, float n)
                        { return n * std::exp (-t / (acc ? 0.0045f : 0.003f)); });
                    break;
                case 2: // Madera
                {
                    const float f1 = acc ? 1900.0f : 1500.0f, f2 = f1 * 1.73f;
                    pair[(size_t) a] = make (0.08f, peak, [=] (float t, float)
                        { return 0.62f * std::sin (twoPi * f1 * t) * std::exp (-t / 0.013f)
                               + 0.38f * std::sin (twoPi * f2 * t) * std::exp (-t / 0.007f); });
                    break;
                }
                case 3: // Beep suave
                {
                    const float f = acc ? 1760.0f : 880.0f;
                    pair[(size_t) a] = make (0.07f, peak * 0.8f, [=] (float t, float)
                        {
                            const float env = t < 0.004f ? t / 0.004f : (t < 0.04f ? 1.0f : jmax (0.0f, 1.0f - (t - 0.04f) / 0.026f));
                            return std::sin (twoPi * f * t) * env;
                        });
                    break;
                }
                case 4: // Aro (rim)
                {
                    const float f = acc ? 620.0f : 470.0f;
                    pair[(size_t) a] = make (0.07f, peak, [=] (float t, float n)
                        { return 0.55f * n * std::exp (-t / 0.005f) + 0.6f * std::sin (twoPi * f * t) * std::exp (-t / 0.02f); });
                    break;
                }
                default: // Cencerro
                {
                    const float d = acc ? 0.09f : 0.06f;
                    pair[(size_t) a] = make (0.22f, peak * 0.85f, [=] (float t, float)
                        {
                            float x = 0.0f;
                            for (float f : { 540.0f, 800.0f })
                                x += std::sin (twoPi * f * t) + std::sin (twoPi * f * 3.0f * t) / 3.0f + std::sin (twoPi * f * 5.0f * t) / 5.0f;
                            return x * std::exp (-t / d);
                        });
                    break;
                }
            }
        }
        sounds.push_back (std::move (pair));
    }
}

void Metronome::startClick (int sound, bool accent, int offset) noexcept
{
    if (sounds.empty()) return;
    playing = &sounds[(size_t) jlimit (0, (int) sounds.size() - 1, sound)][accent ? 1 : 0];
    playPos = 0;
    playStart = offset;
}

void Metronome::process (AudioBuffer<float>& out, int n, const MetronomeSettings& st, AudioPlayHead* playHead)
{
    double bpm = (double) st.bpm.load();
    int beats = st.beats.load();
    int denom = 4;
    bool hostPlaying = false, hostTempo = false;
    double hostPhase = 0.0;

    if (playHead != nullptr && st.followHost.load())
    {
        if (auto pos = playHead->getPosition())
        {
            if (auto b = pos->getBpm()) { bpm = *b; hostTempo = true; }
            if (auto ts = pos->getTimeSignature()) { beats = ts->numerator; denom = ts->denominator; }
            if (pos->getIsPlaying())
                if (auto ppq = pos->getPpqPosition())
                {
                    const double beatQ = 4.0 / (double) jmax (1, denom);
                    const double barQ = beatQ * (double) jmax (1, beats);
                    const double barStart = pos->getPpqPositionOfLastBarStart().orFallback (std::floor (*ppq / barQ) * barQ);
                    hostPhase = (*ppq - barStart) / beatQ;
                    hostPlaying = true;
                }
        }
    }
    bpm = jlimit (20.0, 400.0, bpm);
    beats = jlimit (1, 16, beats);
    bpmNow = bpm;
    barBeats = beats;
    spb = sr * 60.0 / bpm * (4.0 / (double) jmax (1, denom));
    uiTempo.store ((float) bpm);
    uiHostTempo.store (hostTempo);

    const bool on = st.on.load();
    if ((on && ! wasOn) || restartRequest.exchange (false))
        phase = 0.0 - 1.0e-9;   // el primer clic suena ya, y es el fuerte
    wasOn = on;

    double p0 = hostPlaying ? hostPhase : phase;

    // Dónde caen los tiempos dentro de este bloque
    int clickAt[16];
    bool clickAcc[16];
    int numClicks = 0;
    if (on)
    {
        double b = std::ceil (p0 - 1.0e-7);
        double offset = (b - p0) * spb;
        while (offset < (double) n && numClicks < 16)
        {
            const int beatInBar = ((int) std::llround (b) % beats + beats) % beats;
            clickAt[numClicks] = jmax (0, (int) offset);
            clickAcc[numClicks] = beatInBar == 0;
            ++numClicks;
            uiBeat.store (beatInBar);
            uiBeatCount.fetch_add (1);
            b += 1.0;
            offset += spb;
        }
    }
    phase = p0 + (double) n / spb;

    const int pv = previewRequest.exchange (-1);
    if (pv >= 0) startClick (pv, true, 0);

    gain.setTargetValue (Decibels::decibelsToGain (st.volumeDb.load(), -60.0f));
    const int chans = out.getNumChannels();
    int next = 0;
    for (int i = 0; i < n; ++i)
    {
        while (next < numClicks && clickAt[next] == i)
        {
            startClick (st.sound.load(), clickAcc[next], i);
            ++next;
        }
        const float g = gain.getNextValue();
        if (playing == nullptr) continue;
        const float v = (*playing)[(size_t) playPos] * g;
        for (int c = 0; c < chans; ++c)
            out.getWritePointer (c)[i] += v;
        if (++playPos >= (int) playing->size()) playing = nullptr;
    }
}
