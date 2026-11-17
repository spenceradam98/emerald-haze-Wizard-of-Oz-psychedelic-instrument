# Emerald Haze (working name)

A psychedelic storybook synthesiser (VST3 / AU / Standalone) themed on *The Wonderful Wizard of Oz*
(the 1900 public-domain book). Two layers of ten story "characters", Journey / Cyclone / Wind /
Poppies macros on an illustrated Storybook page (plus a Workshop page with every control), Twister, Rainbow Prism, Kansas lo-fi, Road Echo and a shimmer reverb, 62 presets
and 7 storybook themes. See `DESIGN.md`, `docs/screenshots/` and `docs/demo-reel.mp3`.

## Build

```
cmake -B build -DCMAKE_BUILD_TYPE=Release            # fetches JUCE 8.0.9
# or: cmake -B build -DJUCE_DIR=/path/to/JUCE
cmake --build build --config Release --target EmeraldHaze_VST3 EmeraldHaze_Standalone
```

macOS: universal (x86_64 + arm64), deployment target 11.0. No AVX/AVX2 or other `-march` flags.
Windows: x64, static runtime.

Dev tool: `cmake -B build -DEMHAZE_BUILD_TOOLS=ON` then
`EmHazeRender <outDir>` (render + check every preset, theme screenshots) or
`EmHazeRender <outDir> --calibrate Source/FactoryPresetLevels.h` (loudness match).
