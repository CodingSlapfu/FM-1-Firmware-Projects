# Sound Healing 0.1.0 — FM-1

An original generative musical instrument for reflective listening. No medical benefit is claimed. Acoustic names describe synthesized instruments, not recordings. This first software build awaits physical FM-1 verification.

## Start
Turn the physical master down, install, then press PLAY and raise the master gradually. Playback starts only when requested. PLAY stops generation with a short voice fade and effect tails. Musical keys play the selected instrument, including while the generator is stopped.

## Navigation
ALGORITHM selects PLAY, SOUND, EVOLVE, MIX. SELECT chooses one of eight musical roles. PRESET chooses a scene on PLAY, and the selected track's patch on other pages. These controls never change their navigation meaning. HOME returns to PLAY. SEL mutes the selected role; its name dims. EDIT cycles SOUND's five groups. The selected track number and patch remain visible.

| Page or SOUND group | K1 | K2 | K3 | K4 |
|---|---|---|---|---|
| PLAY | Scene | Energy | Fixed tone level | Nature level |
| SOUND / Instrument | Patch | Track level | Pan | Reverb |
| SOUND / Nature | Bed | Bed level | Reverb | Delay |
| SOUND / Frequency | Fixed Hz | Tone level | A4 432/440 toggle | Energy |
| SOUND / Binaural | Carrier Hz | Difference (0.5 Hz steps) | Binaural level | Enable toggle |
| SOUND / Percussion | Kit | Drum level | Density 0–3 | Enable toggle |
| EVOLVE | Motif variation | Freeze toggle | Tempo | Energy |
| MIX | Track level | Pan | Reverb | Delay |

ENV toggles A4 432/440. FX toggles binaural tones. ARP toggles percussion. SEQ freezes/unfreezes motif replacement. GLO opens performance diagnostics. REC forces single-core graphics until reboot. Turn a toggle encoder once to toggle its setting.

## Frequency and stereo
Fixed tones: 174, 285, 396, 417, 528, 639, 741, 852, 963, 111 and 136.1 Hz. Tone level defaults to zero; raise it on Frequency or PLAY. A quieter octave-reduced supporting bass accompanies this tone. A4 tuning changes musical notes separately.

Binaural mode defaults OFF. Left = carrier minus half the difference; right = carrier plus half the difference. Example: carrier 220 Hz, difference 6 Hz gives left 217 Hz and right 223 Hz. Use stereo headphones; speakers and mono are not equivalent. Tones bypass shared effects to retain channel separation, then pass through the output limiter/master. Changes glide and enabling fades in. Start at low volume.

## Instruments and arrangement
24 original algorithmic patches cover bass, drones, piano, plucked strings/guitar, flutes, woodwinds, bells, choir, strings and atmospheric pads. Six scenes vary scale, tempo, density and nature selection. Motifs vary at phrase boundaries; Freeze preserves the motif while its arrangement continues. Sixteen bounded voices share eight tracks. Three synthesized percussion kits offer light rhythms. Seven procedural nature types avoid streaming/decoding loads.

## Diagnostics and engineering
The diagnostics show peak render microseconds since boot; two-second mean, p95/p99 (100-microsecond bins); DMA xruns; CPU percentage; and queue drops. These are real measurements only on the device. Host numbers are not FM-1 benchmarks. Graphics reduce refresh when reported audio load exceeds 60%. Core 1 optionally computes geometry from an isolated snapshot. A 2 ms worker timeout, invalid job or fault retires the worker and recomputes on core 0. No audio callback waits for it. REC forces single-core mode until reboot. No hardware speedup is claimed. Startup master is zero until ADC input arrives. The hottest waveform routine runs from SRAM; no dynamic allocation or decoding occurs in the renderer.

## Update and recovery
Use Chrome or Edge with Web MIDI and SysEx permission, connect FM-1 by USB, open the installer and select its ports. The installer checks package integrity and follows the firmware/loader/restarted device identity. Keep power connected. The bundled updater path preserves the boot area; interrupted application writes still require recovery and are not dual-bank rollback.

If normal startup fails, hold OCT- while powering on to request the platform rescue path. OCT- and OCT+ held together for five seconds enter update mode on the running platform. These procedures come from the selected platform and remain to be confirmed on your FM-1. Preserve a stock backup and the existing independent recovery archives. Do not install another bootloader merely for this release.

## Current limitations
Settings are session-local; power cycling resets them. No sampler, Bluetooth, mirror, looper or retired-product functionality is included. One nature bed is selected at a time. Physical clock calibration, sound quality, stereo separation, sustained CPU budget, display FPS, controls, direct version-to-version updating and interrupted-update recovery still require hardware testing.
