# FM-1 Firmware Projects

EIN-1 0.32.0 engineering checkpoints. Release qualification is pending.

```sh
cat EIN-1/0.32.0/graph.zip.part* > graph.zip
printf '%s  %s\n' '8670945b0bc71d644474257512cf5acdad5aa6381cfdfa1da4e3a466696b8764' graph.zip | sha256sum -c -
unzip graph.zip
```

The earlier camera checkpoint remains available as camera.zip.part files.
