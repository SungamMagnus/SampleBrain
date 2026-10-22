#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Sungam
{
    // For everything the app doesn't draw itself — menus, tooltips, scroll
    // bars, and the stock JUCE audio settings panel — so none of it falls back
    // to JUCE's default look.
    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();

        juce::Font getPopupMenuFont() override;
        void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
        void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                                bool isSeparator, bool isActive, bool isHighlighted,
                                bool isTicked, bool hasSubMenu, const juce::String& text,
                                const juce::String& shortcutKeyText,
                                const juce::Drawable* icon, const juce::Colour* textColour) override;

        juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
        void drawTooltip (juce::Graphics&, const juce::String&, int width, int height) override;

        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getLabelFont (juce::Label&) override;
        juce::Font getTextButtonFont (juce::TextButton&, int) override;
        void drawComboBox (juce::Graphics&, int width, int height, bool down,
                           int bx, int by, int bw, int bh, juce::ComboBox&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                                   bool highlighted, bool down) override;
        void drawTickBox (juce::Graphics&, juce::Component&, float x, float y, float w, float h,
                          bool ticked, bool enabled, bool highlighted, bool down) override;

        void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height,
                            bool vertical, int thumbStart, int thumbSize, bool over, bool down) override;
        int getDefaultScrollbarWidth() override { return 8; }

        void drawDocumentWindowTitleBar (juce::DocumentWindow&, juce::Graphics&, int w, int h,
                                         int titleSpaceX, int titleSpaceW,
                                         const juce::Image* icon, bool drawTitleTextOnLeft) override;
    };
}
