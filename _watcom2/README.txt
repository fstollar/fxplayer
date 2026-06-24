OpenWatcom V2 toolchain — not included in the repo
====================================================

The compiler binaries go in _watcom2/ow2/ (this directory is gitignored).
Only this README is tracked.


Download
--------

Get the Linux snapshot from the Open Watcom V2 GitHub releases page:

  https://github.com/open-watcom/open-watcom-v2/releases

This project uses the 2026-06-19 "Current-build" snapshot
(wpp386 reports "Version 2.0 beta Jun 19 2026"). Any snapshot from around
that date or later should work.

Download the Linux tarball: ow-snapshot.tar.xz


Unpack
------

Run from the repo root (or from inside _watcom2/ — adjust path accordingly):

  tar -xf /path/to/ow-snapshot.tar.xz -C _watcom2/

The tarball unpacks into an ow2/ subdirectory, giving you:

  _watcom2/ow2/binl64/wpp386   <- C++ compiler (Linux 64-bit host)
  _watcom2/ow2/binl64/wasm     <- assembler
  _watcom2/ow2/binl64/wlink    <- linker
  _watcom2/ow2/h/              <- system headers
  _watcom2/ow2/lib386/         <- runtime libraries

Verify the unpack with:

  ls _watcom2/ow2/binl64/wpp386


Build
-----

Once unpacked, cd into _work/ and run:

  ./build-dos.linux.sh

The name reflects both the output (DOS EXE) and the required host (Linux).
It uses binl64/ (Linux 64-bit OpenWatcom binaries) and will not run on Windows.
The script sets WATCOM and PATH itself — no shell environment changes needed.
Output: _work/FX.EXE (~156 K, PMode/W 32-bit DOS extender).

See BUILD.md at the repo root for full build instructions including DOSBox-X
setup and WAV reference rendering.
