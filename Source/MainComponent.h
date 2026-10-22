#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "Engine.h"
#include "ui/Controls.h"
#include "ui/LookAndFeel.h"
#include "ui/SoundList.h"
#include "ui/TargetView.h"

// the brain's window::type order
inline juce::StringArray windowShapes()
{
    return { "dodgy", "bartlett", "blackman", "flat top", "gaussian", "hamming", "hann", "rectangle" };
}

// The whole window: a top bar with the transport, the target across the top,
// then the brain, its search tweaks and the mix side by side, and a status
// line. Every control sends the same message the Qt app sent for it.
class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    // for --target / --sounds on the command line, and drops
    void loadTarget (const juce::File&);
    void addSounds (const juce::Array<juce::File>&);
    void addFolder (const juce::File&);
    void generateAll() { brainGenerate.triggerClick(); targetGenerate.triggerClick(); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void pollFeedback();

    void sendAllParameters();
    void sendTargetBlocks();
    void sendBrainBlocks();

    void chooseTarget();
    void chooseSounds();
    void chooseFolder();
    void loadBrain();
    void saveBrain();
    void loadSession();
    void saveSession();
    void toggleRecord();
    void showAudioSettings();
    void applySession (const juce::File&);

    using Chosen = std::function<void (const juce::File&)>;
    void choose (const juce::String& title, const juce::String& patterns, bool save, bool dirs, Chosen);

    juce::Rectangle<int> topBar, statusBar;

    Sungam::LookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };
    juce::AudioFormatManager formats;

    // the "8890" feedback bus, the brain's status reports; exists before the
    // engine so its first reports land
    OSC_server feedback { "8890" };
    std::unique_ptr<Engine> engine;

    // -- top bar
    Sungam::Latch play { "play", Sungam::amber, false };
    Sungam::Latch stop { "stop", Sungam::ink62, false };
    Sungam::Latch rec { "rec", Sungam::coral, false };
    Sungam::Fader volume { "volume", 0.0, 2.0, 1.0, Sungam::steel };
    Sungam::SegmentMeter meter { Sungam::steel, 14 };
    Sungam::Latch sessionLoad { "load session", Sungam::ink62, false };
    Sungam::Latch sessionSave { "save session", Sungam::ink62, false };
    Sungam::Latch audioSettings { "audio", Sungam::ink62, false };

    // -- target
    Sungam::PanelFrame targetFrame { "target", Sungam::coral };
    TargetView targetView { formats };
    Sungam::Latch targetLoad { "load target", Sungam::coral, false };
    Sungam::NumBox targetBlockSize { "block size", 10, 99999, 1000, Sungam::coral, 1 };
    Sungam::NumBox targetOverlap { "overlap", 0.0, 0.99, 0.75, Sungam::coral, 0.01 };
    Sungam::Dropdown targetWindow { "window", windowShapes(), 0, Sungam::coral };
    Sungam::Latch targetGenerate { "generate blocks", Sungam::coral, false };
    Sungam::Lamp targetLamp { Sungam::amber };

    // -- brain
    Sungam::PanelFrame brainFrame { "brain", Sungam::teal };
    SoundList sounds;
    Sungam::Latch addFile { "add file", Sungam::teal, false };
    Sungam::Latch addDir { "add folder", Sungam::teal, false };
    Sungam::Latch selectAll { "all", Sungam::teal, false };
    Sungam::Latch selectNone { "none", Sungam::teal, false };
    Sungam::Latch clearBrain { "clear", Sungam::coral, false };
    Sungam::NumBox brainBlockSize { "block size", 10, 99999, 1000, Sungam::teal, 1 };
    Sungam::NumBox brainOverlap { "overlap", 0.0, 0.99, 0.75, Sungam::teal, 0.01 };
    Sungam::Dropdown brainWindow { "window", windowShapes(), 0, Sungam::teal };
    Sungam::Latch brainGenerate { "generate brain", Sungam::teal, false };
    Sungam::Latch brainLoad { "load brain", Sungam::ink62, false };
    Sungam::Latch brainSave { "save brain", Sungam::ink62, false };

    // -- search (the Qt app's "brain tweaks")
    Sungam::PanelFrame searchFrame { "search", Sungam::teal };
    Sungam::Selector algorithm { { "basic", "reversed", "synaptic", "slide" }, 0, Sungam::teal };
    Sungam::NumBox ratio { "fft / mfcc", 0, 1, 0.5, Sungam::teal, 0.01 };
    Sungam::NumBox nRatio { "dynamics / freq", 0, 1, 0, Sungam::teal, 0.01 };
    Sungam::NumBox fftStart { "fft start", 0, 99, 0, Sungam::lilac, 1 };
    Sungam::NumBox fftEnd { "fft end", 0, 99, 99, Sungam::lilac, 1 };
    Sungam::NumBox novelty { "novelty", 0, 1, 0, Sungam::teal, 0.01 };
    Sungam::NumBox boredom { "boredom", 0, 1, 0, Sungam::lilac, 0.01 };
    Sungam::NumBox stickyness { "stickyness", 0, 1, 0, Sungam::teal, 0.01 };
    Sungam::NumBox stretch { "search stretch", 1, 50, 1, Sungam::teal, 1 };
    Sungam::NumBox synapses { "synapses", 0, 1000, 20, Sungam::teal, 1 };
    Sungam::NumBox slideError { "slide error", 0, 10000, 1000, Sungam::teal, 100 };

    // -- mix
    Sungam::PanelFrame mixFrame { "mix", Sungam::steel };
    Sungam::Fader targetMix { "target mix", 0, 1, 0, Sungam::steel };
    Sungam::Fader nMix { "normalised", 0, 1, 0, Sungam::steel };
    Sungam::Fader autotune { "autotune", 0, 1, 0, Sungam::steel };
    Sungam::Latch stereo { "stereo", Sungam::steel, true };

    juce::String statusText { "ready" };
    bool targetGenerated = false;
    bool busy = false;
    float meterLevel = 0.0f;

    juce::File lastDir;
    juce::File recordBase;
    int recordIndex = 0;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Component::SafePointer<juce::DialogWindow> audioWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
