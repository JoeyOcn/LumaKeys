#include "Recorder.h"

using namespace juce;

MidiRecorder::MidiRecorder()
{
    ring.resize ((size_t) fifo.getTotalSize());
    take.reserve (4096);
    startTimerHz (20);
}

MidiRecorder::~MidiRecorder()
{
    stopTimer();
}

File MidiRecorder::folder()
{
    return File::getSpecialLocation (File::userMusicDirectory).getChildFile ("Midi Cat").getChildFile ("Grabaciones");
}

Array<File> MidiRecorder::recordings()
{
    auto files = folder().findChildFiles (File::findFiles, false, "*.mid;*.midi");
    std::sort (files.begin(), files.end(), [] (const File& a, const File& b)
               { return a.getLastModificationTime() > b.getLastModificationTime(); });
    return files;
}

void MidiRecorder::start()
{
    if (st.load() != 0) return;
    take.clear();
    stopReq.store (false);
    startReq.store (true);
}

void MidiRecorder::stop()
{
    if (startReq.exchange (false)) return;   // todavía no empezaba
    if (st.load() != 0) stopReq.store (true);
}

double MidiRecorder::elapsedSeconds() const noexcept
{
    if (st.load() != 2) return 0.0;
    return jmax (0.0, (double) (clock.load() - origin.load()) / rate.load());
}

int MidiRecorder::beatsLeftInCountIn() const noexcept
{
    if (st.load() != 1) return 0;
    const double left = (double) (origin.load() - clock.load()) / jmax (1.0, spbAtStart.load());
    return jmax (1, (int) std::ceil (left - 0.02));
}

void MidiRecorder::push (const MidiMessage& m, int64_t time) noexcept
{
    const auto* raw = m.getRawData();
    const int size = m.getRawDataSize();
    if (size < 1 || size > 3) return;
    const uint8_t status = raw[0] & 0xf0;
    if (status < 0x80 || status == 0xf0) return;     // solo mensajes de canal (notas, pedal, ruedas…)

    int s1, n1, s2, n2;
    fifo.prepareToWrite (1, s1, n1, s2, n2);
    Ev* e = n1 > 0 ? &ring[(size_t) s1] : (n2 > 0 ? &ring[(size_t) s2] : nullptr);
    if (e == nullptr) return;
    e->size = (uint8_t) size;
    for (int i = 0; i < 3; ++i) e->d[i] = i < size ? raw[i] : 0;
    e->time = time;
    fifo.finishedWrite (n1 + n2);
}

void MidiRecorder::process (const MidiBuffer& played, const MidiBuffer& mouse, int n,
                            const Metronome& metro, bool metroOn, double sampleRate) noexcept
{
    const int64_t now = clock.load();
    rate.store (sampleRate);

    if (startReq.exchange (false) && st.load() == 0)
    {
        const double spb = metro.samplesPerBeat();
        const int beats = metro.beatsPerBar();
        bpmAtStart.store (metro.tempo());
        beatsAtStart.store (beats);
        spbAtStart.store (spb);
        stopAt.store (-1);
        if (metroOn)
        {
            grid.store (true);
            if (countIn.load())
            {
                // Un compás de cuenta y luego empieza (el metrónomo vuelve al tiempo 1)
                metro.restartBar();
                origin.store (now + (int64_t) std::llround (spb * beats));
                st.store (1);
            }
            else
            {
                // Sin cuenta: el archivo empieza en el tiempo 1 del compás en el que vas
                double inBar = std::fmod (metro.beatPhase(), (double) beats);
                if (inBar < 0.0) inBar += beats;
                origin.store (now - (int64_t) std::llround (inBar * spb));
                st.store (2);
            }
        }
        else
        {
            grid.store (false);
            origin.store (now);
            st.store (2);
        }
    }

    if (stopReq.exchange (false) && st.load() != 0)
    {
        stopAt.store (now - origin.load());
        st.store (0);
    }

    const int s = st.load();
    if (s != 0)
    {
        const int64_t base = now - origin.load();
        for (const auto meta : played) push (meta.getMessage(), base + meta.samplePosition);
        if (recordMouse.load())
            for (const auto meta : mouse) push (meta.getMessage(), base + meta.samplePosition);
        if (s == 1 && now + n >= origin.load()) st.store (2);
    }

    clock.store (now + n);
}

void MidiRecorder::timerCallback()
{
    const int ready = fifo.getNumReady();
    if (ready > 0)
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (ready, s1, n1, s2, n2);
        for (int i = 0; i < n1; ++i) take.push_back (ring[(size_t) (s1 + i)]);
        for (int i = 0; i < n2; ++i) take.push_back (ring[(size_t) (s2 + i)]);
        fifo.finishedRead (n1 + n2);
    }
    if (st.load() == 0 && stopAt.load() >= 0)
    {
        finish();
        stopAt.store (-1);
        take.clear();
    }
}

void MidiRecorder::finish()
{
    const double sr = rate.load();
    const double bpm = bpmAtStart.load();
    const double spb = spbAtStart.load();
    const bool onGrid = grid.load();
    int64_t end = stopAt.load();

    // Qué se guarda
    std::vector<Ev> evs;
    for (auto& e : take)
    {
        if (onGrid && e.time < -(int64_t) (spb * 0.5)) continue;   // muy antes del tiempo 1
        evs.push_back (e);
    }
    bool anyNote = false;
    for (auto& e : evs) if ((e.d[0] & 0xf0) == 0x90 && e.d[2] > 0) anyNote = true;
    if (! anyNote) { if (onEmpty) onEmpty(); return; }

    int64_t shift = 0;
    if (! onGrid)
    {
        // Sin metrónomo: el archivo empieza justo en la primera nota
        int64_t first = std::numeric_limits<int64_t>::max();
        for (auto& e : evs) if ((e.d[0] & 0xf0) == 0x90 && e.d[2] > 0) first = jmin (first, e.time);
        shift = first;
    }

    const double ticksPerSample = (bpm / 60.0) * 960.0 / sr;
    auto toTicks = [&] (int64_t t) { return jmax (0.0, (double) (t - shift) * ticksPerSample); };

    MidiMessageSequence seq;
    auto tempoMsg = MidiMessage::tempoMetaEvent ((int) std::llround (60000000.0 / bpm));
    seq.addEvent (tempoMsg, 0.0);
    seq.addEvent (MidiMessage::timeSignatureMetaEvent (beatsAtStart.load(), 4), 0.0);
    seq.addEvent (MidiMessage::textMetaEvent (3, "Midi Cat"), 0.0);

    std::array<int, 128> open {};
    bool pedalDown = false;
    double lastTick = 0.0;
    for (auto& e : evs)
    {
        MidiMessage m (e.d, (int) e.size, 0.0);
        const double tk = toTicks (e.time);
        lastTick = jmax (lastTick, tk);
        if (m.isNoteOn()) ++open[(size_t) m.getNoteNumber()];
        else if (m.isNoteOff() && open[(size_t) m.getNoteNumber()] > 0) --open[(size_t) m.getNoteNumber()];
        else if (m.isController() && m.getControllerNumber() == 64) pedalDown = m.getControllerValue() >= 64;
        seq.addEvent (m, tk);
    }
    // Lo que seguía sonando al parar se cierra donde paraste
    const double endTick = jmax (lastTick, toTicks (end));
    for (int nn = 0; nn < 128; ++nn)
        for (int c = 0; c < open[(size_t) nn]; ++c)
            seq.addEvent (MidiMessage::noteOff (1, nn), endTick);
    if (pedalDown) seq.addEvent (MidiMessage::controllerEvent (1, 64, 0), endTick);
    seq.sort();
    seq.updateMatchedPairs();

    MidiFile mf;
    mf.setTicksPerQuarterNote (960);
    mf.addTrack (seq);

    auto dir = folder();
    dir.createDirectory();
    auto name = "Midi Cat " + Time::getCurrentTime().formatted ("%Y-%m-%d %H.%M.%S");
    auto file = dir.getNonexistentChildFile (name, ".mid", false);
    FileOutputStream os (file);
    if (os.openedOk())
    {
        mf.writeTo (os, 1);
        os.flush();
    }
    if (os.getStatus().wasOk() && onSaved)
        onSaved (file);
}

//==============================================================================
bool MidiClip::load (const File& f)
{
    file = f;
    notes.clear();
    lengthBeats = seconds = 0.0;
    bpm = 120.0;
    FileInputStream in (f);
    if (! in.openedOk() || ! midi.readFrom (in)) return false;

    const double tpq = (double) jmax (1, (int) midi.getTimeFormat());
    for (int t = 0; t < midi.getNumTracks(); ++t)
    {
        auto* tr = midi.getTrack (t);
        for (int i = 0; i < tr->getNumEvents(); ++i)
        {
            const auto& m = tr->getEventPointer (i)->message;
            if (m.isTempoMetaEvent()) { bpm = 60.0 / jmax (0.0001, m.getTempoSecondsPerQuarterNote()); break; }
        }
    }
    if (midi.getTimeFormat() <= 0) return false;

    // Las notas de todas las pistas (se guarda en qué pista y evento está cada una)
    for (int t = 0; t < midi.getNumTracks(); ++t)
    {
        auto seq = *midi.getTrack (t);
        seq.updateMatchedPairs();
        for (int i = 0; i < seq.getNumEvents(); ++i)
        {
            auto* e = seq.getEventPointer (i);
            if (! e->message.isNoteOn()) continue;
            Note n;
            n.note = e->message.getNoteNumber();
            n.velocity = e->message.getVelocity();
            n.start = e->message.getTimeStamp() / tpq;
            n.end = (e->noteOffObject != nullptr ? e->noteOffObject->message.getTimeStamp() : e->message.getTimeStamp() + tpq * 0.25) / tpq;
            n.onIndex = t * 1000000 + i;
            notes.push_back (n);
            lengthBeats = jmax (lengthBeats, n.end);
        }
    }
    std::sort (notes.begin(), notes.end(), [] (const Note& a, const Note& b) { return a.start < b.start || (a.start == b.start && a.note < b.note); });
    seconds = lengthBeats * 60.0 / bpm;
    return true;
}

bool MidiClip::saveVelocities()
{
    // Se reescribe el archivo con las fuerzas nuevas
    MidiFile out;
    out.setTicksPerQuarterNote (midi.getTimeFormat());
    for (int t = 0; t < midi.getNumTracks(); ++t)
    {
        MidiMessageSequence seq (*midi.getTrack (t));
        for (auto& n : notes)
            if (n.onIndex / 1000000 == t)
            {
                const int idx = n.onIndex % 1000000;
                if (idx < seq.getNumEvents())
                {
                    auto& m = seq.getEventPointer (idx)->message;
                    if (m.isNoteOn()) m.setVelocity ((float) jlimit (1, 127, n.velocity) / 127.0f);
                }
            }
        out.addTrack (seq);
    }
    TemporaryFile tmp (file);
    {
        FileOutputStream os (tmp.getFile());
        if (! os.openedOk()) return false;
        out.writeTo (os, 1);
        os.flush();
        if (! os.getStatus().wasOk()) return false;
    }
    if (! tmp.overwriteTargetFileWithTemporary()) return false;
    midi = out;
    return true;
}
