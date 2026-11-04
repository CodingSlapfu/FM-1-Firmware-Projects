# FM-1 Firmware Hardware and Performance Engineering Standard

Research date: **2026-11-03**. Applies to the new **FM-1 Sound Healing** project and all future M-VAVE FM-1 firmware builds. This is an engineering reference, **not evidence that our new project has been compiled for or tested on the device**.

## Evidence labels
- **[HW]** Observed/benchmarked on community FM-1 hardware; measurements may be one device only.
- **[SDK/CODE]** Confirmed from public SDK, compiled firmware, build files, or working implementations.
- **[DESIGN]** Our proposed standard, not yet measured.
- **[OPEN]** Must be determined by actual hardware or SDK verification.

## 1. Platform facts and boundaries
| Item | Finding | Evidence |
|---|---|---|
| Processor | JieLi AC791N/WL82, pi32v2, two cores; target `-mcpu=r3` | [SDK/CODE] |
| Floating-point | Single-precision hardware FPU available with `-mfprev1`; avoid double-precision DSP | [SDK/CODE]; validate on selected toolchain |
| CPU clock | Stock V15 bootloader measured at 360 MHz; m-bootloader 1.2 at 320 MHz, ~11% slower. SDK advertises up to 320 MHz. V16 boot area may be configured for 320 MHz | [HW] [OPEN for V16] |
| Addressable SRAM | 512 KiB working firmware window, `0x01C00000–0x01C7FFFF` (SDK states 578 KB total); no external RAM installed | [SDK/CODE] |
| SPI NOR flash | 1 MiB physical, but Felucca-style application slot ~581,564 bytes (`0x8DFBC`) | [HW/CODE] |
| Audio | ALNK0 stereo DMA; typical 44.1 kHz with 256-frame ping-pong halves; hardware sample rate under stock may be near 44,118 Hz | [CODE] [OPEN exact rate] |
| Screen | 240 x 240 RGB565, ST7789-like SPI1 panel; no tearing (TE) wire | [HW] |
| Output | Independent stereo samples, 24-bit left-justified in `int32_t` DMA words in Felucca-family driver | [CODE] |
| Inputs | 27 keys, 14 buttons, 7 encoders, master pot; limited UI physical controls | [HW] |

Do not equate physical 1 MiB SPI flash with available app code + preset/sample space. Read the linker map, partition map, and recovery/bootloader constraints for the chosen base.

## 2. Actual published community benchmarks (NOT our own)
**DUB-1: same DSP source, flash vs hot code in SRAM, one FM-1, 360 MHz bootloader, 256-frame blocks (5.80 ms).** For each composition, heaviest block in a ~2s window:
| Piece | Flash peak | SRAM peak | Flash -> SRAM dropouts |
|---|---:|---:|---:|
| Haze | 5.1 ms | 3.7 ms | 2 -> 0 |
| Night Drive | 4.7 ms | 4.4 ms | 1 -> 0 |
| Fog | 4.6 ms | 3.0 ms | 1 -> 0 |
| Pressure | 4.5 ms | 3.4 ms | 1 -> 0 |
| Steppers | 4.6 ms | 3.3 ms | 1 -> 0 |
The wiki characterizes this as 6–35% lower worst-block latency, 5–25% lower mean and no dropouts in these short tests. Moving `~55 KB` hot audio code into SRAM has a cost in precious SRAM; confirm net benefit on *our* firmware.

**SLOOP / sloop-lab dual-core:** on one pattern with cores 1/2 toggled, core-0 audio load reportedly changed **31% -> 23%** (about 26% less on core 0). SLOOP 2.7 advertises about twice the available headroom for sound, but this is a qualitative project claim, NOT a general guaranteed 2x speedup. SLOOP 2.8.3 includes checks against real core-1 mailbox corruption on certain units. Explicit single-core fallback is essential.

## 3. Audio deadline and measurement standard
- At 44,100 samples/s, 256 frames take **5.805 ms**. Render and commit the next half **before the DMA free half changes**. A longer average is not acceptable when rare worst blocks glitch.
- Use the **24 MHz independent TIMER4** to measure block and DSP-stage times. Do not assume the core cycle counter is enabled.
- Measure *per-block total*, oscillator/voices, drums, soundscape decode, FX, final mix, UI impact, USB servicing, main-loop overrun; save mean, p95, p99, max per 2-second window, and cumulative DMA xruns.
- Capture separate **core 0 run time, core 1 busy time, core 0 wait time**, mailbox invalid jobs, late jobs, faults, safe fallbacks, clock, build SHA, RAM high-water, flash size and per-page FPS.
- Device test must compare 1-core/2-core modes, light/worst musical arrangements, with/without soundscape/FX, busiest visualizer, USB/BT off/on, and known bootloader clock variants.
- **[DESIGN acceptance targets]** zero xruns in a 60-minute max-feature hardware soak, peak *audio block* <70% of available deadline after warmup, normal p99 <55%, CPU-1 fallback without crash or frozen audio; if those targets cannot be met, simplify sound/FX/graphics before release. These are proposed release goals, NOT established FM-1 benchmarks.
- Include a debug screen with build version and measurements. Never declare optimized from a desktop profiling result alone.

## 4. SRAM and flash execution placement (high priority)
- Put audio interrupt, master mix, critical oscillators, voice render, per-sample filter and highest-frequency FX loops in an explicit `.fast_text` SRAM section, loaded from flash at startup. M-VAVE's FM1 Open reportedly marks ~60 render/mix/FX functions `sram_required`.
- Keep cold UI, menus, setup, seldom-used modulation and images in XIP flash. Profile cache thrash when graphics and other-core work execute concurrently.
- In the linker: size-bound `.fast_text`, verify copy, flash-to-RAM **far calls**, execute guard openings, and no flash-dependent code while SFC is writing. Validate memory map and reserved vector/boot mailbox at `0x01C7FD50` and above.
- SRAM is not free: allocate headroom for DMA buffers, stacks/guards, voice state, effects, UI canvas, soundscape cache and error log. Do not commit to arbitrary fixed budgets without a map.
- Avoid dynamic allocations in real-time render, unbounded tables, sample fetches requiring flash stalls, large stack frames, or long critical sections.
- From the FM-1 wiki: optimized memcpy/memset in fm1-base reportedly improved by **7–9x** over early byte loops; measure buffer movement/clear paths.

## 5. Dual-core architecture (recommended, but not blindly always-on)
- Core 0 **owns ALNK0 DMA** and the final output mix, event sample-accuracy, limiting and underrun detection.
- Core 1 is an optional **one-job-at-a-time** DSP worker, not a second copy of application state: it may render independent voices, drums, one ambience component, or a precomputed effect bus to a separate accumulator.
- Both sides write to **separate cache-aligned mailbox lines**, publish a sequence/check-word, validate input addresses and job identity before execution, and return an acknowledged descriptor/result. Avoid both cores writing one musical state; choose single-owner controls and explicit snapshot handoff.
- Issue the worker job, render core 0's own audio work in parallel, then merge the buffers **without exceeding the current DMA deadline**. A published 8 ms core-1 timeout is **longer than a 5.8 ms audio half**, so do not naively wait that long in our render ISR; benchmark bounded deadlines/pipelining or proactively shed optional work. Never substitute a long blocking wait for a safe fallback.
- Enable clocks/supply levels only via tested BSP startup sequence; some units misread shared SRAM/flash at heavy CPU/radio load. Validate the second core at boot, count late/bad jobs, quarantine it until reboot on failure.
- Always compile and test single-core mode. On fallback, shed costly FX/voices/soundscape resolution gracefully rather than glitching or rebooting. Expose Core 1 ON/AUTO/OFF if appropriate.
- Run golden-audio comparisons in 1-core, 2-core, delayed worker, missing worker, corrupt mailbox, fault and forced CPU1-OFF host modes; ThreadSanitizer for host version where possible.
- Pipelining one block behind is a candidate for reverb or texture buses only when latency/feedback design permits; assess phase coherence and transitions.

## 6. CPU and compiler optimizations
- Compile hot DSP with `-O2`, optional LTO after performance A/B; cold control code with `-Os`/`-Oz`. Candidate flags in official FM1 Open include `-target pi32v2 -mcpu=r3 -mfprev1 -flto -ffunction-sections -fdata-sections -fno-common`, with `--gc-sections`; test effect, do not assume every flag helps.
- Use **float** and `1.0f`, `sinf` when needed rather than accidental software doubles. Also A/B integer Q-format vs float for oscillator/filter banks; some firmwares use integer math for predictability. Confirm toolchain semantics.
- Note a reported **32x32->64-bit multiply / 16-bit load hardware/compiler interaction** that occasionally corrupted a register in a decoder. Study felucca-bsp `docs/CPU.md`; if relevant, audit disassembly and use tested `noinline` wrappers. Treat as project-specific reported erratum, not verified on all devices.
- Replace division in audio loops with precomputed reciprocals; control-rate smoothing and envelopes every 32–128 samples when perceptually acceptable; schedule parameter recomputation between blocks, never recalc entire patches in one ISR.
- Flush subnormal tiny feedback and protect clipping. Allocate voices by perceptual importance (bass, fundamental tones, lead, then textures). Start-up master level must begin muted until output is ready.

## 7. Display / graphics optimization
- 240x240 RGB565 full frame = **115,200 bytes**. At 30 FPS, moving the full frame consumes **3.456 MB/s**; at 60 FPS **6.912 MB/s**, before protocol overhead.
- Measured SPI effective throughput is around **44 MHz** at the fastest setting (~5.5 MB/s), so one complete frame theoretically takes **~20.9 ms** of bus time even before setup/CPU work. A naive 60 FPS full-screen redraw is physically out of reach on that bus. A 30 FPS full redraw may consume most transfer bandwidth and cause audio/cache pressure. These are theoretical transfer estimates, not panel frame benchmarks.
- Use **dirty rectangles/tiled/strip buffers**, small DMA strips and asynchronous transfers; build next strip while previous DMA writes, use fixed-size SRAM buffers. Never issue a DMA count of zero.
- Static background and fonts should be cached or rendered only when changed. Animate only the reactive layers and use a frame-time budget. Decouple visualizer UI update from audio ISR; drop visual frames before audio blocks.
- Display has **no TE line**; occasional tearing cannot be completely eliminated. Do not claim tear-free graphics without actual measurement.
- Critical hardware caveat: PA2 reportedly controls **both backlight and key matrix enable**. Merely turning the backlight pin off may disable keys. Any battery saver must be tested physically and preserve controls; use safe dimming only when verified.

## 8. Sound Healing instrument design within the hardware envelope
- Generate accurate Solfeggio fixed tones and A4=432 Hz music tuning in separate systems; do not confuse Hz selection with a whole-song reference pitch.
- Generate optional binaural beats on **distinct stereo channels**, stable carrier and beat difference frequencies, soft fades and low listening level, with headphones guidance. Test exact L/R spectral peaks and no accidental mono collapse. Binaural beats are an audio illusion, not a proven medical treatment.
- Prefer efficient algorithmic instruments and carefully budgeted small sampled transients. Percussion/hand drums may be synthesized or represented by short original samples to avoid high memory use. A long stereo 16-bit 44.1k recording consumes **176,400 bytes per second** before compression, incompatible with unrestricted in-RAM beds.
- Nature soundscapes: procedural noise textures and/or small compressed loops, ideally only one or two active layers, low control-rate modulation and crossfades; never do complex file decode, resampling, allocation or flash erase in audio ISR.
- Allocate reverb/delay buffers to a validated small ring and tune sample rate, feedback, damping, and tail lifetime. Consider mono effect feeds with stereo decorrelation when CPU/RAM constrain; protect stereo output essential for binaural listening.
- At audio peak threshold, degrade optional texture/graphics/FX first; preserve bass/frequency tone, direct instruments, drums' attack and phase continuity. Validate heavy scene with ALL chosen features enabled.

## 9. Firmware base, tooling, licensing
**First candidate: [fm1-base](https://github.com/charlesvestal/fm1-base) and [felucca-bsp](https://github.com/charlesvestal/felucca-bsp) (MPL-2.0, beta)** — platform, DMA, UI, simulator, browser tools, optional second core and shared hardware fixes, while the *instrument engine and UI remain new*. Verify current repository names/paths and API before importing. Alternative: official [m-vave/fm1_open](https://github.com/m-vave/fm1_open) with its binary SDK and Windows-specific toolchain. Felucca/SLOOP source is **GPL-3.0**, so reuse only with full licence compliance; research behavior without copying it into a differently licensed codebase.
- Felucca-family native chain: JieLi pi32v2 clang 4.0.1, AC79 SDK loader/config files, reproducible pinned hashes and Python packager. Official FM1 Open V14 has its own Windows x64 toolchain 2.5.3 and PowerShell build.
- Build device ELF, map, `.fwsc`, simulator/web version, tests and software acceptance in one run, all version-matched. Store actual firmware source and checksums in GitHub before publication.
- Avoid flash identity/update regressions: implement correct USB-MIDI device-pull SysEx handshake, staged loader, post-update verification, rescue shortcut, and test in-place **new version -> next version**. The standard sequential app flash erase has a vulnerable interval; study ChoralRoot/TransitFM1's boot-stub/first-sector-last work as a design option rather than assume rollback or dual-bank hardware.
- Preserve physical flash backup and unbrick instructions. A host test cannot prove a safe update on all physical FM-1 units.

## 10. Mandatory acceptance matrix for each future release
| Scenario | Evidence required |
|---|---|
| Idle, average and worst musical scene | 2s-window max + p95/p99 audio time, overrun count, voice count, both cores |
| Hardest combination: drums+bass+binaural+2 beds+reverb+visualizer | CPU/peak/DMA misses, limiter clipping count, stereo channel tests |
| UI visible, visualizer active, page changes and knob spins | Audio peak impact, verified FPS, no disabled controls |
| 1-core fallback, late worker, corrupt mailbox, mixed USB traffic | No crash, stable continued sound, error counters |
| 320/360 MHz boot environments where supported | Report actual register/clock, don't extrapolate from one clock |
| Repeated power cycles, startup volume 0, output disconnect | No blast/buzz, safe startup, crash diagnostics |
| Frequency/tuning fidelity | Output sample-rate calibration, per-channel sine frequency, phase stability, L/R separation |
| 1 hour soak minimum on physical unit | 0 xruns target; max stack watermark; no memory leak/drift |
| Web installer + direct firmware upgrade + recovery procedure | Verified correct identity and source-to-package hashes; physical test tracked separately |

**No completed claims from this research alone.** The next build must gather device-specific numbers and revise targets if reality contradicts our proposed engineering budget.

## 11. Open questions to test
1. Actual boot clock and LSB under stock V16 on the user's hardware.
2. Exact playback sample rate, usable analog output band and stereo separation of specific units.
3. Core-1 reliability under simultaneous LCD DMA and USB backup; cache-line coherency barriers.
4. Which hot routines produce biggest improvements in SRAM for our own synth/ambient/FX mix.
5. Exact memory map/use under chosen platform, with Bluetooth, USB audio and visualization enabled.
6. Hardware clock/sample-rate tolerance over battery/temperature variations.
7. Actual SPI transfer efficiency and graphics FPS under concurrent audio load.
8. Safety/recovery of in-place upgrade on device, including interrupted or same-identity builds.

## Sources / provenance
- FM-1 Developers' Wiki hardware: https://sbegueria.github.io/fm1-dev-wiki/Hardware-Overview.html
- Memory map: https://sbegueria.github.io/fm1-dev-wiki/Memory-Map.html
- Clocks: https://sbegueria.github.io/fm1-dev-wiki/Clocks-and-Power.html
- Audio output: https://sbegueria.github.io/fm1-dev-wiki/Audio-Output.html
- Display: https://sbegueria.github.io/fm1-dev-wiki/Display.html
- Performance + DUB-1 A/B: https://sbegueria.github.io/fm1-dev-wiki/Performance.html
- Second core + errata: https://sbegueria.github.io/fm1-dev-wiki/Second-Core.html
- Programming guide: https://sbegueria.github.io/fm1-dev-wiki/Programming-Guide.html
- Choosing a base: https://sbegueria.github.io/fm1-dev-wiki/Choosing-a-Base.html
- fm1-base details: https://sbegueria.github.io/fm1-dev-wiki/FM1-Base.html
- Safe updating: https://sbegueria.github.io/fm1-dev-wiki/Update-Protocol-and-Installers.html
- SLOOP dualcore experiments, integrity and published 31% -> 23% measurement: https://github.com/isod89/sloop-fm1/blob/main/DUALCORE.md
- SLOOP releases 2.7–2.8.4: https://github.com/isod89/sloop-fm1/releases
- Official open platform and toolchain: https://github.com/m-vave/fm1_open
- FM-1 reverse-engineered stock engine: https://github.com/AL-255/FM-1-RE/blob/main/docs/architecture.md

**Use this document in every new project:** at startup read this standard, then record actual device measurements and update only hypotheses disproved by evidence. Do not import retired project product scope.
