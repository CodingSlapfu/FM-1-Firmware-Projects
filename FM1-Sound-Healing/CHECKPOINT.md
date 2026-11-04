# Recoverable checkpoint — 2026-11-04
New project only. Original Library handoff and pinned platform/SDK packaging files recovered.
The prior local session produced a development native package and passing host tests, but its workspace was cleared before backup. Those binaries and test results are NOT current acceptance evidence.
Reconstructing device integration, audio fixes, UI, tests and release pipeline; native build and tests must be rerun.
Platform: charlesvestal/fm1-base 2d8ed748cdade582c65d9fc8d8cf17da4ddadec8; BSP hugelton/felucca-bsp 773b0b99f5ade1a30d1879b2618c6c3c3e6ba031 (vendored with licenses).
AC79 packaging files: SDK archive 5fa84a9b2f723a5e9337ad79dc2ad741fc98fe2d; Apache-2.0.
Compiler: JieLi 20260730.1; archive SHA256 a1545cddbc451a06bac43eb5cd96d831470c42a97c5c0b1f98879a20fc7647bd. Extract with Python or tar --no-same-owner; verify executable bytes before using.
Website retirement deployed successfully: Sites version 168, source 44767e25b66fd52dd453d23bfd723be4456c05f7, https://parallax8-studio.jminie66.chatgpt.site. Only development landing and 404 pages are served. Original source/history preserved. Existing owner-private access retained; unauthenticated root and installer returned 401; alternate aquacreek8 route returned 307. Do not claim public route inspection succeeded.
Lock: /workspace/.fm1-sound-healing-build.lock, nonblocking exclusive flock across implementation/tests/publication.
No firmware release is yet accepted or published.
