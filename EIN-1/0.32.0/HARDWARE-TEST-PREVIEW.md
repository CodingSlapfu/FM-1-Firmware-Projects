# EIN-1 0.32.0 hardware-test preview

[Download firmware, browser installer, emulator, source and checksums](https://chatgpt.com/library?file_id=libfile_dcc312f910748191bf1692b84835d63f&account_id=personal)

The package is backed up at commit 8987a98f1b967c7515074f93d63c5494f5174f8b. Follow the repository README at that commit to reconstruct the files and verify their checksums.

Native build, simulated OTA and Safe Mode recovery, emulator workflow, and embedded firmware integrity checks passed. Full software acceptance and physical FM1 qualification are pending.

## FM1 checks

- Verify direct EIN-1 updating and the reported installed version. Keep known-good recovery firmware available.
- Compare audio and display responsiveness with Living Soundscapes on and off, including PRESETS changes.
- Check sustained USB Mirror streaming.
- Check AIR calibration, hand response, signal loss and in-key octave routing.
