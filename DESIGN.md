# Emerald Haze: psychedelic storybook synthesiser (design plan, v0.2)

A polyphonic instrument themed on L. Frank Baum's *The Wonderful Wizard of Oz* (1900, public
domain). Every sound, name and picture is original. Names and imagery come from the **book**, not
the 1939 film: silver shoes (not ruby slippers), Kansas grey, green spectacles, the Tin Woodman,
the poppy field, the cyclone. No film audio, songs, song titles, artwork or characters' likenesses.

Stack: JUCE 8.0.9, C++17, CMake, same layout as Paisley. VST3 + AU + Standalone. macOS 11+
universal (Intel + Apple Silicon); no `-march`, AVX or AVX2 flags anywhere (plain scalar C++),
so a 2015 Intel Mac is fine. Windows x64 with the static runtime.

## Sound engine (`Source/SynthEngine.h`)

12 voices, Poly / Mono / Legato, glide, sustain pedal, mod wheel vibrato, pitch bend.

**Two layers per voice (A and B).** Each layer picks one of ten *characters*, each its own
synthesis model; the layer's **Colour** knob morphs within it. Octave, semi, fine, level,
unison (1-5) and detune per layer.

| Character | Model | Colour does |
|---|---|---|
| Brick Road | golden PolyBLEP saw stack | adds a fifth, then an octave (brass) |
| Emerald Glass | 2-op FM, 3.5 ratio, bright strike that decays | brilliance |
| Tin Man | inharmonic FM (1.41 + 2.76 ratios), hollow clang | amount of metal |
| Scarecrow | straw reed (triangle + harmonics) + breath band-noise | breathiness |
| Lion | saw + sub, asymmetric drive, throaty flutter | roar |
| Munchkin Choir | buzz through 3 formants shifted up for small voices | vowel A-E-I-O-U |
| Witch | hollow pulse with tritone ring modulation | pulse width + sneer |
| Poppy Field | soft additive pad (6 partials) | brightness |
| Silver Shoes | twinkling octave partials | sparkle |
| Flying Monkeys | FM whose ratio jumps at random (chatter) | chatter speed |

**The Journey (signature macros)**
- **A > B**: equal-power crossfade from layer A to layer B; **LFO2 Sway** lets LFO 2 move it.
- **Cyclone** + **Spin**: unison copies orbit the stereo field with a pitch swirl and a filter spin.
- **Wind**: a whistling, key-tracked band-passed gust that rides the cyclone.
- **Poppies**: the longer a note is held, the drowsier it gets (sags in pitch, darkens, slow wobble).

Then: ZDF multimode filter (LP24/LP12/Band/High/Notch, drive, env, key track, velocity), filter
and amp ADSRs, LFO 1 (vibrato/cutoff, fade-in), LFO 2 (Colour / Pan / Amp / Journey), analogue drift.

## FX chain (`Source/Effects.h`), fixed order, each with a power switch

Drive (Warm / Roar / Tin) > **Twister** (rotating-horn swirl: doppler, amplitude, auto-pan, ramps
between slow and fast like a rotary cabinet) > **Rainbow Prism** (seven resonant bands that keep
rising and wrapping, panned red-left to violet-right: an endless rainbow) > Chorus / Flanger /
Ensemble > **Kansas** (grey old-record world: band-limit, wow, dust crackle, narrowing) >
**Road Echo** (tempo-sync, ping-pong, wobble, self-oscillating feedback) > **Emerald Hall**
(reverb with an octave-up shimmer loop).

## Presets (`Source/FactoryPresets.cpp`)

62 factory presets in 13 categories: Brick Road, Emerald City, Twister, Tin Man, Scarecrow, Lion,
Munchkins, Witch, Flying Monkeys, Poppy Field, Silver Shoes, Rainbow Dreams, Kansas. Same system as
Paisley: sparse overrides on the Init patch plus shared FX snippets, loudness-matched by
`EmHazeRender --calibrate` (writes `FactoryPresetLevels.h`). User presets are `.emhaze` XML files
in `Documents/EmeraldHaze/Presets/`.

## Storybook themes (`Source/Themes.h`)

All artwork is drawn in code (no bitmaps), so the UI scales cleanly (50-150%).

Two pages, switched from the header (the last one used is remembered):
- **Storybook** (default): Layer A and B characters with Colour and Level, the four Journey macros
  (A > B, Cyclone, Wind, Poppies), the "Spells" FX switches and a Colour & Space strip (cutoff,
  reso, sway, shimmer, echo/hall mix, master). Most of the theme's illustration stays visible.
- **Workshop**: every parameter.

| Theme | Background | Knobs |
|---|---|---|
| Yellow Brick Road | brick road winding to a glowing horizon | chicken-head |
| Emerald City | glowing faceted lattice and spires | cut-emerald gem |
| Twister | spiral funnel with debris | neon ring |
| Poppy Field | rolling hills of poppies | soft dot |
| Kansas Storybook | sepia halftone printed page | bakelite |
| Wicked West | night sky, crescent moon, green haze | neon ring |
| Rainbow's End | big seven-band rainbow and clouds | soft dot |

## Dev tool (`tools/Render.cpp`, `-DEMHAZE_BUILD_TOOLS=ON`)

Renders every preset to WAV, flags NaN / silence / clipping, snapshots the editor in every theme,
and calibrates preset loudness. v0.1 result: 62/62 presets clean.

## Decisions (v0.2)

- Name: **Emerald Haze** (working name, easy to change: `PRODUCT_NAME` in CMakeLists.txt).
- Book-only theming: no ruby slippers, film songs or film imagery (those are Warner Bros IP).
- Next feature chosen and built: the Storybook macro page.

## Next steps (not yet built)

- Arpeggiator / "Munchkin march" step sequencer; tempo-synced LFOs; mod matrix.
- More characters (Glinda bubble pads, Hammerhead thumps, China Country porcelain plucks, Winkies).
- Optional sampled layer (recorded original sources: music box, harmonium, toy piano).
- Installers + CI (copy Paisley's GitHub Actions workflow; a draft is in `docs/ci-build-draft.yml`; installer scripts still to add).
