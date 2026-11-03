# EIN-1 0.32.0 shimmer engineering checkpoint

Unpublished and release incomplete. Builds on the port-fallback checkpoint. Seven crystal tracks now use real note velocity and attack to shape their shimmer, and a track-level held-note mask starts a fading tail when the last held voice releases. This changes display telemetry and rendering only. Multiple notes within one frame are represented by the latest strike, and overlapping notes share a release tail when the track gate closes.

Rebuild using the pinned JieLi toolchain and AC79 SDK documented in source/BUILDING.md and the prior checkpoint. The SDK commit is 5fa84a9b2f723a5e9337ad79dc2ad741fc98fe2d. Verify the compiler archive SHA-256 a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd. Run tools/verify_ble_abi.py and EN_BLE=1 python3 tools/build.py --sdk SDK_PATH --release 0.32.0. The packaged WASM was rebuilt with Zig 0.13.0.

Verified on this checkpoint: native compilation, production USB OTA staging simulation, WASM and JS workflow, installer preflight, exact 57,600-pixel native Mirror reconstruction, and AIR controller fault handling. Complete the active scope audit before release. Measure HD cadence and audio headroom, qualify AIR and continuous Mirror, and perform direct EIN-1-to-EIN-1 update and recovery tests on physical FM1. GitHub returned zero accessible repositories. A designated writable repository and verified independent backup remain mandatory before publication.

Camera checkpoint adds closer fullscreen framing and depth shading. Native and WASM rebuilt. Still unpublished.

Graph checkpoint adds sample-rate-aware EVOLVE timing and passed exact-package simulated recovery. Unpublished.

Installer resume now derives the expected model before verifying the rebooted device. Native firmware and emulator are unchanged from the graph checkpoint.

AIR pitch display shows requested modulation and last note frequency. Native and WASM rebuilt, matching installer verified, simulated OTA and Safe Mode passed. Unpublished.
