#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "SynthEngine.h"

// ============================================================================
//  FX rack, fixed order:
//  Drive -> Twister -> Rainbow Prism -> Chorus/Flanger/Ensemble -> Kansas
//        -> Road Echo -> Emerald Hall (reverb with shimmer)
// ============================================================================

struct DelayLine
{
    std::vector<float> buf;
    int write = 0;
    void prepare (int size) { buf.assign ((size_t) juce::jmax (4, size), 0.0f); write = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
    void push (float x) { buf[(size_t) write] = x; if (++write >= (int) buf.size()) write = 0; }
    // delay in samples (fractional), measured from the most recently pushed sample
    float read (float d) const
    {
        const int size = (int) buf.size();
        d = juce::jlimit (1.0f, (float) size - 3.0f, d);
        float rp = (float) write - d;
        while (rp < 0) rp += (float) size;
        const int i0 = (int) rp; const float fr = rp - (float) i0;
        const int i1 = (i0 + 1) % size;
        return buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * fr;
    }
};

struct OnePole
{
    float z = 0;
    float lp (float x, float coef) { z += coef * (x - z); return z; }
    static float coefFor (float hz, float sr) { return 1.0f - std::exp (-dsp::twoPi * hz / sr); }
};

class FxRack
{
public:
    explicit FxRack (juce::AudioProcessorValueTreeState& s) : apvts (s) {}

    void prepare (double sampleRate, int)
    {
        sr = (float) sampleRate;
        for (auto& d : twsLines) d.prepare ((int) (sr * 0.02f));
        for (auto& d : choLines) d.prepare ((int) (sr * 0.06f));
        for (auto& d : knsLines) d.prepare ((int) (sr * 0.05f));
        for (auto& d : dlyLines) d.prepare ((int) (sr * 4.2f));
        for (auto& d : shimLines) d.prepare ((int) (sr * 0.2f));
        reverb.setSampleRate (sampleRate);
        knsWowLfo.prepare (sampleRate);
        dlyWobble.prepare (sampleRate);
        reset();
    }

    void reset()
    {
        for (auto& d : twsLines) d.clear();
        for (auto& d : choLines) d.clear();
        for (auto& d : knsLines) d.clear();
        for (auto& d : dlyLines) d.clear();
        for (auto& d : shimLines) d.clear();
        for (auto& f : prism) f.reset();
        reverb.reset();
        dlyTime = -1.0f;
        twsSpeed = -1.0f;
    }

    void setBpm (double b) { bpm = b > 0 ? (float) b : 120.0f; }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int n = buffer.getNumSamples();
        if (buffer.getNumChannels() < 2) return;
        float* L = buffer.getWritePointer (0);
        float* R = buffer.getWritePointer (1);

        if (on ("drvOn")) drive (L, R, n);
        if (on ("twsOn")) twister (L, R, n); else twsSpeed = -1.0f;
        if (on ("prsOn")) prismFx (L, R, n);
        if (on ("choOn")) chorus (L, R, n);
        if (on ("knsOn")) kansas (L, R, n);
        if (on ("dlyOn")) delay (L, R, n);
        if (on ("revOn")) reverbFx (L, R, n);
    }

private:
    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    bool on (const char* id) const { return p (id) > 0.5f; }

    // ---------------------------------------------------------------- drive
    void drive (float* L, float* R, int n)
    {
        const int type = (int) p ("drvType");
        const float amt = p ("drvAmt"), mix = p ("drvMix");
        const float tc = OnePole::coefFor (800.0f * std::exp2 (p ("drvTone") * 4.5f), sr);
        const float gain = 1.0f + amt * (type == 1 ? 30.0f : 10.0f);
        const float outGain = 1.0f / std::sqrt (gain) * (type == 1 ? 1.6f : 1.2f);
        float* ch[2] = { L, R };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float x = ch[c][i];
                float y = x * gain;
                if (type == 0)      y = std::tanh (y);                                     // Warm
                else if (type == 1) y = std::tanh (1.5f * y) + 0.25f * std::tanh (y * y * 0.5f);   // Roar: asymmetric, growly
                else                y = std::sin (juce::jlimit (-1.5f, 1.5f, y) * 2.2f) * 0.8f; // Tin: folded, clanky
                y = drvTone[c].lp (y, tc) * outGain;
                y -= drvDc[c].lp (y, 0.0015f); // DC blocker (Roar is asymmetric)
                ch[c][i] = x + (y - x) * mix;
            }
    }

    // -------------------------------------------------------------- twister
    // Rotating horn: doppler (modulated delay) + amplitude + auto-pan, speed ramps between slow and fast.
    void twister (float* L, float* R, int n)
    {
        const float target = on ("twsFast") ? p ("twsFastHz") : p ("twsSlow");
        const float depth = p ("twsDepth"), mix = p ("twsMix");
        if (twsSpeed < 0) twsSpeed = target;
        const float accel = 1.0f - std::exp (-1.0f / ((target > twsSpeed ? 0.9f : 1.6f) * sr));
        const float ms = sr * 0.001f;
        for (int i = 0; i < n; ++i)
        {
            twsSpeed += (target - twsSpeed) * accel;
            twsPhase = dsp::wrap (twsPhase + twsSpeed / sr);
            twsPhase2 = dsp::wrap (twsPhase2 + twsSpeed * 0.83f / sr); // slower "drum" rotor
            const float s = dsp::sine (twsPhase), c = std::cos (dsp::twoPi * twsPhase);
            const float s2 = dsp::sine (twsPhase2);
            const float mono = 0.5f * (L[i] + R[i]);
            twsLines[0].push (mono); twsLines[1].push (mono);
            const float wl = twsLines[0].read ((2.0f + 1.4f * depth * s) * ms);
            const float wr = twsLines[1].read ((2.0f - 1.4f * depth * s) * ms);
            const float amL = 1.0f - 0.35f * depth * (0.5f + 0.5f * c) - 0.1f * depth * s2;
            const float amR = 1.0f - 0.35f * depth * (0.5f - 0.5f * c) + 0.1f * depth * s2;
            L[i] = L[i] * (1.0f - mix) + wl * amL * mix * 1.1f;
            R[i] = R[i] * (1.0f - mix) + wr * amR * mix * 1.1f;
        }
    }

    // ---------------------------------------------------------------- prism
    // Seven resonant bands that keep rising through the spectrum (and wrap,
    // faded at the edges), panned red-left to violet-right: an endless rainbow.
    void prismFx (float* L, float* R, int n)
    {
        const float rate = p ("prsRate"), depth = p ("prsDepth"), mix = p ("prsMix");
        const float k = 1.4f - 1.32f * p ("prsReso");
        const float span = 2.0f + 4.0f * depth; // octaves covered
        const float low = 1200.0f * std::exp2 (-span * 0.5f);
        for (int i = 0; i < n; ++i)
        {
            prsPhase = dsp::wrap (prsPhase + rate * 0.25f / sr);
            const float mono = 0.5f * (L[i] + R[i]);
            float wl = 0, wr = 0;
            for (int b = 0; b < 7; ++b)
            {
                const float pos = dsp::wrap ((float) b / 7.0f + prsPhase);
                const float f = juce::jmin (low * std::exp2 (pos * span), sr * 0.42f);
                const float g = std::tan (dsp::pi * f / sr);
                const float win = std::sin (dsp::pi * pos);
                const float y = prism[b].process (mono, g, k).bp * win;
                const float pan = (float) b / 6.0f;
                wl += y * std::cos (pan * dsp::pi * 0.5f);
                wr += y * std::sin (pan * dsp::pi * 0.5f);
            }
            const float wet = 1.4f * (0.4f + 0.6f * k);
            L[i] = L[i] * (1.0f - mix * 0.7f) + wl * wet * mix;
            R[i] = R[i] * (1.0f - mix * 0.7f) + wr * wet * mix;
        }
    }

    // -------------------------------------------------- chorus / flanger / ensemble
    void chorus (float* L, float* R, int n)
    {
        const int mode = (int) p ("choMode");
        const float rate = p ("choRate"), depth = p ("choDepth"), fb = p ("choFb"), mix = p ("choMix");
        const float ms = sr * 0.001f;
        const float centre = mode == 1 ? 2.5f : (mode == 2 ? 9.0f : 14.0f);
        const float range  = mode == 1 ? 2.2f : (mode == 2 ? 3.5f : 7.0f);
        float* ch[2] = { L, R };
        for (int i = 0; i < n; ++i)
        {
            choPhase = dsp::wrap (choPhase + rate / sr);
            choPhase2 = dsp::wrap (choPhase2 + 6.3f / sr);
            for (int c = 0; c < 2; ++c)
            {
                const float x = ch[c][i];
                choLines[c].push (x + choFbState[c] * fb);
                float wet = 0;
                if (mode == 2)
                {
                    for (int t = 0; t < 3; ++t)
                    {
                        const float ph = choPhase + (float) t / 3.0f + (float) c * 0.17f;
                        const float m = std::sin (dsp::twoPi * ph) * depth + 0.15f * std::sin (dsp::twoPi * (choPhase2 + (float) t / 3.0f));
                        wet += choLines[c].read ((centre + range * m) * ms);
                    }
                    wet *= 0.45f;
                }
                else
                {
                    const float m = std::sin (dsp::twoPi * (choPhase + (float) c * 0.25f));
                    wet = choLines[c].read ((centre + range * depth * m) * ms);
                }
                choFbState[c] = wet;
                ch[c][i] = x * (1.0f - mix * 0.5f) + wet * mix;
            }
        }
    }

    // --------------------------------------------------------------- kansas
    // Grey, dusty, old-record world before the colour arrives.
    void kansas (float* L, float* R, int n)
    {
        const float wow = p ("knsWow"), band = p ("knsBand"), dust = p ("knsDust"), narrow = p ("knsMono");
        const float lpc = OnePole::coefFor (14000.0f * std::exp2 (-band * 2.6f), sr);
        const float hpc = OnePole::coefFor (40.0f * std::exp2 (band * 3.5f), sr);
        const float ms = sr * 0.001f;
        const float crackleProb = dust * dust * 60.0f / sr;
        float* ch[2] = { L, R };
        for (int i = 0; i < n; ++i)
        {
            knsPhase = dsp::wrap (knsPhase + 0.55f / sr);
            const float w = 0.6f * knsWowLfo.next (5, 0.7f) + 0.4f * dsp::sine (knsPhase);
            const float d = (5.0f + w * wow * 3.5f) * ms;
            // crackle: sparse random clicks through a short decay
            if ((rng.next() * 0.5f + 0.5f) < crackleProb) crackle = rng.next() * 0.25f * (0.5f + dust);
            crackle *= 0.86f;
            const float hiss = rng.next() * dust * 0.003f;
            float y[2];
            for (int c = 0; c < 2; ++c)
            {
                knsLines[c].push (ch[c][i]);
                float v = knsLines[c].read (d);
                v = knsLp[c].lp (knsLp2[c].lp (v, lpc), lpc);
                v -= knsHp[c].lp (v, hpc);
                y[c] = std::tanh (v * 1.4f) / 1.2f;
            }
            const float mid = 0.5f * (y[0] + y[1]), side = 0.5f * (y[0] - y[1]) * (1.0f - narrow);
            L[i] = mid + side + crackle + hiss;
            R[i] = mid - side + crackle + hiss;
        }
    }

    // ----------------------------------------------------------- road echo
    void delay (float* L, float* R, int n)
    {
        float timeMs;
        if (on ("dlySync"))
        {
            const int div = juce::jlimit (0, Params::syncDivs.size() - 1, (int) p ("dlyDiv"));
            timeMs = Params::syncBeats[div] * 60000.0f / bpm;
        }
        else timeMs = p ("dlyMs");
        timeMs = juce::jlimit (5.0f, 4000.0f, timeMs);
        const float target = timeMs * sr * 0.001f;
        if (dlyTime < 0) dlyTime = target;

        const float fb = p ("dlyFb"), mix = p ("dlyMix"), mod = p ("dlyMod");
        const bool ping = on ("dlyPing");
        const float lpc = OnePole::coefFor (1200.0f * std::exp2 (p ("dlyTone") * 4.0f), sr);
        const float hpc = OnePole::coefFor (90.0f, sr);
        const float smooth = 1.0f - std::exp (-1.0f / (0.25f * sr));

        for (int i = 0; i < n; ++i)
        {
            dlyTime += (target - dlyTime) * smooth;
            dlyModPhase = dsp::wrap (dlyModPhase + 0.7f / sr);
            const float m = (dsp::sine (dlyModPhase) + 0.4f * dlyWobble.next (5, 2.3f)) * mod * mod * 0.012f;
            const float wl = dlyLines[0].read (dlyTime * (1.0f + m));
            const float wr = dlyLines[1].read (dlyTime * (1.0f - m * 0.8f));
            auto shapeFb = [&] (float x, int c)
            {
                x = dlyLp[c].lp (x, lpc);
                x -= dlyHp[c].lp (x, hpc);
                return std::tanh (x * fb * 1.1f) / 1.1f;
            };
            const float fl = shapeFb (wl, 0), fr = shapeFb (wr, 1);
            if (ping)
            {
                dlyLines[0].push (0.5f * (L[i] + R[i]) + fr);
                dlyLines[1].push (fl);
            }
            else
            {
                dlyLines[0].push (L[i] + fl);
                dlyLines[1].push (R[i] + fr);
            }
            L[i] = L[i] * (1.0f - mix * 0.4f) + wl * mix;
            R[i] = R[i] * (1.0f - mix * 0.4f) + wr * mix;
        }
    }

    // ---------------------------------------------------------- emerald hall
    // juce::Reverb with an octave-up pitch shifter in a feedback loop (shimmer).
    // Processed in short chunks so the shimmer loop latency stays small.
    void reverbFx (float* L, float* R, int n)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.3f + 0.69f * p ("revSize");
        rp.damping = p ("revDamp");
        rp.width = p ("revWidth");
        rp.wetLevel = 1.0f;
        rp.dryLevel = 0.0f;
        reverb.setParameters (rp);
        const float mix = p ("revMix"), shimmer = p ("revShimmer");
        const float win = 0.06f * sr; // pitch-shifter window

        constexpr int chunk = 64;
        float wl[chunk], wr[chunk];
        for (int pos = 0; pos < n; pos += chunk)
        {
            const int m = juce::jmin (chunk, n - pos);
            for (int i = 0; i < m; ++i)
            {
                float sh[2] = { 0, 0 };
                if (shimmer > 0.001f)
                {
                    // two taps sweeping through the window at double speed = one octave up
                    shimPhase = dsp::wrap (shimPhase + 1.0f / win);
                    for (int c = 0; c < 2; ++c)
                        for (int t = 0; t < 2; ++t)
                        {
                            const float ph = dsp::wrap (shimPhase + 0.5f * (float) t);
                            const float d = (float) (chunk + 8 - i) + win * (1.0f - ph);
                            sh[c] += shimLines[c].read (d) * std::sin (dsp::pi * ph);
                        }
                }
                wl[i] = L[pos + i] * 0.5f + sh[0] * shimmer * 0.45f;
                wr[i] = R[pos + i] * 0.5f + sh[1] * shimmer * 0.45f;
            }
            reverb.processStereo (wl, wr, m);
            for (int i = 0; i < m; ++i)
            {
                shimLines[0].push (shimHp[0].lp (wl[i], 0.02f) * -1.0f + wl[i]); // high-passed into the shifter
                shimLines[1].push (shimHp[1].lp (wr[i], 0.02f) * -1.0f + wr[i]);
                L[pos + i] = L[pos + i] * (1.0f - mix * 0.5f) + wl[i] * mix * 0.45f;
                R[pos + i] = R[pos + i] * (1.0f - mix * 0.5f) + wr[i] * mix * 0.45f;
            }
        }
    }

    juce::AudioProcessorValueTreeState& apvts;
    float sr = 44100.0f, bpm = 120.0f;
    dsp::Rng rng;

    OnePole drvTone[2], drvDc[2];
    DelayLine twsLines[2]; float twsPhase = 0, twsPhase2 = 0, twsSpeed = -1.0f;
    dsp::SVF prism[7]; float prsPhase = 0;
    DelayLine choLines[2]; float choPhase = 0, choPhase2 = 0, choFbState[2] {};
    DelayLine knsLines[2]; OnePole knsLp[2], knsLp2[2], knsHp[2]; GlobalLfo knsWowLfo; float knsPhase = 0, crackle = 0;
    DelayLine dlyLines[2]; OnePole dlyLp[2], dlyHp[2]; float dlyTime = -1.0f, dlyModPhase = 0; GlobalLfo dlyWobble;
    DelayLine shimLines[2]; OnePole shimHp[2]; float shimPhase = 0;
    juce::Reverb reverb;
};
