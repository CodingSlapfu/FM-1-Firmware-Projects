# FM-1 Firmware Projects

## EIN-1

The EIN-1 0.32.0 camera engineering checkpoint includes firmware, installer, emulator, source, checksums, and rebuild instructions. Firmware release and physical FM1 qualification are pending.

Restore from a checkout of this repository:

```sh
cat EIN-1/0.32.0/camera.zip.part* > EIN1-0.32.0-camera-checkpoint.zip
printf '%s  %s\n' '30749da124ff7b2c5c0095e0de6d28ebbcd2d4dd9079882da8735a9c16a29572' EIN1-0.32.0-camera-checkpoint.zip | sha256sum -c -
unzip EIN1-0.32.0-camera-checkpoint.zip
```
