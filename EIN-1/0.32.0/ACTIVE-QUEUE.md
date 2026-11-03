# EIN-1 active queue

Authority: EIN1_NEXT_RELEASE_SCOPE.md and EIN1_RESOLVED_FIXES_REGISTRY.md. Baseline: 21eb51c57c85c911cdd6f7987b5d34e67cc396a8.

| Area | Status and next action |
| --- | --- |
| Startup volume blast | RESOLVED / PRESERVE / REGRESSION CHECK ONLY. main_fm1.c still gates gain on ADC readiness. Existing runtime ADC host regression passed. Remove historical patch from implementation queue. |
| Navigation, Intentions, Bass, Strum, Saver, sampler removal | IMPLEMENTED / PRESERVE. Host workflow and WASM checks passed. Physical control and LCD checks pending. |
| AIR calibration and pitch | PARTIAL. Existing octave routing, FAR/NEAR capture and pitch readout are implemented. Pending working-source change in ui.c distinguishes failed capture reasons and passed host workflow. Live calibration diagnostics, temporary audible test and exact applied pitch evidence remain open. |
| SHAPE and EVOLVE | IMPLEMENTED / VALIDATE. Live graph paths exist and graph checkpoint corrects sample-rate timing. Full graph acceptance remains open. |
| Visualizer | IMPLEMENTED / VALIDATE. Camera and note shimmer are integrated. Verify remaining music-reactive behaviors and physical pacing. |
| Audio and PRESETS transitions | OPEN. Host stress and PCM checks passed. Trace and measure target deadlines and transition spikes. |
| USB Mirror | HARDWARE TEST PENDING for continuous streaming. Host packet reconstruction and browser restart checks passed. |
| Direct update and Safe Mode | HARDWARE TEST PENDING. Simulated staging, loader, browser handoff and resume checks passed. |
| Release | OPEN. Batch remaining software work, build matching artifacts, verify backups and publish hardware-test candidate. |
