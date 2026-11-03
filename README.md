# FM-1 Firmware Projects

EIN-1 0.32.0 engineering checkpoints. Release qualification is pending.

Restore the latest AIR display checkpoint from a checkout:

```sh
base=EIN-1/0.32.0
cat "$base"/graph.zip.part* > graph.zip
unzip graph.zip -d restore
cp "$base"/resume/source/web/fm1ota.js restore/source/web/
for f in RESUME.md SHA256SUMS.txt native-validation.log source/firmware/src/app/ui.c; do cp "$base/air-display/$f" "restore/$f"; done
for f in amb-0.32.0.fwsc emu/amb.wasm install-0.32.0.html; do cat "$base/air-display/$f".part* > "restore/$f"; done
(cd restore && sha256sum -c SHA256SUMS.txt)
```

Earlier camera, graph, and installer-resume files are retained.
