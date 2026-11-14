#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// All parameter IDs and the parameter layout live here so the engine,
// editor and preset bank agree on names.
namespace Params
{
    // The ten "characters" a layer can play. Each has its own synthesis model;
    // the layer's Colour knob morphs within it (see SynthEngine.h).
    inline const juce::StringArray characters { "Brick Road", "Emerald Glass", "Tin Man", "Scarecrow", "Lion",
                                                "Munchkin Choir", "Witch", "Poppy Field", "Silver Shoes", "Flying Monkeys" };
    inline const juce::StringArray filtModes { "LP 24", "LP 12", "Band", "High", "Notch" };
    inline const juce::StringArray lfoWaves  { "Sine", "Triangle", "Saw", "Square", "S&H", "Drift" };
    inline const juce::StringArray voiceModes{ "Poly", "Mono", "Legato" };
    inline const juce::StringArray driveTypes{ "Warm", "Roar", "Tin" };
    inline const juce::StringArray chorusModes{ "Chorus", "Flanger", "Ensemble" };
    inline const juce::StringArray syncDivs  { "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D",
                                               "1/4T", "1/4", "1/4D", "1/2", "1/2D", "1/1" };
    inline const float syncBeats[] = { 0.125f, 1.0f / 6.0f, 0.25f, 0.375f, 1.0f / 3.0f, 0.5f, 0.75f,
                                       2.0f / 3.0f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f };

    using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
    using NR = juce::NormalisableRange<float>;

    inline NR skewed (float lo, float hi, float centre)
    {
        NR r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    inline Layout createLayout()
    {
        Layout layout;
        auto pid = [] (const juce::String& id) { return juce::ParameterID { id, 1 }; };
        auto addF = [&] (const juce::String& id, const juce::String& name, NR range, float def, const char* suffix = "")
        {
            layout.add (std::make_unique<juce::AudioParameterFloat> (pid (id), name, range, def,
                juce::AudioParameterFloatAttributes().withLabel (suffix)));
        };
        auto addI = [&] (const juce::String& id, const juce::String& name, int lo, int hi, int def)
        {
            layout.add (std::make_unique<juce::AudioParameterInt> (pid (id), name, lo, hi, def));
        };
        auto addC = [&] (const juce::String& id, const juce::String& name, const juce::StringArray& items, int def)
        {
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid (id), name, items, def));
        };
        auto addB = [&] (const juce::String& id, const juce::String& name, bool def)
        {
            layout.add (std::make_unique<juce::AudioParameterBool> (pid (id), name, def));
        };

        // ---- Layers A and B
        for (auto L : { "A", "B" })
        {
            const juce::String p = juce::String ("lay") + L, n = juce::String ("Layer ") + L + " ";
            const bool a = juce::String (L) == "A";
            addC (p + "Char",   n + "Character", characters, a ? 0 : 7);
            addI (p + "Oct",    n + "Octave", -3, 3, 0);
            addI (p + "Semi",   n + "Semi", -12, 12, 0);
            addF (p + "Fine",   n + "Fine", NR (-100.0f, 100.0f), 0.0f, "ct");
            addF (p + "Colour", n + "Colour", NR (0.0f, 1.0f), 0.5f);
            addF (p + "Level",  n + "Level", NR (0.0f, 1.0f), 0.8f);
            addI (p + "Unison", n + "Unison", 1, 5, 1);
            addF (p + "Detune", n + "Detune", NR (0.0f, 1.0f), 0.2f);
        }

        // ---- The Journey: signature macros
        addF ("journey",    "Journey (A > B)", NR (0.0f, 1.0f), 0.0f);
        addF ("journeyMod", "LFO2 > Journey", NR (0.0f, 1.0f), 0.0f);
        addF ("cyclone",    "Cyclone", NR (0.0f, 1.0f), 0.0f);
        addF ("cycRate",    "Cyclone Speed", skewed (0.05f, 8.0f, 1.0f), 0.7f, "Hz");
        addF ("wind",       "Wind", NR (0.0f, 1.0f), 0.0f);
        addF ("poppies",    "Poppies", NR (0.0f, 1.0f), 0.0f);

        // ---- Filter
        addC ("filtMode",  "Filter Mode", filtModes, 0);
        addF ("cutoff",    "Cutoff", skewed (20.0f, 20000.0f, 1000.0f), 12000.0f, "Hz");
        addF ("reso",      "Resonance", NR (0.0f, 1.0f), 0.1f);
        addF ("filtDrive", "Filter Drive", NR (0.0f, 1.0f), 0.0f);
        addF ("filtEnv",   "Filter Env Amt", NR (-1.0f, 1.0f), 0.0f);
        addF ("keyTrack",  "Key Track", NR (0.0f, 1.0f), 0.3f);
        addF ("filtVel",   "Filter Velocity", NR (0.0f, 1.0f), 0.0f);

        // ---- Envelopes
        auto timeRange = skewed (0.001f, 10.0f, 0.5f);
        addF ("fA", "Filter Attack",  timeRange, 0.005f, "s");
        addF ("fD", "Filter Decay",   timeRange, 0.4f, "s");
        addF ("fS", "Filter Sustain", NR (0.0f, 1.0f), 0.5f);
        addF ("fR", "Filter Release", timeRange, 0.4f, "s");
        addF ("aA", "Amp Attack",  timeRange, 0.005f, "s");
        addF ("aD", "Amp Decay",   timeRange, 0.3f, "s");
        addF ("aS", "Amp Sustain", NR (0.0f, 1.0f), 0.8f);
        addF ("aR", "Amp Release", timeRange, 0.4f, "s");
        addF ("ampVel", "Amp Velocity", NR (0.0f, 1.0f), 0.5f);

        // ---- LFOs
        addC ("lfo1Wave",  "LFO1 Wave", lfoWaves, 0);
        addF ("lfo1Rate",  "LFO1 Rate", skewed (0.02f, 20.0f, 2.0f), 5.0f, "Hz");
        addF ("lfo1Delay", "LFO1 Fade In", NR (0.0f, 3.0f), 0.0f, "s");
        addF ("lfo1Pitch", "LFO1 > Pitch", NR (0.0f, 1.0f), 0.0f);
        addF ("lfo1Cutoff","LFO1 > Cutoff", NR (0.0f, 1.0f), 0.0f);
        addC ("lfo2Wave",  "LFO2 Wave", lfoWaves, 1);
        addF ("lfo2Rate",  "LFO2 Rate", skewed (0.02f, 20.0f, 2.0f), 0.2f, "Hz");
        addF ("lfo2Colour","LFO2 > Colour", NR (0.0f, 1.0f), 0.0f);
        addF ("lfo2Pan",   "LFO2 > Pan", NR (0.0f, 1.0f), 0.0f);
        addF ("lfo2Amp",   "LFO2 > Amp", NR (0.0f, 1.0f), 0.0f);

        // ---- Voice
        addC ("voiceMode", "Voice Mode", voiceModes, 0);
        addF ("glide",     "Glide", skewed (0.0f, 2.0f, 0.3f), 0.0f, "s");
        addF ("drift",     "Analog Drift", NR (0.0f, 1.0f), 0.15f);
        addF ("spread",    "Stereo Spread", NR (0.0f, 1.0f), 0.5f);
        addI ("bendRange", "Bend Range", 0, 12, 2);
        addF ("master",    "Master", skewed (0.0f, 2.0f, 0.7f), 0.7f);

        // ---- FX: Drive
        addB ("drvOn",  "Drive On", false);
        addC ("drvType","Drive Type", driveTypes, 0);
        addF ("drvAmt", "Drive Amount", NR (0.0f, 1.0f), 0.3f);
        addF ("drvTone","Drive Tone", NR (0.0f, 1.0f), 0.6f);
        addF ("drvMix", "Drive Mix", NR (0.0f, 1.0f), 1.0f);
        // Twister: rotating-speaker swirl (doppler + amplitude + auto-pan) that spins up and down
        addB ("twsOn",    "Twister On", false);
        addB ("twsFast",  "Twister Fast", false);
        addF ("twsSlow",  "Twister Slow Speed", skewed (0.1f, 3.0f, 0.8f), 0.7f, "Hz");
        addF ("twsFastHz","Twister Fast Speed", skewed (2.0f, 12.0f, 6.0f), 6.5f, "Hz");
        addF ("twsDepth", "Twister Depth", NR (0.0f, 1.0f), 0.6f);
        addF ("twsMix",   "Twister Mix", NR (0.0f, 1.0f), 0.8f);
        // Rainbow Prism: seven resonant bands sweeping through the spectrum, spread red (L) to violet (R)
        addB ("prsOn",    "Prism On", false);
        addF ("prsRate",  "Prism Rate", skewed (0.01f, 4.0f, 0.3f), 0.12f, "Hz");
        addF ("prsDepth", "Prism Depth", NR (0.0f, 1.0f), 0.6f);
        addF ("prsReso",  "Prism Resonance", NR (0.0f, 1.0f), 0.6f);
        addF ("prsMix",   "Prism Mix", NR (0.0f, 1.0f), 0.5f);
        // Chorus / flanger / ensemble
        addB ("choOn",   "Chorus On", false);
        addC ("choMode", "Chorus Mode", chorusModes, 0);
        addF ("choRate", "Chorus Rate", skewed (0.02f, 10.0f, 0.8f), 0.5f, "Hz");
        addF ("choDepth","Chorus Depth", NR (0.0f, 1.0f), 0.5f);
        addF ("choFb",   "Chorus Feedback", NR (0.0f, 0.95f), 0.0f);
        addF ("choMix",  "Chorus Mix", NR (0.0f, 1.0f), 0.5f);
        // Kansas: grey, dusty old-gramophone lo-fi (band-limit, wow, crackle, narrow)
        addB ("knsOn",     "Kansas On", false);
        addF ("knsWow",    "Kansas Wow", NR (0.0f, 1.0f), 0.3f);
        addF ("knsBand",   "Kansas Band-Limit", NR (0.0f, 1.0f), 0.5f);
        addF ("knsDust",   "Kansas Dust", NR (0.0f, 1.0f), 0.3f);
        addF ("knsMono",   "Kansas Narrow", NR (0.0f, 1.0f), 0.5f);
        // Road Echo
        addB ("dlyOn",   "Echo On", false);
        addB ("dlySync", "Echo Sync", true);
        addC ("dlyDiv",  "Echo Division", syncDivs, 6);
        addF ("dlyMs",   "Echo Time", skewed (10.0f, 2000.0f, 400.0f), 350.0f, "ms");
        addF ("dlyFb",   "Echo Feedback", NR (0.0f, 1.05f), 0.4f);
        addF ("dlyTone", "Echo Tone", NR (0.0f, 1.0f), 0.6f);
        addF ("dlyMod",  "Echo Wobble", NR (0.0f, 1.0f), 0.2f);
        addB ("dlyPing", "Echo Ping-Pong", true);
        addF ("dlyMix",  "Echo Mix", NR (0.0f, 1.0f), 0.3f);
        // Emerald Hall reverb with shimmer (octave-up feedback)
        addB ("revOn",     "Reverb On", false);
        addF ("revSize",   "Reverb Size", NR (0.0f, 1.0f), 0.6f);
        addF ("revDamp",   "Reverb Damping", NR (0.0f, 1.0f), 0.5f);
        addF ("revShimmer","Reverb Shimmer", NR (0.0f, 1.0f), 0.0f);
        addF ("revWidth",  "Reverb Width", NR (0.0f, 1.0f), 1.0f);
        addF ("revMix",    "Reverb Mix", NR (0.0f, 1.0f), 0.25f);

        return layout;
    }
}
