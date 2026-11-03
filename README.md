# FM-1 Firmware Projects

EIN-1 0.32.0 RSSI engineering backup. Release qualification is pending.

```sh
base=EIN-1/0.32.0
cat "$base"/graph.zip.part* > graph.zip
unzip graph.zip -d restore
cp "$base"/resume/source/web/fm1ota.js restore/source/web/
cp "$base"/air-calibration/native-validation.log restore/
for f in RESUME.md SHA256SUMS.txt source/firmware/src/app/ui.c source/firmware/src/app/air_control.h; do cp "$base/air-rssi/$f" "restore/$f"; done
for f in amb-0.32.0.fwsc emu/amb.wasm install-0.32.0.html; do cat "$base/air-rssi/$f".part* > "restore/$f"; done
(cd restore && sha256sum -c SHA256SUMS.txt)
```
