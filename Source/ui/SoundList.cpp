#include "SoundList.h"
#include "Controls.h"

using namespace Sungam;

int SoundList::rowHeight() { return px (17.0f); }

SoundList::SoundList()
{
    viewport.setViewedComponent (&rows, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (8);
    addAndMakeVisible (viewport);
}

void SoundList::add (const std::string& path, bool enabled)
{
    for (auto& i : items)
        if (i.path == path) { i.enabled = enabled; refresh(); return; }
    items.push_back ({ path, enabled, State::waiting });
    refresh();
}

void SoundList::remove (const std::string& path)
{
    items.erase (std::remove_if (items.begin(), items.end(), [&] (const Item& i) { return i.path == path; }), items.end());
    refresh();
}

void SoundList::clear()
{
    items.clear();
    refresh();
}

void SoundList::setAllEnabled (bool e)
{
    for (auto& i : items) i.enabled = e;
    rows.repaint();
}

void SoundList::setStateFromColour (const std::string& path, const std::string& colour)
{
    State s = State::waiting;
    if (colour == "yellow") s = State::chopping;
    else if (colour == "lightblue") s = State::readyA;
    else if (colour == "pink") s = State::readyB;

    for (auto& i : items)
        if (i.path == path) i.state = s;
    rows.repaint();
}

void SoundList::refresh()
{
    rows.setSize (viewport.getMaximumVisibleWidth(), juce::jmax (viewport.getHeight(), (int) items.size() * rowHeight()));
    rows.repaint();
    repaint();
}

void SoundList::resized()
{
    viewport.setBounds (getLocalBounds().reduced (1));
    refresh();
}

void SoundList::paint (juce::Graphics& g)
{
    g.setColour (stage);
    g.fillRect (getLocalBounds());
}

void SoundList::paintOverChildren (juce::Graphics& g)
{
    g.setColour (dragOver ? teal : ink22);
    g.drawRect (getLocalBounds().toFloat().reduced (0.5f), dragOver ? borderStrong : hairline);

    if (items.empty())
        drawCaption (g, "drop sounds here, or add a file or folder", getLocalBounds(), ink38,
                     juce::Justification::centred);
}

void SoundList::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    repaint();
    if (onFilesDropped) onFilesDropped (files);
}

//==============================================================================
void SoundList::Rows::paint (juce::Graphics& g)
{
    const int h = rowHeight();
    const int w = getWidth();
    auto font = mono (textSm);

    for (int n = 0; n < (int) owner.items.size(); ++n)
    {
        const auto& item = owner.items[(size_t) n];
        auto r = juce::Rectangle<int> (0, n * h, w, h);

        if (n == hover)
        {
            g.setColour (ink04);
            g.fillRect (r);
        }

        // the processing strip: where the brain is with this sound. Ready
        // sounds alternate shade, as the Qt app's colours did, so neighbouring
        // sounds' blocks read as separate runs.
        juce::Colour strip = ink13;
        if (item.state == State::chopping) strip = amber;
        else if (item.state == State::readyA) strip = teal;
        else if (item.state == State::readyB) strip = teal.darker (0.45f);
        g.setColour (strip);
        g.fillRect (r.removeFromRight (4).reduced (0, 1));

        // enable lamp
        auto lamp = r.removeFromLeft (h).toFloat().withSizeKeepingCentre (8.0f, 8.0f);
        if (item.enabled)
        {
            g.setColour (teal);
            g.fillRect (lamp);
        }
        else
        {
            g.setColour (ink38);
            g.drawRect (lamp.reduced (0.5f), hairline);
        }

        // delete, shown on hover only
        auto del = r.removeFromRight (px (30.0f));
        if (n == hover)
        {
            g.setFont (tracked (text2xs, 0.06f));
            g.setColour (coral);
            g.drawText ("DEL", del, juce::Justification::centred, false);
        }

        const auto file = juce::File (juce::String::fromUTF8 (item.path.c_str()));
        g.setFont (font);
        g.setColour (item.enabled ? ink85 : ink38);
        g.drawText (file.getFileName(), r.withTrimmedLeft (2), juce::Justification::centredLeft, true);

        g.setColour (ink08);
        g.fillRect (0, (n + 1) * h - 1, w, 1);
    }
}

void SoundList::Rows::mouseMove (const juce::MouseEvent& e)
{
    const int i = e.y / rowHeight();
    const int next = i < (int) owner.items.size() ? i : -1;
    if (next != hover) { hover = next; repaint(); }
    if (hover >= 0)
        setTooltip (juce::String::fromUTF8 (owner.items[(size_t) hover].path.c_str()));
}

void SoundList::Rows::mouseExit (const juce::MouseEvent&)
{
    hover = -1;
    repaint();
}

void SoundList::Rows::mouseDown (const juce::MouseEvent& e)
{
    const int i = e.y / rowHeight();
    if (i < 0 || i >= (int) owner.items.size()) return;

    auto& item = owner.items[(size_t) i];
    if (e.x >= getWidth() - 4 - px (30.0f) && e.x < getWidth() - 4)
    {
        const auto path = item.path;
        owner.remove (path);
        if (owner.onDelete) owner.onDelete (path);
        hover = -1;
        return;
    }

    item.enabled = ! item.enabled;
    repaint();
    if (owner.onEnable) owner.onEnable (item.path, item.enabled);
}
