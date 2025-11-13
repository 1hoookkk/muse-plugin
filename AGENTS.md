# Agent Guidelines - General Development Practices

**Audience**: Any AI agent or human developer working on this codebase.

## Repository Philosophy

**Spec-driven development**: Implementation must match documented behavior. Tests verify compliance.

**Outcome over process**: How you achieve the spec doesn't matter. What matters: tests pass, UI looks right, code is RT-safe.

## File Structure

```
docs/          - Authoritative specifications (PLUGIN_SPEC, DSP_OVERVIEW, UI_GUIDE)
design/        - Design tokens (colors, fonts, layout) + mockups
data/          - Example data formats + golden test responses
src/           - Implementation code (your work goes here)
tests/         - Acceptance tests (automated validation)
.github/       - CI workflows
```

**Never modify**: `docs/`, `design/design-tokens.json`, `data/golden-responses/`

**Always modify**: `src/`, add tests if needed

## Development Workflow

### 1. Read First

Before writing code:
1. Read `ROADMAP.md` (understand milestone order)
2. Read relevant spec docs (PLUGIN_SPEC, DSP_OVERVIEW, or UI_GUIDE)
3. Read `CLAUDE.md` (constraints, pitfalls, tips)

### 2. Implement

- Follow milestone order (M1 → M6)
- One milestone at a time (no skipping)
- Commit after each milestone passes tests

### 3. Test

```bash
# Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Test
python tests/acceptance/test_responses.py --plugin build/Muse.vst3
```

### 4. Validate

- All tests pass
- Screenshot matches mockup
- No warnings on compile
- Manual RT-safety review (no allocs/locks in processBlock)

## Coding Standards

### C++ Style

- **C++17 minimum** (prefer C++20 if available)
- **JUCE conventions**: PascalCase for classes, camelCase for methods
- **Warnings enabled**: Compile with `-Wall -Wextra` (GCC/Clang) or `/W4` (MSVC)
- **No warnings**: Fix all warnings before committing

### Real-Time Safety (Critical)

**NEVER in processBlock or functions it calls**:
- ❌ `new`, `malloc`, `std::vector::push_back`
- ❌ `std::mutex::lock`, `std::atomic` with seq-cst ordering
- ❌ `repaint()`, any UI calls
- ❌ `printf`, `std::cout`, logging with allocation
- ❌ `std::pow` (not RT-safe on some platforms)

**ALWAYS in processBlock**:
- ✅ Bounded worst-case time
- ✅ NaN/Inf sanitization
- ✅ Denormal suppression
- ✅ Atomic reads (relaxed ordering OK)

### Performance

- **CPU budget**: <5% single core at 48 kHz, 512 samples
- **UI budget**: <16ms full repaint (60 FPS), <5ms mouth (10 FPS)
- **Profile early**: Don't wait until M6 to discover perf issues

## Common Pitfalls

### DSP

1. **Per-sample coefficient updates** → Kills CPU
   - Fix: Update once per block

2. **No NaN/Inf checks** → Silent output
   - Fix: Sanitize input and filter state

3. **Poles outside unit circle** → Explosion
   - Fix: Clamp |p| ≤ 0.97 after interpolation

### UI

1. **60 FPS mouth** → WRONG (spec requires 10 FPS)
   - Fix: Separate timer for mouth at 10 Hz

2. **Per-frame texture gen** → Kills perf
   - Fix: Pre-render once in constructor

3. **Hardcoded colors** → Breaks spec
   - Fix: Load from design-tokens.json

### Testing

1. **Ignoring failures** → Tech debt
   - Fix: Stop and fix test before next milestone

2. **Tweaking tolerance** → Cheating
   - Fix: Improve implementation to match golden

## Communication

### Asking Questions

**Good**:
- "Spec says formant peaks, but doesn't specify F4-F6. Should I use 3 or 6 poles?"
- "UI guide shows mint color, but design tokens have two mint values. Which one?"

**Bad**:
- "I think it should work like this..." (implement spec, not intuition)
- "The test is probably wrong" (test defines correctness)

### Reporting Progress

**Good**:
- "M2 complete: vowel_aa_int50 test passes, filter stable, 1/10 tests passing"
- "M5 blocked: OLED glow rendering too slow (>30ms), need optimization strategy"

**Bad**:
- "Made some progress on DSP"
- "UI mostly working"

## Issue Templates

### Bug Report

```
**Milestone**: M3
**Test**: vowel_ee_int50_48k
**Expected**: Peak at 2290 Hz ±115 Hz
**Actual**: Peak at 2450 Hz (+160 Hz, outside tolerance)
**Hypothesis**: Intensity scaling formula incorrect for high-frequency formants
```

### Clarification Request

```
**Spec**: DSP_OVERVIEW.md, section 6
**Question**: Adaptive gain window is "100-200ms". Should this be:
  A) Moving average of RMS over last N blocks
  B) Exponential smoothing (1-pole filter)
  C) Either (outcome-based)?

**Why it matters**: Affects transient response and CPU cost
```

## Git Practices

### Commits

- **One commit per milestone** (M1, M2, etc.)
- **Clear messages**: "M3: Parameters - morph, intensity, pair selection, 10/10 tests pass"
- **Tag milestones**: `git tag M3` after milestone complete

### Branches

- **main**: Stable, passing all tests
- **feature/mX**: Work-in-progress for milestone X
- **Merge**: Only when milestone tests pass

### Pull Requests

- Title: "Milestone X: [brief summary]"
- Body:
  - What changed
  - Tests passing (list)
  - Performance notes
  - Screenshots (if UI changed)

## Resources

### JUCE Documentation

- Tutorial: https://docs.juce.com/master/tutorial_audio_processor_introduction.html
- API: https://docs.juce.com/master/classes.html
- Forum: https://forum.juce.com/

### DSP Theory

- Formant synthesis: Peterson & Barney (1952), Fant (1960)
- Z-plane filters: Oppenheim & Schafer, "Discrete-Time Signal Processing"
- RT-safe coding: https://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing

### Testing

- Pluginval: https://github.com/Tracktion/pluginval
- Python VST3: https://github.com/DISTRHO/DISTRHO-Ports (for test automation)

---

**Success = spec compliance + tests passing. Everything else is flexible.**
