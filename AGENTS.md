# AGENTS.md

Notes for coding agents working on ffmt. Read `README.md` for user-facing
documentation; this file is about how to change the code.

## Goal

ffmt prints exactly what `fmt::println` / `fmt::format` prints, from device
code (SYCL, OpenMP target, CUDA). {fmt} is the oracle: every test compares
ffmt's output to it.

- The oracle is the **pinned {fmt}** (`FMT_REF` in the `Makefile`), not
  `std::format` and not the {fmt} on your machine. Where {fmt} and
  `std::format` disagree, follow {fmt}.
- Accept what {fmt} accepts, reject (at compile time) what {fmt} rejects,
  and print the same bytes. Do not invent extensions or extra restrictions.
- When unsure how {fmt} behaves, compile a small program against
  `third_party/fmt` and check; do not answer from memory.

## Public API

Public:

- `ffmt::print` and `ffmt::println`.
- The `FFMT_PRINT` / `FFMT_PRINTLN` / `FFMT_PRINTF` / `FFMT_PRINTLNF`
  macros. They are the only spelling that works on every path today:
  `ffmt::println("{}", x)` compiles on the buffer path only, the specifiers
  path needs `ffmt::println<"{}">(x)` (see "Two formatting paths").
- `ffmt::formatter<T>`, in its current shape: a static `format(v)` that
  returns a compile-time `formatted<"...", Args...>`. It is not the
  `std::formatter` / `fmt::formatter` shape (`parse` + `format` writing
  through an iterator at run time), because the specifiers path needs the
  whole format string known at compile time.

Everything else in the header, `ffmt::detail` included, is internal and may
change.

Do not add new public names to support a feature. If {fmt} needs a helper
for something (e.g. `fmt::underlying`), ffmt users convert explicitly
instead (`static_cast`, `std::to_underlying`).

## Code

- **Single header.** All the library is `include/ffmt/base.hpp`. Do not
  split it or add headers.
- **Small code matters.** Prefer the smallest change that is correct;
  delete code when you can. Device code size and compile time are
  constraints, not afterthoughts. No new dependencies.
- C++20 is the baseline. Anything newer must be behind a feature-test macro.
- The real target is the GPU / freestanding code. Running on the host is
  a sanity check, and the place for valgrind and sanitizers; host-only
  success proves little.
- Everything reachable from `print`/`println` runs on the device: no
  exceptions, no heap allocation, no `std::string`, no virtual calls, no
  runtime recursion. Format strings are parsed and validated at compile time
  (`consteval`). Functions reachable from the entry points need `FMT_HD`
  (CUDA).
- Match the surrounding style: terse comments that explain *why*, and the
  existing naming.

## Two formatting paths

`FFMT_BUFFER_PATH` selects how output is produced. It is chosen
automatically from the compiler (top of the header); the host tests force
each value to test both.

- **Specifiers path (`0`)**, used on SPIR-V (icpx SYCL, icpx OpenMP
  `spir64`). SPIR-V printf needs a literal format string, so ffmt rewrites
  the `{}` string into a printf format at compile time and lets the device
  printf format the arguments. Intel GPU printf supports neither `*` widths
  nor `%s` on computed strings, so ffmt cannot format into a buffer there.
  Hence the format string is a template argument (`println<"...">`),
  formatters must compose at compile time, and specs printf cannot express
  (`{:b}`, `{:^}`, custom fill, dynamic width, ...) are rejected at compile
  time. The few remaining differences from {fmt} (shortest float form,
  Unicode width) are listed in README "Backend differences"; do not add to
  that list.
- **Buffer path (`1`)**, everywhere else (AdaptiveCpp, clang OpenMP NVPTX,
  clang CUDA, host). ffmt formats into a fixed char buffer
  (`FFMT_BUFFER_SIZE`, output truncated beyond it) and emits it once with
  `printf("%s", buf)`. Supports the full spec.

A change must keep **both** paths building and matching {fmt}. A spec the
specifiers path cannot express must be rejected there at compile time,
never printed differently. Specs that only the buffer path supports are
tested in `test/test_buffer_path.cpp`.

## Tests

Reuse the existing framework; do not add a new test harness.

- `test/test_<name>.cpp`: each `RUN(PRINT(...))` line is printed by ffmt and
  by the pinned {fmt}, and the outputs are diffed. Add cases to the file
  that fits (`integers`, `floats`, `strings`, `layout`, `misc`,
  `formatter`, `buffer_path` for buffer-only specs).
- `test/negative/reject_*.cpp`: one `NEG_EXPECT_REJECTED(...)` per file;
  it must fail to compile on both ffmt paths *and* with {fmt}. Picked up
  automatically by `make test-negative`.
- Compiler bugs are gated with `SKIP_IF(<gate>, ...)` (existing gates:
  `FFMT_SPIRV_O0`, `FFMT_PTX_CLANG_O0`), with a reproducer in
  `implementation_limitation/`. Never change expected output to hide a bug.
  Keep the number of gates small: reuse an existing one before adding a new
  one.
- README examples are built from `examples/`; keep the two in sync.

Commands (the default compiler is `icpx`; on a machine without it pass
`CXX=clang++` or `CXX=g++`):

```sh
make CXX=clang++ test-host       # CPU, both paths: run this first
make CXX=clang++ test-negative   # compile-time rejections
make                             # icpx -fsycl on the device, -O0..-O3
```

Host tests are not enough for device-facing changes: say which backends you
actually ran, and which you did not.
