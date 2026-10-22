#include "MainComponent.h"

#include "brain.h"
#include "renderer.h"

using namespace Sungam;

namespace
{
    const char* audioPatterns = "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3";
    const char* audioExtensions = "wav;aif;aiff;flac;ogg;mp3";

    juce::String asInt (double v) { return juce::String (juce::roundToInt (v)); }

    std::string utf8 (const juce::File& f) { return f.getFullPathName().toStdString(); }
}

MainComponent::MainComponent()
{
    // the default too, so dialogs and menus opened from here match
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf);
    setLookAndFeel (&lnf);
    formats.registerBasicFormats();
    feedback.run();
    engine = std::make_unique<Engine>();
    lastDir = juce::File::getSpecialLocation (juce::File::userMusicDirectory);

    // -- top bar ---------------------------------------------------------------
    play.onClick = [this] { engine->audio ("/start"); };
    stop.onClick = [this] { engine->audio ("/pause"); };
    rec.onClick = [this] { toggleRecord(); };
    rec.setTooltip ("Record the output to a wav file; each take gets the next number");
    volume.format = [] (double v) { return juce::Decibels::toString (juce::Decibels::gainToDecibels (v), 1); };
    volume.onChange = [this] (double v) { engine->audio ("/volume", (float) (v / 10.0)); };
    sessionLoad.onClick = [this] { loadSession(); };
    sessionSave.onClick = [this] { saveSession(); };
    sessionLoad.setTooltip ("A session holds the brain, the target and every setting");
    audioSettings.onClick = [this] { showAudioSettings(); };

    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &play, &stop, &rec, &volume, &meter, &sessionLoad, &sessionSave, &audioSettings })
        addAndMakeVisible (c);

    // -- target ----------------------------------------------------------------
    targetLoad.onClick = [this] { chooseTarget(); };
    targetView.onFileDropped = [this] (const juce::File& f) { loadTarget (f); };
    targetBlockSize.format = asInt;
    targetBlockSize.onChange = [this] (double) { sendTargetBlocks(); };
    targetOverlap.onChange = [this] (double) { sendTargetBlocks(); };
    targetWindow.onChange = [this] (int i) { engine->process ("/target_window_type", i); };
    targetGenerate.filled = true;
    targetGenerate.onClick = [this]
    {
        engine->process ("/generate_target");
        targetGenerated = true;
        targetView.setBlocks ((int) targetBlockSize.getValue(), targetOverlap.getValue(), true);
    };
    targetGenerate.setTooltip ("Chop the target into blocks — needed after changing its block settings");

    addAndMakeVisible (targetFrame);
    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &targetView, &targetLoad, &targetBlockSize, &targetOverlap, &targetWindow, &targetGenerate })
        addAndMakeVisible (c);

    // -- brain -----------------------------------------------------------------
    sounds.onEnable = [this] (const std::string& path, bool on)
    { engine->process (on ? "/activate_sound" : "/deactivate_sound", path); };
    sounds.onDelete = [this] (const std::string& path) { engine->process ("/delete_sample", path); };
    sounds.onFilesDropped = [this] (const juce::StringArray& files)
    {
        juce::Array<juce::File> fs;
        for (auto& f : files)
        {
            juce::File file (f);
            if (file.isDirectory()) addFolder (file);
            else fs.add (file);
        }
        addSounds (fs);
    };
    addFile.onClick = [this] { chooseSounds(); };
    addDir.onClick = [this] { chooseFolder(); };
    selectAll.onClick = [this]
    {
        sounds.setAllEnabled (true);
        for (auto& i : sounds.getItems()) engine->process ("/activate_sound", i.path);
    };
    selectNone.onClick = [this]
    {
        sounds.setAllEnabled (false);
        for (auto& i : sounds.getItems()) engine->process ("/deactivate_sound", i.path);
    };
    clearBrain.onClick = [this]
    {
        for (auto& i : sounds.getItems()) engine->process ("/delete_sample", i.path);
        sounds.clear();
    };
    brainBlockSize.format = asInt;
    brainBlockSize.onChange = [this] (double) { sendBrainBlocks(); };
    brainOverlap.onChange = [this] (double) { sendBrainBlocks(); };
    brainWindow.onChange = [this] (int i) { engine->process ("/window_type", i); };
    brainGenerate.filled = true;
    brainGenerate.onClick = [this] { engine->process ("/generate_brain"); };
    brainGenerate.setTooltip ("Chop every enabled sound into blocks and grow the synapses between them");
    brainLoad.onClick = [this] { loadBrain(); };
    brainSave.onClick = [this] { saveBrain(); };

    addAndMakeVisible (brainFrame);
    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &sounds, &addFile, &addDir, &selectAll, &selectNone, &clearBrain,
             &brainBlockSize, &brainOverlap, &brainWindow, &brainGenerate, &brainLoad, &brainSave })
        addAndMakeVisible (c);

    // -- search ----------------------------------------------------------------
    algorithm.onChange = [this] (int i) { engine->audio ("/search_algo", i); };
    algorithm.setTooltip ("basic: closest block. reversed: furthest. synaptic: search the closest block's "
                          "connections. slide: keep following connections until the error grows too big");
    ratio.onChange = [this] (double v) { engine->audio ("/ratio", (float) v); };
    ratio.setTooltip ("Match on the plain fft or on the mfcc values");
    nRatio.onChange = [this] (double v) { engine->audio ("/n_ratio", (float) v); };
    nRatio.setTooltip ("Match the original blocks (frequency and dynamics) or the normalised ones (frequency only)");
    fftStart.format = asInt;
    fftStart.onChange = [this] (double v) { engine->audio ("/fft1_start", juce::roundToInt (v)); };
    fftStart.setTooltip ("Lowest fft bin used for the match");
    fftEnd.format = asInt;
    fftEnd.onChange = [this] (double v) { engine->audio ("/fft1_end", juce::roundToInt (v)); };
    fftEnd.setTooltip ("Highest fft bin used for the match");
    novelty.onChange = [this] (double v) { engine->audio ("/novelty", (float) v); };
    novelty.setTooltip ("Use new blocks rather than similar ones");
    boredom.onChange = [this] (double v) { engine->audio ("/boredom", (float) v); };
    boredom.setTooltip ("How long it takes for the novelty to wear off");
    stickyness.onChange = [this] (double v) { engine->audio ("/stickyness", (float) v); };
    stickyness.setTooltip ("Likelihood of playing the next block rather than the closest");
    stretch.format = asInt;
    stretch.onChange = [this] (double v) { engine->audio ("/search-stretch", juce::roundToInt (v)); };
    stretch.setTooltip ("Repeat each search this many times — with novelty at 0, a dodgy jungle timestretch");
    synapses.format = asInt;
    synapses.onChange = [this] (double v) { engine->audio ("/synapses", juce::roundToInt (v)); };
    synapses.setTooltip ("How many connections to search, closest first (synaptic and slide)");
    slideError.format = asInt;
    slideError.onChange = [this] (double v) { engine->audio ("/slide-error", juce::roundToInt (v)); };
    slideError.setTooltip ("A block difference above this starts a new search (slide)");

    addAndMakeVisible (searchFrame);
    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &algorithm, &ratio, &nRatio, &fftStart, &fftEnd, &novelty, &boredom,
             &stickyness, &stretch, &synapses, &slideError })
        addAndMakeVisible (c);

    // -- mix -------------------------------------------------------------------
    targetMix.setEnds ("brain", "target");
    targetMix.onChange = [this] (double v) { engine->audio ("/target_mix", (float) v); };
    targetMix.setTooltip ("Mix in the original target blocks");
    nMix.setEnds ("original", "normalised");
    nMix.onChange = [this] (double v) { engine->audio ("/n_mix", (float) v); };
    nMix.setTooltip ("Mix in the normalised brain blocks");
    autotune.setEnds ("off", "matched");
    autotune.onChange = [this] (double v) { engine->audio ("/autotune", (float) v); };
    autotune.setTooltip ("How far to retune each brain block to the target's frequency");
    stereo.onClick = [this] { engine->audio ("/stereo", stereo.getToggleState() ? 1 : 0); };
    stereo.setTooltip ("Render the target's left and right channels separately");

    addAndMakeVisible (mixFrame);
    for (juce::Component* c : std::initializer_list<juce::Component*> { &targetMix, &nMix, &autotune, &stereo })
        addAndMakeVisible (c);

    sendAllParameters();
    setSize (1200, 800);
    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    stopTimer();
    if (audioWindow != nullptr) delete audioWindow.getComponent();
    engine.reset();
    setLookAndFeel (nullptr);
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
}

//==============================================================================
// The Qt app never sent its controls' starting values, so the engine ran on
// its own defaults until something was touched. Send them all, once.
void MainComponent::sendAllParameters()
{
    sendTargetBlocks();
    sendBrainBlocks();
    engine->process ("/window_type", brainWindow.getSelected());
    engine->process ("/target_window_type", targetWindow.getSelected());
    engine->audio ("/search_algo", algorithm.getSelected());
    engine->audio ("/ratio", (float) ratio.getValue());
    engine->audio ("/n_ratio", (float) nRatio.getValue());
    engine->audio ("/fft1_start", (int) fftStart.getValue());
    engine->audio ("/fft1_end", (int) fftEnd.getValue());
    engine->audio ("/novelty", (float) novelty.getValue());
    engine->audio ("/boredom", (float) boredom.getValue());
    engine->audio ("/stickyness", (float) stickyness.getValue());
    engine->audio ("/search-stretch", (int) stretch.getValue());
    engine->audio ("/synapses", (int) synapses.getValue());
    engine->audio ("/slide-error", (int) slideError.getValue());
    engine->audio ("/target_mix", (float) targetMix.getValue());
    engine->audio ("/n_mix", (float) nMix.getValue());
    engine->audio ("/autotune", (float) autotune.getValue());
    engine->audio ("/volume", (float) (volume.getValue() / 10.0));
    engine->audio ("/stereo", stereo.getToggleState() ? 1 : 0);
}

// The process thread turns an overlap fraction into samples using the block
// size it holds at that moment, so a size change has to re-send the overlap.
void MainComponent::sendTargetBlocks()
{
    engine->process ("/target_block_size", (int) targetBlockSize.getValue());
    engine->process ("/target_overlap", (float) targetOverlap.getValue());
    if (targetGenerated)
        targetView.setBlocks ((int) targetBlockSize.getValue(), targetOverlap.getValue(), false);
    targetGenerated = false;
}

void MainComponent::sendBrainBlocks()
{
    engine->process ("/source_block_size", (int) brainBlockSize.getValue());
    engine->process ("/source_overlap", (float) brainOverlap.getValue());
}

//==============================================================================
void MainComponent::loadTarget (const juce::File& f)
{
    if (! f.existsAsFile()) return;
    lastDir = f.getParentDirectory();
    engine->process ("/load_target", utf8 (f));
    targetView.setFile (f);
    targetGenerated = false;
}

void MainComponent::addSounds (const juce::Array<juce::File>& files)
{
    for (auto& f : files)
    {
        if (! f.existsAsFile() || ! f.hasFileExtension (audioExtensions)) continue;
        lastDir = f.getParentDirectory();
        engine->process ("/load_sample", utf8 (f));
        sounds.add (utf8 (f), true);
    }
}

void MainComponent::addFolder (const juce::File& dir)
{
    auto files = dir.findChildFiles (juce::File::findFiles, true, audioPatterns);
    files.sort();
    addSounds (files);
    lastDir = dir;
}

void MainComponent::choose (const juce::String& title, const juce::String& patterns, bool save, bool dirs, Chosen done)
{
    chooser = std::make_unique<juce::FileChooser> (title, lastDir, patterns);
    int flags = save ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting
                     : juce::FileBrowserComponent::openMode;
    flags |= dirs ? juce::FileBrowserComponent::canSelectDirectories : juce::FileBrowserComponent::canSelectFiles;
    chooser->launchAsync (flags, [done] (const juce::FileChooser& fc)
    {
        const auto f = fc.getResult();
        if (f != juce::File()) done (f);
    });
}

void MainComponent::chooseTarget()
{
    choose ("Load a target sound", audioPatterns, false, false, [this] (const juce::File& f) { loadTarget (f); });
}

void MainComponent::chooseSounds()
{
    chooser = std::make_unique<juce::FileChooser> ("Add sounds to the brain", lastDir, audioPatterns);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::canSelectMultipleItems,
                          [this] (const juce::FileChooser& fc) { addSounds (fc.getResults()); });
}

void MainComponent::chooseFolder()
{
    choose ("Add a folder of sounds", {}, false, true, [this] (const juce::File& f) { addFolder (f); });
}

void MainComponent::loadBrain()
{
    choose ("Load a brain", "*.brain", false, false, [this] (const juce::File& f)
    {
        lastDir = f.getParentDirectory();
        engine->process ("/load_brain", utf8 (f));

        // a brain file carries its sounds: read their names back for the list
        juce::Thread::launch ([sp = juce::Component::SafePointer<MainComponent> (this), path = utf8 (f)]
        {
            std::ifstream ifs (path.c_str(), std::ios::binary);
            if (! ifs) return;
            spiralcore::brain b;
            ifs || b;
            auto names = std::make_shared<std::vector<std::pair<std::string, bool>>>();
            for (auto& i : b.get_samples()) names->push_back ({ i.m_filename, i.m_enabled });
            juce::MessageManager::callAsync ([sp, names]
            {
                if (sp == nullptr) return;
                sp->sounds.clear();
                for (auto& [n, e] : *names) sp->sounds.add (n, e);
            });
        });
    });
}

void MainComponent::saveBrain()
{
    choose ("Save the brain", "*.brain", true, false, [this] (const juce::File& f)
    {
        lastDir = f.getParentDirectory();
        engine->process ("/save_brain", utf8 (f.withFileExtension ("brain")));
    });
}

void MainComponent::loadSession()
{
    choose ("Load a session", "*.samplebrain", false, false, [this] (const juce::File& f)
    {
        lastDir = f.getParentDirectory();
        engine->process ("/load_session", utf8 (f));
        applySession (f);
    });
}

void MainComponent::saveSession()
{
    choose ("Save the session", "*.samplebrain", true, false, [this] (const juce::File& f)
    {
        lastDir = f.getParentDirectory();
        engine->process ("/save_session", utf8 (f.withFileExtension ("samplebrain")));
    });
}

// Reads the settings back out of a session file to set the controls, the way
// the Qt app's init_from_session did. The engine loads the same file itself;
// this copy only feeds the interface, on a background thread because it
// decodes all the sample data too.
void MainComponent::applySession (const juce::File& file)
{
    struct Settings
    {
        float ratio, nRatio, novelty, boredom, stickyness, targetMix, nMix, autotune;
        int fftStart, fftEnd, stretch, algo, synapses, slideError;
        int targetBlock, sourceBlock, targetWindow, sourceWindow;
        float targetOverlap, sourceOverlap;
        std::vector<std::pair<std::string, bool>> samples;
    };

    busy = true;
    juce::Thread::launch ([sp = juce::Component::SafePointer<MainComponent> (this), path = utf8 (file)]
    {
        using namespace spiralcore;
        std::ifstream ifs (path.c_str(), std::ios::binary);
        if (! ifs) return;

        brain s, t;
        u32 version = 0;
        ifs || version;
        renderer r (s, t);
        ifs || r;
        ifs || r;

        u32 targetWindowSize, sourceWindowSize;
        window::type targetWindow, sourceWindow;
        float targetOverlap, sourceOverlap;
        ifs || sourceWindowSize || sourceOverlap;
        ifs || targetWindowSize || targetOverlap;
        ifs || sourceWindow || targetWindow;
        ifs || s;
        ifs || t;

        auto settings = std::make_shared<Settings>();
        auto* p = r.get_params();
        settings->ratio = p->m_ratio;
        settings->nRatio = p->m_n_ratio;
        settings->fftStart = (int) p->m_fft1_start;
        settings->fftEnd = (int) p->m_fft1_end;
        settings->novelty = p->m_usage_importance;
        settings->boredom = s.get_usage_falloff();
        settings->stickyness = p->m_stickyness;
        settings->stretch = (int) r.get_stretch();
        settings->algo = (int) r.get_search_algo();
        settings->synapses = (int) p->m_num_synapses;
        settings->slideError = (int) r.get_slide_error();
        settings->targetBlock = (int) t.get_block_size();
        settings->targetOverlap = t.get_block_size() > 0 ? t.get_overlap() / (float) t.get_block_size() : 0.75f;
        settings->targetWindow = (int) targetWindow;
        settings->sourceBlock = (int) s.get_block_size();
        settings->sourceOverlap = s.get_block_size() > 0 ? s.get_overlap() / (float) s.get_block_size() : 0.75f;
        settings->sourceWindow = (int) sourceWindow;
        settings->targetMix = r.get_target_mix();
        settings->nMix = r.get_n_mix();
        settings->autotune = r.get_autotune();
        for (auto& i : s.get_samples())
            settings->samples.push_back ({ i.m_filename, i.m_enabled });

        juce::MessageManager::callAsync ([sp, settings]
        {
            if (sp == nullptr) return;
            auto& m = *sp;
            const auto quiet = juce::dontSendNotification;
            m.ratio.setValue (settings->ratio, quiet);
            m.nRatio.setValue (settings->nRatio, quiet);
            m.fftStart.setValue (settings->fftStart, quiet);
            m.fftEnd.setValue (settings->fftEnd, quiet);
            m.novelty.setValue (settings->novelty, quiet);
            m.boredom.setValue (settings->boredom, quiet);
            m.stickyness.setValue (settings->stickyness, quiet);
            m.stretch.setValue (settings->stretch, quiet);
            m.algorithm.setSelected (settings->algo, quiet);
            m.synapses.setValue (settings->synapses, quiet);
            m.slideError.setValue (settings->slideError, quiet);
            m.targetBlockSize.setValue (settings->targetBlock, quiet);
            m.targetOverlap.setValue (settings->targetOverlap, quiet);
            m.targetWindow.setSelected (settings->targetWindow, quiet);
            m.brainBlockSize.setValue (settings->sourceBlock, quiet);
            m.brainOverlap.setValue (settings->sourceOverlap, quiet);
            m.brainWindow.setSelected (settings->sourceWindow, quiet);
            m.targetMix.setValue (settings->targetMix, quiet);
            m.nMix.setValue (settings->nMix, quiet);
            m.autotune.setValue (settings->autotune, quiet);

            m.sounds.clear();
            for (auto& [path, enabled] : settings->samples)
                m.sounds.add (path, enabled);
            m.busy = false;
        });
    });
}

void MainComponent::toggleRecord()
{
    if (engine->isRecording())
    {
        engine->stopRecording();
        statusText = "recording saved";
        return;
    }

    auto start = [this]
    {
        auto f = recordBase.getSiblingFile (recordBase.getFileNameWithoutExtension()
                                            + "-" + juce::String (recordIndex++) + ".wav");
        if (engine->startRecording (f))
            statusText = "recording to " + f.getFileName();
        else
            statusText = "could not write " + f.getFullPathName();
    };

    if (recordBase != juce::File())
    {
        start();
        return;
    }

    choose ("Record to", "*.wav", true, false, [this, start] (const juce::File& f)
    {
        recordBase = f.withFileExtension ("wav");
        recordIndex = 0;
        start();
    });
}

void MainComponent::showAudioSettings()
{
    if (audioWindow != nullptr)
    {
        audioWindow->toFront (true);
        return;
    }

    auto* selector = new juce::AudioDeviceSelectorComponent (engine->deviceManager, 0, 2, 0, 2,
                                                             false, false, true, false);
    selector->setSize (520, 380);

    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned (selector);
    o.dialogTitle = "Audio";
    o.dialogBackgroundColour = paper;
    o.componentToCentreAround = this;
    o.useNativeTitleBar = true;
    o.resizable = false;
    // the window deletes itself when closed; the SafePointer then goes null
    audioWindow = o.launchAsync();
}

//==============================================================================
void MainComponent::pollFeedback()
{
    command_ring_buffer::command cmd;
    while (feedback.get (cmd))
    {
        const std::string name = cmd.m_name;
        if (name == "/report")
        {
            statusText = juce::String::fromUTF8 (cmd.get_string (0));
            const bool working = statusText.contains ("%") || statusText.startsWith ("processing")
                                 || statusText.startsWith ("building");
            busy = working;
        }
        else if (name == "/sound-item")
        {
            sounds.setStateFromColour (cmd.get_string (0), cmd.get_string (1));
        }
    }
}

void MainComponent::timerCallback()
{
    pollFeedback();

    const bool playing = engine->isPlaying();
    play.setToggleState (playing, juce::dontSendNotification);
    rec.setToggleState (engine->isRecording(), juce::dontSendNotification);
    targetView.setPlayhead (playing || engine->getTargetTime() > 0 ? engine->getTargetTime() : -1.0);

    // fast attack, slow release
    const float peak = engine->takeOutputPeak();
    meterLevel = juce::jmax (peak, meterLevel * 0.85f);
    meter.setLevel (meterLevel > 0.0f ? juce::jmap (juce::Decibels::gainToDecibels (meterLevel, -60.0f), -60.0f, 0.0f, 0.0f, 1.0f) : 0.0f);

    repaint (statusBar);
    repaint (topBar);
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (paper);

    // top bar
    auto bar = topBar.reduced (px (12.0f), 0);
    drawWordmark (g, "samplebrain", bar.removeFromLeft (px (190.0f)), textLg, ink85);
    g.setColour (ink13);
    g.fillRect (topBar.getX(), topBar.getBottom() - 1, topBar.getWidth(), 1);

    // status line
    g.setColour (ink13);
    g.fillRect (statusBar.getX(), statusBar.getY(), statusBar.getWidth(), 1);
    auto s = statusBar.reduced (px (12.0f), 0);

    auto lamp = s.removeFromLeft (px (9.0f)).toFloat().withSizeKeepingCentre ((float) px (7.0f), (float) px (7.0f));
    if (busy) { g.setColour (amber); g.fillRect (lamp); }
    else      { g.setColour (ink28); g.drawRect (lamp.reduced (0.5f), hairline); }
    s.removeFromLeft (px (8.0f));

    juce::String device;
    if (auto* d = engine->deviceManager.getCurrentAudioDevice())
        device = d->getName() + "  " + juce::String (d->getCurrentSampleRate() / 1000.0, 1) + " kHz  "
                 + juce::String (d->getCurrentBufferSizeSamples());
    else
        device = "no audio device";

    g.setFont (mono (textXs));
    g.setColour (ink38);
    const int dw = (int) stringWidth (g.getCurrentFont(), device) + 4;
    g.drawText (device, s.removeFromRight (dw), juce::Justification::centredRight, false);
    g.setColour (ink62);
    g.drawText (statusText, s, juce::Justification::centredLeft, true);

    // credit, quietly, under the mix controls
    auto credit = mixFrame.getBounds().reduced (px (10.0f)).removeFromBottom (px (40.0f));
    g.setFont (mono (text2xs));
    g.setColour (ink28);
    g.drawFittedText ("designed by Richard D. James\nand Then Try This", credit, juce::Justification::bottomLeft, 2);
}

void MainComponent::resized()
{
    const int gutter = px (12.0f);
    const int latchH = px (17.0f);
    const int numH = NumBox::preferredHeight();
    const int faderH = Fader::preferredHeight() + px (12.0f);

    auto area = getLocalBounds();
    topBar = area.removeFromTop (px (40.0f));
    statusBar = area.removeFromBottom (px (22.0f));
    area.reduce (gutter, gutter);

    // -- top bar: transport left of centre, volume and meter, files right
    {
        auto bar = topBar.reduced (gutter, 0);
        bar.removeFromLeft (px (190.0f));
        auto row = bar.withSizeKeepingCentre (bar.getWidth(), latchH);

        audioSettings.setBounds (row.removeFromRight (px (60.0f)));
        row.removeFromRight (6);
        sessionSave.setBounds (row.removeFromRight (px (104.0f)));
        row.removeFromRight (6);
        sessionLoad.setBounds (row.removeFromRight (px (104.0f)));

        play.setBounds (row.removeFromLeft (px (56.0f)));
        row.removeFromLeft (6);
        stop.setBounds (row.removeFromLeft (px (56.0f)));
        row.removeFromLeft (6);
        rec.setBounds (row.removeFromLeft (px (48.0f)));
        row.removeFromLeft (px (24.0f));

        auto vol = bar.withX (row.getX()).withWidth (px (150.0f));
        volume.setBounds (vol.withSizeKeepingCentre (vol.getWidth(), Fader::preferredHeight()));
        row.removeFromLeft (vol.getWidth() + px (14.0f));
        meter.setBounds (row.removeFromLeft (px (120.0f)));
    }

    // -- target across the top
    auto target = area.removeFromTop (px (150.0f));
    targetFrame.setBounds (target);
    {
        auto c = targetFrame.getContentBounds() + target.getPosition();
        auto controls = c.removeFromRight (px (300.0f));
        c.removeFromRight (gutter);
        targetView.setBounds (c);

        auto row1 = controls.removeFromTop (numH);
        const int w = (row1.getWidth() - 12) / 3;
        targetBlockSize.setBounds (row1.removeFromLeft (w));
        row1.removeFromLeft (6);
        targetOverlap.setBounds (row1.removeFromLeft (w));
        row1.removeFromLeft (6);
        targetWindow.setBounds (row1);

        auto row2 = controls.removeFromBottom (latchH);
        targetLoad.setBounds (row2.removeFromLeft ((row2.getWidth() - 6) / 2));
        row2.removeFromLeft (6);
        targetGenerate.setBounds (row2);
    }

    area.removeFromTop (gutter);

    // -- brain | search | mix
    auto brainArea = area.removeFromLeft (juce::roundToInt (area.getWidth() * 0.36f));
    area.removeFromLeft (gutter);
    auto mixArea = area.removeFromRight (px (230.0f));
    area.removeFromRight (gutter);
    auto searchArea = area;

    brainFrame.setBounds (brainArea);
    {
        auto c = brainFrame.getContentBounds() + brainArea.getPosition();

        auto files = c.removeFromBottom (latchH);
        brainSave.setBounds (files.removeFromRight ((files.getWidth() - 6) / 3));
        files.removeFromRight (6);
        brainLoad.setBounds (files.removeFromRight ((files.getWidth() - 6) / 2));
        files.removeFromRight (6);
        brainGenerate.setBounds (files);
        c.removeFromBottom (px (10.0f));

        auto blocks = c.removeFromBottom (numH);
        const int w = (blocks.getWidth() - 12) / 3;
        brainBlockSize.setBounds (blocks.removeFromLeft (w));
        blocks.removeFromLeft (6);
        brainOverlap.setBounds (blocks.removeFromLeft (w));
        blocks.removeFromLeft (6);
        brainWindow.setBounds (blocks);
        c.removeFromBottom (px (10.0f));

        auto edit = c.removeFromBottom (latchH);
        const int ew = (edit.getWidth() - 24) / 5;
        for (auto* b : { &addFile, &addDir, &selectAll, &selectNone })
        {
            b->setBounds (edit.removeFromLeft (b == &addFile || b == &addDir ? ew + ew / 2 : ew - ew / 2 + 2));
            edit.removeFromLeft (6);
        }
        clearBrain.setBounds (edit);
        c.removeFromBottom (6);

        sounds.setBounds (c);
    }

    searchFrame.setBounds (searchArea);
    {
        auto c = searchFrame.getContentBounds() + searchArea.getPosition();
        algorithm.setBounds (c.removeFromTop (px (21.0f)));
        c.removeFromTop (px (14.0f));

        NumBox* grid[][2] = { { &ratio, &nRatio }, { &fftStart, &fftEnd }, { &novelty, &boredom },
                              { &stickyness, &stretch }, { &synapses, &slideError } };
        const int rowsN = 5;
        const int rowGap = juce::jlimit (8, px (16.0f), (c.getHeight() - rowsN * numH) / (rowsN - 1));
        for (auto& row : grid)
        {
            auto r = c.removeFromTop (numH);
            const int w = (r.getWidth() - 12) / 2;
            row[0]->setBounds (r.removeFromLeft (w));
            r.removeFromLeft (12);
            row[1]->setBounds (r);
            c.removeFromTop (rowGap);
        }
    }

    mixFrame.setBounds (mixArea);
    {
        auto c = mixFrame.getContentBounds() + mixArea.getPosition();
        for (auto* f : { &targetMix, &nMix, &autotune })
        {
            f->setBounds (c.removeFromTop (faderH));
            c.removeFromTop (px (14.0f));
        }
        stereo.setBounds (c.removeFromTop (latchH));
    }
}
