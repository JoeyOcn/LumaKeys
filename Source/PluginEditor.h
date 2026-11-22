#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Views.h"

//==============================================================================
class MidiSkyLook : public juce::LookAndFeel_V4
{
public:
    explicit MidiSkyLook (Fonts& f) : fonts (f) {}
    void setPalette (const Palette& p);
    juce::Font getPopupMenuFont() override { return fonts.ui (15.0f); }
    void drawCallOutBoxBackground (juce::CallOutBox&, juce::Graphics&, const juce::Path&, juce::Image&) override;

private:
    Fonts& fonts;
    Palette pal;
};

//==============================================================================
class MidiSkyEditor : public juce::AudioProcessorEditor,
                      private juce::Timer,
                      private juce::ChangeListener
{
public:
    explicit MidiSkyEditor (MidiSkyProcessor&);
    ~MidiSkyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void changeListenerCallback (juce::ChangeBroadcaster*) override;

    void refreshPalette();
    void rebuildBackground();
    void updateKeyState (bool& keyChanged);
    void updateMidiStatus();

    void showKeyMenu();
    void showScaleMenu();
    void showMainMenu();
    void showMidiMenu();
    void showSkinPicker();
    void showColourPicker();
    void showAbout();

    void previewChord (int index);
    void stopPreview();

    void setChoice (const juce::String& id, int index);
    void setBool (const juce::String& id, bool value);
    bool isStandalone() const;

    MidiSkyProcessor& proc;
    juce::SharedResourcePointer<Fonts> fonts;
    juce::SharedResourcePointer<Prefs> prefs;
    MidiSkyLook look;

    Vis vis;
    UiContext ctx { vis, *fonts };

    TopBar top { ctx };
    StaffView staff { ctx };
    ChordScreen screen { ctx };
    CircleView circle { ctx };
    ControlStrip strip { ctx };
    KeyboardView keys { ctx };

    juce::Image background;
    float bgScale = 1.0f;

    // Detección automática de tonalidad
    std::array<float, 12> hist {};
    int autoRoot = 0;
    bool autoMinor = false;
    bool autoHas = false;

    double lastTick = 0.0;
    int tickCount = 0;
    std::vector<int> previewNotes;
    double previewOffAt = 0.0;
    juce::Array<juce::MidiDeviceInfo> knownMidiInputs;

    // Últimos valores para saber qué repintar
    int lastRoot = -1, lastScale = -1, lastMask = -1, lastLowest = -2;
    bool lastLatin = false, lastSevenths = false, lastHighlight = true, lastPedal = false, lastAuto = true, lastAutoHas = false;
    std::vector<int> lastActive;

    juce::Component::SafePointer<juce::ColourSelector> colourSelector;
    std::unique_ptr<juce::ParameterAttachment> volumeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiSkyEditor)
};
