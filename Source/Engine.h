#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>

#include "process_thread.h"
#include "spiralcore/local_bus.h"

// The JUCE stand-in for the Qt app's audio_thread: it owns the two renderers
// and the block stream, runs them from a JUCE audio device instead of
// PortAudio, and reads the same "/ratio", "/novelty", ... commands from the
// "8888" bus that the Qt GUI sent over OSC. The process thread ("8889") is
// the original, unchanged.
class Engine : private juce::AudioIODeviceCallback
{
public:
    Engine();
    ~Engine() override;

    // GUI -> engine, using the original message names
    template <class... Args> void audio (const char* path, Args... args) { local_bus::send ("8888", path, args...); }
    template <class... Args> void process (const char* path, Args... args) { local_bus::send ("8889", path, args...); }

    // recording writes one stereo wav, rather than the Qt app's -left/-right pair
    bool startRecording (const juce::File&);
    void stopRecording();
    bool isRecording() const { return activeWriter.load() != nullptr; }

    // peak output since the last call, for the meter
    float takeOutputPeak() { return outputPeak.exchange (0.0f); }
    // samples into the target, approximately; for the playhead only
    float getTargetTime() const { return leftRenderer->get_target_time(); }
    bool isPlaying() const { return leftRenderer->is_playing(); }

    juce::AudioDeviceManager deviceManager;

private:
    void audioDeviceIOCallbackWithContext (const float* const* in, int numIn,
                                           float* const* out, int numOut, int numSamples,
                                           const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart (juce::AudioIODevice*) override;
    void audioDeviceStopped() override {}

    void handleCommands();

    spiralcore::process_thread processThread;
    OSC_server osc { "8888" };
    spiralcore::renderer* leftRenderer = nullptr;
    spiralcore::renderer* rightRenderer = nullptr;
    spiralcore::block_stream* blockStream = nullptr;
    pthread_mutex_t* brainMutex = nullptr;

    bool stereoMode = false;
    juce::AudioBuffer<float> scratch;
    std::atomic<float> outputPeak { 0.0f };

    juce::TimeSliceThread writerThread { "samplebrain recorder" };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    juce::CriticalSection writerLock;
    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Engine)
};
