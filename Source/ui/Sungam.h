#pragma once

#include <juce_graphics/juce_graphics.h>

// The Sungam design system, graphite edition — the same tokens Beygla uses
// (SungamKit.swift), so the two desktop apps sit side by side.
//
// Rules carried over from the system: colour is signal and never decoration,
// backgrounds are flat paper, type is one monospace face at every size, no
// shadows, no icons, square corners, and borders are hairlines drawn at
// partial ink opacity rather than in a separate grey.
//
// Panel tokens are sized for a plug-in window where a label sits around 9px;
// a desktop window scales the same ratios by `scale` rather than keeping a
// second type ramp.
namespace Sungam
{
    constexpr float scale = 1.25f;

    // -- Base: paper and ink trade places in graphite ------------------------
    static const juce::Colour paper { 0xff2e2d2b };
    static const juce::Colour ink   { 0xffece8df };
    // a step darker than the panels, behind waveforms only
    static const juce::Colour stage { 0xff201f1e };

    inline juce::Colour inkA (float a) { return ink.withAlpha (a); }
    static const juce::Colour ink04 = ink.withAlpha (0.04f);
    static const juce::Colour ink08 = ink.withAlpha (0.08f);
    static const juce::Colour ink13 = ink.withAlpha (0.13f);
    static const juce::Colour ink18 = ink.withAlpha (0.18f);
    static const juce::Colour ink22 = ink.withAlpha (0.22f);
    static const juce::Colour ink28 = ink.withAlpha (0.28f);
    static const juce::Colour ink38 = ink.withAlpha (0.38f);
    static const juce::Colour ink45 = ink.withAlpha (0.45f);
    static const juce::Colour ink55 = ink.withAlpha (0.55f);
    static const juce::Colour ink62 = ink.withAlpha (0.62f);
    static const juce::Colour ink85 = ink.withAlpha (0.85f);

    // -- Signal palette ------------------------------------------------------
    //
    // Samplebrain's assignment:
    //
    //   coral   the target — the sound being interpreted
    //   teal    the brain — its contents and how it searches
    //   steel   the output — mix, volume, the meter
    //   amber   live state only: playing, recording, processing
    //   lilac   trims subordinate to the control they hang off
    //   violet  reserved for modulation (none yet)
    static const juce::Colour coral  { 0xffed8159 };
    static const juce::Colour teal   { 0xff52b0a4 };
    static const juce::Colour steel  { 0xff6695c2 };
    static const juce::Colour violet { 0xff8c7de0 };
    static const juce::Colour amber  { 0xffd9a329 };
    static const juce::Colour lilac  { 0xffd3a7dc };

    // -- Type ----------------------------------------------------------------
    constexpr float text2xs = 7.5f;
    constexpr float textXs  = 8.0f;
    constexpr float textSm  = 8.5f;
    constexpr float textBase = 9.5f;
    constexpr float textMd  = 10.5f;
    constexpr float textLg  = 13.0f;
    constexpr float textXl  = 16.0f;

    constexpr float trackingWordmark = 0.28f;

    // sizes are token px; the scale is applied here, once
    inline juce::Font mono (float px, bool bold = false)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(),
                                              px * scale,
                                              bold ? juce::Font::bold : juce::Font::plain));
    }

    inline juce::Font tracked (float px, float tracking, bool bold = false)
    {
        return mono (px, bold).withExtraKerningFactor (tracking);
    }

    inline float stringWidth (const juce::Font& font, const juce::String& text)
    {
        return juce::GlyphArrangement::getStringWidth (font, text);
    }

    // -- Geometry ------------------------------------------------------------
    constexpr float hairline = 1.0f;
    constexpr float borderDefault = 1.2f;
    constexpr float borderStrong = 1.4f;

    inline int px (float tokenPx) { return juce::roundToInt (tokenPx * scale); }
}
