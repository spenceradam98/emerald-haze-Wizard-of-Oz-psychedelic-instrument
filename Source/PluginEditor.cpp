#include "PluginEditor.h"

// =========================================================== ParamControl
ParamControl::ParamControl (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& text)
{
    auto* param = s.getParameter (id);
    jassert (param != nullptr);
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (11.5f));
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
    {
        combo = std::make_unique<juce::ComboBox>();
        combo->addItemList (choice->choices, 1);
        addAndMakeVisible (*combo);
        cAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, *combo);
    }
    else if (dynamic_cast<juce::AudioParameterBool*> (param) != nullptr)
    {
        toggle = std::make_unique<juce::ToggleButton>();
        addAndMakeVisible (*toggle);
        bAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, *toggle);
    }
    else
    {
        slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
        slider->setPopupDisplayEnabled (true, true, nullptr);
        slider->setTooltip (param->getName (64));
        addAndMakeVisible (*slider);
        sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, *slider);
        slider->setDoubleClickReturnValue (true, (double) dynamic_cast<juce::RangedAudioParameter*> (param)
                                                     ->convertFrom0to1 (param->getDefaultValue()));
    }
}

void ParamControl::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromBottom (16));
    if (slider) slider->setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth(), r.getHeight()), juce::jmin (r.getWidth(), r.getHeight())));
    if (combo)  combo->setBounds (r.withSizeKeepingCentre (r.getWidth() - 2, 22));
    if (toggle) toggle->setBounds (r.withSizeKeepingCentre (r.getWidth(), 24));
}

// ================================================================ Section
Section::Section (juce::AudioProcessorValueTreeState& s, const juce::String& t,
                  std::vector<std::pair<const char*, const char*>> list, const char* powerId)
    : apvts (s), title (t)
{
    for (auto& [id, name] : list)
        addAndMakeVisible (*controls.emplace_back (std::make_unique<ParamControl> (s, id, name)));
    if (powerId != nullptr)
    {
        powerParam = powerId;
        power = std::make_unique<juce::ToggleButton>();
        power->setTooltip ("Switch " + title + " on/off");
        addAndMakeVisible (*power);
        powerAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, powerParam, *power);
        apvts.addParameterListener (powerParam, this);
    }
}

Section::~Section() { if (powerParam.isNotEmpty()) apvts.removeParameterListener (powerParam, this); }

void Section::parameterChanged (const juce::String&, float)
{
    juce::MessageManager::callAsync ([sp = juce::Component::SafePointer<Section> (this)] { if (sp) sp->repaint(); });
}

void Section::paint (juce::Graphics& g)
{
    if (auto* l = dynamic_cast<HazeLookAndFeel*> (&getLookAndFeel()))
    {
        const bool active = powerParam.isEmpty() || apvts.getRawParameterValue (powerParam)->load() > 0.5f;
        l->drawPanel (g, getLocalBounds().toFloat(), title, active);
    }
}

void Section::resized()
{
    auto r = getLocalBounds().reduced (6, 4);
    auto header = r.removeFromTop (20);
    if (power) power->setBounds (header.removeFromRight (40));
    if (controls.empty()) return;
    const float unit = (float) r.getWidth() / numControls();
    float x = (float) r.getX();
    for (auto& c : controls)
    {
        const float w = unit * (c->isCombo() ? 1.6f : 1.0f);
        c->setBounds (juce::Rectangle<float> (x, (float) r.getY(), w, (float) r.getHeight()).toNearestInt().reduced (2, 0));
        x += w;
    }
}

float Section::numControls() const
{
    float n = 0;
    for (auto& c : controls) n += c->isCombo() ? 1.6f : 1.0f;
    return n;
}

// ================================================================= Editor
EmeraldHazeEditor::EmeraldHazeEditor (EmeraldHazeProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&laf);
    auto& s = p.apvts;
    auto add = [&] (int row, std::unique_ptr<Section> sec) { content.addAndMakeVisible (*sec); rows[row].push_back (std::move (sec)); };

    using List = std::vector<std::pair<const char*, const char*>>;

    // ---- Row 0: the two layers + the Journey macros
    add (0, std::make_unique<Section> (s, "Layer A", List {
        { "layAChar", "Character" }, { "layAOct", "Octave" }, { "layASemi", "Semi" }, { "layAFine", "Fine" },
        { "layAColour", "Colour" }, { "layALevel", "Level" }, { "layAUnison", "Unison" }, { "layADetune", "Detune" } }));
    add (0, std::make_unique<Section> (s, "Layer B", List {
        { "layBChar", "Character" }, { "layBOct", "Octave" }, { "layBSemi", "Semi" }, { "layBFine", "Fine" },
        { "layBColour", "Colour" }, { "layBLevel", "Level" }, { "layBUnison", "Unison" }, { "layBDetune", "Detune" } }));
    add (0, std::make_unique<Section> (s, "The Journey", List {
        { "journey", "A > B" }, { "journeyMod", "LFO2 Sway" }, { "cyclone", "Cyclone" }, { "cycRate", "Spin" },
        { "wind", "Wind" }, { "poppies", "Poppies" } }));

    // ---- Row 1: filter + envelopes
    add (1, std::make_unique<Section> (s, "Filter", List {
        { "filtMode", "Mode" }, { "cutoff", "Cutoff" }, { "reso", "Reso" }, { "filtDrive", "Drive" },
        { "filtEnv", "Env Amt" }, { "keyTrack", "Key Trk" }, { "filtVel", "Velocity" } }));
    add (1, std::make_unique<Section> (s, "Filter Envelope", List {
        { "fA", "Attack" }, { "fD", "Decay" }, { "fS", "Sustain" }, { "fR", "Release" } }));
    add (1, std::make_unique<Section> (s, "Amp Envelope", List {
        { "aA", "Attack" }, { "aD", "Decay" }, { "aS", "Sustain" }, { "aR", "Release" }, { "ampVel", "Velocity" } }));

    // ---- Row 2: modulation + voice
    add (2, std::make_unique<Section> (s, "LFO 1  (vibrato / sweep)", List {
        { "lfo1Wave", "Wave" }, { "lfo1Rate", "Rate" }, { "lfo1Delay", "Fade In" }, { "lfo1Pitch", "> Pitch" }, { "lfo1Cutoff", "> Cutoff" } }));
    add (2, std::make_unique<Section> (s, "LFO 2  (motion)", List {
        { "lfo2Wave", "Wave" }, { "lfo2Rate", "Rate" }, { "lfo2Colour", "> Colour" }, { "lfo2Pan", "> Pan" }, { "lfo2Amp", "> Amp" } }));
    add (2, std::make_unique<Section> (s, "Voice", List {
        { "voiceMode", "Mode" }, { "glide", "Glide" }, { "drift", "Drift" }, { "spread", "Spread" },
        { "bendRange", "Bend" }, { "master", "Master" } }));

    // ---- Row 3: FX part 1
    add (3, std::make_unique<Section> (s, "Drive", List {
        { "drvType", "Type" }, { "drvAmt", "Amount" }, { "drvTone", "Tone" }, { "drvMix", "Mix" } }, "drvOn"));
    add (3, std::make_unique<Section> (s, "Twister", List {
        { "twsFast", "Fast" }, { "twsSlow", "Slow Hz" }, { "twsFastHz", "Fast Hz" }, { "twsDepth", "Depth" }, { "twsMix", "Mix" } }, "twsOn"));
    add (3, std::make_unique<Section> (s, "Rainbow Prism", List {
        { "prsRate", "Rate" }, { "prsDepth", "Span" }, { "prsReso", "Reso" }, { "prsMix", "Mix" } }, "prsOn"));
    add (3, std::make_unique<Section> (s, "Chorus / Flanger", List {
        { "choMode", "Mode" }, { "choRate", "Rate" }, { "choDepth", "Depth" }, { "choFb", "Feedback" }, { "choMix", "Mix" } }, "choOn"));

    // ---- Row 4: FX part 2
    add (4, std::make_unique<Section> (s, "Kansas", List {
        { "knsWow", "Wow" }, { "knsBand", "Band" }, { "knsDust", "Dust" }, { "knsMono", "Narrow" } }, "knsOn"));
    add (4, std::make_unique<Section> (s, "Road Echo", List {
        { "dlySync", "Sync" }, { "dlyDiv", "Division" }, { "dlyMs", "Time" }, { "dlyFb", "Feedback" },
        { "dlyTone", "Tone" }, { "dlyMod", "Wobble" }, { "dlyPing", "Ping-Pong" }, { "dlyMix", "Mix" } }, "dlyOn"));
    add (4, std::make_unique<Section> (s, "Emerald Hall", List {
        { "revSize", "Size" }, { "revDamp", "Damping" }, { "revShimmer", "Shimmer" }, { "revWidth", "Width" }, { "revMix", "Mix" } }, "revOn"));

    // ---- Storybook page: the characters, the four Journey macros and the spell switches
    auto addStory = [&] (int row, std::unique_ptr<Section> sec) { content.addChildComponent (*sec); story[row].push_back (std::move (sec)); };
    addStory (0, std::make_unique<Section> (s, "Layer A", List {
        { "layAChar", "Character" }, { "layAColour", "Colour" }, { "layALevel", "Level" } }));
    addStory (0, std::make_unique<Section> (s, "The Journey", List {
        { "journey", "A > B" }, { "cyclone", "Cyclone" }, { "wind", "Wind" }, { "poppies", "Poppies" } }));
    addStory (0, std::make_unique<Section> (s, "Layer B", List {
        { "layBChar", "Character" }, { "layBColour", "Colour" }, { "layBLevel", "Level" } }));
    addStory (1, std::make_unique<Section> (s, "Spells", List {
        { "twsOn", "Twister" }, { "twsFast", "Spin Fast" }, { "prsOn", "Prism" }, { "knsOn", "Kansas" },
        { "dlyOn", "Echo" }, { "revOn", "Hall" } }));
    addStory (1, std::make_unique<Section> (s, "Colour & Space", List {
        { "cutoff", "Cutoff" }, { "reso", "Reso" }, { "journeyMod", "Sway" }, { "revShimmer", "Shimmer" },
        { "dlyMix", "Echo Mix" }, { "revMix", "Hall Mix" }, { "master", "Master" } }));

    // ---- Header
    for (auto* c : { (juce::Component*) &categoryBox, (juce::Component*) &presetBox, (juce::Component*) &themeBox,
                     (juce::Component*) &viewButton, (juce::Component*) &prevButton, (juce::Component*) &nextButton, (juce::Component*) &saveButton,
                     (juce::Component*) &initButton, (juce::Component*) &keyboard })
        content.addAndMakeVisible (c);

    categoryBox.setTooltip ("Preset category");
    presetBox.setTooltip ("Preset");
    themeBox.setTooltip ("UI theme");
    for (int i = 0; i < (int) getThemes().size(); ++i) themeBox.addItem (getThemes()[(size_t) i].name, i + 1);
    themeBox.onChange = [this] { applyTheme (themeBox.getSelectedItemIndex()); };

    categoryBox.onChange = [this] { shownCategory = categoryBox.getText(); refreshPresetBox(); };
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id > 0 && id - 1 != proc.presets.getCurrentIndex()) proc.presets.load (id - 1);
    };
    viewButton.setTooltip ("Switch between the Storybook page (big macros) and the Workshop page (every control)");
    viewButton.onClick = [this] { setStoryView (! proc.getStoryView()); };
    prevButton.onClick = [this] { proc.presets.step (-1); };
    nextButton.onClick = [this] { proc.presets.step (1); };
    initButton.onClick = [this] { proc.presets.loadInit(); };
    saveButton.onClick = [this] { showSaveDialog(); };
    proc.presets.onPresetChanged = [sp = juce::Component::SafePointer<EmeraldHazeEditor> (this)]
    {
        if (sp) { sp->refreshCategoryBox(); }
    };

    keyboard.setKeyWidth (22.0f);
    keyboard.setAvailableRange (24, 108);
    keyboard.setLowestVisibleKey (36);

    addAndMakeVisible (content);
    content.setSize (designW, designH);
    refreshCategoryBox();
    applyTheme (proc.getThemeIndex());
    setStoryView (proc.getStoryView());

    setResizable (true, true);
    setResizeLimits (designW / 2, designH / 2, designW * 3 / 2, designH * 3 / 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) designW / designH);
    setSize (designW * 4 / 5, designH * 4 / 5);
}

EmeraldHazeEditor::~EmeraldHazeEditor()
{
    proc.presets.onPresetChanged = nullptr;
    setLookAndFeel (nullptr);
}

void EmeraldHazeEditor::setStoryView (bool storyView)
{
    proc.setStoryView (storyView);
    viewButton.setButtonText (storyView ? "Workshop" : "Storybook");
    for (auto& row : rows) for (auto& sec : row) sec->setVisible (! storyView);
    for (auto& row : story) for (auto& sec : row) sec->setVisible (storyView);
    content.resized();
    content.repaint();
}

void EmeraldHazeEditor::applyTheme (int index)
{
    laf.setTheme (index);
    proc.setThemeIndex (laf.getThemeIndex());
    themeBox.setSelectedItemIndex (laf.getThemeIndex(), juce::dontSendNotification);
    sendLookAndFeelChange();
    keyboard.repaint();
    repaint();
}

void EmeraldHazeEditor::refreshCategoryBox()
{
    auto& pm = proc.presets;
    const int cur = pm.getCurrentIndex();
    if (cur >= 0) shownCategory = pm.getEntries()[cur].category;
    auto cats = pm.getCategories();
    categoryBox.clear (juce::dontSendNotification);
    categoryBox.addItemList (cats, 1);
    if (shownCategory.isEmpty() && ! cats.isEmpty()) shownCategory = cats[0];
    categoryBox.setSelectedItemIndex (juce::jmax (0, cats.indexOf (shownCategory)), juce::dontSendNotification);
    refreshPresetBox();
}

void EmeraldHazeEditor::refreshPresetBox()
{
    auto& pm = proc.presets;
    presetBox.clear (juce::dontSendNotification);
    for (int idx : pm.getIndicesInCategory (shownCategory))
        presetBox.addItem (pm.getEntries()[idx].name, idx + 1);
    if (pm.getCurrentIndex() >= 0 && pm.getEntries()[pm.getCurrentIndex()].category == shownCategory)
        presetBox.setSelectedId (pm.getCurrentIndex() + 1, juce::dontSendNotification);
    else
        presetBox.setText (pm.getCurrentIndex() < 0 ? pm.getCurrentName() : juce::String ("-- choose --"), juce::dontSendNotification);
}

void EmeraldHazeEditor::showSaveDialog()
{
    saveDialog = std::make_unique<juce::AlertWindow> ("Save Preset", "Saved to " + PresetManager::getUserDir().getFullPathName(),
                                                      juce::MessageBoxIconType::NoIcon, this);
    saveDialog->addTextEditor ("name", proc.presets.getCurrentName(), "Name");
    saveDialog->addTextEditor ("cat", "User", "Folder");
    saveDialog->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    saveDialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    saveDialog->enterModalState (true, juce::ModalCallbackFunction::create ([this] (int result)
    {
        if (result == 1 && saveDialog != nullptr)
            proc.presets.saveUser (saveDialog->getTextEditorContents ("name"), saveDialog->getTextEditorContents ("cat"));
        saveDialog.reset();
    }), false);
}

void EmeraldHazeEditor::paint (juce::Graphics& g) { g.fillAll (laf.theme().bgBottom); }

void EmeraldHazeEditor::resized()
{
    const float scale = (float) getWidth() / (float) designW;
    content.setTransform (juce::AffineTransform::scale (scale));
    content.setBounds (0, 0, designW, designH);
}

// ---------------------------------------------------------------- Content
void EmeraldHazeEditor::Content::paint (juce::Graphics& g)
{
    auto& laf = owner.laf;
    const auto& t = laf.theme();
    laf.drawBackground (g, getLocalBounds().toFloat());

    // Header strip + logo
    auto header = getLocalBounds().removeFromTop (64).toFloat().reduced (10, 8);
    g.setColour (t.panel);
    g.fillRoundedRectangle (header, t.corner);
    g.setColour (t.panelEdge.withAlpha (0.6f));
    g.drawRoundedRectangle (header, t.corner, 1.4f);

    auto logo = header.removeFromLeft (300).reduced (14, 0);
    juce::String word = t.uppercaseTitles ? "EMERALD HAZE" : "emerald haze";
    auto sub = logo.removeFromBottom (14);
    g.setFont (laf.titleFont (27.0f));
    // offset "trails" for a psychedelic echo effect
    for (int i = 3; i > 0; --i)
    {
        g.setColour ((i % 2 ? t.accent2 : t.accent).withAlpha (0.25f + 0.1f * (3 - i)));
        g.drawText (word, logo.translated ((float) i * 2.0f, (float) i * 1.5f), juce::Justification::centredLeft);
    }
    g.setColour (t.title);
    g.drawText (word, logo, juce::Justification::centredLeft);
    g.setColour (t.dimText);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (t.uppercaseTitles ? "A PSYCHEDELIC STORYBOOK SYNTH" : "a psychedelic storybook synth", sub, juce::Justification::centredLeft);
}

void EmeraldHazeEditor::Content::resized()
{
    auto& e = owner;
    auto r = getLocalBounds().reduced (10, 0);
    auto header = r.removeFromTop (64).reduced (0, 8);
    header.removeFromLeft (310);
    auto hdr = header.reduced (6, 10);
    e.themeBox.setBounds (hdr.removeFromRight (190));
    hdr.removeFromRight (20);
    e.saveButton.setBounds (hdr.removeFromRight (60)); hdr.removeFromRight (6);
    e.initButton.setBounds (hdr.removeFromRight (50)); hdr.removeFromRight (16);
    e.nextButton.setBounds (hdr.removeFromRight (30)); hdr.removeFromRight (4);
    e.prevButton.setBounds (hdr.removeFromRight (30)); hdr.removeFromRight (6);
    e.presetBox.setBounds (hdr.removeFromRight (260)); hdr.removeFromRight (6);
    e.categoryBox.setBounds (hdr.removeFromRight (150)); hdr.removeFromRight (16);
    e.viewButton.setBounds (hdr.removeFromRight (100));

    e.keyboard.setBounds (r.removeFromBottom (78).reduced (0, 6));
    e.keyboard.setKeyWidth ((float) e.keyboard.getWidth() / 50.0f); // 24..108 = 50 white keys
    e.keyboard.setLowestVisibleKey (24);

    auto layoutRow = [] (std::vector<std::unique_ptr<Section>>& row, juce::Rectangle<int> rr, int gap)
    {
        float total = 0;
        for (auto& s : row) total += s->numControls();
        const float unit = (float) (rr.getWidth() - gap * ((int) row.size() - 1)) / total;
        float x = (float) rr.getX();
        for (auto& s : row)
        {
            const float w = unit * s->numControls();
            s->setBounds (juce::Rectangle<float> (x, (float) rr.getY(), w, (float) rr.getHeight()).toNearestInt());
            x += w + (float) gap;
        }
    };

    if (e.proc.getStoryView())
    {   // big characters + macros on top, spells along the bottom, the theme's artwork in between
        auto top = r.removeFromTop (290).reduced (40, 10);
        layoutRow (e.story[0], top, 24);
        auto bottom = r.removeFromBottom (150).reduced (40, 10);
        layoutRow (e.story[1], bottom, 24);
        return;
    }

    const int rowH = (r.getHeight() - 4) / 5;
    for (auto& row : e.rows)
    {
        auto rr = r.removeFromTop (rowH).reduced (0, 4);
        float total = 0;
        for (auto& s : row) total += s->numControls();
        const int gap = 10;
        const float unit = (float) (rr.getWidth() - gap * ((int) row.size() - 1)) / total;
        float x = (float) rr.getX();
        for (auto& s : row)
        {
            const float w = unit * s->numControls();
            s->setBounds (juce::Rectangle<float> (x, (float) rr.getY(), w, (float) rr.getHeight()).toNearestInt());
            x += w + (float) gap;
        }
    }
}
