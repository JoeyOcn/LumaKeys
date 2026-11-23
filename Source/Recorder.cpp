#include "Recorder.h"
#include "Theory.h"

using namespace juce;

MidiRecorder::MidiRecorder()
{
    ring.resize ((size_t) fifo.getTotalSize());
    take.reserve (4096);
    writerThread.startThread();
    startTimerHz (20);
}

MidiRecorder::~MidiRecorder()
{
    stopTimer();
    closeWriter (false);
    writerThread.stopThread (2000);
}

//==============================================================================
bool MidiRecorder::openWriter (const File& target, double sr)
{
    closeWriter (false);
    target.getParentDirectory().createDirectory();
    auto temp = target.getParentDirectory().getNonexistentChildFile ("~" + target.getFileNameWithoutExtension(), ".tmp", false);
    auto fos = std::make_unique<FileOutputStream> (temp);
    if (! fos->openedOk()) return false;
    std::unique_ptr<OutputStream> os (fos.release());
    WavAudioFormat wav;
    auto w = wav.createWriterFor (os, AudioFormatWriterOptions().withSampleRate (sr > 0.0 ? sr : 44100.0)
                                                                .withNumChannels (2)
                                                                .withBitsPerSample (24));
    if (w == nullptr) { temp.deleteFile(); return false; }
    auto tw = std::make_unique<AudioFormatWriter::ThreadedWriter> (w.release(), writerThread, 1 << 19);   // ~11 s de colchón por si el disco se atora
    {
        const SpinLock::ScopedLockType sl (writerLock);
        writer = std::move (tw);
    }
    writerTemp = temp;
    writerTarget = target;
    audioDone.store (false);
    return true;
}

void MidiRecorder::closeWriter (bool keep)
{
    std::unique_ptr<AudioFormatWriter::ThreadedWriter> old;
    {
        const SpinLock::ScopedLockType sl (writerLock);
        old = std::move (writer);
    }
    audioMode.store (0);
    if (old == nullptr) return;
    old.reset();   // termina de escribir y cierra el archivo
    if (keep && writerTarget != File())
    {
        writerTarget.deleteFile();
        if (writerTemp.moveFileTo (writerTarget) && onAudioSaved) onAudioSaved (writerTarget);
    }
    else writerTemp.deleteFile();
    writerTemp = writerTarget = File();
}

void MidiRecorder::writeAudio (const AudioBuffer<float>& buffer, int n) noexcept
{
    const int mode = audioMode.load();
    if (mode == 0 || audioDone.load()) return;
    const SpinLock::ScopedTryLockType sl (writerLock);
    if (! sl.isLocked() || writer == nullptr) return;

    const int chans = buffer.getNumChannels();
    if (chans <= 0) return;
    auto writeRange = [&] (int from, int to)
    {
        if (to <= from) return;
        const float* ptrs[2] = { buffer.getReadPointer (0, from), buffer.getReadPointer (jmin (1, chans - 1), from) };
        writer->write (ptrs, to - from);
    };

    if (mode == 2)
    {
        if (! bounceRunning.load()) return;
        writeRange (0, n);
        if (bouncePos.load() >= bounceLen.load()) { bounceRunning.store (false); audioDone.store (true); }
        return;
    }

    // Grabando: desde el tiempo 1 (con metrónomo) o desde la primera nota, hasta un poquito después de parar
    const int64_t blockEnd = clock.load();
    const int64_t blockStart = blockEnd - n;
    const int64_t from = audioFrom.load();
    if (from < 0) return;
    const int64_t until = audioUntil.load();
    const int64_t s0 = jmax (blockStart, from);
    const int64_t s1 = until >= 0 ? jmin (blockEnd, until) : blockEnd;
    writeRange ((int) (s0 - blockStart), (int) (s1 - blockStart));
    if (until >= 0 && blockEnd >= until) audioDone.store (true);
}

//==============================================================================
bool MidiRecorder::startBounce (const File& midiFile)
{
    if (isBouncing() || st.load() != 0) return false;
    MidiFile mf;
    FileInputStream in (midiFile);
    if (! in.openedOk() || ! mf.readFrom (in) || mf.getTimeFormat() <= 0) return false;
    mf.convertTimestampTicksToSeconds();

    const double sr = rate.load();
    std::vector<BEv> evs;
    double last = 0.0;
    for (int t = 0; t < mf.getNumTracks(); ++t)
    {
        auto* tr = mf.getTrack (t);
        for (int i = 0; i < tr->getNumEvents(); ++i)
        {
            const auto& m = tr->getEventPointer (i)->message;
            if (m.isMetaEvent() || m.isSysEx() || m.getRawDataSize() > 3) continue;
            BEv e;
            e.sample = (int64_t) std::llround (m.getTimeStamp() * sr);
            e.size = (uint8_t) m.getRawDataSize();
            for (int k = 0; k < (int) e.size; ++k) e.d[k] = m.getRawData()[k];
            evs.push_back (e);
            last = jmax (last, m.getTimeStamp());
        }
    }
    if (evs.empty()) return false;
    std::stable_sort (evs.begin(), evs.end(), [] (const BEv& a, const BEv& b) { return a.sample < b.sample; });

    if (! openWriter (audioFileFor (midiFile), sr)) return false;
    bounceEvents = std::move (evs);
    bounceNext = 0;
    bouncePos.store (0);
    bounceLen.store ((int64_t) ((last + 2.0) * sr));   // dos segundos extra para que se apague solo
    bounceTarget = audioFileFor (midiFile);
    audioMode.store (2);
    bounceStop.store (false);
    bounceArm.store (true);
    return true;
}

void MidiRecorder::cancelBounce()
{
    if (! isBouncing()) return;
    bounceStop.store (true);
}

float MidiRecorder::bounceProgress() const noexcept
{
    return jlimit (0.0f, 1.0f, (float) ((double) bouncePos.load() / (double) jmax ((int64_t) 1, bounceLen.load())));
}

void MidiRecorder::injectBounce (MidiBuffer& midi, int n) noexcept
{
    if (bounceArm.exchange (false)) bounceRunning.store (true);
    if (! bounceRunning.load()) return;
    if (bounceStop.load())
    {
        for (int ch = 1; ch <= 16; ++ch) midi.addEvent (MidiMessage::allNotesOff (ch), 0);
        bounceRunning.store (false);
        audioDone.store (true);
        return;
    }
    const int64_t pos = bouncePos.load();
    while (bounceNext < bounceEvents.size() && bounceEvents[bounceNext].sample < pos + n)
    {
        const auto& e = bounceEvents[bounceNext++];
        midi.addEvent (e.d, (int) e.size, (int) jlimit ((int64_t) 0, (int64_t) n - 1, e.sample - pos));
    }
    bouncePos.store (pos + n);
}

File MidiRecorder::folder()
{
    const auto music = File::getSpecialLocation (File::userMusicDirectory);
    const auto dir = music.getChildFile ("MidiCat");
    // Antes se llamaba "Midi Cat": se pasa la carpeta vieja una sola vez
    static bool migrated = false;
    if (! migrated)
    {
        migrated = true;
        const auto old = music.getChildFile ("Midi Cat");
        if (old.isDirectory() && ! dir.exists())
            old.moveFileTo (dir);
    }
    return dir.getChildFile ("Grabaciones");
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
    if (st.load() != 0 || isBouncing()) return;
    take.clear();
    stopReq.store (false);
    audioFrom.store (-1);
    audioUntil.store (-1);
    if (recordAudio.load() && audioMode.load() == 0)
        if (openWriter (folder().getChildFile ("toma.wav"), rate.load()))
            audioMode.store (1);
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
            audioFrom.store (jmax (now, origin.load()));
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
        audioUntil.store (now + (int64_t) (sampleRate * 1.5));   // un poquito más para que no se corte el sonido
    }

    const int s = st.load();
    if (s != 0)
    {
        const int64_t base = now - origin.load();
        const bool takeMouse = recordMouse.load();
        if (audioFrom.load() < 0)
        {
            // Sin metrónomo, el audio empieza junto con la primera nota
            auto firstNote = [] (const MidiBuffer& b) { for (const auto meta : b) if (meta.getMessage().isNoteOn()) return meta.samplePosition; return -1; };
            int fp = firstNote (played);
            if (takeMouse) { const int fm = firstNote (mouse); if (fm >= 0 && (fp < 0 || fm < fp)) fp = fm; }
            if (fp >= 0) audioFrom.store (now + fp);
        }
        for (const auto meta : played) push (meta.getMessage(), base + meta.samplePosition);
        if (takeMouse)
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
        if (audioMode.load() == 1 && audioFrom.load() < 0) closeWriter (false);   // no se tocó nada
    }
    if (audioDone.load())
    {
        audioDone.store (false);
        const int mode = audioMode.load();
        if (mode == 1)
        {
            if (! takeEmpty && takeBase.isNotEmpty()) writerTarget = folder().getChildFile (takeBase + ".wav");
            closeWriter (! takeEmpty && takeBase.isNotEmpty());
        }
        else if (mode == 2)
        {
            const bool ok = ! bounceStop.load();
            bounceEvents.clear();
            closeWriter (ok);
        }
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
    takeEmpty = ! anyNote;
    takeBase = {};
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
    if (keyRoot.load() >= 0) addKeyInfo (seq, keyRoot.load(), keyScale.load());
    else seq.addEvent (MidiMessage::textMetaEvent (3, "MidiCat"), 0.0);

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
    auto name = "MidiCat " + Time::getCurrentTime().formatted ("%Y-%m-%d %H.%M.%S");
    auto file = dir.getNonexistentChildFile (name, ".mid", false);
    takeBase = file.getFileNameWithoutExtension();
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
// La tonalidad dentro del MIDI
//==============================================================================
String MidiRecorder::keyText (int root, int scale) { return "MidiCat key=" + String (root) + "," + String (scale); }

void MidiRecorder::addKeyInfo (MidiMessageSequence& seq, int root, int scale)
{
    root = theory::pcOf (root);
    scale = jlimit (0, theory::numScales() - 1, scale);
    const auto sig = theory::keySignature (root, scale);
    const bool minor = theory::scales()[(size_t) scale].minorFlavour;
    // Nombre de la pista = la tonalidad (así se ve en FL, Logic y Ableton)
    seq.addEvent (MidiMessage::textMetaEvent (3, theory::keyTitle (root, scale, true)), 0.0);
    seq.addEvent (MidiMessage::keySignatureMetaEvent (sig.flats ? -sig.count : sig.count, minor), 0.0);
    seq.addEvent (MidiMessage::textMetaEvent (1, keyText (root, scale)), 0.0);
}

MidiRecorder::KeyInfo MidiRecorder::readKeyInfo (const File& f)
{
    KeyInfo info;
    MidiFile mf;
    FileInputStream in (f);
    if (! in.openedOk() || ! mf.readFrom (in) || mf.getTimeFormat() <= 0) return info;
    const double tpq = (double) mf.getTimeFormat();
    bool tempoSet = false, haveSig = false, haveText = false;
    std::array<float, 12> hist {};
    for (int t = 0; t < mf.getNumTracks(); ++t)
    {
        MidiMessageSequence seq (*mf.getTrack (t));
        seq.updateMatchedPairs();
        for (int i = 0; i < seq.getNumEvents(); ++i)
        {
            auto* e = seq.getEventPointer (i);
            const auto& m = e->message;
            if (m.isTempoMetaEvent() && ! tempoSet) { info.bpm = 60.0 / jmax (0.0001, m.getTempoSecondsPerQuarterNote()); tempoSet = true; }
            else if (m.isTextMetaEvent() && m.getMetaEventType() == 1)
            {
                const auto txt = m.getTextFromTextMetaEvent();
                if (txt.startsWith ("MidiCat key="))
                {
                    const auto parts = StringArray::fromTokens (txt.fromFirstOccurrenceOf ("=", false, false), ",", "");
                    if (parts.size() == 2)
                    {
                        info.root = theory::pcOf (parts[0].getIntValue());
                        info.scale = jlimit (0, theory::numScales() - 1, parts[1].getIntValue());
                        haveText = true;
                    }
                }
            }
            else if (m.isKeySignatureMetaEvent() && ! haveText && ! haveSig)
            {
                const int sf = m.getKeySignatureNumberOfSharpsOrFlats();
                const bool minor = ! m.isKeySignatureMajorKey();
                const int major = theory::pcOf (sf * 7);
                info.root = minor ? theory::pcOf (major + 9) : major;
                info.scale = minor ? 1 : 0;
                haveSig = true;
            }
            else if (m.isNoteOn() && e->noteOffObject != nullptr)
            {
                const double len = (e->noteOffObject->message.getTimeStamp() - m.getTimeStamp()) / tpq;
                hist[(size_t) (m.getNoteNumber() % 12)] += (float) jlimit (0.1, 4.0, len) * (0.5f + m.getFloatVelocity());
            }
        }
    }
    info.fromFile = haveText || haveSig;
    if (! info.fromFile)
    {
        float total = 0.0f;
        for (auto h : hist) total += h;
        if (total > 0.0f)
        {
            const auto guess = theory::guessKey (hist);
            info.root = guess.root;
            info.scale = guess.minor ? 1 : 0;
        }
    }
    return info;
}

File MidiRecorder::dragCopy (const File& f)
{
    if (! f.existsAsFile()) return f;
    const bool wav = f.hasFileExtension ("wav");
    const auto midi = wav ? f.withFileExtension (".mid") : f;
    if (! midi.existsAsFile()) return f;
    const auto info = readKeyInfo (midi);
    const String name = theory::keyTitle (info.root, info.scale, true) + " - " + String (roundToInt (info.bpm)) + " BPM";
    // Una carpeta por archivo para que dos grabaciones en la misma tonalidad no se pisen
    auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("MidiCat arrastrar")
                   .getChildFile (String::toHexString (f.getFullPathName().hashCode64()));
    dir.createDirectory();
    // (el # se queda: sí se puede en Mac y Windows)
    auto out = dir.getChildFile (name.removeCharacters ("\\/:*?\"<>|") + (wav ? ".wav" : ".mid"));
    if (out.getLastModificationTime() < f.getLastModificationTime() || ! out.existsAsFile())
        if (! f.copyFileTo (out)) return f;
    return out;
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
