#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Sungam.h"

// The Sungam control vocabulary, ported from Beygla's SungamKit: a panel frame
// with its title set into the rule, the NumBox as the compact value control,
// latches for toggles and actions, a selector, lamps, a fader and a meter.
namespace Sungam
{
    // A section drawn as the box its controls sit inside: a hairline rule in
    // the section colour, the title inset into the rule.
    class PanelFrame : public juce::Component
    {
    public:
        PanelFrame (juce::String title, juce::Colour colour);

        // the area inside the rule, for laying out children
        juce::Rectangle<int> getContentBounds() const;

        void paint (juce::Graphics&) override;

        juce::String title;
        juce::Colour colour;
    };

    // A number in a box. A faint fill shows where the value sits in its range.
    // Drag up/down to change it, shift for fine, double-click to type, the
    // wheel nudges by one step.
    class NumBox : public juce::Component,
                   public juce::SettableTooltipClient
    {
    public:
        NumBox (juce::String label, double min, double max, double value,
                juce::Colour colour, double step = 0.0);

        void setValue (double v, juce::NotificationType = juce::sendNotificationSync);
        double getValue() const { return value; }

        std::function<void (double)> onChange;
        std::function<juce::String (double)> format;

        // label beside the box rather than above it
        bool inlineLabel = false;

        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

        static int preferredHeight();

    private:
        juce::Rectangle<int> boxBounds() const;
        double norm() const;
        double snap (double v) const;
        void commitEdit();

        juce::String label;
        double minimum, maximum, value, step;
        juce::Colour colour;
        double dragStartNorm = 0.0;
        bool dragging = false;
        std::unique_ptr<juce::TextEditor> editor;
    };

    // A toggle: filled in its colour when on, outlined when off. Also serves as
    // a momentary action (setClickingTogglesState (false)) and, filled, as the
    // one primary action in a section.
    class Latch : public juce::Button
    {
    public:
        Latch (juce::String label, juce::Colour colour, bool toggles = true);

        // a momentary latch drawn filled — the section's primary action
        bool filled = false;

        void paintButton (juce::Graphics&, bool highlighted, bool down) override;

        juce::Colour colour;
    };

    // A choice made rarely enough not to spend a row: one latch showing the
    // current option, a click opens a plain menu.
    class Dropdown : public juce::Component,
                     public juce::SettableTooltipClient
    {
    public:
        Dropdown (juce::String label, juce::StringArray options, int selected, juce::Colour colour);

        void setSelected (int index, juce::NotificationType = juce::sendNotificationSync);
        int getSelected() const { return selected; }

        std::function<void (int)> onChange;

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;

    private:
        juce::String label;
        juce::StringArray options;
        int selected;
        juce::Colour colour;
    };

    // N positions, every one labelled.
    class Selector : public juce::Component,
                     public juce::SettableTooltipClient
    {
    public:
        Selector (juce::StringArray options, int selected, juce::Colour colour);

        void setSelected (int index, juce::NotificationType = juce::sendNotificationSync);
        int getSelected() const { return selected; }

        std::function<void (int)> onChange;

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;

    private:
        juce::StringArray options;
        int selected;
        juce::Colour colour;
    };

    // Lit when something is happening, outlined when not. Always a square.
    class Lamp : public juce::Component
    {
    public:
        explicit Lamp (juce::Colour c = ink) : colour (c) {}
        void setOn (bool shouldBeOn) { if (on != shouldBeOn) { on = shouldBeOn; repaint(); } }
        void setColour (juce::Colour c) { if (colour != c) { colour = c; repaint(); } }
        void paint (juce::Graphics&) override;

    private:
        bool on = false;
        juce::Colour colour;
    };

    // A flat track with a small handle, horizontal. Optional captions at each
    // end name what the two extremes mean ("brain" / "target").
    class Fader : public juce::Component,
                  public juce::SettableTooltipClient
    {
    public:
        Fader (juce::String label, double min, double max, double value, juce::Colour colour);

        void setEnds (juce::String left, juce::String right) { leftEnd = left; rightEnd = right; repaint(); }
        void setValue (double v, juce::NotificationType = juce::sendNotificationSync);
        double getValue() const { return value; }

        std::function<void (double)> onChange;
        std::function<juce::String (double)> format;

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;

        double defaultValue;
        static int preferredHeight();

    private:
        juce::Rectangle<float> trackBounds() const;
        void setFromX (float x);

        juce::String label, leftEnd, rightEnd;
        double minimum, maximum, value;
        juce::Colour colour;
    };

    // A level meter in discrete segments; the last segment reads amber.
    class SegmentMeter : public juce::Component
    {
    public:
        explicit SegmentMeter (juce::Colour c = steel, int n = 12) : colour (c), segments (n) {}
        void setLevel (float l);
        void paint (juce::Graphics&) override;

    private:
        juce::Colour colour;
        int segments;
        float level = 0.0f;
    };

    // Letter-spaced bold caps, for the product name and nothing fancier.
    void drawWordmark (juce::Graphics&, const juce::String& text, juce::Rectangle<int> area,
                       float size, juce::Colour colour,
                       juce::Justification = juce::Justification::centredLeft);

    // A tracked caption in secondary ink.
    void drawCaption (juce::Graphics&, const juce::String& text, juce::Rectangle<int> area,
                      juce::Colour colour = ink55,
                      juce::Justification = juce::Justification::centredLeft);
}
