#include "TargetView.h"
#include "Controls.h"

using namespace Sungam;

TargetView::TargetView (juce::AudioFormatManager& f)
    : formats (f), thumbnail (256, f, cache)
{
    thumbnail.addChangeListener (this);
}

TargetView::~TargetView()
{
    thumbnail.removeChangeListener (this);
}

void TargetView::setFile (const juce::File& f)
{
    file = f;
    blocksGenerated = false;
    totalSamples = 0.0;
    // the brain loads samples at their own rate, so its position is in file samples
    if (std::unique_ptr<juce::AudioFormatReader> r { formats.createReaderFor (f) })
        totalSamples = (double) r->lengthInSamples;
    thumbnail.setSource (new juce::FileInputSource (f));
    repaint();
}

void TargetView::setBlocks (int size, double ov, bool generated)
{
    blockSize = size;
    overlap = ov;
    blocksGenerated = generated;
    repaint();
}

void TargetView::setPlayhead (double samples)
{
    if (std::abs (samples - playhead) < 1.0) return;
    playhead = samples;
    repaint();
}

void TargetView::paint (juce::Graphics& g)
{
    auto b = getLocalBounds();
    g.setColour (stage);
    g.fillRect (b);

    auto wave = b.reduced (1, 4);

    if (thumbnail.getTotalLength() <= 0.0)
    {
        drawCaption (g, file.existsAsFile() ? "reading..." : "drop a target sound here, or load one",
                     b, ink38, juce::Justification::centred);
    }
    else
    {
        // block grid: a faint tick at every hop the target was chopped at
        if (blocksGenerated && totalSamples > 0)
        {
            const double hop = juce::jmax (1.0, blockSize * (1.0 - overlap));
            const double pxPerHop = wave.getWidth() * hop / totalSamples;
            if (pxPerHop >= 3.0)
            {
                g.setColour (ink13);
                for (double s = 0; s < totalSamples; s += hop)
                    g.fillRect ((float) wave.getX() + (float) (s / totalSamples * wave.getWidth()),
                                (float) wave.getY(), 1.0f, (float) wave.getHeight());
            }
        }

        g.setColour (ink13);
        g.fillRect (wave.getX(), wave.getCentreY(), wave.getWidth(), 1);
        g.setColour (coral.withAlpha (0.85f));
        thumbnail.drawChannels (g, wave, 0.0, thumbnail.getTotalLength(), 0.9f);

        if (playhead >= 0.0 && totalSamples > 0)
        {
            const float x = (float) wave.getX() + (float) juce::jlimit (0.0, 1.0, playhead / totalSamples) * (float) wave.getWidth();
            g.setColour (amber);
            g.fillRect (x - 0.5f, (float) b.getY(), 1.5f, (float) b.getHeight());
        }

        drawCaption (g, file.getFileName(), b.reduced (8, 4).removeFromTop (px (11.0f)), ink55,
                     juce::Justification::topLeft);
    }

    g.setColour (dragOver ? coral : ink22);
    g.drawRect (b.toFloat().reduced (0.5f), dragOver ? borderStrong : hairline);
}

void TargetView::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    repaint();
    if (! files.isEmpty() && onFileDropped)
        onFileDropped (juce::File (files[0]));
}
