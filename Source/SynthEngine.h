#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "Params.h"

// ============================================================================
//  Emerald Haze synth engine.
//  12 voices. Each voice plays two LAYERS (A and B); each layer is one of ten
//  "characters" (own synthesis model, morphed by its Colour knob) with up to
//  5 unison copies. The JOURNEY knob crossfades A into B. CYCLONE spins the
//  unison copies around the stereo field with a pitch swirl and filter spin,
//  WIND adds a whistling key-tracked gust, POPPIES makes held notes drowsy
//  (they sag in pitch and darken as they are held).
//  Then: multimode ZDF filter, filter + amp ADSR, two global LFOs.
// ============================================================================

namespace dsp
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;
    constexpr float pi    = juce::MathConstants<float>::pi;

    inline float polyBlep (float t, float dt)
    {
        if (t < dt)        { t /= dt;            return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    inline float wrap (float x) { return x - std::floor (x); }
    inline float sine (float ph) { return std::sin (twoPi * ph); }

    struct Rng
    {
        uint32_t s = 0x12345678u;
        void seed (uint32_t v) { s = v ? v : 1u; }
        float next() // -1..1
        {
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            return (float) (s & 0xffffff) / (float) 0x7fffff - 1.0f;
        }
    };

    // Zero-delay-feedback state variable filter (Simper / Zavalishin)
    struct SVF
    {
        float ic1 = 0, ic2 = 0;
        void reset() { ic1 = ic2 = 0; }
        struct Out { float lp, bp, hp; };
        Out process (float v0, float g, float k)
        {
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1, a3 = g * a2;
            const float v3 = v0 - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            return { v2, v1, v0 - k * v1 - v2 };
        }
    };
}

// Snapshot of all synth (non-FX) parameters, taken once per block.
struct SynthParams
{
    struct Layer { int chr; float pitchOffset; float colour, level; int unison; float detune; } layer[2];
    float journey, journeyMod, cyclone, cycRate, wind, poppies;
    int filtMode; float cutoff, reso, filtDrive, filtEnv, keyTrack, filtVel;
    float fA, fD, fS, fR, aA, aD, aS, aR, ampVel;
    float lfo1Delay, lfo1Pitch, lfo1Cutoff, lfo2Colour, lfo2Pan, lfo2Amp;
    int voiceMode; float glide, drift, spread; int bendRange;
};

class Voice
{
public:
    static constexpr int maxUnison = 5;

    void prepare (double sr)
    {
        sampleRate = (float) sr;
        ampEnv.setSampleRate (sr);
        filtEnv.setSampleRate (sr);
        strikeCoef = std::exp (-1.0f / (0.35f * sampleRate));
    }

    bool isActive() const  { return ampEnv.isActive(); }
    bool isGateOn() const  { return gate; }
    int  getNote() const   { return note; }

    void start (int midiNote, float vel, const SynthParams& p, float glideFromPitch, bool retrigger, uint32_t seed)
    {
        const bool wasActive = isActive();
        note = midiNote;
        velocity = vel;
        gate = true;
        sustained = false;
        targetPitch = (float) midiNote;
        if (! wasActive || p.glide <= 0.0005f)
            pitch = (p.glide > 0.0005f && glideFromPitch >= 0.0f) ? glideFromPitch : targetPitch;

        if (! wasActive)
        {
            rng.seed (seed);
            driftVal = rng.next() * 0.5f;
            for (auto& l : osc) for (auto& u : l) { u.ph = rng.next() * 0.5f + 0.5f; u.ph2 = 0; u.ph3 = rng.next() * 0.5f + 0.5f; u.ratio = 1.5f; }
            for (auto& f : filt) f.reset();
            for (auto& f : post) f.reset();
            for (auto& f : windFilt) f.reset();
            cycPhase = rng.next() * 0.5f + 0.5f;
            lfoFadeSamples = 0;
        }
        if (retrigger || ! wasActive)
        {
            setEnvParams (p);
            ampEnv.noteOn();
            filtEnv.noteOn();
            lfoFadeSamples = 0;
            strike = 1.0f;
            heldSamples = 0;
        }
        age = 0;
    }

    void setTarget (int midiNote) { note = midiNote; targetPitch = (float) midiNote; }
    void release() { gate = false; ampEnv.noteOff(); filtEnv.noteOff(); }
    void kill() { ampEnv.reset(); filtEnv.reset(); gate = false; }

    void setEnvParams (const SynthParams& p)
    {
        ampEnv.setParameters ({ p.aA, p.aD, p.aS, p.aR });
        filtEnv.setParameters ({ p.fA, p.fD, p.fS, p.fR });
    }

    bool sustained = false;
    uint64_t age = 0;

    void render (float* outL, float* outR, int start, int num, const SynthParams& p,
                 const float* lfo1, const float* lfo2, float bend, float modWheel)
    {
        if (! isActive()) return;
        setEnvParams (p);

        const float nyq = sampleRate * 0.5f;
        const float glideCoef = p.glide > 0.0005f ? 1.0f - std::exp (-1.0f / (p.glide * 0.35f * sampleRate)) : 1.0f;
        const float velAmp = 1.0f - p.ampVel + p.ampVel * velocity;
        const float lfoFadeLen = p.lfo1Delay * sampleRate;
        const float vibDepth = juce::jlimit (0.0f, 1.0f, p.lfo1Pitch + modWheel * 0.5f);
        const float vibSemis = vibDepth * vibDepth * 2.0f;
        const float cycInc = p.cycRate * (0.4f + 1.2f * p.cyclone) / sampleRate;

        for (int i = start; i < start + num; ++i)
        {
            pitch += (targetPitch - pitch) * glideCoef;
            driftVal = juce::jlimit (-1.0f, 1.0f, driftVal + rng.next() * 0.0004f);
            strike *= strikeCoef;
            cycPhase = dsp::wrap (cycPhase + cycInc);
            ++heldSamples;

            const float lfoFade = lfoFadeLen > 1.0f ? juce::jmin (1.0f, (float) lfoFadeSamples / lfoFadeLen) : 1.0f;
            ++lfoFadeSamples;

            // Poppies: the longer a note is held, the drowsier it gets
            const float drowsy = p.poppies * juce::jmin (1.0f, (float) heldSamples / (4.0f * sampleRate));

            const float pitchMod = bend * (float) p.bendRange
                                 + lfo1[i] * vibSemis * lfoFade
                                 + driftVal * p.drift * 0.15f
                                 - drowsy * 0.6f
                                 + drowsy * 0.12f * std::sin (dsp::twoPi * cycPhase * 0.37f);

            const float jm = juce::jlimit (0.0f, 1.0f, p.journey + lfo2[i] * p.journeyMod * 0.5f);
            const float layerGain[2] = { std::cos (jm * dsp::pi * 0.5f), std::sin (jm * dsp::pi * 0.5f) };

            float sumL = 0, sumR = 0;
            for (int l = 0; l < 2; ++l)
            {
                const auto& lp = p.layer[l];
                const float g = lp.level * layerGain[l];
                if (g <= 0.0001f) continue;

                const float colour = juce::jlimit (0.0f, 1.0f, lp.colour + lfo2[i] * p.lfo2Colour * 0.5f);
                const float freq = 440.0f * std::exp2 ((pitch + lp.pitchOffset + pitchMod - 69.0f) / 12.0f);
                const int n = lp.unison;
                const float norm = 1.0f / std::sqrt ((float) n);
                float lL = 0, lR = 0;

                for (int u = 0; u < n; ++u)
                {
                    const float pos = n == 1 ? 0.0f : (2.0f * u / (n - 1) - 1.0f);
                    const float rot = std::sin (dsp::twoPi * (cycPhase + (float) u / (float) n));
                    const float cents = pos * lp.detune * lp.detune * 50.0f
                                      + (u == 0 ? 0.0f : 1.3f * (float) u * lp.detune)
                                      + p.cyclone * p.cyclone * 18.0f * rot;
                    const float f = freq * std::exp2 (cents / 1200.0f);
                    const float s = character (lp.chr, osc[l][u], f, colour);

                    // static spread position, swept into a rotating one by the cyclone
                    const float panPos = (1.0f - p.cyclone) * pos * p.spread + p.cyclone * rot * juce::jmax (0.6f, p.spread);
                    const float pan = 0.5f + 0.5f * juce::jlimit (-1.0f, 1.0f, panPos);
                    lL += s * std::cos (pan * dsp::pi * 0.5f) * 1.41421f;
                    lR += s * std::sin (pan * dsp::pi * 0.5f) * 1.41421f;
                }
                lL *= norm; lR *= norm;
                postProcess (l, lp.chr, lL, lR, freq, colour, nyq);
                sumL += lL * g;
                sumR += lR * g;
            }

            // Wind: whistling band-passed gust that follows the note and the cyclone
            if (p.wind > 0.0001f)
            {
                const float wf = juce::jlimit (80.0f, nyq * 0.8f, 440.0f * std::exp2 ((pitch - 57.0f) / 12.0f)
                                               * std::exp2 (1.2f * std::sin (dsp::twoPi * cycPhase * 0.5f) + 0.6f * driftVal));
                const float g = std::tan (dsp::pi * wf / sampleRate);
                const float amt = p.wind * p.wind * 1.4f;
                sumL += windFilt[0].process (rng.next(), g, 0.08f).bp * amt;
                sumR += windFilt[1].process (rng.next(), g, 0.08f).bp * amt;
            }

            // ---- Filter
            const float fe = filtEnv.getNextSample();
            const float octs = p.filtEnv * 6.0f * fe
                             + lfo1[i] * p.lfo1Cutoff * 3.0f * lfoFade
                             + p.keyTrack * (pitch - 60.0f) / 12.0f
                             + p.filtVel * (velocity - 0.7f) * 3.0f
                             + p.cyclone * 1.2f * std::sin (dsp::twoPi * cycPhase)
                             - drowsy * 2.5f;
            const float fc = juce::jlimit (16.0f, nyq * 0.92f, p.cutoff * std::exp2 (octs));
            const float g = std::tan (dsp::pi * fc / sampleRate);
            const float k = 2.0f - 1.96f * p.reso;

            if (p.filtDrive > 0.001f)
            {
                const float d = 1.0f + p.filtDrive * 5.0f;
                const float nn = 1.0f / std::tanh (d);
                sumL = std::tanh (sumL * d) * nn;
                sumR = std::tanh (sumR * d) * nn;
            }

            float yL, yR;
            switch (p.filtMode)
            {
                case 0:
                    yL = filt[1].process (filt[0].process (sumL, g, 1.414f).lp, g, k).lp;
                    yR = filt[3].process (filt[2].process (sumR, g, 1.414f).lp, g, k).lp;
                    break;
                case 1: yL = filt[0].process (sumL, g, k).lp; yR = filt[2].process (sumR, g, k).lp; break;
                case 2: yL = filt[0].process (sumL, g, k).bp * 1.5f; yR = filt[2].process (sumR, g, k).bp * 1.5f; break;
                case 3: yL = filt[0].process (sumL, g, k).hp; yR = filt[2].process (sumR, g, k).hp; break;
                default:
                {
                    auto a = filt[0].process (sumL, g, k); auto b = filt[2].process (sumR, g, k);
                    yL = a.lp + a.hp; yR = b.lp + b.hp; break;
                }
            }

            // ---- Amp
            const float env = ampEnv.getNextSample();
            const float amp = env * velAmp * (1.0f - p.lfo2Amp * 0.5f * (1.0f - lfo2[i]));
            float panL = 1.0f, panR = 1.0f;
            if (p.lfo2Pan > 0.0001f)
            {
                const float pan = 0.5f + 0.5f * lfo2[i] * p.lfo2Pan;
                panL = std::cos (pan * dsp::pi * 0.5f) * 1.41421f;
                panR = std::sin (pan * dsp::pi * 0.5f) * 1.41421f;
            }
            outL[i] += yL * amp * panL * 0.3f;
            outR[i] += yR * amp * panR * 0.3f;

            if (! ampEnv.isActive()) break;
        }
        age += (uint64_t) num;
    }

private:
    struct OscState { float ph = 0, ph2 = 0, ph3 = 0, ratio = 1.5f; };

    float saw (float ph, float dt) const { return 2.0f * ph - 1.0f - dsp::polyBlep (ph, dt); }

    // One sample of one unison copy of a character. Advances that copy's phases.
    float character (int chr, OscState& o, float f, float c)
    {
        const float dt = juce::jmin (0.45f, f / sampleRate);
        float s = 0.0f;
        switch (chr)
        {
            case 0: // Brick Road: golden brass-like saw, Colour stacks a fifth and an octave in
            {
                const float dt2 = juce::jmin (0.45f, dt * 1.4983f);
                s = saw (o.ph, dt) + c * 0.6f * saw (o.ph2, dt2) + c * c * 0.35f * saw (o.ph3, juce::jmin (0.45f, dt * 2.0f));
                s /= 1.0f + c * 0.7f;
                o.ph2 = dsp::wrap (o.ph2 + dt2);
                o.ph3 = dsp::wrap (o.ph3 + juce::jmin (0.45f, dt * 2.0f));
                break;
            }
            case 1: // Emerald Glass: glassy FM bell, Colour = brilliance; strike decays the brightness
            {
                const float idx = (0.3f + 2.7f * c) * (0.25f + 0.75f * strike);
                s = std::sin (dsp::twoPi * o.ph + idx * dsp::sine (o.ph2)) * 0.8f + 0.2f * dsp::sine (o.ph3);
                o.ph2 = dsp::wrap (o.ph2 + juce::jmin (0.45f, dt * 3.5f));
                o.ph3 = dsp::wrap (o.ph3 + juce::jmin (0.45f, dt * 2.0f));
                break;
            }
            case 2: // Tin Man: hollow metallic clang from inharmonic FM, Colour = how much metal
            {
                const float idx = (0.4f + 3.2f * c) * (0.35f + 0.65f * strike);
                s = std::sin (dsp::twoPi * o.ph + idx * (dsp::sine (o.ph2) + 0.5f * dsp::sine (o.ph3)));
                o.ph2 = dsp::wrap (o.ph2 + juce::jmin (0.45f, dt * 1.4142f));
                o.ph3 = dsp::wrap (o.ph3 + juce::jmin (0.45f, dt * 2.7613f));
                break;
            }
            case 3: // Scarecrow: hollow straw reed (breath noise added in postProcess)
            {
                const float tri = 4.0f * std::abs (o.ph - 0.5f) - 1.0f;
                s = tri * 0.8f + 0.25f * dsp::sine (dsp::wrap (o.ph * 2.0f)) + 0.1f * dsp::sine (dsp::wrap (o.ph * 3.0f));
                break;
            }
            case 4: // Lion: growling saw with a sub and a throaty flutter; Colour = roar
            {
                const float growl = 1.0f + c * 0.6f * dsp::sine (o.ph3) * (0.6f + 0.4f * rng.next());
                s = saw (o.ph, dt) + 0.5f * dsp::sine (o.ph2);
                const float d = 1.0f + c * 5.0f;
                s = std::tanh (s * d * growl) / std::tanh (d) * 0.8f;
                o.ph2 = dsp::wrap (o.ph2 + dt * 0.5f);
                o.ph3 = dsp::wrap (o.ph3 + (24.0f + 14.0f * c) / sampleRate);
                break;
            }
            case 5: // Munchkin Choir: buzz source, formant-filtered in postProcess
                s = saw (o.ph, dt);
                break;
            case 6: // Witch: hollow pulse, Colour brings in a tritone ring-mod sneer
            {
                const float pw = 0.5f - 0.35f * c;
                float p = (o.ph < pw ? 1.0f : -1.0f) + dsp::polyBlep (o.ph, dt);
                p -= dsp::polyBlep (dsp::wrap (o.ph - pw), dt);
                s = p * (1.0f - 0.6f * c) + p * dsp::sine (o.ph2) * c * 0.9f;
                o.ph2 = dsp::wrap (o.ph2 + juce::jmin (0.45f, dt * 1.4142f));
                break;
            }
            case 7: // Poppy Field: soft drowsy additive pad, Colour = brightness
            {
                const float tilt = 3.0f - 2.2f * c;
                float acc = 0, nrm = 0;
                for (int h = 1; h <= 6; ++h)
                {
                    if (f * (float) h > sampleRate * 0.45f) break;
                    const float a = std::pow ((float) h, -tilt);
                    acc += a * dsp::sine (dsp::wrap (o.ph * (float) h));
                    nrm += a;
                }
                s = acc / juce::jmax (0.5f, nrm) * 1.2f;
                break;
            }
            case 8: // Silver Shoes: twinkling octave partials, Colour = sparkle
            {
                static const float partial[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 8.0f };
                static const float twRate[5]  = { 0.0f, 3.1f, 4.7f, 6.3f, 9.1f };
                float acc = 0;
                for (int k = 0; k < 5; ++k)
                {
                    if (f * partial[k] > sampleRate * 0.45f) break;
                    const float base = k == 0 ? 1.0f : (0.5f * c + 0.15f) / (float) k + 0.4f * strike * c;
                    const float tw = k == 0 ? 1.0f : 0.5f + 0.5f * dsp::sine (dsp::wrap (o.ph3 * twRate[k] + 0.13f * (float) k));
                    acc += base * tw * dsp::sine (dsp::wrap (o.ph * partial[k]));
                }
                s = acc * 0.6f;
                o.ph3 = dsp::wrap (o.ph3 + (0.3f + 1.5f * c) / sampleRate);
                break;
            }
            default: // Flying Monkeys: chattering FM, the modulator ratio jumps around; Colour = chatter speed
            {
                static const float ratios[6] = { 0.5f, 1.0f, 1.5f, 2.37f, 3.1f, 4.0f };
                o.ph3 += (4.0f + 26.0f * c) / sampleRate;
                if (o.ph3 >= 1.0f) { o.ph3 -= 1.0f; o.ratio = ratios[(int) ((rng.next() * 0.5f + 0.5f) * 5.99f)]; }
                const float chatter = 0.55f + 0.45f * std::sin (dsp::twoPi * o.ph3);
                s = std::sin (dsp::twoPi * o.ph + (1.5f + 2.5f * c) * chatter * dsp::sine (o.ph2)) * (0.6f + 0.4f * chatter);
                o.ph2 = dsp::wrap (o.ph2 + juce::jmin (0.45f, dt * o.ratio));
                break;
            }
        }
        o.ph += dt;
        if (o.ph >= 1.0f) o.ph -= 1.0f;
        return s;
    }

    // Per-layer processing on the summed unison signal.
    void postProcess (int l, int chr, float& L, float& R, float freq, float c, float nyq)
    {
        dsp::SVF* f = &post[l * 6];
        if (chr == 3) // Scarecrow breath: band-passed noise around the 2nd harmonic
        {
            const float g = std::tan (dsp::pi * juce::jmin (freq * 2.0f, nyq * 0.8f) / sampleRate);
            const float amt = c * 0.9f;
            L += f[0].process (rng.next(), g, 0.5f).bp * amt;
            R += f[1].process (rng.next(), g, 0.5f).bp * amt;
        }
        else if (chr == 5) // Munchkin choir: three formants, shifted up for small voices; Colour = vowel
        {
            static const float vowels[5][3] = { { 800, 1150, 2900 }, { 400, 1600, 2700 }, { 350, 2300, 3000 },
                                                { 450, 800, 2830 }, { 325, 700, 2530 } };
            const float vpos = c * 3.999f;
            const int v0 = (int) vpos; const float vf = vpos - (float) v0;
            float outL = 0, outR = 0;
            for (int k = 0; k < 3; ++k)
            {
                const float fk = 1.35f * (vowels[v0][k] + (vowels[v0 + 1][k] - vowels[v0][k]) * vf);
                const float g = std::tan (dsp::pi * juce::jmin (fk, nyq * 0.9f) / sampleRate);
                const float gain = k == 0 ? 1.0f : (k == 1 ? 0.7f : 0.35f);
                if (k == 2) { outL += f[4].process (L, g, 0.2f).bp * gain; outR += f[5].process (R, g, 0.2f).bp * gain; }
                else        { outL += f[k * 2].process (L, g, 0.15f).bp * gain; outR += f[k * 2 + 1].process (R, g, 0.15f).bp * gain; }
            }
            L = outL * 0.8f; R = outR * 0.8f;
        }
    }

    float sampleRate = 44100.0f;
    int note = -1;
    float velocity = 1.0f;
    bool gate = false;
    float pitch = 60.0f, targetPitch = 60.0f;
    float driftVal = 0.0f, strike = 0.0f, strikeCoef = 0.9999f, cycPhase = 0.0f;
    int64_t lfoFadeSamples = 0, heldSamples = 0;
    OscState osc[2][maxUnison];
    dsp::SVF filt[4], post[12], windFilt[2];
    dsp::Rng rng;
    juce::ADSR ampEnv, filtEnv;
};

// ---------------------------------------------------------------------------
class GlobalLfo
{
public:
    void prepare (double sr) { sampleRate = (float) sr; }
    void reset() { phase = 0; }
    float next (int wave, float rate)
    {
        phase += rate / sampleRate;
        bool wrapped = false;
        if (phase >= 1.0f) { phase -= 1.0f; wrapped = true; }
        if (wrapped) { prevSh = curSh; curSh = rng.next(); }
        switch (wave)
        {
            case 0: return std::sin (dsp::twoPi * phase);
            case 1: return 4.0f * std::abs (phase - 0.5f) - 1.0f;
            case 2: return 1.0f - 2.0f * phase;
            case 3: return phase < 0.5f ? 1.0f : -1.0f;
            case 4: return curSh;
            default:
            {
                const float t = phase * phase * (3.0f - 2.0f * phase);
                return prevSh + (curSh - prevSh) * t;
            }
        }
    }
private:
    float sampleRate = 44100.0f, phase = 0.0f, curSh = 0.0f, prevSh = 0.0f;
    dsp::Rng rng;
};

// ---------------------------------------------------------------------------
class SynthEngine
{
public:
    static constexpr int numVoices = 12;

    explicit SynthEngine (juce::AudioProcessorValueTreeState& s) : apvts (s)
    {
        auto get = [&] (const juce::String& id) { auto* p = apvts.getRawParameterValue (id); jassert (p != nullptr); return p; };
        for (int l = 0; l < 2; ++l)
        {
            auto pre = juce::String ("lay") + (l == 0 ? "A" : "B");
            lay[l] = { get (pre + "Char"), get (pre + "Oct"), get (pre + "Semi"), get (pre + "Fine"),
                       get (pre + "Colour"), get (pre + "Level"), get (pre + "Unison"), get (pre + "Detune") };
        }
        journey = get ("journey"); journeyMod = get ("journeyMod"); cyclone = get ("cyclone"); cycRate = get ("cycRate");
        wind = get ("wind"); poppies = get ("poppies");
        filtMode = get ("filtMode"); cutoff = get ("cutoff"); reso = get ("reso"); filtDrive = get ("filtDrive");
        filtEnvAmt = get ("filtEnv"); keyTrack = get ("keyTrack"); filtVel = get ("filtVel");
        fA = get ("fA"); fD = get ("fD"); fS = get ("fS"); fR = get ("fR");
        aA = get ("aA"); aD = get ("aD"); aS = get ("aS"); aR = get ("aR"); ampVel = get ("ampVel");
        lfo1Wave = get ("lfo1Wave"); lfo1Rate = get ("lfo1Rate"); lfo1Delay = get ("lfo1Delay");
        lfo1Pitch = get ("lfo1Pitch"); lfo1Cutoff = get ("lfo1Cutoff");
        lfo2Wave = get ("lfo2Wave"); lfo2Rate = get ("lfo2Rate"); lfo2Colour = get ("lfo2Colour");
        lfo2Pan = get ("lfo2Pan"); lfo2Amp = get ("lfo2Amp");
        voiceMode = get ("voiceMode"); glide = get ("glide"); drift = get ("drift");
        spread = get ("spread"); bendRange = get ("bendRange");
    }

    void prepare (double sr, int maxBlock)
    {
        for (auto& v : voices) v.prepare (sr);
        lfo1.prepare (sr); lfo2.prepare (sr);
        lfo1Buf.assign ((size_t) maxBlock, 0.0f);
        lfo2Buf.assign ((size_t) maxBlock, 0.0f);
    }

    void allNotesOff() { for (auto& v : voices) v.kill(); heldNotes.clear(); }

    void process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
    {
        const int n = buffer.getNumSamples();
        if ((int) lfo1Buf.size() < n) { lfo1Buf.resize ((size_t) n); lfo2Buf.resize ((size_t) n); }
        snapshot();

        const int w1 = (int) lfo1Wave->load(), w2 = (int) lfo2Wave->load();
        const float r1 = lfo1Rate->load(), r2 = lfo2Rate->load();
        for (int i = 0; i < n; ++i)
        {
            lfo1Buf[(size_t) i] = lfo1.next (w1, r1);
            lfo2Buf[(size_t) i] = lfo2.next (w2, r2);
        }

        buffer.clear();
        auto* L = buffer.getWritePointer (0);
        float* R;
        if (buffer.getNumChannels() > 1) R = buffer.getWritePointer (1);
        else { scratch.assign ((size_t) n, 0.0f); R = scratch.data(); }

        int pos = 0;
        for (const auto meta : midi)
        {
            const int t = juce::jlimit (0, n, meta.samplePosition);
            renderVoices (L, R, pos, t - pos);
            pos = t;
            handleMidi (meta.getMessage());
        }
        renderVoices (L, R, pos, n - pos);
    }

private:
    void snapshot()
    {
        for (int l = 0; l < 2; ++l)
        {
            auto& d = sp.layer[l]; auto& s = lay[l];
            d.chr = juce::jlimit (0, Params::characters.size() - 1, (int) s.chr->load());
            d.pitchOffset = s.oct->load() * 12.0f + s.semi->load() + s.fine->load() / 100.0f;
            d.colour = s.colour->load(); d.level = s.level->load();
            d.unison = juce::jlimit (1, Voice::maxUnison, (int) s.unison->load());
            d.detune = s.detune->load();
        }
        sp.journey = journey->load(); sp.journeyMod = journeyMod->load(); sp.cyclone = cyclone->load();
        sp.cycRate = cycRate->load(); sp.wind = wind->load(); sp.poppies = poppies->load();
        sp.filtMode = (int) filtMode->load(); sp.cutoff = cutoff->load(); sp.reso = reso->load();
        sp.filtDrive = filtDrive->load(); sp.filtEnv = filtEnvAmt->load(); sp.keyTrack = keyTrack->load(); sp.filtVel = filtVel->load();
        sp.fA = fA->load(); sp.fD = fD->load(); sp.fS = fS->load(); sp.fR = fR->load();
        sp.aA = aA->load(); sp.aD = aD->load(); sp.aS = aS->load(); sp.aR = aR->load(); sp.ampVel = ampVel->load();
        sp.lfo1Delay = lfo1Delay->load(); sp.lfo1Pitch = lfo1Pitch->load(); sp.lfo1Cutoff = lfo1Cutoff->load();
        sp.lfo2Colour = lfo2Colour->load(); sp.lfo2Pan = lfo2Pan->load(); sp.lfo2Amp = lfo2Amp->load();
        sp.voiceMode = (int) voiceMode->load(); sp.glide = glide->load(); sp.drift = drift->load();
        sp.spread = spread->load(); sp.bendRange = (int) bendRange->load();
    }

    void renderVoices (float* L, float* R, int start, int num)
    {
        if (num <= 0) return;
        for (auto& v : voices)
            v.render (L, R, start, num, sp, lfo1Buf.data(), lfo2Buf.data(), bend, modWheel);
    }

    void handleMidi (const juce::MidiMessage& m)
    {
        if (m.isNoteOn())            noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())      noteOff (m.getNoteNumber());
        else if (m.isPitchWheel())   bend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
        else if (m.isController())
        {
            if (m.getControllerNumber() == 1) modWheel = (float) m.getControllerValue() / 127.0f;
            else if (m.getControllerNumber() == 64)
            {
                sustainPedal = m.getControllerValue() >= 64;
                if (! sustainPedal)
                    for (auto& v : voices) if (v.sustained) { v.sustained = false; v.release(); }
            }
        }
        if (m.isAllNotesOff() || m.isAllSoundOff()) allNotesOff();
    }

    void noteOn (int note, float vel)
    {
        if (sp.voiceMode != 0)
        {
            heldNotes.removeFirstMatchingValue (note);
            heldNotes.add (note);
            auto& v = voices[0];
            const bool legato = sp.voiceMode == 2 && v.isGateOn();
            v.start (note, vel, sp, lastPitch, ! legato, ++seedCounter);
            if (legato) v.setTarget (note);
            lastPitch = (float) note;
            return;
        }

        Voice* target = nullptr;
        for (auto& v : voices) if (v.isActive() && v.getNote() == note) { target = &v; break; }
        if (! target) for (auto& v : voices) if (! v.isActive()) { target = &v; break; }
        if (! target)
        {   // steal: oldest released voice first, then oldest overall
            uint64_t best = 0;
            for (auto& v : voices) if (! v.isGateOn() && v.age >= best) { best = v.age; target = &v; }
            if (! target) for (auto& v : voices) if (v.age >= best) { best = v.age; target = &v; }
        }
        target->start (note, vel, sp, lastPitch, true, ++seedCounter);
        lastPitch = (float) note;
    }

    void noteOff (int note)
    {
        if (sp.voiceMode != 0)
        {
            heldNotes.removeFirstMatchingValue (note);
            auto& v = voices[0];
            if (v.getNote() != note) return;
            if (heldNotes.isEmpty())
            {
                if (sustainPedal) v.sustained = true; else v.release();
            }
            else
            {
                const int prev = heldNotes.getLast();
                v.setTarget (prev);
                lastPitch = (float) prev;
            }
            return;
        }
        for (auto& v : voices)
            if (v.isGateOn() && v.getNote() == note)
            {
                if (sustainPedal) v.sustained = true; else v.release();
            }
    }

    struct LayerPtrs { std::atomic<float> *chr, *oct, *semi, *fine, *colour, *level, *unison, *detune; } lay[2];
    std::atomic<float> *journey, *journeyMod, *cyclone, *cycRate, *wind, *poppies,
        *filtMode, *cutoff, *reso, *filtDrive, *filtEnvAmt, *keyTrack, *filtVel,
        *fA, *fD, *fS, *fR, *aA, *aD, *aS, *aR, *ampVel, *lfo1Wave, *lfo1Rate, *lfo1Delay, *lfo1Pitch, *lfo1Cutoff,
        *lfo2Wave, *lfo2Rate, *lfo2Colour, *lfo2Pan, *lfo2Amp, *voiceMode, *glide, *drift, *spread, *bendRange;

    juce::AudioProcessorValueTreeState& apvts;
    std::array<Voice, numVoices> voices;
    SynthParams sp {};
    GlobalLfo lfo1, lfo2;
    std::vector<float> lfo1Buf, lfo2Buf, scratch;
    juce::Array<int> heldNotes;
    float bend = 0, modWheel = 0, lastPitch = -1.0f;
    bool sustainPedal = false;
    uint32_t seedCounter = 1;
};
