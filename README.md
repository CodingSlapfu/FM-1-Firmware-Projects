# FM-1 Firmware Projects

EIN-1 0.32.0 AIR applied-modulation engineering backup. Release qualification is pending.

```sh
base=EIN-1/0.32.0
cat "$base"/graph.zip.part* > graph.zip
unzip graph.zip -d restore
cp "$base"/resume/source/web/fm1ota.js restore/source/web/
cp "$base"/air-rssi/source/firmware/src/app/air_control.h restore/source/firmware/src/app/
for f in RESUME.md SHA256SUMS.txt native-validation.log source/firmware/src/app/ui.c source/firmware/src/dsp/amb.c source/firmware/src/dsp/amb.h; do cp "$base/air-applied/$f" "restore/$f"; done
for f in amb-0.32.0.fwsc emu/amb.wasm install-0.32.0.html; do cat "$base/air-applied/$f".part* > "restore/$f"; done
(cd restore && sha256sum -c SHA256SUMS.txt)
```
