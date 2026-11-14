#include "FactoryPresets.h"
#if __has_include("FactoryPresetLevels.h")
 #include "FactoryPresetLevels.h"
#else
 #include <map>
 #include <string>
 static const std::map<std::string, float> factoryPresetLevels;
#endif

// ============================================================================
//  Emerald Haze factory bank.
//  Character: 0 Brick Road 1 Emerald Glass 2 Tin Man 3 Scarecrow 4 Lion
//             5 Munchkin Choir 6 Witch 7 Poppy Field 8 Silver Shoes 9 Flying Monkeys
//  Defaults:  Layer A = Brick Road, Layer B = Poppy Field, Journey 0 (all A).
//  Filter mode: 0 LP24 1 LP12 2 Band 3 High 4 Notch
//  LFO wave:    0 Sine 1 Tri 2 Saw 3 Square 4 S&H 5 Drift
//  Voice mode:  0 Poly 1 Mono 2 Legato
//  Drive type:  0 Warm 1 Roar 2 Tin     Chorus mode: 0 Chorus 1 Flanger 2 Ensemble
//  Echo div:    5 = 1/8, 6 = 1/8D, 7 = 1/4T, 8 = 1/4, 9 = 1/4D, 10 = 1/2
// ============================================================================

namespace
{
using V = std::vector<std::pair<const char*, float>>;
V operator+ (V a, const V& b) { a.insert (a.end(), b.begin(), b.end()); return a; }

// Reusable FX "pedalboard" snippets
const V roadEcho   { { "dlyOn", 1 }, { "dlyDiv", 6 }, { "dlyFb", 0.42f }, { "dlyTone", 0.5f }, { "dlyMod", 0.35f }, { "dlyMix", 0.28f } };
const V longEcho   { { "dlyOn", 1 }, { "dlyDiv", 9 }, { "dlyFb", 0.55f }, { "dlyTone", 0.4f }, { "dlyMod", 0.45f }, { "dlyMix", 0.3f } };
const V room       { { "revOn", 1 }, { "revSize", 0.45f }, { "revMix", 0.2f } };
const V hall       { { "revOn", 1 }, { "revSize", 0.8f }, { "revDamp", 0.4f }, { "revMix", 0.32f } };
const V shimmer    { { "revOn", 1 }, { "revSize", 0.9f }, { "revDamp", 0.3f }, { "revShimmer", 0.55f }, { "revMix", 0.4f } };
const V heaven     { { "revOn", 1 }, { "revSize", 1.0f }, { "revDamp", 0.2f }, { "revShimmer", 0.8f }, { "revMix", 0.5f } };
const V twistSlow  { { "twsOn", 1 }, { "twsSlow", 0.8f }, { "twsDepth", 0.6f }, { "twsMix", 0.8f } };
const V twistFast  { { "twsOn", 1 }, { "twsFast", 1 }, { "twsFastHz", 6.5f }, { "twsDepth", 0.7f }, { "twsMix", 0.85f } };
const V prism      { { "prsOn", 1 }, { "prsRate", 0.15f }, { "prsDepth", 0.6f }, { "prsReso", 0.6f }, { "prsMix", 0.45f } };
const V prismFast  { { "prsOn", 1 }, { "prsRate", 1.2f }, { "prsDepth", 0.8f }, { "prsReso", 0.75f }, { "prsMix", 0.55f } };
const V kansasOld  { { "knsOn", 1 }, { "knsWow", 0.35f }, { "knsBand", 0.55f }, { "knsDust", 0.35f }, { "knsMono", 0.7f } };
const V ensemble   { { "choOn", 1 }, { "choMode", 2 }, { "choRate", 0.6f }, { "choDepth", 0.7f }, { "choMix", 0.55f } };
const V chorus     { { "choOn", 1 }, { "choMode", 0 }, { "choRate", 0.45f }, { "choDepth", 0.6f }, { "choMix", 0.45f } };
const V jetFlange  { { "choOn", 1 }, { "choMode", 1 }, { "choRate", 0.12f }, { "choDepth", 0.9f }, { "choFb", 0.7f }, { "choMix", 0.55f } };
const V vibrato    { { "lfo1Rate", 5.3f }, { "lfo1Delay", 0.6f }, { "lfo1Pitch", 0.22f } };
const V pad        { { "aA", 1.2f }, { "aD", 1.0f }, { "aS", 0.9f }, { "aR", 2.5f } };
const V bell       { { "aA", 0.001f }, { "aD", 2.2f }, { "aS", 0.0f }, { "aR", 1.8f }, { "ampVel", 0.7f } };
const V pluck      { { "aA", 0.001f }, { "aD", 0.6f }, { "aS", 0.0f }, { "aR", 0.5f }, { "filtEnv", 0.45f }, { "fA", 0.001f }, { "fD", 0.35f }, { "fS", 0.0f }, { "cutoff", 900 } };

const std::vector<FactoryPreset> bank =
{
    // ======================================================== YELLOW BRICK ROAD (bright, golden, marching)
    { "Follow The Bricks", "Brick Road", V {
        { "layAChar", 0 }, { "layAColour", 0.4f }, { "layAUnison", 3 }, { "layADetune", 0.25f },
        { "cutoff", 2400 }, { "reso", 0.2f }, { "filtEnv", 0.35f }, { "fA", 0.02f }, { "fD", 0.5f }, { "fS", 0.4f },
        { "aA", 0.02f }, { "aS", 0.85f }, { "aR", 0.3f } } + vibrato + roadEcho + room },
    { "Golden Brass Parade", "Brick Road", V {
        { "layAChar", 0 }, { "layAColour", 0.75f }, { "layAUnison", 4 }, { "layADetune", 0.3f },
        { "cutoff", 1500 }, { "reso", 0.15f }, { "filtEnv", 0.5f }, { "fA", 0.06f }, { "fD", 0.4f }, { "fS", 0.5f }, { "filtVel", 0.5f },
        { "aA", 0.04f }, { "aS", 0.9f }, { "aR", 0.25f } } + vibrato + ensemble + hall },
    { "Sunlit Road Lead", "Brick Road", V {
        { "layAChar", 0 }, { "layAColour", 0.55f }, { "layAUnison", 2 }, { "layADetune", 0.15f }, { "voiceMode", 2 }, { "glide", 0.08f },
        { "cutoff", 3200 }, { "reso", 0.3f }, { "filtEnv", 0.3f }, { "fD", 0.3f }, { "fS", 0.5f },
        { "aA", 0.005f }, { "aS", 1.0f }, { "aR", 0.2f }, { "drvOn", 1 }, { "drvAmt", 0.3f } } + vibrato + roadEcho + room },
    { "Bricks To Glass", "Brick Road", V {
        { "layAChar", 0 }, { "layAColour", 0.5f }, { "layAUnison", 3 }, { "layBChar", 1 }, { "layBColour", 0.6f }, { "layBOct", 1 },
        { "journey", 0.3f }, { "journeyMod", 0.8f }, { "lfo2Rate", 0.15f }, { "cutoff", 4000 } } + pad + prism + shimmer },
    { "Marching Fifths", "Brick Road", V {
        { "layAChar", 0 }, { "layAColour", 1.0f }, { "layAOct", -1 }, { "layAUnison", 2 }, { "voiceMode", 1 }, { "glide", 0.05f },
        { "cutoff", 700 }, { "reso", 0.45f }, { "filtEnv", 0.55f }, { "fD", 0.25f }, { "fS", 0.1f },
        { "aA", 0.002f }, { "aD", 0.4f }, { "aS", 0.6f }, { "aR", 0.15f }, { "drvOn", 1 }, { "drvAmt", 0.4f } } + roadEcho },

    // ======================================================== EMERALD CITY (glass, gems, shimmering towers)
    { "Emerald Spires", "Emerald City", V {
        { "layAChar", 1 }, { "layAColour", 0.55f }, { "layAUnison", 3 }, { "layADetune", 0.2f }, { "spread", 0.9f } } + bell + shimmer + roadEcho },
    { "Glass Harmonica", "Emerald City", V {
        { "layAChar", 1 }, { "layAColour", 0.2f }, { "layBChar", 7 }, { "layBColour", 0.3f }, { "journey", 0.35f },
        { "aA", 0.25f }, { "aS", 0.9f }, { "aR", 1.5f } } + vibrato + chorus + hall },
    { "Green Spectacles", "Emerald City", V {
        { "layAChar", 1 }, { "layAColour", 0.85f }, { "layAOct", 1 }, { "cutoff", 7000 } } + bell + prism + shimmer },
    { "City Gates Opening", "Emerald City", V {
        { "layAChar", 7 }, { "layAColour", 0.6f }, { "layAUnison", 4 }, { "layADetune", 0.3f },
        { "layBChar", 1 }, { "layBColour", 0.7f }, { "layBOct", 1 }, { "journey", 0.0f }, { "journeyMod", 1.0f },
        { "lfo2Wave", 1 }, { "lfo2Rate", 0.06f }, { "aA", 3.0f }, { "aS", 1.0f }, { "aR", 4.0f } } + heaven },
    { "Jewel Box", "Emerald City", V {
        { "layAChar", 1 }, { "layAColour", 0.4f }, { "layAOct", 1 }, { "layBChar", 8 }, { "layBColour", 0.7f }, { "layBOct", 1 },
        { "journey", 0.5f } } + bell + V { { "dlyOn", 1 }, { "dlyDiv", 5 }, { "dlyFb", 0.5f }, { "dlyPing", 1 }, { "dlyMix", 0.32f } } + hall },

    // ======================================================== TWISTER (spinning, windy, rising)
    { "Into The Funnel", "Twister", V {
        { "layAChar", 0 }, { "layAColour", 0.3f }, { "layAUnison", 5 }, { "layADetune", 0.35f },
        { "cyclone", 0.8f }, { "cycRate", 1.2f }, { "wind", 0.45f }, { "cutoff", 2000 }, { "reso", 0.35f } } + pad + twistFast + hall },
    { "Storm Cellar", "Twister", V {
        { "layAChar", 7 }, { "layAColour", 0.2f }, { "layAOct", -1 }, { "wind", 0.8f }, { "cyclone", 0.5f }, { "cycRate", 0.4f },
        { "cutoff", 900 }, { "reso", 0.2f }, { "aA", 2.0f }, { "aS", 1.0f }, { "aR", 3.0f } } + kansasOld + hall },
    { "Spinning House", "Twister", V {
        { "layAChar", 4 }, { "layAColour", 0.3f }, { "layAUnison", 3 }, { "layADetune", 0.4f }, { "cyclone", 1.0f }, { "cycRate", 3.0f },
        { "cutoff", 1600 }, { "reso", 0.5f }, { "aA", 0.5f }, { "aS", 1.0f }, { "aR", 1.5f } } + twistSlow + roadEcho + room },
    { "Whistling Gale", "Twister", V {
        { "layAChar", 3 }, { "layAColour", 0.7f }, { "layALevel", 0.5f }, { "wind", 1.0f }, { "cyclone", 0.6f }, { "cycRate", 0.25f },
        { "aA", 1.5f }, { "aS", 1.0f }, { "aR", 2.5f } } + longEcho + hall },
    { "Cyclone Arp Spin", "Twister", V {
        { "layAChar", 1 }, { "layAColour", 0.6f }, { "layAUnison", 3 }, { "layADetune", 0.2f }, { "cyclone", 0.7f }, { "cycRate", 5.0f } }
        + pluck + V { { "cutoff", 2400 } } + roadEcho + room },

    // ======================================================== TIN MAN (metal, clockwork, hollow heart)
    { "Hollow Heart", "Tin Man", V {
        { "layAChar", 2 }, { "layAColour", 0.35f } } + bell + hall },
    { "Oil Can Clank", "Tin Man", V {
        { "layAChar", 2 }, { "layAColour", 0.9f }, { "layAOct", -1 }, { "cutoff", 5000 },
        { "aA", 0.001f }, { "aD", 0.5f }, { "aS", 0.0f }, { "aR", 0.4f }, { "drvOn", 1 }, { "drvType", 2 }, { "drvAmt", 0.35f } } + roadEcho + room },
    { "Clockwork Chimes", "Tin Man", V {
        { "layAChar", 2 }, { "layAColour", 0.55f }, { "layAOct", 1 }, { "layBChar", 8 }, { "layBColour", 0.4f }, { "layBOct", 1 }, { "journey", 0.4f } }
        + bell + V { { "dlyOn", 1 }, { "dlyDiv", 2 }, { "dlyFb", 0.35f }, { "dlyPing", 1 }, { "dlyMix", 0.25f } } + room },
    { "Rusted Drone", "Tin Man", V {
        { "layAChar", 2 }, { "layAColour", 0.7f }, { "layAOct", -2 }, { "layAUnison", 3 }, { "layADetune", 0.15f },
        { "cutoff", 1200 }, { "reso", 0.4f }, { "lfo1Wave", 5 }, { "lfo1Rate", 0.2f }, { "lfo1Cutoff", 0.4f } } + pad + jetFlange + hall },
    { "Tin Woodman Pad", "Tin Man", V {
        { "layAChar", 2 }, { "layAColour", 0.2f }, { "layAUnison", 4 }, { "layADetune", 0.3f }, { "layBChar", 7 }, { "layBColour", 0.4f },
        { "journey", 0.5f }, { "cutoff", 3500 } } + pad + ensemble + shimmer },

    // ======================================================== SCARECROW (straw, breath, wooden flutes)
    { "Straw Flute", "Scarecrow", V {
        { "layAChar", 3 }, { "layAColour", 0.35f }, { "cutoff", 5000 }, { "aA", 0.06f }, { "aS", 0.9f }, { "aR", 0.3f } } + vibrato + roadEcho + room },
    { "Cornfield Panpipes", "Scarecrow", V {
        { "layAChar", 3 }, { "layAColour", 0.7f }, { "layAUnison", 2 }, { "layADetune", 0.15f },
        { "aA", 0.03f }, { "aD", 0.6f }, { "aS", 0.5f }, { "aR", 0.5f } } + kansasOld + hall },
    { "Stuffed With Straw", "Scarecrow", V {
        { "layAChar", 3 }, { "layAColour", 0.5f }, { "layBChar", 5 }, { "layBColour", 0.3f }, { "journey", 0.3f },
        { "voiceMode", 2 }, { "glide", 0.12f }, { "aA", 0.05f }, { "aS", 1.0f }, { "aR", 0.4f } } + vibrato + longEcho + hall },
    { "Rustling Scarecrow", "Scarecrow", V {
        { "layAChar", 3 }, { "layAColour", 1.0f }, { "layAOct", -1 }, { "wind", 0.3f }, { "cutoff", 2500 } } + pad + prism + hall },
    { "Crow Calls", "Scarecrow", V {
        { "layAChar", 3 }, { "layAColour", 0.8f }, { "layAOct", 1 }, { "voiceMode", 1 }, { "glide", 0.2f },
        { "lfo1Rate", 7.0f }, { "lfo1Pitch", 0.4f }, { "aA", 0.01f }, { "aD", 0.3f }, { "aS", 0.3f }, { "aR", 0.2f } } + roadEcho + room },

    // ======================================================== LION (growl, roar, brave basses)
    { "Courage Bass", "Lion", V {
        { "layAChar", 4 }, { "layAColour", 0.4f }, { "layAOct", -1 }, { "voiceMode", 1 }, { "glide", 0.03f },
        { "cutoff", 450 }, { "reso", 0.35f }, { "filtEnv", 0.5f }, { "fD", 0.3f }, { "fS", 0.2f },
        { "aA", 0.002f }, { "aS", 0.9f }, { "aR", 0.15f } } },
    { "Brave At Last", "Lion", V {
        { "layAChar", 4 }, { "layAColour", 0.85f }, { "layAOct", -1 }, { "voiceMode", 2 }, { "glide", 0.06f },
        { "cutoff", 900 }, { "reso", 0.45f }, { "filtEnv", 0.45f }, { "fD", 0.4f }, { "fS", 0.3f },
        { "aA", 0.002f }, { "aS", 1.0f }, { "aR", 0.2f }, { "drvOn", 1 }, { "drvType", 1 }, { "drvAmt", 0.45f } } + room },
    { "Roaring Lead", "Lion", V {
        { "layAChar", 4 }, { "layAColour", 0.7f }, { "layAUnison", 2 }, { "layADetune", 0.12f }, { "voiceMode", 2 }, { "glide", 0.1f },
        { "cutoff", 2800 }, { "reso", 0.3f }, { "aA", 0.005f }, { "aS", 1.0f }, { "aR", 0.25f },
        { "drvOn", 1 }, { "drvType", 1 }, { "drvAmt", 0.5f } } + vibrato + twistSlow + roadEcho },
    { "Cowardly Purr", "Lion", V {
        { "layAChar", 4 }, { "layAColour", 0.15f }, { "layAOct", -1 }, { "layAUnison", 3 }, { "layADetune", 0.2f },
        { "cutoff", 600 }, { "reso", 0.2f }, { "lfo1Rate", 0.4f }, { "lfo1Cutoff", 0.3f } } + pad + chorus + hall },
    { "Jungle Of Doubt", "Lion", V {
        { "layAChar", 4 }, { "layAColour", 0.6f }, { "layAOct", -2 }, { "layBChar", 6 }, { "layBColour", 0.5f }, { "layBOct", -1 },
        { "journey", 0.4f }, { "journeyMod", 0.6f }, { "lfo2Rate", 0.3f }, { "cutoff", 1100 }, { "reso", 0.4f } } + pad + longEcho + hall },

    // ======================================================== MUNCHKINS (tiny choirs, sing-song)
    { "Munchkin Choir", "Munchkins", V {
        { "layAChar", 5 }, { "layAColour", 0.2f }, { "layAUnison", 5 }, { "layADetune", 0.3f }, { "layAOct", 1 } }
        + V { { "aA", 0.15f }, { "aS", 0.9f }, { "aR", 0.8f } } + vibrato + ensemble + hall },
    { "Little Voices Ooh", "Munchkins", V {
        { "layAChar", 5 }, { "layAColour", 0.95f }, { "layAUnison", 4 }, { "layADetune", 0.25f }, { "layAOct", 1 } }
        + pad + vibrato + shimmer },
    { "Village Bells", "Munchkins", V {
        { "layAChar", 8 }, { "layAColour", 0.5f }, { "layAOct", 1 }, { "layBChar", 5 }, { "layBColour", 0.4f }, { "layBOct", 1 }, { "journey", 0.35f } }
        + bell + roadEcho + hall },
    { "Candy Cane Strut", "Munchkins", V {
        { "layAChar", 5 }, { "layAColour", 0.5f }, { "layAOct", 1 }, { "voiceMode", 2 }, { "glide", 0.07f },
        { "lfo2Wave", 0 }, { "lfo2Rate", 3.0f }, { "lfo2Colour", 0.6f }, { "aA", 0.01f }, { "aS", 1.0f }, { "aR", 0.3f } } + roadEcho + room },
    { "Tiny Lullaby", "Munchkins", V {
        { "layAChar", 5 }, { "layAColour", 0.75f }, { "layAUnison", 3 }, { "layAOct", 1 }, { "layBChar", 7 }, { "layBColour", 0.5f },
        { "journey", 0.45f }, { "poppies", 0.3f } } + pad + kansasOld + hall },

    // ======================================================== WITCH (dark, sneering, crackling)
    { "Wicked Ring", "Witch", V {
        { "layAChar", 6 }, { "layAColour", 0.75f }, { "layAOct", -1 }, { "cutoff", 2200 }, { "reso", 0.3f } } + pad + jetFlange + hall },
    { "Crystal Ball Hourglass", "Witch", V {
        { "layAChar", 6 }, { "layAColour", 0.5f }, { "layBChar", 1 }, { "layBColour", 0.9f }, { "layBSemi", 6 }, { "journey", 0.4f },
        { "lfo1Wave", 4 }, { "lfo1Rate", 4.0f }, { "lfo1Cutoff", 0.35f }, { "cutoff", 1800 }, { "reso", 0.55f } } + pad + longEcho + shimmer },
    { "Broomstick Skywriter", "Witch", V {
        { "layAChar", 6 }, { "layAColour", 0.4f }, { "voiceMode", 2 }, { "glide", 0.25f },
        { "cutoff", 2600 }, { "reso", 0.5f }, { "lfo1Rate", 6.0f }, { "lfo1Pitch", 0.3f },
        { "aA", 0.01f }, { "aS", 1.0f }, { "aR", 0.4f } } + twistSlow + longEcho + hall },
    { "Melting Away", "Witch", V {
        { "layAChar", 6 }, { "layAColour", 0.9f }, { "layAOct", -1 }, { "layAUnison", 3 }, { "layADetune", 0.4f },
        { "poppies", 1.0f }, { "cutoff", 3000 }, { "reso", 0.3f } } + pad + prismFast + hall },
    { "Cackle Bass", "Witch", V {
        { "layAChar", 6 }, { "layAColour", 0.6f }, { "layAOct", -2 }, { "voiceMode", 1 },
        { "cutoff", 500 }, { "reso", 0.55f }, { "filtEnv", 0.6f }, { "fD", 0.25f }, { "fS", 0.1f },
        { "aA", 0.002f }, { "aS", 0.8f }, { "aR", 0.12f }, { "drvOn", 1 }, { "drvType", 2 }, { "drvAmt", 0.3f } } },

    // ======================================================== FLYING MONKEYS (chatter, swarm, chaos)
    { "Monkey Swarm", "Flying Monkeys", V {
        { "layAChar", 9 }, { "layAColour", 0.6f }, { "layAUnison", 4 }, { "layADetune", 0.4f }, { "cyclone", 0.6f }, { "cycRate", 2.0f },
        { "cutoff", 4000 } } + pad + roadEcho + hall },
    { "Chattering Wings", "Flying Monkeys", V {
        { "layAChar", 9 }, { "layAColour", 0.9f }, { "layAOct", 1 } } + pluck + V { { "cutoff", 3000 } } + longEcho + room },
    { "Dark Sky Raid", "Flying Monkeys", V {
        { "layAChar", 9 }, { "layAColour", 0.4f }, { "layAOct", -1 }, { "layBChar", 6 }, { "layBColour", 0.7f }, { "layBOct", -1 },
        { "journey", 0.5f }, { "journeyMod", 1.0f }, { "lfo2Wave", 4 }, { "lfo2Rate", 6.0f }, { "wind", 0.4f },
        { "cutoff", 1500 }, { "reso", 0.4f } } + pad + twistFast + hall },
    { "Winged Chaos Lead", "Flying Monkeys", V {
        { "layAChar", 9 }, { "layAColour", 0.25f }, { "voiceMode", 2 }, { "glide", 0.08f },
        { "aA", 0.005f }, { "aS", 1.0f }, { "aR", 0.3f }, { "drvOn", 1 }, { "drvAmt", 0.3f } } + vibrato + roadEcho + room },

    // ======================================================== POPPY FIELD (drowsy, soft, sleepy)
    { "Sleepy Poppies", "Poppy Field", V {
        { "layAChar", 7 }, { "layAColour", 0.4f }, { "layAUnison", 4 }, { "layADetune", 0.3f }, { "poppies", 0.7f } } + pad + chorus + hall },
    { "Field Of Red", "Poppy Field", V {
        { "layAChar", 7 }, { "layAColour", 0.7f }, { "layAUnison", 3 }, { "layBChar", 5 }, { "layBColour", 0.85f }, { "journey", 0.3f },
        { "poppies", 0.4f }, { "cutoff", 4000 } } + pad + prism + shimmer },
    { "Heavy Eyelids", "Poppy Field", V {
        { "layAChar", 7 }, { "layAColour", 0.25f }, { "layAOct", -1 }, { "poppies", 1.0f }, { "lfo1Wave", 5 }, { "lfo1Rate", 0.3f }, { "lfo1Pitch", 0.15f } }
        + pad + kansasOld + longEcho + hall },
    { "Snowfall Wakes You", "Poppy Field", V {
        { "layAChar", 7 }, { "layAColour", 0.5f }, { "layBChar", 8 }, { "layBColour", 0.8f }, { "layBOct", 1 },
        { "journey", 0.0f }, { "journeyMod", 1.0f }, { "lfo2Rate", 0.08f } } + pad + heaven },
    { "Poppy Keys", "Poppy Field", V {
        { "layAChar", 7 }, { "layAColour", 0.8f }, { "filtVel", 0.5f }, { "cutoff", 3000 },
        { "aA", 0.002f }, { "aD", 1.5f }, { "aS", 0.3f }, { "aR", 0.6f } } + twistSlow + room },

    // ======================================================== SILVER SHOES (sparkle, twinkle, magic)
    { "Three Heel Taps", "Silver Shoes", V {
        { "layAChar", 8 }, { "layAColour", 0.75f }, { "layAOct", 1 } } + bell + V { { "dlyOn", 1 }, { "dlyDiv", 4 }, { "dlyFb", 0.5f }, { "dlyPing", 1 }, { "dlyMix", 0.3f } } + shimmer },
    { "Homeward Heels", "Silver Shoes", V {
        { "layAChar", 8 }, { "layAColour", 0.45f }, { "layAUnison", 3 }, { "layADetune", 0.15f } } + pad + vibrato + kansasOld + hall },
    { "Twinkle Dust", "Silver Shoes", V {
        { "layAChar", 8 }, { "layAColour", 1.0f }, { "layAOct", 2 }, { "aA", 0.5f }, { "aS", 1.0f }, { "aR", 3.0f } } + prismFast + heaven },
    { "Silver Music Box", "Silver Shoes", V {
        { "layAChar", 8 }, { "layAColour", 0.3f }, { "layAOct", 1 }, { "aA", 0.001f }, { "aD", 1.2f }, { "aS", 0.0f }, { "aR", 1.0f } }
        + kansasOld + room },

    // ======================================================== RAINBOW DREAMS (multicolour psychedelic textures)
    { "Beyond The Storm", "Rainbow Dreams", V {
        { "layAChar", 7 }, { "layAColour", 0.6f }, { "layAUnison", 5 }, { "layADetune", 0.35f },
        { "layBChar", 1 }, { "layBColour", 0.5f }, { "layBOct", 1 }, { "journey", 0.4f }, { "journeyMod", 0.6f }, { "lfo2Rate", 0.1f } }
        + pad + prism + ensemble + heaven },
    { "Prism Wash", "Rainbow Dreams", V {
        { "layAChar", 0 }, { "layAColour", 0.3f }, { "layAUnison", 5 }, { "layADetune", 0.45f }, { "cyclone", 0.3f }, { "cycRate", 0.3f },
        { "cutoff", 3000 } } + pad + prismFast + jetFlange + hall },
    { "Kansas To Colour", "Rainbow Dreams", V {
        { "layAChar", 3 }, { "layAColour", 0.4f }, { "layBChar", 8 }, { "layBColour", 0.8f }, { "layBOct", 1 },
        { "journey", 0.0f }, { "journeyMod", 1.0f }, { "lfo2Wave", 0 }, { "lfo2Rate", 0.05f } } + pad + prism + shimmer },
    { "Melting Rainbow Keys", "Rainbow Dreams", V {
        { "layAChar", 1 }, { "layAColour", 0.35f }, { "layBChar", 7 }, { "layBColour", 0.6f }, { "journey", 0.5f },
        { "aA", 0.002f }, { "aD", 1.4f }, { "aS", 0.4f }, { "aR", 0.8f }, { "lfo1Wave", 5 }, { "lfo1Rate", 0.7f }, { "lfo1Pitch", 0.2f } }
        + prism + longEcho + hall },
    { "Bluebirds Fly", "Rainbow Dreams", V {
        { "layAChar", 5 }, { "layAColour", 0.9f }, { "layAOct", 1 }, { "layAUnison", 3 }, { "layBChar", 8 }, { "layBColour", 0.6f }, { "layBOct", 1 },
        { "journey", 0.3f }, { "lfo2Colour", 0.4f }, { "lfo2Rate", 0.2f } } + pad + vibrato + roadEcho + heaven },

    // ======================================================== KANSAS (grey, dusty, homespun)
    { "Farmhouse Organ", "Kansas", V {
        { "layAChar", 7 }, { "layAColour", 0.9f }, { "aA", 0.005f }, { "aS", 1.0f }, { "aR", 0.1f }, { "ampVel", 0.0f } } + twistSlow + kansasOld + room },
    { "Sepia Lullaby", "Kansas", V {
        { "layAChar", 8 }, { "layAColour", 0.2f }, { "layBChar", 3 }, { "layBColour", 0.3f }, { "journey", 0.4f } } + bell + kansasOld + hall },
    { "Dusty Gramophone Strings", "Kansas", V {
        { "layAChar", 0 }, { "layAColour", 0.1f }, { "layAUnison", 5 }, { "layADetune", 0.3f }, { "cutoff", 2500 } } + pad + ensemble
        + V { { "knsOn", 1 }, { "knsWow", 0.5f }, { "knsBand", 0.75f }, { "knsDust", 0.6f }, { "knsMono", 0.9f } } + room },
    { "Prairie Wind", "Kansas", V {
        { "layAChar", 3 }, { "layAColour", 0.9f }, { "layALevel", 0.3f }, { "wind", 0.7f }, { "cycRate", 0.1f } } + pad + kansasOld + hall },
};
} // namespace

const std::vector<FactoryPreset>& getFactoryPresets()
{
    // Append the loudness-matched master level to every factory preset.
    static const std::vector<FactoryPreset> levelled = []
    {
        auto b = bank;
        for (auto& p : b)
        {
            auto it = factoryPresetLevels.find (p.name);
            if (it != factoryPresetLevels.end()) p.values.push_back ({ "master", it->second });
        }
        return b;
    }();
    return levelled;
}
