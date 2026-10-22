#include "Controls.h"

namespace Sungam
{
    namespace
    {
        const int captionH = px (11.0f);
        const int captionGap = 3;
        const int boxH = px (19.0f);

        juce::String defaultFormat (double v) { return juce::String (v, 2); }
    }

    void drawWordmark (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                       float size, juce::Colour colour, juce::Justification j)
    {
        g.setColour (colour);
        g.setFont (tracked (size, trackingWordmark, true));
        g.drawText (text.toUpperCase(), area, j, false);
    }

    void drawCaption (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                      juce::Colour colour, juce::Justification j)
    {
        g.setColour (colour);
        g.setFont (tracked (text2xs, 0.06f));
        g.drawText (text.toUpperCase(), area, j, true);
    }

    //==========================================================================
    PanelFrame::PanelFrame (juce::String t, juce::Colour c) : title (std::move (t)), colour (c)
    {
        setInterceptsMouseClicks (false, true);
    }

    juce::Rectangle<int> PanelFrame::getContentBounds() const
    {
        return getLocalBounds().withTrimmedTop (px (6.0f)).reduced (px (10.0f), 0)
                               .withTrimmedTop (px (12.0f)).withTrimmedBottom (px (10.0f));
    }

    void PanelFrame::paint (juce::Graphics& g)
    {
        auto font = tracked (textSm, 0.04f);
        const float titleH = font.getHeight();
        auto rule = getLocalBounds().toFloat().withTrimmedTop (titleH * 0.5f).reduced (0.5f);

        g.setColour (colour);
        g.drawRect (rule, hairline);

        // the title sits in a gap cut from the top rule
        const auto text = title.toUpperCase();
        const float w = stringWidth (font, text) + 12.0f;
        juce::Rectangle<float> gap (rule.getX() + px (14.0f), 0.0f, w, titleH);
        g.setColour (paper);
        g.fillRect (gap);
        g.setColour (colour);
        g.setFont (font);
        g.drawText (text, gap, juce::Justification::centred, false);
    }

    //==========================================================================
    NumBox::NumBox (juce::String l, double mn, double mx, double v, juce::Colour c, double s)
        : label (std::move (l)), minimum (mn), maximum (mx), value (v), step (s), colour (c)
    {
        format = defaultFormat;
        setTooltip ("Drag up/down to change, shift for fine, double-click to type");
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    }

    int NumBox::preferredHeight() { return captionH + captionGap + boxH; }

    juce::Rectangle<int> NumBox::boxBounds() const
    {
        auto r = getLocalBounds();
        if (inlineLabel)
            return r.removeFromRight (px (62.0f)).withSizeKeepingCentre (px (62.0f), boxH);
        return r.withTrimmedTop (captionH + captionGap).withHeight (boxH);
    }

    double NumBox::norm() const
    {
        if (maximum <= minimum) return 0.0;
        return juce::jlimit (0.0, 1.0, (value - minimum) / (maximum - minimum));
    }

    double NumBox::snap (double v) const
    {
        if (step > 0.0) v = minimum + std::round ((v - minimum) / step) * step;
        return juce::jlimit (minimum, maximum, v);
    }

    void NumBox::setValue (double v, juce::NotificationType n)
    {
        v = juce::jlimit (minimum, maximum, v);
        if (v == value) return;
        value = v;
        repaint();
        if (n != juce::dontSendNotification && onChange) onChange (value);
    }

    void NumBox::paint (juce::Graphics& g)
    {
        auto box = boxBounds();

        if (inlineLabel)
            drawCaption (g, label, getLocalBounds().withRight (box.getX() - 6));
        else
            drawCaption (g, label, getLocalBounds().withHeight (captionH));

        auto b = box.toFloat();
        g.setColour (ink04);
        g.fillRect (b);
        g.setColour (colour.withAlpha (0.22f));
        g.fillRect (b.withWidth (b.getWidth() * (float) norm()));
        g.setColour (editor != nullptr ? colour : ink22);
        g.drawRect (b.reduced (0.5f), hairline);

        if (editor == nullptr)
        {
            g.setColour (colour);
            g.setFont (mono (textBase, true));
            g.drawText (format (value), box.reduced (6, 0), juce::Justification::centredLeft, true);
        }
    }

    void NumBox::resized()
    {
        if (editor != nullptr) editor->setBounds (boxBounds().reduced (2));
    }

    void NumBox::mouseDown (const juce::MouseEvent&)
    {
        dragStartNorm = norm();
        dragging = false;
    }

    void NumBox::mouseDrag (const juce::MouseEvent& e)
    {
        if (editor != nullptr) return;
        if (! dragging && e.getDistanceFromDragStart() < 2) return;
        dragging = true;
        const double gain = e.mods.isShiftDown() ? 0.001 : 0.005;
        const double n = juce::jlimit (0.0, 1.0, dragStartNorm - e.getDistanceFromDragStartY() * gain);
        setValue (snap (minimum + n * (maximum - minimum)));
    }

    void NumBox::mouseUp (const juce::MouseEvent&) { dragging = false; }

    void NumBox::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
    {
        if (editor != nullptr) return;
        const double unit = step > 0.0 ? step : (maximum - minimum) / 100.0;
        const double dir = (w.deltaY != 0.0f ? w.deltaY : -w.deltaX) > 0 ? 1.0 : -1.0;
        setValue (snap (value + dir * unit * (e.mods.isShiftDown() ? 1.0 : 1.0)));
    }

    void NumBox::mouseDoubleClick (const juce::MouseEvent&)
    {
        editor = std::make_unique<juce::TextEditor>();
        editor->setFont (mono (textBase, true));
        editor->setColour (juce::TextEditor::backgroundColourId, paper);
        editor->setColour (juce::TextEditor::textColourId, ink);
        editor->setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        editor->setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        editor->setColour (juce::TextEditor::highlightColourId, colour.withAlpha (0.35f));
        editor->setColour (juce::CaretComponent::caretColourId, colour);
        editor->setBorder ({});
        editor->setIndents (4, (boxH - 4 - (int) editor->getFont().getHeight()) / 2);
        editor->setText (juce::String (value, step >= 1.0 ? 0 : 3)
                             .trimCharactersAtEnd ("0").trimCharactersAtEnd ("."), false);
        editor->onReturnKey = [this] { commitEdit(); };
        editor->onFocusLost = [this] { commitEdit(); };
        editor->onEscapeKey = [this] { juce::MessageManager::callAsync ([sp = juce::Component::SafePointer<NumBox> (this)]
                                       { if (sp != nullptr) { sp->editor.reset(); sp->repaint(); } }); };
        addAndMakeVisible (*editor);
        resized();
        editor->selectAll();
        editor->grabKeyboardFocus();
        repaint();
    }

    void NumBox::commitEdit()
    {
        if (editor == nullptr) return;
        const auto text = editor->getText().retainCharacters ("0123456789.-+");
        // destroy the editor after this callback returns, not inside it
        juce::MessageManager::callAsync ([sp = juce::Component::SafePointer<NumBox> (this)]
                                         { if (sp != nullptr) { sp->editor.reset(); sp->repaint(); } });
        if (text.isNotEmpty())
            setValue (snap (text.getDoubleValue()));
    }

    //==========================================================================
    Latch::Latch (juce::String label, juce::Colour c, bool toggles)
        : juce::Button (std::move (label)), colour (c)
    {
        setClickingTogglesState (toggles);
    }

    void Latch::paintButton (juce::Graphics& g, bool highlighted, bool down)
    {
        auto b = getLocalBounds().toFloat().reduced (0.6f);
        const bool lit = getToggleState() || filled || down;
        const bool enabled = isEnabled();

        g.setColour (lit ? (down && ! getToggleState() ? colour.darker (0.25f) : colour) : paper);
        g.fillRect (b);
        g.setColour (lit ? colour : (enabled ? (highlighted ? ink45 : ink28) : ink13));
        g.drawRect (b, borderDefault);

        g.setColour (lit ? paper : (enabled ? ink62 : ink28));
        g.setFont (tracked (textXs, 0.02f));
        g.drawText (getButtonText().toUpperCase(), getLocalBounds().reduced (6, 0),
                    juce::Justification::centred, true);
    }

    //==========================================================================
    Dropdown::Dropdown (juce::String l, juce::StringArray o, int s, juce::Colour c)
        : label (std::move (l)), options (std::move (o)), selected (s), colour (c)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void Dropdown::setSelected (int index, juce::NotificationType n)
    {
        index = juce::jlimit (0, options.size() - 1, index);
        if (index == selected) return;
        selected = index;
        repaint();
        if (n != juce::dontSendNotification && onChange) onChange (selected);
    }

    void Dropdown::paint (juce::Graphics& g)
    {
        auto b = getLocalBounds();
        drawCaption (g, label, b.removeFromTop (captionH));
        auto box = b.withTrimmedTop (captionGap).withHeight (boxH).toFloat();
        g.setColour (colour);
        g.fillRect (box);
        g.setColour (paper);
        g.setFont (tracked (textXs, 0.02f));
        g.drawText (options[selected].toUpperCase() + juce::String::fromUTF8 ("  \xe2\x96\xbe"),
                    box.reduced (8.0f, 0.0f), juce::Justification::centredLeft, true);
    }

    void Dropdown::mouseDown (const juce::MouseEvent&)
    {
        juce::PopupMenu m;
        for (int i = 0; i < options.size(); ++i)
            m.addItem (i + 1, options[i], true, i == selected);

        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                         [sp = juce::Component::SafePointer<Dropdown> (this)] (int r)
                         { if (sp != nullptr && r > 0) sp->setSelected (r - 1); });
    }

    //==========================================================================
    Selector::Selector (juce::StringArray o, int s, juce::Colour c)
        : options (std::move (o)), selected (s), colour (c)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void Selector::setSelected (int index, juce::NotificationType n)
    {
        index = juce::jlimit (0, options.size() - 1, index);
        if (index == selected) return;
        selected = index;
        repaint();
        if (n != juce::dontSendNotification && onChange) onChange (selected);
    }

    void Selector::paint (juce::Graphics& g)
    {
        auto b = getLocalBounds().toFloat();
        const float w = b.getWidth() / (float) options.size();
        g.setFont (tracked (textXs, 0.02f));

        for (int i = 0; i < options.size(); ++i)
        {
            auto cell = juce::Rectangle<float> (b.getX() + w * (float) i, b.getY(), w, b.getHeight());
            const bool active = i == selected;
            if (active)
            {
                g.setColour (colour);
                g.fillRect (cell);
            }
            else if (i > 0 && i - 1 != selected)
            {
                g.setColour (ink18);
                g.fillRect (cell.getX(), cell.getY(), hairline, cell.getHeight());
            }
            g.setColour (active ? paper : ink62);
            g.drawText (options[i].toUpperCase(), cell.reduced (2.0f, 0.0f), juce::Justification::centred, true);
        }
        g.setColour (ink28);
        g.drawRect (b.reduced (0.5f), hairline);
    }

    void Selector::mouseDown (const juce::MouseEvent& e)
    {
        const int i = (int) (e.position.x / ((float) getWidth() / (float) options.size()));
        setSelected (i);
    }

    //==========================================================================
    void Lamp::paint (juce::Graphics& g)
    {
        const float s = (float) juce::jmin (getWidth(), getHeight(), px (9.0f));
        auto b = getLocalBounds().toFloat().withSizeKeepingCentre (s, s);
        if (on)
        {
            g.setColour (colour);
            g.fillRect (b);
        }
        else
        {
            g.setColour (colour.withAlpha (0.45f));
            g.drawRect (b.reduced (0.5f), hairline);
        }
    }

    //==========================================================================
    Fader::Fader (juce::String l, double mn, double mx, double v, juce::Colour c)
        : defaultValue (v), label (std::move (l)), minimum (mn), maximum (mx), value (v), colour (c)
    {
        format = defaultFormat;
        setTooltip ("Drag to set, double-click to reset");
    }

    int Fader::preferredHeight() { return captionH + captionGap + px (16.0f); }

    juce::Rectangle<float> Fader::trackBounds() const
    {
        return getLocalBounds().toFloat().withTrimmedTop ((float) (captionH + captionGap)).withHeight ((float) px (16.0f));
    }

    void Fader::setValue (double v, juce::NotificationType n)
    {
        v = juce::jlimit (minimum, maximum, v);
        if (v == value) return;
        value = v;
        repaint();
        if (n != juce::dontSendNotification && onChange) onChange (value);
    }

    void Fader::paint (juce::Graphics& g)
    {
        auto cap = getLocalBounds().withHeight (captionH);
        drawCaption (g, label, cap);
        g.setColour (colour);
        g.setFont (mono (textXs, true));
        g.drawText (format (value), cap, juce::Justification::centredRight, false);

        auto t = trackBounds();
        const float handleW = (float) px (14.0f);
        const float trackH = (float) px (6.0f);
        const float n = (float) ((value - minimum) / (maximum - minimum));

        auto track = t.withSizeKeepingCentre (t.getWidth(), trackH);
        g.setColour (ink13);
        g.fillRoundedRectangle (track, trackH * 0.5f);
        g.setColour (colour.withAlpha (0.55f));
        g.fillRoundedRectangle (track.withWidth (juce::jmax (handleW * 0.5f, track.getWidth() * n)), trackH * 0.5f);

        // end captions ride in the track, under the handle
        if (leftEnd.isNotEmpty() || rightEnd.isNotEmpty())
        {
            g.setFont (tracked (text2xs, 0.06f));
            g.setColour (ink38);
            auto ends = getLocalBounds().withTrimmedTop (captionH + captionGap + px (16.0f));
            g.drawText (leftEnd.toUpperCase(), ends, juce::Justification::topLeft, false);
            g.drawText (rightEnd.toUpperCase(), ends, juce::Justification::topRight, false);
        }

        // the handle: one of the system's two rounded corners
        const float x = juce::jlimit (t.getX(), t.getRight() - handleW, t.getX() + t.getWidth() * n - handleW * 0.5f);
        auto handle = juce::Rectangle<float> (x, t.getY(), handleW, t.getHeight());
        g.setColour (paper);
        g.fillRoundedRectangle (handle, 3.0f);
        g.setColour (colour);
        g.drawRoundedRectangle (handle.reduced (0.6f), 3.0f, borderDefault);
    }

    void Fader::setFromX (float x)
    {
        auto t = trackBounds();
        const double n = juce::jlimit (0.0, 1.0, (double) ((x - t.getX()) / t.getWidth()));
        setValue (minimum + n * (maximum - minimum));
    }

    void Fader::mouseDown (const juce::MouseEvent& e) { setFromX (e.position.x); }
    void Fader::mouseDrag (const juce::MouseEvent& e) { setFromX (e.position.x); }
    void Fader::mouseDoubleClick (const juce::MouseEvent&) { setValue (defaultValue); }

    //==========================================================================
    void SegmentMeter::setLevel (float l)
    {
        l = juce::jlimit (0.0f, 1.0f, l);
        if (std::abs (l - level) < 0.001f) return;
        level = l;
        repaint();
    }

    void SegmentMeter::paint (juce::Graphics& g)
    {
        const int lit = juce::roundToInt (level * (float) segments);
        const float gap = 3.0f;
        const float w = ((float) getWidth() - gap * (float) (segments - 1)) / (float) segments;
        const float h = (float) px (6.0f);
        const float y = ((float) getHeight() - h) * 0.5f;

        for (int i = 0; i < segments; ++i)
        {
            auto seg = juce::Rectangle<float> ((w + gap) * (float) i, y, w, h);
            if (i < lit)
            {
                g.setColour (i == segments - 1 ? amber : colour);
                g.fillRect (seg);
            }
            else
            {
                g.setColour (ink22);
                g.drawRect (seg.reduced (0.5f), hairline);
            }
        }
    }
}
