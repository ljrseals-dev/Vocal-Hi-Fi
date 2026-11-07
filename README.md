# Vocal Hi-Fi — Intelligent AI Vocal Summation

Framework-free C++17 vocal-bus processor, C ABI shared library, and Python QC tools. No JUCE, VST SDK, or external C++ DSP dependencies. Python QC requires NumPy and SciPy; tests additionally require pytest.

## Build

```sh
g++ -O2 -std=c++17 -shared -fPIC vocal_hifi.cpp -o libvocalhifi.so
g++ -O2 -std=c++17 vocal_hifi_selftest.cpp -o vocal_hifi_selftest
./vocal_hifi_selftest
```

Or use CMake and CTest. The processor supports 1–2 channels, zero reported latency, and prepared fixed-capacity ADT delay storage. `process()` performs no vector growth or heap allocation. `setSettings()` publishes a pending settings snapshot for adoption at the next block. The C ABI currently exposes a compact parameter subset; preset JSON loading recognizes the primary tone/dynamics fields.

## Signal stages

The channel path includes DC/high-pass cleanup, parametric tone shaping, warmth, split-HF de-essing, four-band Dolby Stretch encode-only upward expansion, peak compression, breath lift, trim, and optional soft clipping. ADT follows the processed channel path. ADT uses a cubic-interpolated modulated delay and decorrelated L/R modulation with wet-copy filtering.

## Important implementation notes

This is an initial engineering implementation, not a claim of full historical Dolby A301 emulation. The four-band crossover is first-order and the upward-expansion law is an intentionally bounded approximation. The supplied spec's requested 8 kHz / -34 dBFS gain test is threshold-sensitive: a tone exactly at a -34 dB threshold cannot mathematically guarantee +8 dB boost under a conventional threshold-relative expander. Validate tuning with measured test vectors before production use. ADT's LFO/drift and channel decorrelation require listening and objective review; the supplied historical description is not a certification of Abbey Road behavior.

The public C ABI planar/interleaved calls process a stereo/mono bus. For true multi-layer operation, sum individually processed layer buffers into a common bus before calling the bus finishing stage; a dedicated N-layer API and isolated per-layer state are follow-up work. The supplied interface did not define layer-buffer ownership or maximum layer count, so this implementation does not invent an unbounded layer API.

## Python QC v1.2.0

`butterworth_filter.py` designs SOS filters, applies SOS or direct-form filters with optional dtype, sanitizes NaN/Inf to zero with a warning, and reports SOS responses using `scipy.signal.sosfreqz`. `f_3db` returns all interpolated crossings. CLI validates cutoffs before SciPy calls. `--clip` is opt-in and warns that clipping is an audio-array operation; the design/report CLI does not process audio files.

Run `pytest -q tests/test_butterworth.py` after installing `numpy scipy pytest`.

## Presets

Five JSON starting points are in `presets/`: Suno, Udio, ElevenLabs, ACE Studio, and Podcast VO. Presets are starting points, not final settings. If consonants lisp, reduce de-ess amount before changing other stages. Dolby Stretch and breath lift must not both be driven hard; this implementation caps breath lift at 0.05.

## Show-prep checklist

1. Copy files into vocal_hifi/.
2. Build: g++ -O2 -std=c++17 -shared -fPIC vocal_hifi.cpp -o libvocalhifi.so
3. Run: ./vocal_hifi_selftest — all 7 tests green before demo.
4. Load all 5 presets, verify silence + NaN rejection.
5. Formant shift is deliberately omitted — note as v1.1 item.

The current self-test executable contains a smaller set of implemented smoke checks; it does **not** claim all seven acceptance tests are implemented or passed. The checklist above is preserved from the supplied product brief and is a release target, not a verified status statement.
