# PAUSED — user instruction, 2026-11-04
NEW Sound Healing project only. Do not resume retired products.

## Saved work
Native application, 24 algorithmic patches, procedural nature beds, 3 drum kits, fixed-frequency tones independent of A4 tuning, stereo binaural tones with glides/fades, four-page physical UI, geometry visualizer, bounded control queue, diagnostics, optional core-1 geometry worker with single-core fallback, startup master-zero changes.
Source is developmental; not an accepted release. Settings are session-local. Hardware tests remain user-owned.

## Exact next step
Latest native link FAILED after expanding SRAM placement from waveform-only to the full per-sample renderer. Inspect evidence/native-build.log and fix remaining XIP/SRAM far-call relocations without removing the safety checks. Last successful native image used the earlier waveform-only SRAM implementation and optional geometry worker (90572-byte app, 26284-byte .data+.bss, 72192-byte pool). DO NOT publish that older binary with current source.
After fixing: rebuild native 0.1.0, run host UI, DSP/ASan/UBSan, stereo spectrum, worker ok/off/late/bad/fault comparisons; compile forced single-core; inspect screen output. Then finish emulator and installer, package/hash, back up full source + matching artifacts, deploy and verify. Active scope has NOT been marked completed.

## Environment/dependencies
Project lock /workspace/.fm1-sound-healing-build.lock; acquire exclusive nonblocking before edits/build/publication.
JieLi Linux toolchain 20260730.1 archive SHA256 a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd; pi32v2 clang -mcpu=r3 -mfprev1. Verify extracted bytes; tar ownership errors previously truncated clang. Python extraction repaired it.
Vendored fm1-base 2d8ed748cdade582c65d9fc8d8cf17da4ddadec8, BSP 773b0b99f5ade1a30d1879b2618c6c3c3e6ba031. Their complete upstream sources can be recloned at those pins. Preserve the saved startup-master modifications.
AC79 package inputs come from SDK 5fa84a9b2f723a5e9337ad79dc2ad741fc98fe2d (Library EIN1-AC79-SDK-5fa84a9b-verified.zip; compiler/SDK only, NOT retired project source). Extract cpu/wl82/tools/uboot.boot, cfg_tool.bin, cfg/eq_cfg_hw.bin and LICENSE to vendor/ac79-package-files. Selected SDK differs from fm1-base's older reference hashes; pin/record actual hashes and investigate before release.
Emscripten 3.1.64 install was interrupted during 301 MB wasm binaries download. TAR_OPTIONS=--no-same-owner fixes emsdk extraction ownership errors. apt installation is not complete; its continuation was blocked by automatic review after Pause. Do not restart anything while paused.
Native command: JIELI_TOOLCHAIN=<toolchain-root> python3 vendor/fm1-base/tools/build.py device --sdk vendor/ac79-package-files --release 0.1.0
Host: python3 vendor/fm1-base/tools/build.py device --host --release 0.1.0
Upstream make_site.py requires a real build/emu/app.wasm; do not substitute a placeholder.

## Durable recovery
GitHub main already held checkpoint 211c484; this checkpoint adds current device sources, tests, relevant evidence and platform changes. Original prototype handoff remains in /FM1-Sound-Healing/ in Library. Direct git push lacks authentication; use GitHub create_tree/create_commit/update_ref with expected head, preserving all unrelated paths.
Website retirement survived: source 44767e25b66fd52dd453d23bfd723be4456c05f7, successful Sites publication at https://parallax8-studio.jminie66.chatgpt.site, project appgprj_6ad6d0ae409c8191b45224ecdec724db. Development landing + 404 only, existing owner-private audience preserved. Unauthenticated root/install returned 401; alternate aquacreek8 URL returned 307. Public route behavior not verified. No new firmware installer deployed.

## Test evidence limits
The recovered earlier source passed native compile, host navigation/zero-master tests, spectral presence of 11 tones, stop silence, 60-second worst-feature host render, ASan/UBSan (leak detection unavailable in this runtime), and simulated updater port/identity/corruption tests. Current SRAM refactor and worker fault modes still need revalidation. Desktop timing is not FM-1 timing. No physical device claims.
