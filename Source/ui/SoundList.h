#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Sungam.h"

// The brain's contents: one row per sound, its enable lamp, its name, and a
// strip on the right showing where the brain has got to with it. Sounds can
// be dropped straight onto the list.
class SoundList : public juce::Component,
                  public juce::FileDragAndDropTarget
{
public:
    // what the brain last reported for a sound
    enum class State { waiting, chopping, readyA, readyB };

    struct Item
    {
        std::string path;
        bool enabled = true;
        State state = State::waiting;
    };

    SoundList();

    void add (const std::string& path, bool enabled);
    void remove (const std::string& path);
    void clear();
    void setAllEnabled (bool);
    // the Qt app's colour names: lightgrey, yellow, lightblue, pink
    void setStateFromColour (const std::string& path, const std::string& colour);

    const std::vector<Item>& getItems() const { return items; }

    std::function<void (const std::string&, bool)> onEnable;
    std::function<void (const std::string&)> onDelete;
    std::function<void (const juce::StringArray&)> onFilesDropped;

    void resized() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;

    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    class Rows : public juce::Component,
                 public juce::SettableTooltipClient
    {
    public:
        explicit Rows (SoundList& o) : owner (o) {}
        void paint (juce::Graphics&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseDown (const juce::MouseEvent&) override;

        SoundList& owner;
        int hover = -1;
    };

    void refresh();
    static int rowHeight();

    std::vector<Item> items;
    Rows rows { *this };
    juce::Viewport viewport;
    bool dragOver = false;
};
