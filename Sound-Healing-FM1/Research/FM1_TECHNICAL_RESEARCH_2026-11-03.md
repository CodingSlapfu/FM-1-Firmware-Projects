# FM-1 Sound Healing — Hardware, Firmware & Performance Research Baseline
**Researched:** 2026-11-03 (America/New_York)
**Project:** New original M-VAVE FM-1 generative sound-healing music instrument
**Status:** Research and engineering guidance only. **Not a firmware build or hardware-verified result.**
**Persistence:** Keep this file under the new project; do not substitute abandoned EIN-1, PARALLAX-8 or Crystal Water source.

## 0. READ FIRST — start of next build
1. Read `Sound-Healing-FM1/WORK_START_HERE.md` and this reference. Use verified *current* repository/toolchain docs and pinned upstream commits; do not blindly copy code from retired projects.
2. Inspect a proven, licensable FM-1 hardware/platform base: shortlist `charlesvestal/fm1-base` + `felucca-bsp` (MPL-2.0; beta), or vendor `m-vave/fm1_open` (GPL app with SDK linking exception, binary engine), and decide by actual compile/toolchain/feature trade-offs. Keep original sound engine, patches, audio and user-facing interface.
3. Do NOT claim a flashable release until native PI32v2 compile/link, package identity, updater, host tests, firmware hashes and live installer have passed. User performs physical FM-1 tests.
4. Protect live music above everything: audio hard deadlines > controls > MIDI/USB > graphics > background. Every new feature must be measured with the busiest screen and a full arrangement.
5. Implement generative ambient music, Solfeggio frequency tones, sacred bass, drums, *true stereo* binaural beats, original instrument patches, living nature soundscapes, live editing, one superb 240x240 visualizer, safe startup and update/recovery.

## 1. Hardware evidence and limits
Facts below come from the community FM-1 Developers' Wiki; its labels distinguish hardware measured [HW], vendor SDK [SDK], reverse engineering [RE], working code [CODE] and unknown [?]. Treat these as a dated baseline, not a silicon datasheet.
- SoC: JieLi AC791N / WL82 family; pi32v2 32-bit little-endian CPU, `-mcpu=r3`, 2 cores. Hardware *single-precision* float with `-mfprev1`; double precision runs in software. Source: https://sbegueria.github.io/fm1-dev-wiki/Hardware-Overview.html
- Stock boot clock about **360 MHz** and 60 MHz peripheral clock measured on one path; m-bootloader 1.2 may start at **320 MHz** / 53 MHz peripheral. Firmware should verify/restore expected clocks early and safely rather than assume a frequency. Source: https://sbegueria.github.io/fm1-dev-wiki/Clocks-and-Power.html
- SRAM: SDK describes **578 KiB on-chip total**; the usual firmware-visible mapped window is **512 KiB**, with some regions reserved for boot/update information and vectors. **No external RAM.** This is NOT 512 KiB of free heap. Source: https://sbegueria.github.io/fm1-dev-wiki/Memory-Map.html
- Flash: **1 MiB SPI NOR**, code normally executes in-place (XIP) through the flash controller. Stock app area `[0x4000,0x93000)` = ~**572 KiB / 581,632 bytes**; persistent data, updater staging and boot regions must not be overlaid. Actual app budget depends on chosen platform/linker. Source: https://sbegueria.github.io/fm1-dev-wiki/Flash-Storage.html
- Platform example: `fm1-base` documents ~**581 KiB app slot**, **96 KiB** RAM for static `.data/.bss` and a **336 KiB** pool when WiFi is disabled. These are *its linker-plan budgets*, not an FM-1 universal partition. Bluetooth and especially WiFi substantially reduce resources. Source: https://github.com/charlesvestal/fm1-base
- Screen: 240x240 ST7789-class TFT, **RGB565** (16 bpp), SPI1 write-only; no tearing-sync (TE) pin. Full RGB565 frame = **115,200 bytes**. Backlight PA2 is also involved in key-matrix enabling: **do not casually shut PA2 off** or keys may stop working. Source: https://sbegueria.github.io/fm1-dev-wiki/Display.html
- Controls: 27 note keys, 14 buttons, 7 encoders and analog master volume pot per community pinout (validate our own mapping). USB-MIDI device, TRS MIDI input, optional Bluetooth. Source: https://sbegueria.github.io/fm1-dev-wiki/Hardware-Overview.html
- Audio: Felucca-family firmware uses ALNK0 (I2S-like) stereo 24-bit-left-justified DMA with two half-buffers. At **256 stereo frames / 44.1 kHz**, each half has a **5.805 ms** deadline. Stock firmware may use a different half size and a reported ~44,118 Hz actual rate; **measure the clock** on real FM-1 for precise Hz / binaural accuracy. Source: https://sbegueria.github.io/fm1-dev-wiki/Audio-Output.html

## 2. Comparative firmware research (look for techniques, not copy-paste)
| Project | Useful lesson | What to verify before borrowing |
|---|---|---|
| **SLOOP** — https://github.com/isod89/sloop-fm1 | Proven web-updater flow, explicit 4-track performance design, drum synthesis/kits, responsive controls, user storage compatibility, second-core job verification and fallback | Exact current tag, the license (GPL-3.0), how update-loader handshake/staging is integrated and how dual-core failures are handled |
| **Felucca** — https://github.com/kurogedelic/Felucca | Hardware-layer foundations, rendering and audio-interrupt structure, MIDI and sample engines, inherited custom-firmware ecosystem | Find active maintained project/source and GPL compliance; don't transplant outdated code blindly |
| **Melodee** — https://github.com/keremimo/melodee | CPU1 worker architecture, UI knob/value feedback, audio RAM management, shared voice budgets, effects tail and bass management | Verify current revision and CPU1 stability on more than one FM-1 |
| **X0X** — https://github.com/charlesvestal/fm1-x0x | Drum/synth tradeoffs and second-core regression/fallback measurements | UI not necessarily suited to quiet generative soundscapes |
| **DUB-1** (results via wiki) | A/B timer evidence of XIP cache misses; hot DSP in SRAM decreases worst-case audio latency | Measurements are from one hardware unit, one clock/bootloader, specific patches |
| **FM1 Open (M-VAVE)** — https://github.com/m-vave/fm1_open | Official source/SDK build and web workbench; vendor places DSP in SRAM; large 8-part instrument reference | Current source/toolchain provenance, license/SDK exception, binary core restrictions; vendor README and release version labels may differ |
| **fm1-base / felucca-bsp** — https://github.com/charlesvestal/fm1-base | Recommended candidate for a new original application: MPL-2.0 board/platform layer, host simulator, browser emulator, installer/recovery, simple app interface | Beta API, project/toolchain compatibility and real device compilation |
| **fm1-nes** — https://github.com/Keitark/fm1-nes | Working SDK integration, timing display/audio/controls together, native package examples | It is a demonstration, not an audio performance proof |
| **JIANT FM** — https://github.com/JalopyMusic/fm1-juanjiant-bit-jiant | Performance UI and synthesis/FX routing ideas | Only take lessons permitted by its GPL-derived licensing and verified features |

For a wider vetted directory and primary links, consult:
https://sbegueria.github.io/fm1-dev-wiki/Known-Projects.html
https://github.com/aroum/fm1-custom-fw
https://github.com/isod89/sloop-fm1/blob/main/DUALCORE.md

## 3. Most important measured performance finding
The FM-1 executes flash code via XIP, competing with graphics/fonts and other accesses. A community **DUB-1 physical-device A/B** (2026-10-26, same single FM-1 at 360 MHz, identical logic; 55 KiB hot DSP placed in a 64 KiB SRAM region) timed 256-frame audio blocks using hardware TIMER4. It found:

| Test arrangement | Peak block running from flash | Peak block with hot DSP in SRAM | Audio dropouts A -> B |
|---|---:|---:|---:|
| Haze | 5.1 ms (88% of 5.8 ms budget) | 3.7 ms (64%) | 2 -> 0 |
| Night Drive | 4.7 ms | 4.4 ms | 1 -> 0 |
| Fog | 4.6 ms | 3.0 ms | 1 -> 0 |
| Pressure | 4.5 ms | 3.4 ms | 1 -> 0 |
| Steppers | 4.6 ms | 3.3 ms | 1 -> 0 |

Reported peak improvements were **6–35%**; mean reductions **5–25%**. This does not mean another firmware gets those gains automatically, but it makes **SRAM-resident audio hot paths** a high-priority engineering test. Source: https://sbegueria.github.io/fm1-dev-wiki/Performance.html

**Instrument first, measure first:** time the complete 5.8 ms render deadline with the crystal-based TIMER4; show max and mean time, peak over ~2 second windows, DMA underrun count, voice count, FX, soundscape cost, graphics frame time, SPI DMA duration, core1 faults/fallback and actual clock. A host PC benchmark is not hardware proof. Ship only after zero underruns over long stress sessions on real FM-1.

## 4. Audio architecture and priority optimization plan
- Base audio on bounded fixed-size blocks, preallocated voices and buffers; never allocate, flash-write, decompress unknown-length files or block on UI/USB in the audio ISR.
- Prefer **one precision master oscillator/tuning path** for selectable exact-Hz drones, plus musical A4 tuning policy (e.g. A4=432 Hz). Do not confuse 432 Hz tuning with the Solfeggio frequency list.
- Solfeggio selector target: 174, 285, 396, 417, 528, 639, 741, 852 and 963 Hz, with sensible fundamental/subharmonic musical arrangements. Make it musically beautiful, and distinguish actual generated pure-tone Hz from notes approximately tuned around them.
- Sound roles: bass, lead/flute/woodwind, guitar/pluck, pads/strings, keys/bells, choir, soft percussion/drums, nature textures. Begin with **12 or fewer simultaneously expensive synth voices**, then experimentally increase only with device headroom proof. Provide per-role caps and graceful note-steal/fade rules.
- Original patches: combine economical band-limited oscillators, small wavetables, noise shaping, simple resonators and envelopes. Reuse a small number of quality synthesis algorithms; store *parameters* rather than massive PCM banks. Nature beds should be procedural/short compressed textures with explicit CPU and RAM caps, NOT a continuously heavy general-purpose decoder in the hard audio path.
- Drums are mandatory: soft kicks and hand drums, brushed/shaker textures, gentle toms, airy percussion, subtle cymbals, ambient pulses. A generative rhythm conductor controls density, accents, tempo, activity and arrangement; default gentle, always manually editable/mutable. Give drum DSP its own measured budget.
- Binaural beats are mandatory: **true separated stereo** carrier oscillators, e.g. L=200 Hz, R=206 Hz gives a 6 Hz *difference*. Adjustable carrier, difference, level and ON/OFF; OFF by default; put these tones after global stereo-crossfeed/reverb or through a dry split path to preserve separation, and avoid accidental mono summing. Headphones are needed for binaural perception. No claims of curing illness or precise medical outcomes. Keep headphone levels conservative. Verify DAC channel mapping, resampling/clock drift, L/R isolation and level with a stereo recording.
- Shared rather than per-voice heavy FX: one stereo reverb and one stereo delay bus (quality interpolation/damping), gentle chorus/modulation, resonant filter where it matters, output limiter, bass highpass in reverb sends to reduce rumble. Model each effect's worst case and keep hard effect-buffer caps.
- Use **float32**, not double, on FM-1 hot DSP; precompute reciprocals/coefficients when params change. Check `-mcpu=r3 -mfprev1`, `-O2` hot, `-Os/-Oz` cold, linker garbage collection; compare fast-math only with listening and tests. Avoid risky inline 64-bit multiply sequences reported in an AC791N erratum without reviewing generated assembly.
- Under overload, first shed expensive nonessential work: visual particles -> UI update rate -> nature voices -> reverb quality/tail -> secondary harmonies. **Do not stop the essential tone/drone, drums or output processing abruptly**; use crossfades.
- P0 cold-start gain: zero audio DMA buffer, honor master-volume pot before sending first sample, fade audio in, no blast.

## 5. Graphics and sacred visualizer: honest hardware envelope
The 240x240 panel is **RGB565 at 115,200 bytes per full frame**. At **30 FPS** a full-screen renderer moves **3,456,000 bytes/s** excluding commands/overhead; at **60 FPS** it moves 6,912,000 bytes/s. Community tests show nominal 60 MHz SPI clock but ~44 MHz effective transfer. Ideal bus-only full-frame transfer at 44 Mbit/s is about **20.9 ms per frame**. This is a *calculated lower bound* from reported SPI rate, not a confirmed 30 FPS application benchmark. With normal overhead, graphics computation and audio interruption, 30 FPS full-screen is challenging and requires DMA overlap.

- Use RAM-backed, **double-buffered horizontal strips**, e.g. 240x40 RGB565 = 19,200 bytes per strip (38,400 for two); adjust to actual RAM pool/transfer measurements. Never DMA from flash; never launch a zero-count DMA.
- Rasterize strip B while DMA sends strip A; overlap carefully and only wait when reusing a buffer or sending the next command.
- Aggressively use **dirty rectangles** / changed-strips for UI text, menus, meters, transient events; do not clear/redraw all 115.2 KB if only a small fraction changed. Cache static background layers and pre-render glyphs with an SRAM budget.
- Design **one** extraordinary but inexpensive visualizer: 2.5D sacred rings/crystalline forms, slowly evolving geometry, parallax/tilt, 3D-looking light shading via lookup tables/palette gradients, sparse layered glows, event particles with hard cap. Avoid per-pixel full-screen 3D shading, general z-buffer, full-screen blur, unbounded object trails and expensive transcendental calls in per-pixel loops.
- Keep geometry in fixed-point or float32 coordinates at the *object* level; precompute trig tables; project a modest number of control points, render lines/shapes in bounded strips; spatial bins/tile clipping avoid redundant fragments.
- Feed the visuals **real engine events**: bass envelopes, drum transients, lead note onset, track changes, motif evolution, tone selection, FX energy, binaural state; not only generic RMS/FFT. Smooth parameters and avoid flicker. Separate update tick (e.g. audio feature snapshot per block) from visual render cadence.
- Target **30 FPS only as an acceptance goal**; never lock a claim before real hardware. Under stress, drop graphics cadence gracefully (e.g. 30 -> 20 -> 10) without audio discontinuity. A steady 20 FPS with clean audio is preferable to nominal 30 with clicks.
- If direct PA2 backlight off disables keys, use a safe black-on-screen or verified panel brightness control, *not* GPIO shutdown. Power-mode behavior must be tested with buttons and keys.
- Required counters: audio peak/deadlines, UI frame time, actual FPS, dirty area fraction, DMA busy time, over-budget frames; timestamp with TIMER4 or known reliable monotonic clock.

Source: https://sbegueria.github.io/fm1-dev-wiki/Display.html and https://sbegueria.github.io/fm1-dev-wiki/Performance.html

## 6. Dual core — potentially valuable, not guaranteed
There are **two CPU cores**, and some firmware uses both for DSP. However some real FM-1 units have exhibited **core 1 shared-RAM misreads**. Never design the product assuming core 1 always works.
- First make all critical music sound right on one core; classify optional jobs (extra reverb, auxiliary texture, secondary voices) that can be shed.
- If enabling core 1, use tiny descriptors, guard/check words, deadline timeouts, fault counters, safe fallback to single core and boot-time voltage/clock initialization. A broken job must not crash.
- Keeping audio hot code in SRAM remains important even with two cores because XIP flash is shared. Separate UI/graphics from audio without causing USB/MIDI schedule regressions.
- Second-core test: 1 hour stress at maximum musical density, visualizer active; job errors, fallback events, audio underruns and crashes counted.
Source: https://sbegueria.github.io/fm1-dev-wiki/Second-Core.html

## 7. Build environment, software quality and safe publishing
- **Two base/toolchain routes:**
  - Felucca/fm1-base family: PI32v2 clang 4.0.1 `-mcpu=r3`, AC79 SDK bits, community `build.py` / Linux x86-64 / Docker or WSL2 setups. See https://sbegueria.github.io/fm1-dev-wiki/Getting-Started.html.
  - M-VAVE official `fm1_open`: Windows x64 JieLi toolchain **2.5.3** with PI32v2 clang 4.0.1; package manifest checks all toolchain files and source kit matching. See https://github.com/m-vave/fm1_open.
- Favor a **licensable board-support layer** plus an original app. Respect GPLv3 vs MPL-2.0 vs SDK/binary linking restrictions; record upstream commits and retain notices.
- `.fwsc` updater is **two-stage USB-MIDI SysEx**: running firmware stages a loader; loader writes the app; device later re-enumerates and identity is rechecked. The host must check model/version/length/hashes and the real device's transfer state, not declare success after upload. Review SLOOP's operational installer and the wiki update protocol. Source: https://sbegueria.github.io/fm1-dev-wiki/Update-Protocol-and-Installers.html
- Design upgrade paths **new -> new** and **stock -> new**; physical user tests both. Do not assume 'stock restore first' acceptable. Include boot-safe fallback/recovery documented and preserve unit-specific flash calibration/settings; never ship untested bootloader rewrites.
- Flash app partition, VM, staging, BT pairing, user data overlap among firmware families; protect every reserved address, validate saved structs with magic/schema/version/CRC and use A/B commits for power-loss resilience. Never save during live audio if flash stalls XIP. Source: https://sbegueria.github.io/fm1-dev-wiki/Flash-Storage.html
- Build deterministically: native binary + host simulator + browser emulator sharing source; unit DSP golden tests; bounded fuzz tests for MIDI and malformed projects; render sample and clipping tests; updater integration tests; artifact manifest with SHA-256, source commit and matching installer/version. Preserve separate backup and recovery path.
- **No fake completion**: native firmware compile, instrument sound tests, web installer publication and real-device qualification are distinct evidence columns.

## 8. Preflight benchmark/acceptance checklist for our project
1. Verify native toolchain and stable base on the actual current source; lock branch/checkout so no overlapping writer.
2. Prove silent/safe power-on with master at zero, stable 44.1 kHz stereo output; instrument one pure tone and compare frequency on recording.
3. Record base CPU clock, core availability, RAM/flash budget, source commit, firmware identity and hardware unit.
4. Profile audio render with TIMER4 (min/mean/peak and >threshold), and count ALNK0 DMA xruns. Begin at 12 voices plus FX, then increase to stress case with drums + live soundscapes + binaural path.
5. Repeat with screen static and animated; compare display DMA impact, transfer time and audio peaks. Record 10/20/30 FPS target tests and dirty area percent.
6. Toggle nature sounds / drums / binaural individually and in combination; find real CPU spikes, memory leaks and phase/drift.
7. Test repeated note-ons, changing musical mode/tuning while notes ring, long performance, all 7 encoders, keypad/keys with blank-screen power state.
8. Test CPU1 enabled and safe-disabled; hard fault/load tests without hangs.
9. Bench stock -> new and new -> new updater behavior in browser; reject wrong firmware, corrupted archive, unplugged/reconnected MIDI, cancelled download; protect boot and user sectors. User owns real-hardware confirmation.
10. Hold publication unless release firmware/source/installer hashes match and every mandatory scope item has completed evidence. Preserve a compact checked-out manifest/checkpoint and GitHub copy.

**Proposed initial engineering targets (not measured claims):** 44.1 kHz stereo; 256-frame half buffers / 5.8 ms if selected HAL uses them; worst-case render ideally < 3.5 ms (~60% of deadline), 0 xruns; original 12-voice arrangement baseline; 30 FPS flagship visualizer with a 20/10 adaptive fallback; exact Hz with calibrated output; no master-volume startup blast; safe update. Do not freeze voice/FPS numbers until profiling.

## 9. Gaps and facts not to overclaim
- Exact silicon part suffix, external DAC/codec identity, precise sample clock, I-cache geometry, true maximal audio polyphony, DMA contention limits, and reliable second-core behavior are either disputed or device-dependent.
- '60 MHz SPI' is a register/peripheral rate, **not** sustainable end-to-end full-frame FPS.
- The DUB-1 SRAM measurements are published **one-device** empirical data, not a benchmark of our as-yet-unbuilt firmware.
- SLOOP/other upstream versions change rapidly. Pin current commits when building.
- No physical FM-1 tests have been performed in this research session.
- Music described as 'sound healing', 'therapy' or 'Solfeggio' is a design/intention framework; do not market the device as treating medical conditions or claim proven effects for specific frequencies. Binaural beats are optional relaxation audio; use sensible listening volume.

## Sources checked November 3, 2026
1. Hardware Overview — https://sbegueria.github.io/fm1-dev-wiki/Hardware-Overview.html
2. Performance / DUB-1 hardware A/B — https://sbegueria.github.io/fm1-dev-wiki/Performance.html
3. Display / SPI / backlight — https://sbegueria.github.io/fm1-dev-wiki/Display.html
4. Audio Output / DMA deadline — https://sbegueria.github.io/fm1-dev-wiki/Audio-Output.html
5. Second Core / failure fallback — https://sbegueria.github.io/fm1-dev-wiki/Second-Core.html
6. Clocks and Power — https://sbegueria.github.io/fm1-dev-wiki/Clocks-and-Power.html
7. Memory Map — https://sbegueria.github.io/fm1-dev-wiki/Memory-Map.html
8. Flash Storage — https://sbegueria.github.io/fm1-dev-wiki/Flash-Storage.html
9. Update Protocol — https://sbegueria.github.io/fm1-dev-wiki/Update-Protocol-and-Installers.html
10. Choosing a Base — https://sbegueria.github.io/fm1-dev-wiki/Choosing-a-Base.html
11. Known Projects — https://sbegueria.github.io/fm1-dev-wiki/Known-Projects.html
12. SLOOP — https://github.com/isod89/sloop-fm1/blob/main/SLOOP.md
13. SLOOP releases — https://github.com/isod89/sloop-fm1/releases
14. SLOOP installer guide — https://github.com/isod89/sloop-fm1/blob/main/GUIDE.md
15. fm1-base — https://github.com/charlesvestal/fm1-base
16. M-VAVE FM1 Open — https://github.com/m-vave/fm1_open
17. Community firmware hub — https://github.com/aroum/fm1-custom-fw
18. FM1-nes — https://github.com/Keitark/fm1-nes

## Workflow rule
This document is the *reference*, not the active task queue. Keep one short CURRENT_SCOPE.md with only open requirements, one dated changelog for completed tasks, and a verifiable software/hardware acceptance matrix. Continue the entire build rather than stopping after a minor patch. If session/tool limits interrupt it, preserve checkpoint and use supported continuation; never claim impossible uninterrupted execution.
