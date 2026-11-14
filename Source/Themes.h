#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// ============================================================================
//  Storybook theme system: each theme is a palette + an illustrated
//  background pattern + a knob style. HazeLookAndFeel renders every control
//  from the active theme. All artwork is procedural (no bitmaps).
// ============================================================================

struct Theme
{
    enum class Pattern { BrickRoad, EmeraldLattice, Cyclone, Poppies, Halftone, Stars, RainbowArcs };
    enum class Knob    { ChickenHead, NeonRing, Bakelite, SoftDot, Gem };

    juce::String name;
    juce::Colour bgTop, bgBottom, patternA, patternB;
    juce::Colour panel, panelEdge, title, text, dimText;
    juce::Colour accent, accent2, knobBody, knobPointer, track;
    Pattern pattern;
    Knob knob;
    float corner;
    bool uppercaseTitles;
};

inline const std::vector<Theme>& getThemes()
{
    using C = juce::Colour;
    static const std::vector<Theme> themes =
    {
        { "Yellow Brick Road",
          C (0xff3a2610), C (0xff6b4316), C (0xffd9a62e), C (0xffa8761c),
          C (0xd8301e0c), C (0xffe9bf4a), C (0xffffd34d), C (0xfff7ead0), C (0xffc4a679),
          C (0xffffc83a), C (0xff3fae5a), C (0xff22160a), C (0xffffe08a), C (0xff5a4020),
          Theme::Pattern::BrickRoad, Theme::Knob::ChickenHead, 10.0f, true },

        { "Emerald City",
          C (0xff021a10), C (0xff06402a), C (0x5531e08a), C (0x3300ffaa),
          C (0xcc03170e), C (0xff2fe39a), C (0xff6dffc2), C (0xffe6fff3), C (0xff7fb9a0),
          C (0xff2fe39a), C (0xffd8ff5a), C (0xff0a2a1c), C (0xffb8ffe0), C (0xff154a33),
          Theme::Pattern::EmeraldLattice, Theme::Knob::Gem, 12.0f, true },

        { "Twister",
          C (0xff2b2f3a), C (0xff5c5a52), C (0x40d8d2c0), C (0x26201c18),
          C (0xcc1d2028), C (0xffb9b2a0), C (0xfff0e9d6), C (0xffeceadf), C (0xff9a978c),
          C (0xffe0c27a), C (0xff8fb3d9), C (0xff15171c), C (0xfff0e9d6), C (0xff3a3d46),
          Theme::Pattern::Cyclone, Theme::Knob::NeonRing, 8.0f, true },

        { "Poppy Field",
          C (0xff4a0d14), C (0xff1f3a1a), C (0xccd8242f), C (0x99161f10),
          C (0xcc2a0a0e), C (0xffff6b5e), C (0xffffb3a0), C (0xfffff0ea), C (0xffd49a90),
          C (0xffff4d4d), C (0xff7ecf5a), C (0xff1c0608), C (0xffffd0c0), C (0xff5a2028),
          Theme::Pattern::Poppies, Theme::Knob::SoftDot, 16.0f, false },

        { "Kansas Storybook",
          C (0xffeee3c8), C (0xffd6c39c), C (0x33503c20), C (0x22503c20),
          C (0xccf7efdc), C (0xff4a3a24), C (0xff6a4a28), C (0xff2a2016), C (0xff7a6a52),
          C (0xff8a5a2a), C (0xff4a6a6a), C (0xff3a2c1c), C (0xfff7efdc), C (0xffc4b28e),
          Theme::Pattern::Halftone, Theme::Knob::Bakelite, 3.0f, true },

        { "Wicked West",
          C (0xff0a0612), C (0xff1c0a2a), C (0xaa9cff3a), C (0x66ffffff),
          C (0xcc0e0816), C (0xff7dff3a), C (0xffa6ff4d), C (0xffefe6ff), C (0xff8a7aa0),
          C (0xff7dff3a), C (0xffb04dff), C (0xff160c22), C (0xffa6ff4d), C (0xff2e1c42),
          Theme::Pattern::Stars, Theme::Knob::NeonRing, 6.0f, true },

        { "Rainbow's End",
          C (0xffcfe8ff), C (0xfffde2f0), C (0x00000000), C (0x00000000),
          C (0xa8ffffff), C (0xffffffff), C (0xff7a4fd6), C (0xff3e3360), C (0xff8a80aa),
          C (0xffff7aa8), C (0xff4fc3f7), C (0xfffdfbff), C (0xff7a4fd6), C (0xffe6def8),
          Theme::Pattern::RainbowArcs, Theme::Knob::SoftDot, 18.0f, false },
    };
    return themes;
}

// ---------------------------------------------------------------------------
class HazeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HazeLookAndFeel() { setTheme (0); }
    const Theme& theme() const { return getThemes()[(size_t) themeIndex]; }
    int getThemeIndex() const { return themeIndex; }

    void setTheme (int index)
    {
        themeIndex = juce::jlimit (0, (int) getThemes().size() - 1, index);
        const auto& t = theme();
        const auto onBody = t.knobBody.contrasting (1.0f).interpolatedWith (t.knobBody.getPerceivedBrightness() > 0.5f ? t.text : t.knobPointer, 0.25f);
        const auto blackKey = t.knobBody.getPerceivedBrightness() > 0.5f ? t.text : t.knobBody;
        setColour (juce::Label::textColourId, t.text);
        setColour (juce::ComboBox::backgroundColourId, t.knobBody);
        setColour (juce::ComboBox::textColourId, onBody);
        setColour (juce::ComboBox::outlineColourId, t.panelEdge.withAlpha (0.6f));
        setColour (juce::ComboBox::arrowColourId, t.accent);
        setColour (juce::PopupMenu::backgroundColourId, t.knobBody.withAlpha (0.98f));
        setColour (juce::PopupMenu::textColourId, onBody);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, t.accent);
        setColour (juce::PopupMenu::highlightedTextColourId, t.knobBody);
        setColour (juce::PopupMenu::headerTextColourId, t.title);
        setColour (juce::TextButton::buttonColourId, t.knobBody);
        setColour (juce::TextButton::buttonOnColourId, t.accent);
        setColour (juce::TextButton::textColourOffId, onBody);
        setColour (juce::TextButton::textColourOnId, t.knobBody);
        setColour (juce::Slider::textBoxTextColourId, onBody);
        setColour (juce::Label::textWhenEditingColourId, onBody);
        setColour (juce::BubbleComponent::backgroundColourId, t.knobBody);
        setColour (juce::BubbleComponent::outlineColourId, t.accent);
        setColour (juce::TooltipWindow::backgroundColourId, t.knobBody);
        setColour (juce::TooltipWindow::textColourId, onBody);
        setColour (juce::AlertWindow::backgroundColourId, t.knobBody);
        setColour (juce::AlertWindow::textColourId, onBody);
        setColour (juce::TextEditor::backgroundColourId, t.panel.withAlpha (1.0f));
        setColour (juce::TextEditor::textColourId, t.text);
        setColour (juce::TextEditor::outlineColourId, t.panelEdge);
        setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, t.accent.withAlpha (0.7f));
        setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, t.accent2.withAlpha (0.35f));
        setColour (juce::MidiKeyboardComponent::whiteNoteColourId, t.knobPointer.interpolatedWith (juce::Colours::white, 0.85f));
        setColour (juce::MidiKeyboardComponent::blackNoteColourId, blackKey);
        setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, t.knobBody.withAlpha (0.5f));
        setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
        setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, t.knobBody);
        setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, t.accent);
    }

    juce::Font titleFont (float h) const
    {
        return juce::Font (juce::FontOptions (h, juce::Font::bold)).withExtraKerningFactor (theme().uppercaseTitles ? 0.12f : 0.03f);
    }


    // ------------------------------------------------------------ background
    void drawBackground (juce::Graphics& g, juce::Rectangle<float> r) const
    {
        const auto& t = theme();
        g.setGradientFill (juce::ColourGradient (t.bgTop, r.getTopLeft(), t.bgBottom, r.getBottomRight(), false));
        g.fillRect (r);
        const auto c = r.getCentre();
        const float maxR = std::hypot (r.getWidth(), r.getHeight());
        const float twoPi = juce::MathConstants<float>::twoPi;
        juce::Random rnd (1900);

        switch (t.pattern)
        {
            case Theme::Pattern::BrickRoad:
            {   // a brick road curling in from the bottom edge toward a far-off glow on the horizon
                const auto vanish = juce::Point<float> (r.getCentreX() + r.getWidth() * 0.18f, r.getY() + r.getHeight() * 0.28f);
                g.setGradientFill (juce::ColourGradient (t.accent2.withAlpha (0.35f), vanish, t.accent2.withAlpha (0.0f), vanish.translated (260, 0), true));
                g.fillEllipse (juce::Rectangle<float> (520, 520).withCentre (vanish));
                const int rows = 26;
                for (int row = 0; row < rows; ++row)
                {
                    const float t0 = (float) row / rows, t1 = (float) (row + 1) / rows;
                    auto at = [&] (float tt, float side)
                    {   // road centre bends with a sine as it recedes; width shrinks with perspective
                        const float y = r.getBottom() + 40.0f - (r.getBottom() + 40.0f - vanish.y) * std::pow (tt, 0.8f);
                        const float cx = vanish.x - 380.0f * (1.0f - tt) * std::cos (tt * 3.6f);
                        const float w = 520.0f * std::pow (1.0f - tt, 1.3f) + 6.0f;
                        return juce::Point<float> (cx + side * w, y);
                    };
                    const int bricks = 6;
                    for (int b = 0; b < bricks; ++b)
                    {
                        const float off = (row % 2) ? 0.5f : 0.0f;
                        const float s0 = juce::jlimit (-1.0f, 1.0f, -1.0f + 2.0f * ((float) b + off) / bricks);
                        const float s1 = juce::jlimit (-1.0f, 1.0f, -1.0f + 2.0f * ((float) b + 1 + off) / bricks);
                        juce::Path p;
                        p.startNewSubPath (at (t0, s0)); p.lineTo (at (t0, s1)); p.lineTo (at (t1, s1)); p.lineTo (at (t1, s0)); p.closeSubPath();
                        g.setColour (((row + b) % 3 == 0 ? t.patternB : t.patternA).withMultipliedAlpha (0.55f + 0.4f * (1.0f - t0)));
                        g.fillPath (p);
                        g.setColour (t.bgTop.withAlpha (0.6f));
                        g.strokePath (p, juce::PathStrokeType (1.6f * (1.0f - t0) + 0.4f));
                    }
                }
                break;
            }
            case Theme::Pattern::EmeraldLattice:
            {   // glowing faceted diamonds and distant spires
                const float s = 70.0f;
                for (float y = r.getY() - s; y < r.getBottom() + s; y += s * 0.5f)
                    for (float x = r.getX() - s; x < r.getRight() + s; x += s)
                    {
                        const float ox = (std::fmod ((y - r.getY()) / (s * 0.5f), 2.0f) < 1.0f) ? 0.0f : s * 0.5f;
                        juce::Path d;
                        d.startNewSubPath (x + ox, y - s * 0.25f); d.lineTo (x + ox + s * 0.5f, y);
                        d.lineTo (x + ox, y + s * 0.25f); d.lineTo (x + ox - s * 0.5f, y); d.closeSubPath();
                        g.setColour (t.patternA.withMultipliedAlpha (rnd.nextFloat() * 0.8f));
                        g.strokePath (d, juce::PathStrokeType (1.2f));
                        if (rnd.nextInt (7) == 0) { g.setColour (t.patternB); g.fillPath (d); }
                    }
                for (int i = 0; i < 9; ++i)
                {
                    const float x = r.getX() + r.getWidth() * (0.08f + 0.105f * (float) i);
                    const float h = 120.0f + 220.0f * std::abs (std::sin ((float) i * 1.7f));
                    juce::Path spire;
                    spire.addTriangle (x - 22, r.getBottom(), x + 22, r.getBottom(), x, r.getBottom() - h);
                    g.setColour (t.accent.withAlpha (0.12f));
                    g.fillPath (spire);
                }
                break;
            }
            case Theme::Pattern::Cyclone:
            {   // a spiral funnel of wind lines with tumbling debris
                for (int arm = 0; arm < 7; ++arm)
                {
                    juce::Path p;
                    for (float a = 0; a < 15.0f; a += 0.05f)
                    {
                        const float rad = 8.0f + a * a * 2.6f;
                        const auto pt = juce::Point<float> (c.x + std::cos (a + arm * twoPi / 7.0f) * rad * 1.5f,
                                                            c.y + std::sin (a + arm * twoPi / 7.0f) * rad * 0.7f - a * 6.0f);
                        if (p.isEmpty()) p.startNewSubPath (pt); else p.lineTo (pt);
                    }
                    g.setColour (arm % 2 ? t.patternA : t.patternB);
                    g.strokePath (p, juce::PathStrokeType (arm % 2 ? 10.0f : 22.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                }
                for (int i = 0; i < 40; ++i)
                {
                    const float a = rnd.nextFloat() * twoPi, rad = 60.0f + rnd.nextFloat() * maxR * 0.4f;
                    g.setColour (t.patternA.withMultipliedAlpha (1.5f));
                    g.fillRect (juce::Rectangle<float> (4.0f + rnd.nextFloat() * 8.0f, 3.0f).withCentre ({ c.x + std::cos (a) * rad * 1.4f, c.y + std::sin (a) * rad * 0.6f }));
                }
                break;
            }
            case Theme::Pattern::Poppies:
            {   // rolling hills of stylised poppies
                for (int hill = 0; hill < 4; ++hill)
                {
                    juce::Path p;
                    const float base = r.getY() + r.getHeight() * (0.35f + 0.18f * (float) hill);
                    p.startNewSubPath (r.getX(), r.getBottom());
                    for (float x = r.getX(); x <= r.getRight() + 10; x += 10.0f)
                        p.lineTo (x, base + 30.0f * std::sin (x * 0.004f + (float) hill * 1.3f));
                    p.lineTo (r.getRight(), r.getBottom()); p.closeSubPath();
                    g.setColour (t.patternB.withMultipliedAlpha (0.5f + 0.15f * (float) hill));
                    g.fillPath (p);
                }
                for (int i = 0; i < 70; ++i)
                {
                    const float x = r.getX() + rnd.nextFloat() * r.getWidth();
                    const float y = r.getY() + r.getHeight() * (0.3f + 0.7f * rnd.nextFloat());
                    const float s = 6.0f + 26.0f * (y - r.getY()) / r.getHeight();
                    for (int k = 0; k < 4; ++k)
                    {
                        juce::Path petal;
                        petal.addEllipse (-s * 0.45f, -s, s * 0.9f, s);
                        g.setColour (t.patternA.withMultipliedAlpha (0.8f));
                        g.fillPath (petal, juce::AffineTransform::rotation ((float) k * twoPi / 4.0f + (float) i).translated (x, y));
                    }
                    g.setColour (juce::Colours::black.withAlpha (0.55f));
                    g.fillEllipse (x - s * 0.2f, y - s * 0.2f, s * 0.4f, s * 0.4f);
                }
                break;
            }
            case Theme::Pattern::Halftone:
            {   // old printed-page halftone, ruled lines, a faded circular vignette
                const float s = 14.0f;
                for (float y = r.getY(); y < r.getBottom(); y += s)
                    for (float x = r.getX() + (std::fmod (y / s, 2.0f) < 1.0f ? 0.0f : s * 0.5f); x < r.getRight(); x += s)
                    {
                        const float d = juce::Point<float> (x, y).getDistanceFrom (c) / (maxR * 0.5f);
                        const float rad = juce::jlimit (0.0f, s * 0.45f, s * 0.45f * d * d);
                        g.setColour (t.patternA);
                        g.fillEllipse (x - rad, y - rad, rad * 2, rad * 2);
                    }
                g.setColour (t.patternB);
                for (float y = r.getY() + 20; y < r.getBottom(); y += 28.0f)
                    g.drawHorizontalLine ((int) y, r.getX(), r.getRight());
                break;
            }
            case Theme::Pattern::Stars:
            {   // night sky, a crescent moon, and a lone broom-shaped silhouette streak
                for (int i = 0; i < 180; ++i)
                {
                    const float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight();
                    const float s = 0.8f + rnd.nextFloat() * 2.2f;
                    g.setColour ((i % 9 == 0 ? t.patternA : t.patternB).withMultipliedAlpha (0.3f + rnd.nextFloat() * 0.7f));
                    g.fillEllipse (x - s, y - s, s * 2, s * 2);
                }
                const auto m = juce::Point<float> (r.getRight() - 220.0f, r.getY() + 170.0f);
                g.setColour (t.patternA.withAlpha (0.35f));
                g.fillEllipse (juce::Rectangle<float> (220, 220).withCentre (m));
                g.setColour (t.bgTop);
                g.fillEllipse (juce::Rectangle<float> (200, 200).withCentre (m.translated (-50, -24)));
                for (int k = 0; k < 5; ++k)
                {
                    juce::Path wisp;
                    wisp.startNewSubPath (r.getX(), r.getY() + 120.0f + (float) k * 140.0f);
                    for (float x = r.getX(); x <= r.getRight(); x += 12.0f)
                        wisp.lineTo (x, r.getY() + 120.0f + (float) k * 140.0f + 30.0f * std::sin (x * 0.006f + (float) k));
                    g.setColour (t.accent2.withAlpha (0.08f));
                    g.strokePath (wisp, juce::PathStrokeType (40.0f));
                }
                break;
            }
            case Theme::Pattern::RainbowArcs:
            {   // a big soft seven-band rainbow and drifting clouds
                static const juce::uint32 bands[7] = { 0xffff6b6b, 0xffffa94d, 0xffffe066, 0xff8ce99a, 0xff74c0fc, 0xff9775fa, 0xffe599f7 };
                const auto o = juce::Point<float> (r.getCentreX(), r.getBottom() + 120.0f);
                for (int b = 0; b < 7; ++b)
                {
                    const float rad = maxR * 0.62f - (float) b * 46.0f;
                    juce::Path arc; arc.addCentredArc (o.x, o.y, rad, rad, 0, -1.45f, 1.45f, true);
                    g.setColour (juce::Colour (bands[b]).withAlpha (0.42f));
                    g.strokePath (arc, juce::PathStrokeType (46.0f));
                }
                for (int i = 0; i < 9; ++i)
                {
                    const float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight() * 0.8f;
                    g.setColour (juce::Colours::white.withAlpha (0.55f));
                    for (int k = 0; k < 4; ++k)
                        g.fillEllipse (x + (float) k * 34.0f - 20.0f, y - (k % 2 ? 18.0f : 0.0f), 70.0f, 46.0f);
                }
                break;
            }
        }
    }

    void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title, bool active) const
    {
        const auto& t = theme();
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (r.translated (0, 3), t.corner);
        g.setColour (t.panel);
        g.fillRoundedRectangle (r, t.corner);
        g.setColour (t.panelEdge.withAlpha (active ? 0.9f : 0.35f));
        g.drawRoundedRectangle (r.reduced (0.5f), t.corner, 1.4f);
        g.setColour (active ? t.title : t.dimText);
        g.setFont (titleFont (13.0f));
        g.drawText (t.uppercaseTitles ? title.toUpperCase() : title, r.reduced (12, 4).withHeight (18),
                    juce::Justification::centredLeft);
    }

    // ----------------------------------------------------------------- knobs
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, juce::Slider& s) override
    {
        const auto& t = theme();
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
        const float size = juce::jmin (r.getWidth(), r.getHeight());
        r = r.withSizeKeepingCentre (size, size);
        const auto c = r.getCentre();
        const float rad = size * 0.5f;
        const float ang = startA + pos * (endA - startA);
        const bool bipolar = s.getMinimum() < 0 && s.getMaximum() > 0;
        const float zeroAng = bipolar ? startA + (float) s.valueToProportionOfLength (0.0) * (endA - startA) : startA;

        auto arc = [&] (float radius, float a0, float a1, juce::Colour col, float thick)
        {
            juce::Path p; p.addCentredArc (c.x, c.y, radius, radius, 0, a0, a1, true);
            g.setColour (col); g.strokePath (p, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };
        auto pointerAt = [&] (float r0, float r1) { return std::make_pair (c.getPointOnCircumference (r0, ang), c.getPointOnCircumference (r1, ang)); };

        switch (t.knob)
        {
            case Theme::Knob::ChickenHead:
            {
                for (int i = 0; i <= 10; ++i)
                {   // scale ticks
                    const float a = startA + (endA - startA) * i / 10.0f;
                    g.setColour (t.dimText.withAlpha (0.8f));
                    g.drawLine ({ c.getPointOnCircumference (rad * 0.86f, a), c.getPointOnCircumference (rad, a) }, 1.2f);
                }
                arc (rad * 0.78f, juce::jmin (zeroAng, ang), juce::jmax (zeroAng, ang), t.accent.withAlpha (0.8f), 2.0f);
                g.setColour (t.knobBody);
                g.fillEllipse (r.reduced (rad * 0.3f));
                juce::Path head; // pointer-shaped "chicken head" knob
                const float hl = rad * 0.72f, hw = rad * 0.26f;
                head.startNewSubPath (0, -hl);
                head.quadraticTo (hw, -hl * 0.2f, hw * 1.2f, hl * 0.35f);
                head.quadraticTo (0, hl * 0.6f, -hw * 1.2f, hl * 0.35f);
                head.quadraticTo (-hw, -hl * 0.2f, 0, -hl);
                head.closeSubPath();
                const auto tr = juce::AffineTransform::rotation (ang).translated (c);
                g.setColour (juce::Colours::black.withAlpha (0.3f));
                g.fillPath (head, tr.translated (1.5f, 2.0f));
                g.setColour (t.knobBody.brighter (0.15f));
                g.fillPath (head, tr);
                g.setColour (t.knobPointer);
                auto [p0, p1] = pointerAt (rad * 0.05f, rad * 0.66f);
                g.drawLine ({ p0, p1 }, 2.2f);
                break;
            }
            case Theme::Knob::NeonRing:
            {
                arc (rad * 0.86f, startA, endA, t.track, 4.0f);
                for (int glow = 3; glow > 0; --glow)
                    arc (rad * 0.86f, juce::jmin (zeroAng, ang), juce::jmax (zeroAng, ang), t.accent.withAlpha (0.18f), 4.0f + glow * 3.0f);
                arc (rad * 0.86f, juce::jmin (zeroAng, ang), juce::jmax (zeroAng, ang), t.accent, 3.5f);
                g.setColour (t.knobBody);
                g.fillEllipse (r.reduced (rad * 0.3f));
                g.setColour (t.accent2.withAlpha (0.5f));
                g.drawEllipse (r.reduced (rad * 0.3f), 1.0f);
                auto [p0, p1] = pointerAt (rad * 0.25f, rad * 0.62f);
                g.setColour (t.knobPointer);
                g.drawLine ({ p0, p1 }, 2.5f);
                break;
            }
            case Theme::Knob::Bakelite:
            {
                arc (rad * 0.92f, startA, endA, t.track, 1.5f);
                arc (rad * 0.92f, juce::jmin (zeroAng, ang), juce::jmax (zeroAng, ang), t.accent, 3.0f);
                auto body = r.reduced (rad * 0.18f);
                g.setGradientFill (juce::ColourGradient (t.knobBody.brighter (0.35f), body.getTopLeft(), t.knobBody, body.getBottomRight(), false));
                g.fillEllipse (body);
                g.setColour (t.knobBody.brighter (0.6f).withAlpha (0.5f));
                for (int i = 0; i < 24; ++i)
                {   // knurled edge
                    const float a = juce::MathConstants<float>::twoPi * i / 24.0f + ang;
                    g.drawLine ({ c.getPointOnCircumference (rad * 0.66f, a), c.getPointOnCircumference (rad * 0.8f, a) }, 1.0f);
                }
                auto [p0, p1] = pointerAt (rad * 0.4f, rad * 0.62f);
                g.setColour (t.knobPointer);
                g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (p1));
                juce::ignoreUnused (p0);
                break;
            }
            case Theme::Knob::Gem:
            {   // faceted emerald: an octagonal cut stone with a glowing value arc
                arc (rad * 0.9f, startA, endA, t.track, 3.0f);
                arc (rad * 0.9f, juce::jmin (zeroAng, ang), juce::jmax (zeroAng, ang), t.accent.withAlpha (0.25f), 8.0f);
                arc (rad * 0.9f, juce::jmin (zeroAng, ang), juce::jmax (zeroAng, ang), t.accent, 3.0f);
                const float gr = rad * 0.66f;
                juce::Path stone, table;
                for (int k = 0; k < 8; ++k)
                {
                    const float a = ang + juce::MathConstants<float>::twoPi * ((float) k + 0.5f) / 8.0f;
                    const auto pt = c.getPointOnCircumference (gr, a), pt2 = c.getPointOnCircumference (gr * 0.55f, a);
                    if (k == 0) { stone.startNewSubPath (pt); table.startNewSubPath (pt2); } else { stone.lineTo (pt); table.lineTo (pt2); }
                }
                stone.closeSubPath(); table.closeSubPath();
                g.setGradientFill (juce::ColourGradient (t.accent.brighter (0.3f), c.translated (-gr, -gr), t.knobBody, c.translated (gr, gr), false));
                g.fillPath (stone);
                g.setColour (t.accent.withAlpha (0.35f));
                g.fillPath (table);
                g.setColour (t.knobPointer.withAlpha (0.6f));
                g.strokePath (stone, juce::PathStrokeType (1.0f));
                g.strokePath (table, juce::PathStrokeType (0.8f));
                auto [p0, p1] = pointerAt (rad * 0.3f, rad * 0.62f);
                g.setColour (t.knobPointer);
                g.drawLine ({ p0, p1 }, 2.4f);
                break;
            }
            case Theme::Knob::SoftDot:
            {
                arc (rad * 0.88f, startA, endA, t.track, 5.0f);
                arc (rad * 0.88f, juce::jmin (zeroAng, ang), juce::jmax (zeroAng, ang), t.accent, 5.0f);
                auto body = r.reduced (rad * 0.28f);
                g.setColour (juce::Colours::black.withAlpha (0.08f));
                g.fillEllipse (body.translated (0, 2));
                g.setColour (t.knobBody);
                g.fillEllipse (body);
                auto [p0, p1] = pointerAt (0, rad * 0.5f);
                juce::ignoreUnused (p0);
                g.setColour (t.accent2);
                g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (p1));
                break;
            }
        }
    }

    // --------------------------------------------------------------- toggles
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool) override
    {
        const auto& t = theme();
        auto r = b.getLocalBounds().toFloat();
        const bool on = b.getToggleState();
        auto pill = r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 4, 34.0f), juce::jmin (r.getHeight() - 4, 16.0f));
        g.setColour (on ? t.accent : t.track);
        g.fillRoundedRectangle (pill, pill.getHeight() * 0.5f);
        if (over) { g.setColour (t.text.withAlpha (0.15f)); g.fillRoundedRectangle (pill, pill.getHeight() * 0.5f); }
        const float d = pill.getHeight() - 4;
        auto dot = juce::Rectangle<float> (d, d).withCentre ({ on ? pill.getRight() - d * 0.5f - 2 : pill.getX() + d * 0.5f + 2, pill.getCentreY() });
        g.setColour (on ? t.knobBody : t.dimText);
        g.fillEllipse (dot);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        const auto& t = theme();
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        g.setColour (t.knobBody);
        g.fillRoundedRectangle (r, juce::jmin (6.0f, t.corner));
        g.setColour (box.hasKeyboardFocus (false) ? t.accent : t.panelEdge.withAlpha (0.5f));
        g.drawRoundedRectangle (r, juce::jmin (6.0f, t.corner), 1.0f);
        juce::Path arrow;
        const float ax = (float) w - 10.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
        g.setColour (t.accent);
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (12.0f)); }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (2, 1, box.getWidth() - 16, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        const auto& t = theme();
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? t.accent : (over ? t.knobBody.brighter (0.2f) : t.knobBody));
        g.fillRoundedRectangle (r, juce::jmin (6.0f, t.corner));
        g.setColour (t.panelEdge.withAlpha (0.5f));
        g.drawRoundedRectangle (r, juce::jmin (6.0f, t.corner), 1.0f);
    }

private:
    int themeIndex = 0;
};
