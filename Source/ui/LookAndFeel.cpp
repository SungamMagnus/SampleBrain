#include "LookAndFeel.h"
#include "Sungam.h"

namespace Sungam
{
    LookAndFeel::LookAndFeel()
    {
        setColourScheme ({ paper, paper, paper, ink22, ink, ink13, paper, teal, ink });

        setColour (juce::ResizableWindow::backgroundColourId, paper);
        setColour (juce::DocumentWindow::textColourId, ink62);
        setColour (juce::PopupMenu::backgroundColourId, paper);
        setColour (juce::PopupMenu::textColourId, ink85);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, teal);
        setColour (juce::PopupMenu::highlightedTextColourId, paper);
        setColour (juce::TooltipWindow::backgroundColourId, stage);
        setColour (juce::TooltipWindow::textColourId, ink85);
        setColour (juce::TooltipWindow::outlineColourId, ink22);
        setColour (juce::Label::textColourId, ink62);
        setColour (juce::ComboBox::backgroundColourId, paper);
        setColour (juce::ComboBox::textColourId, ink85);
        setColour (juce::ComboBox::outlineColourId, ink28);
        setColour (juce::ComboBox::arrowColourId, ink55);
        setColour (juce::TextButton::buttonColourId, paper);
        setColour (juce::TextButton::buttonOnColourId, teal);
        setColour (juce::TextButton::textColourOffId, ink62);
        setColour (juce::TextButton::textColourOnId, paper);
        setColour (juce::ToggleButton::textColourId, ink62);
        setColour (juce::ToggleButton::tickColourId, teal);
        setColour (juce::ToggleButton::tickDisabledColourId, ink28);
        setColour (juce::ListBox::backgroundColourId, stage);
        setColour (juce::ListBox::outlineColourId, ink22);
        setColour (juce::ListBox::textColourId, ink85);
        setColour (juce::TextEditor::backgroundColourId, paper);
        setColour (juce::TextEditor::textColourId, ink);
        setColour (juce::TextEditor::outlineColourId, ink28);
        setColour (juce::TextEditor::focusedOutlineColourId, teal);
        setColour (juce::Slider::thumbColourId, teal);
        setColour (juce::Slider::trackColourId, teal.withAlpha (0.55f));
        setColour (juce::Slider::backgroundColourId, ink13);
        setColour (juce::ScrollBar::thumbColourId, ink28);
        setColour (juce::AlertWindow::backgroundColourId, paper);
        setColour (juce::AlertWindow::textColourId, ink85);
        setColour (juce::AlertWindow::outlineColourId, ink22);

        setDefaultSansSerifTypefaceName (juce::Font::getDefaultMonospacedFontName());
    }

    juce::Font LookAndFeel::getPopupMenuFont() { return mono (textBase); }

    void LookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
    {
        g.fillAll (paper);
        g.setColour (ink28);
        g.drawRect (0, 0, w, h, 1);
    }

    void LookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                         bool isSeparator, bool isActive, bool isHighlighted,
                                         bool isTicked, bool, const juce::String& text,
                                         const juce::String&, const juce::Drawable*, const juce::Colour*)
    {
        if (isSeparator)
        {
            g.setColour (ink18);
            g.fillRect (area.withSizeKeepingCentre (area.getWidth() - 12, 1));
            return;
        }

        auto r = area.reduced (1, 0);
        if (isHighlighted && isActive)
        {
            g.setColour (teal);
            g.fillRect (r);
        }

        // a ticked item gets a small square, the system's lamp, not a check
        auto lamp = r.removeFromLeft (r.getHeight()).toFloat().withSizeKeepingCentre (7.0f, 7.0f);
        if (isTicked)
        {
            g.setColour (isHighlighted ? paper : teal);
            g.fillRect (lamp);
        }

        g.setColour (isHighlighted && isActive ? paper : (isActive ? ink85 : ink28));
        g.setFont (getPopupMenuFont());
        g.drawText (text, r.reduced (2, 0), juce::Justification::centredLeft, true);
    }

    juce::Rectangle<int> LookAndFeel::getTooltipBounds (const juce::String& tip, juce::Point<int> pos,
                                                        juce::Rectangle<int> parent)
    {
        const auto font = mono (textXs);
        const int w = juce::jmin (360, (int) stringWidth (font, tip) + 16);
        juce::AttributedString s;
        s.append (tip, font, ink85);
        juce::TextLayout tl;
        tl.createLayoutWithBalancedLineLengths (s, (float) w - 16.0f);
        const int h = (int) tl.getHeight() + 10;
        return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 24,
                                     pos.y > parent.getCentreY() ? pos.y - (h + 6) : pos.y + 6,
                                     w, h).constrainedWithin (parent);
    }

    void LookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& tip, int w, int h)
    {
        g.fillAll (stage);
        g.setColour (ink22);
        g.drawRect (0, 0, w, h, 1);
        juce::AttributedString s;
        s.append (tip, mono (textXs), ink85);
        s.setJustification (juce::Justification::centredLeft);
        juce::TextLayout tl;
        tl.createLayoutWithBalancedLineLengths (s, (float) w - 16.0f);
        tl.draw (g, { 8.0f, 5.0f, (float) w - 16.0f, (float) h - 10.0f });
    }

    juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox&) { return mono (textBase); }
    juce::Font LookAndFeel::getLabelFont (juce::Label&) { return mono (textBase); }
    juce::Font LookAndFeel::getTextButtonFont (juce::TextButton&, int) { return tracked (textXs, 0.02f); }

    void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
    {
        auto b = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h);
        g.setColour (ink04);
        g.fillRect (b);
        g.setColour (box.hasKeyboardFocus (true) ? teal : ink28);
        g.drawRect (b.reduced (0.5f), hairline);
        g.setColour (ink55);
        g.setFont (mono (textXs));
        g.drawText (juce::String::fromUTF8 ("\xe2\x96\xbe"), b.removeFromRight ((float) h), juce::Justification::centred);
    }

    void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                            bool highlighted, bool down)
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.6f);
        const bool on = b.getToggleState() || down;
        g.setColour (on ? teal : paper);
        g.fillRect (r);
        g.setColour (on ? teal : (highlighted ? ink45 : ink28));
        g.drawRect (r, borderDefault);
    }

    void LookAndFeel::drawTickBox (juce::Graphics& g, juce::Component&, float x, float y, float w, float h,
                                   bool ticked, bool enabled, bool, bool)
    {
        auto r = juce::Rectangle<float> (x, y, w, h).withSizeKeepingCentre (10.0f, 10.0f);
        if (ticked)
        {
            g.setColour (enabled ? teal : ink28);
            g.fillRect (r);
        }
        else
        {
            g.setColour (enabled ? ink45 : ink22);
            g.drawRect (r, hairline);
        }
    }

    void LookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h,
                                     bool vertical, int thumbStart, int thumbSize, bool over, bool down)
    {
        auto thumb = vertical ? juce::Rectangle<int> (x + 2, thumbStart, w - 4, thumbSize)
                              : juce::Rectangle<int> (thumbStart, y + 2, thumbSize, h - 4);
        g.setColour (down ? ink45 : (over ? ink38 : ink22));
        g.fillRect (thumb);
    }

    void LookAndFeel::drawDocumentWindowTitleBar (juce::DocumentWindow& win, juce::Graphics& g, int w, int h,
                                                  int titleX, int titleW, const juce::Image*, bool)
    {
        g.fillAll (paper);
        g.setColour (ink13);
        g.fillRect (0, h - 1, w, 1);
        g.setColour (ink62);
        g.setFont (tracked (textSm, 0.08f));
        g.drawText (win.getName().toUpperCase(), titleX, 0, titleW, h, juce::Justification::centred, true);
    }
}
