# FM-1 Firmware Projects

EIN-1 0.32.0 engineering backups. Firmware release qualification is pending.

```sh
cat EIN-1/0.32.0/graph.zip.part* > graph.zip
cat EIN-1/0.32.0/resume/install.html.part* > install-0.32.0.html
printf '%s  %s\n' '8670945b0bc71d644474257512cf5acdad5aa6381cfdfa1da4e3a466696b8764' graph.zip 'eef728158822e4679f4fc901310baaf5ad7120df0c4af8303fabab5a58477a55' install-0.32.0.html | sha256sum -c -
```

The corrected installer uses the firmware from graph.zip. Its matching updater source is under resume/source. The earlier camera checkpoint is retained.
