# Vendored third-party code

Copied in-tree (not git submodules) so the repo builds on its own.

| Directory          | Upstream                                             | Base      | License | How it is kept                                           |
| ------------------ | ---------------------------------------------------- | --------- | ------- | -------------------------------------------------------- |
| `iii/`             | https://codeberg.org/tehn/iii                        | `1b1cbe2` | GPL-3.0 | **git subtree**, v1.1.3 + main; our changes are commits on top  |
| `lua/`             | https://codeberg.org/tehn/lua                        | `86c9ee5` | MIT     | unmodified export; monome's Lua 5.4 fork (32-bit config) |
| `littlefs/`        | https://codeberg.org/tehn/littlefs                   | `e3eefe8` | BSD-3   | unmodified export; only `lfs*.{c,h}`, license and readme |
| `FREEPOET_Keypad/` | https://github.com/FREEPOET-OFFICIAL/FREEPOET_Keypad | `00893c1` | MIT     | unmodified export; register reference for our C driver   |

Not vendored: **pico-sdk 2.2.0** (large, with its own submodules). PlatformIO
installs it (`framework-picosdk`), or point CMake at a checkout with
`PICO_SDK_PATH`.

**Licensing:** iii is GPL-3.0, so firmware images built from `iii-open-grid/`
are GPL-3.0 as a whole.

## iii: local changes

`iii/` is a git subtree, so changes are ordinary commits in this repo:

- **One topic per commit**, touching only `third_party/iii`, subject prefixed
  `iii:`, so each one can go upstream as its own PR.
- **iii's Lua API stays the same**: existing scripts must run unchanged.
- **Nothing Open Grid-specific** inside iii; that lives in `iii-open-grid/`.
  Changes here should make sense on every iii device (grids, arc, Pico).
- **Measure** memory/speed changes before and after with
  `og_bench.lua` (in `iii-scripts`), and record the numbers in the table below.

| Make target              | What it does                                                        |
| ------------------------ | ------------------------------------------------------------------- |
| `make iii-changes`       | list our iii commits                                                |
| `make iii-patches`       | export them to `dist/patches/iii/`, with paths relative to tehn/iii |
| `make iii-pull-upstream` | merge the latest tehn/iii `main` into `third_party/iii`             |

To check a patch against upstream: `git -C <tehn/iii clone> apply --check <patch>`.

| Commit               | Change                                                           | Why / measured effect                                                                                                                                                                                   | Upstream  |
| -------------------- | ---------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------- |
| `e7107bf`            | `serial()`: don't write past the TX buffer                       | fixes RAM corruption when >16 KB is printed in one Lua call                                                                                                                                             | merged #10 |
| `3ee8dd2`            | stream Lua files into the compiler                               | no full source copy while loading: kria_iii (29 KB) now loads after og_bench's tests, was out of memory; fixups `9340efe` (stop at the NUL uploads end with), `f47cc2c` (no big buffers on the C stack) | merged #11 |
| `b45e254`            | stream uploads to flash, no 32 KB buffer                         | heap high water after uploading 4 files: 168 KB (was 197 KB after one upload, of 221 KB); no 32 KB script limit                                                                                         | merged #11 |
| `f8be883`            | no reboot on a failed allocation                                 | og_bench with kria_iii loaded no longer reboots the device (og-0.2.5 did); a real OOM is a script error, panics still reboot                                                                            | merged #10 |
| `e537ece`            | GC pause 120% instead of 200%                                    | collect before garbage can outgrow RAM; no measurable speed cost (midi_to_msg 81 µs, redraw 1.04 ms)                                                                                                    | merged #10 |
| `289097f`            | name the file in upload compile errors                           | no NULL+3 crash on messages without a quote; verified: `bogus.lua:3: unexpected symbol`                                                                                                                 | merged #11 |
| `bd89884`            | MIDI helpers in C (fixup `5dd8ec6`)                              | same results as the Lua versions; no measurable gain on its own (code fetch dominates)                                                                                                                  | held back |
| `fb29f50`            | terminate the lib.lua version buffer                             | strstr() read past a 24-byte stack buffer                                                                                                                                                               | merged #10 |
| `e9d184b`            | truncate lib.lua when rewriting it                               | a shorter new lib.lua kept the old tail and failed to compile (nothing in lib.lua ran)                                                                                                                  | merged #10 |
| `9d0a494`            | 4 KB serial TX that drains when full; 512 B RX (fixups `8ac65d0`, `35ac790`) | −15.5 KB static RAM; prints and downloads of any size get through; a host that never reads costs one 250 ms wait                                                                                        | merged #12 |
| `e4e1b89`            | keep output printed before a host connects (fixup `9dabcc7`)     | startup prints (e.g. many-roads' script list, `-- init:` lines) show up when diii connects                                                                                                              | merged #12 |
| `2370d87`            | optional fixed offset for the status sector and filesystem       | a device can pin them (`III_FLASH_STATUS_OFFSET`) so updates that grow the program keep the files; we use 0x80000 | merged #15 |
| `d5d6ecd`            | lib.lua leaves the MIDI helpers to C                             | since the v1.1.3 merge lib.lua defined them again in Lua, replacing `bd89884`'s C ones (every note through Lua); found on achene, whose sequencer sends notes from the clock                              | held back |
| `896afdf`            | an upload short of memory stops the script and compiles again    | the upload's check compiled inside the running script's memory; on achene a 139 KB script uploaded over itself failed (not enough memory) and now is written                                             | ask first |
| `cb0a27c`            | scripts of 8 KB or more run from cached bytecode (`name.luac`)   | no compile at load; `vm_cache_strip()` (weak, no by default) lets a device drop debug info. achene, 139 KB script: Lua 243 KB, with its bundled copy stripped 206 KB (peak 284 → 240). Untested on a grid | ask first |
| `572f8df`            | the repl's print sends a file in pieces                          | it held the whole file in RAM and used it as `serial()`'s format, where a `%` (Lua's modulo) was taken for a conversion                                                                                 | sent #16 |
| `354caae`            | an upload keeps lines longer than the repl's 512-byte buffer      | the repl dropped an overlong line, cutting the file: achene's trellis presets (a track a line, thousands of characters) failed to upload. Tested on the PC (repl.c with stand-ins): a 6000-character line arrives whole, was cut to 384 bytes | sent #16 |
| `262253f`, `08e814d`, `49a5762`, `0b36e9a`, `74bb23a` | lib.lua version `v1.1.x-ogN`                       | fork-only, so devices replace their lib.lua; don't send                                                                                                                                                 | fork only |
| (upstream)           | USB MIDI sends only a message's own bytes                        | from achene; a clock went out as F8 00 00, the zeros as stray packets; came in with the merge `ff6bb66`                                                                                                 | merged #14 |
| (upstream)           | `_quote` keeps the zeros of whole numbers                        | from achene; `pset_write` wrote 0 as nothing and 100 as 1; came in with the merge `ff6bb66`                                                                                                             | merged #13 |

PR numbers are on [tehn/iii](https://codeberg.org/tehn/iii/pulls): #10 to #12 posted 2026-10-15
(squashed, no `iii:` prefix; branches `pr1-fixes`, `pr2-loading`, `pr3-serial` in carlosedp/iii),
#13 and #14 from achene (branches `quote`, `midibytes`), #15 (branch `flash-offset`) on
2026-11-25. All six are merged on 2026-11-25: #10 to #14 (pulled in with `make
iii-pull-upstream`, merge `ff6bb66`), #15 the same day, with v1.1.3 (merge `0b36e9a`). The
MIDI helpers are the only code change left, held back: no gain on their own (see
docs/BENCHMARKS.md). Upstream's checked-in `resource/lib_lua.c` still holds the v1.1.2
`lib.lua` (without #13), even in v1.1.3; our build regenerates it, so that only matters to
devices built from upstream as is.

**What's left to send** (reviewed 2026-11-26). *to send*: bugs in
upstream's own code that any iii device can hit, small and with nothing
device-specific: `572f8df` (the repl's `^^p` printed a file as `serial()`'s
format, so a Lua `%` was taken for a conversion, and it held the whole file
in RAM) and `354caae` (an upload drops a line over 512 bytes). Sent
together as PR #16 (branch `repl-files`) on 2026-11-26. *ask first*: changes
of behaviour, worth an issue before a PR: `896afdf` stops the running script
when an upload can't compile beside it (helps big scripts; users see their
script stop), and `cb0a27c` writes `name.luac` next to each script of 8 KB
or more (faster, lighter loads, measured on achene and a grid; but files
users didn't make, and flash). *held back*: the C MIDI helpers (`bd89884`,
`d5d6ecd`), no gain measured. The `-ogN` version bumps are fork-only.

`make iii-patches` exports every iii commit since the subtree was added. When a
change lands upstream, mark it *merged* here; to leave it out of later
exports, pass a newer base, e.g. `make iii-patches III_BASE=<last merged commit>`.

## Other directories

`lua/`, `littlefs/` and `FREEPOET_Keypad/` are plain exports. To update one,
re-export it at a new commit and update the table (run from the repo root, in a
clone of the upstream repo at `$UP`):

```sh
rm -rf third_party/lua && mkdir third_party/lua
git -C "$UP" archive <commit> | tar -x -C third_party/lua
```

If one of them ever needs local changes, convert it to a subtree the way
`iii/` was (commit `337d710`).
