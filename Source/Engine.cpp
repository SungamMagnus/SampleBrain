#include "Engine.h"
#include "sound_file.h"

using namespace spiralcore;

// The brain asks the host to decode files (SAMPLEBRAIN_HOST_SOUNDFILE), so
// anything JUCE reads — wav, aiff, flac, ogg, mp3 — can go in.
bool spiralcore::read_sound_file (const std::string& filename, std::vector<float>& interleaved,
                                  long& frames, int& channels)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (juce::File (juce::String::fromUTF8 (filename.c_str()))));
    if (reader == nullptr || reader->lengthInSamples <= 0) return false;

    channels = (int) reader->numChannels;
    frames = (long) reader->lengthInSamples;
    juce::AudioBuffer<float> buf (channels, (int) frames);
    reader->read (&buf, 0, (int) frames, 0, true, true);

    interleaved.resize ((size_t) frames * (size_t) channels);
    for (int c = 0; c < channels; ++c)
    {
        auto* src = buf.getReadPointer (c);
        for (long i = 0; i < frames; ++i)
            interleaved[(size_t) i * (size_t) channels + (size_t) c] = src[i];
    }
    return true;
}

Engine::Engine()
{
    brainMutex = processThread.m_brain_mutex;

    // the same construction order as the Qt app's main()
    pthread_mutex_lock (brainMutex);
    leftRenderer = new renderer (processThread.m_source, processThread.m_left_target);
    rightRenderer = new renderer (processThread.m_source, processThread.m_right_target);
    blockStream = new block_stream();
    pthread_mutex_unlock (brainMutex);
    osc.run();
    processThread.register_renderer (leftRenderer, rightRenderer, blockStream);

    writerThread.startThread();

    // the brain plays samples at their own rate, so ask for the rate most
    // material is in; the device may still pick another
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    setup.sampleRate = 44100.0;
    setup.bufferSize = 512;
    deviceManager.initialise (0, 2, nullptr, true, {}, &setup);
    deviceManager.addAudioCallback (this);
}

Engine::~Engine()
{
    stopRecording();
    deviceManager.removeAudioCallback (this);
    deviceManager.closeAudioDevice();
    writerThread.stopThread (2000);

    pthread_mutex_lock (brainMutex);
    delete leftRenderer;
    delete rightRenderer;
    pthread_mutex_unlock (brainMutex);
}

bool Engine::startRecording (const juce::File& file)
{
    stopRecording();
    file.deleteFile();
    std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
    if (stream == nullptr) return false;

    juce::WavAudioFormat wav;
    auto* w = wav.createWriterFor (stream.get(), sampleRate, 2, 24, {}, 0);
    if (w == nullptr) return false;
    stream.release();

    const juce::ScopedLock sl (writerLock);
    writer = std::make_unique<juce::AudioFormatWriter::ThreadedWriter> (w, writerThread, 1 << 16);
    activeWriter = writer.get();
    audio ("/start");
    return true;
}

void Engine::stopRecording()
{
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> finished;
    {
        const juce::ScopedLock sl (writerLock);
        activeWriter = nullptr;
        finished = std::move (writer);
    }
    finished.reset(); // flushes and closes the file
}

void Engine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
    scratch.setSize (2, device->getCurrentBufferSizeSamples() * 2);
}

void Engine::handleCommands()
{
    command_ring_buffer::command cmd;
    while (osc.get (cmd))
    {
        const std::string name = cmd.m_name;
        auto both = [&] (auto fn) { fn (leftRenderer); fn (rightRenderer); };

        if (name == "/start")          both ([] (renderer* r) { r->set_playing (true); });
        else if (name == "/pause")     both ([] (renderer* r) { r->set_playing (false); });
        else if (name == "/ratio")     both ([&] (renderer* r) { r->get_params()->m_ratio = cmd.get_float (0); });
        else if (name == "/n_ratio")   both ([&] (renderer* r) { r->get_params()->m_n_ratio = cmd.get_float (0); });
        else if (name == "/fft1_start") both ([&] (renderer* r) { r->get_params()->m_fft1_start = cmd.get_int (0); });
        else if (name == "/fft1_end")  both ([&] (renderer* r) { r->get_params()->m_fft1_end = cmd.get_int (0); });
        else if (name == "/novelty")   both ([&] (renderer* r) { r->get_params()->m_usage_importance = cmd.get_float (0); });
        else if (name == "/stickyness") both ([&] (renderer* r) { r->get_params()->m_stickyness = cmd.get_float (0); });
        else if (name == "/autotune")  both ([&] (renderer* r) { r->set_autotune (cmd.get_float (0)); });
        else if (name == "/volume")    both ([&] (renderer* r) { r->set_volume (cmd.get_float (0) * 10); });
        else if (name == "/n_mix")     both ([&] (renderer* r) { r->set_n_mix (cmd.get_float (0)); });
        else if (name == "/target_mix") both ([&] (renderer* r) { r->set_target_mix (cmd.get_float (0)); });
        else if (name == "/boredom")   both ([&] (renderer* r) { r->get_source().set_usage_falloff (cmd.get_float (0)); });
        else if (name == "/synapses")  both ([&] (renderer* r) { r->get_params()->m_num_synapses = cmd.get_int (0); });
        else if (name == "/search-stretch") both ([&] (renderer* r) { r->set_stretch (cmd.get_int (0)); });
        else if (name == "/slide-error") both ([&] (renderer* r) { r->set_slide_error (cmd.get_int (0)); });
        else if (name == "/search_algo")
        {
            static const renderer::search_algo algos[] = { renderer::BASIC, renderer::REV_BASIC,
                                                           renderer::SYNAPTIC, renderer::SYNAPTIC_SLIDE };
            const int i = cmd.get_int (0);
            if (i >= 0 && i < 4) both ([&] (renderer* r) { r->set_search_algo (algos[i]); });
        }
        else if (name == "/stereo")
        {
            stereoMode = cmd.get_int (0) != 0;
            both ([] (renderer* r) { r->reset(); });
        }
    }
}

void Engine::audioDeviceIOCallbackWithContext (const float* const*, int,
                                               float* const* out, int numOut, int numSamples,
                                               const juce::AudioIODeviceCallbackContext&)
{
    handleCommands();

    if (scratch.getNumSamples() < numSamples) scratch.setSize (2, numSamples, false, false, true);
    scratch.clear();
    float* left = scratch.getWritePointer (0);
    float* right = scratch.getWritePointer (1);

    if (! pthread_mutex_trylock (brainMutex))
    {
        leftRenderer->process ((u32) numSamples, left, nullptr);
        if (stereoMode)
            rightRenderer->process ((u32) numSamples, right, nullptr);
        else
            juce::FloatVectorOperations::copy (right, left, numSamples);
        pthread_mutex_unlock (brainMutex);
    }

    for (int c = 0; c < numOut; ++c)
        if (out[c] != nullptr)
            juce::FloatVectorOperations::copy (out[c], c == 1 ? right : left, numSamples);

    const auto range = juce::FloatVectorOperations::findMinAndMax (left, numSamples);
    const float peak = juce::jmax (std::abs (range.getStart()), std::abs (range.getEnd()));
    float prev = outputPeak.load();
    while (peak > prev && ! outputPeak.compare_exchange_weak (prev, peak)) {}

    // try, never wait: stopRecording() holds the lock only to drop the writer
    const juce::ScopedTryLock sl (writerLock);
    if (sl.isLocked())
        if (auto* w = activeWriter.load())
        {
            const float* chans[] = { left, right };
            w->write (chans, numSamples);
        }
}
