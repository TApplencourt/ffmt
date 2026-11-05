# AGENTS.md

Notes for coding agents working on ffmt. Read `README.md` for user-facing
documentation; this file is about how to change the code.

## Goal

ffmt aims to print exactly what `fmt::println` / `fmt::format` prints, from
device code for any target (PTX, GenASM, SPIR-V, ...) and any programming
model (SYCL, OpenMP target, CUDA, ...). {fmt} is the oracle: the tests
compare ffmt against it, both the output and which format strings compile.

- The oracle is the **pinned {fmt}** (`FMT_REF` in `Makefile.id`), not
  `std::format` and not the {fmt} on your machine. Where {fmt} and
  `std::format` disagree, follow {fmt}.
- Accept what {fmt} accepts, reject (at compile time) what {fmt} rejects,
  and print the same bytes. Do not invent extensions or extra restrictions.
  The only exceptions are the backend limits documented in the README
  (see "Two formatting paths").
- When unsure how {fmt} behaves, compile a small program against the pinned
  {fmt} and check; do not answer from memory. `test/run_id.sh` fetches it
  into `third_party/fmt` on first use (or set `FMT_DIR` to a checkout).

## Public API

Functions:

- `ffmt::print` and `ffmt::println`. Their call shape depends on the path:
  `ffmt::println<"{}">(x)` on the specifiers path, `ffmt::println("{}", x)`
  on the buffer path (see "Two formatting paths").
- The `FFMT_PRINT` / `FFMT_PRINTLN` macros, which expand to the right shape
  and so are the only spelling that works on every path.
- `ffmt::formatter<T>`, in its current shape: a static `format(v)` that
  returns `ffmt::formatted<"({}, {})", int, int>{{v.x, v.y}}`. It is not the
  `std::formatter` / `fmt::formatter` shape (`parse` + `format` writing
  through an iterator at run time), because the specifiers path needs the
  whole format string known at compile time.

Configuration macros, documented in the README: `FFMT_BUFFER_PATH`,
`FFMT_BUFFER_SIZE`, `FFMT_UNICODE`, and the `FFMT_EMIT_PRINTF` /
`FFMT_EMIT_BUFFER` hooks.

Everything else in the header, `ffmt::detail` included, is internal and may
change.

Do not add new public names to support a feature. If {fmt} offers a helper
(e.g. `fmt::underlying`), do not add an ffmt equivalent; users convert
explicitly instead (`static_cast`, `std::to_underlying`).

## Code

- **Single header.** All the library is `include/ffmt/base.hpp`. Do not
  add a new `.hpp` file or split this one: new code goes in `base.hpp`.
- **Small code matters.** Prefer the smallest change that is correct;
  delete code when you can. Device code size and compile time are
  constraints, not afterthoughts. No new dependencies.
- C++20 is the baseline. Anything newer must be behind a feature-test macro.
- The real target is device code ("freestanding" like) on the GPU. Running
  on the host is a sanity check, and the place for valgrind and sanitizers;
  host-only success proves little.
- Everything reachable from `print`/`println` runs on the device: no
  exceptions, no heap allocation, no `std::string`, no virtual calls, no
  runtime recursion (SYCL rejects it at compile time). Format strings are
  validated at compile time (`consteval`). Functions reachable from the
  entry points need `FMT_HD` (CUDA).
- Match the surrounding style: terse comments that explain *why*, and the
  existing naming.

## Two formatting paths

`FFMT_BUFFER_PATH` selects how output is produced. The header picks it from
the compiler (a user may `#define` it first); the host tests force each
value to test both.

- **Specifiers path (`0`)**, for DPC++ (`icpx -fsycl`, any target) and icpx
  OpenMP; the reason is the SPIR-V printf. Its format string must be in
  constant memory, i.e. known at compile time, so ffmt rewrites the `{}`
  string into a printf format at compile time and lets the device printf
  format the arguments. Intel GPU printf cannot print `%s` of a string
  built in the kernel (it crashes), so ffmt cannot format into a buffer
  there. Hence the format string is a
  template argument (`println<"...">`), formatters compose at compile time,
  and specs printf cannot express (`{:b}`, `{:^}`, custom fill, dynamic
  width, ...) are rejected at compile time.
- **Buffer path (`1`)**, everywhere else (AdaptiveCpp, clang OpenMP NVPTX,
  clang CUDA, host). ffmt formats into a fixed char buffer and emits it once
  with `printf("%s", buf)`; some backends use a different print intrinsic
  where required (`fputs` on the host, `__acpp_sscp_print` on AdaptiveCpp).
  Supports the full spec syntax.

A change must keep **both** paths building and matching {fmt}. A spec the
specifiers path cannot express must be rejected there at compile time, not
printed differently. The known exceptions are listed in the README:
shortest float form and Unicode width on the specifiers path, and output
truncated at `FFMT_BUFFER_SIZE` (default 128) on the buffer path. The
buffer path also rejects, at compile time, format strings over 256
characters or with more than 16 placeholders. Do not introduce new
differences.

## Tests

Reuse the existing framework; do not add a new test harness.

- `test/test_<name>.cpp`: each `RUN(PRINT(...))` line is printed by ffmt and
  by the pinned {fmt}, and the outputs are diffed. Add cases to the file
  that fits. Every id compiles and runs every case.
- `test/isolated/test_<name>.cpp`: cases that some ids cannot compile
  (specs the specifiers path rejects, C++23, SYCL types, a compiler crash).
  Each file is built as its own binary, so a compile failure stays
  contained. A case goes here only if it does not compile somewhere; group
  cases by the reason.
- `test/negative/reject_*.cpp`: one `NEG_EXPECT_REJECTED(...)` per file.
  Every id compiles it three ways: it must compile with the call made a
  no-op, and fail to compile through ffmt *and* through {fmt}.
- The tests use no condition but `FFMT_ID_<id>`. A case that fails on an id
  is listed under that id's `expected_fail` in `test/configs.json`:
  `compile` for a file that does not build (`isolated:<name>`,
  `negative:<name>`), `runtime` for a test whose output differs
  (`<file>:<number>`). Never change expected output to hide a bug. For a
  new compiler bug, add a reproducer to `implementation_limitation/`.
- The README code blocks are copies of files in `examples/` (built and run
  by the old `Makefile`'s `make readme-examples`). Nothing checks that they
  match: edit both.

An id is a self-contained `test/setup/setup_<id>.sh` (modules, compiler,
every compile flag) plus its row in `test/configs.json`. Ids 0 and 1 are the
host references (g++; specifiers path and buffer path); start a new id's
`expected_fail` from the reference for its path. A new toolchain or machine
gets a new id.

```sh
test/run_id.sh 1;  python3 test/check_id.py 1    # host, buffer path: run first
test/run_id.sh 0;  python3 test/check_id.py 0    # host, specifiers path
test/run_id.sh 26; python3 test/check_id.py 26   # g++ ASan+UBSan, as CI
```

`test/run_id.sh` exits non-zero whenever a test fails, expected or not;
`test/check_id.py` is what decides. Valgrind has no target: run it on
`build/id-<id>/test`.

Host tests are not enough for device-facing changes: say which ids you
actually ran, and which you did not.

## CI

Never relax the CI to make it pass: do not skip, loosen or delete a check,
lower the coverage gate (region coverage of `base.hpp` must not drop), or
add to an `expected_fail` list just to avoid implementing a fix. The GitHub
device ids (36-42) only build, as GitHub has no GPU: green there means the
device code builds, not that it runs correctly.
