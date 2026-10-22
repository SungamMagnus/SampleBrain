#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "Sungam.h"

// The target sound as a waveform on the dark stage, with the renderer's
// position through it. Accepts a dropped file as the new target.
class TargetView : public juce::Component,
                   public juce::FileDragAndDropTarget,
                   private juce::ChangeListener
{
public:
    explicit TargetView (juce::AudioFormatManager&);
    ~TargetView() override;

    void setFile (const juce::File&);
    juce::File getFile() const { return file; }

    // the block grid: one tick per hop, once blocks have been generated
    void setBlocks (int blockSize, double overlap, bool generated);
    // samples into the target; negative hides the playhead
    void setPlayhead (double samples);

    std::function<void (const juce::File&)> onFileDropped;

    void paint (juce::Graphics&) override;

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }

    juce::AudioFormatManager& formats;
    juce::AudioThumbnailCache cache { 4 };
    juce::AudioThumbnail thumbnail;
    juce::File file;
    double totalSamples = 0.0;
    double playhead = -1.0;
    int blockSize = 1000;
    double overlap = 0.75;
    bool blocksGenerated = false;
    bool dragOver = false;
};
