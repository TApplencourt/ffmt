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
  (see "Two formatting paths"), and a null `const char*`, which {fmt}
  reports as an error and ffmt prints as `(null)` (README, "Spec target").
- When unsure how {fmt} behaves, compile a small program against the pinned
  {fmt} and check; do not answer from memory. `test/run_id.sh` fetches it
  into `third_party/fmt`, and again when the pin changes (or set `FMT_DIR`
  to a checkout of the pinned commit: `test/check_id.py` refuses another).

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
  entry points need `FMT_HD` (CUDA, HIP).
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
truncated at `FFMT_BUFFER_SIZE` (default 128) on the buffer path. Do not
introduce new differences.

## Tests

Reuse the existing framework; do not add a new test harness.

- `test/test_<name>.cpp`: each `RUN(PRINT(...))` line is printed by ffmt and
  by the pinned {fmt}, and the outputs are compared. Every id builds every
  file, each as its own binary, and runs every case. A file that does not
  compile on an id, or a case that crashes its binary, takes down only that
  file's tests: keep cases that some ids cannot compile (specs the
  specifiers path rejects, C++23, SYCL types, a compiler crash) or that
  crash in files of their own, grouped by the reason. Add other cases to the file that fits, at its end: a test is
  named by its position, so one added in the middle renames those after it.
- `test/negative/reject_*.cpp`: one `NEG_EXPECT_REJECTED(...)` per file.
  Every id compiles it three ways: it must compile with the call made a
  no-op, and fail to compile through ffmt *and* through {fmt}.
- The test harness never selects a device: the id's setup script does,
  through the runtime's environment (`ONEAPI_DEVICE_SELECTOR`,
  `OMP_TARGET_OFFLOAD=MANDATORY`, `ACPP_VISIBILITY_MASK`,
  `CUDA_VISIBLE_DEVICES`, ...), so that a missing device is an error and
  not a silent run on the host. Each test binary prints the device it
  runs on, and the row's `device` is the one of the results last accepted:
  another device is a problem.
- The tests use no condition but `FFMT_ID_<id>`. A test is
  `<file>:<number>` (`floats:104`), and each id's row in
  `test/configs.json` lists every test that is not expected to pass there:
  `failed_to_compile` (its file does not build), `failed_to_run` (its
  output differs, or it crashes), `compiled_but_not_run` (an earlier test
  of its file crashed the binary). The same lists hold `extra:<name>`, a
  program of the id's `FFMT_EXTRA_FILES` that does not build or run, and
  `negative:accept_valid` where nothing builds through ffmt. A negative
  test passes when ffmt rejects it; no id may expect otherwise. Never change expected output to hide a
  bug. For a new compiler bug, add a reproducer to
  `toolchain_bugs/`.
- Do not edit the lists of `test/configs.json` or `test/tests.json` (the
  number of tests of each file) by hand: see "Updating `test/configs.json`
  from a run" below.
- The README programs are copies of files in `examples/`, built and run
  by each id that lists them in `FFMT_EXTRA_FILES`; `test/check_readme.py`
  (in CI) fails if a README program is not an exact copy: edit both.

An id is a self-contained `test/setup/setup_<id>.sh` (modules, compiler,
every compile flag) plus its row in `test/configs.json`. The row's `ci` is
the CI job that runs the id (`github:<runner>` or `jlse:<job>` for a job of
`.gitlab-ci.yml`; none for an id run by hand) and `goal` is `build` if that
job only builds it: both CI files read them, so an id is added to a job
there and nowhere else. The row's
`previous_main_commit` is the commit of `main` before the one that
accepted the id's results (`N/A` if none ever was). For example, if
`main` has commit A, then commit B, and B updates `test/configs.json`,
then `previous_main_commit` is A. For a PR, B is the squash
merge and A the `git merge-base` of the PR with `main`; rebase on `main`
before the run, so that A is the parent of B. Ids 0 and 1 are the host references (g++;
specifiers path and buffer path). A new toolchain or machine gets a new id:
add a row with empty lists, run it, and accept its results. A program
that runs longer than `FFMT_TIMEOUT` seconds (1800 unless the setup says
otherwise) is killed, which counts as a crash in the test it was in.

```sh
test/run_id.sh 1;  python3 test/check_id.py 1    # host, buffer path: run first
test/run_id.sh 0;  python3 test/check_id.py 0    # host, specifiers path
test/run_id.sh 26; python3 test/check_id.py 26   # g++ ASan+UBSan, as CI
```

`test/run_id.sh` only records what happens in `results/id-<id>/`;
`test/check_id.py` is what decides. `test/check_id.py --summary` prints
which ids share which failures. Valgrind has no target: run it on a binary
of `build/id-<id>/bin/`.

Host tests are not enough for device-facing changes: say which ids you
actually ran, and which you did not.

## Updating `test/configs.json` from a run

Nobody edits its lists by hand. To accept what a run found, locally or in
CI:

1. Get the results into `results/`, where the checker reads them. A local
   `test/run_id.sh <id>` writes them there. For a CI run, download them:
   - GitHub: `gh run download <run-id> --dir results` (each id uploads an
     `id-<n>` artifact, so this gives `results/id-<n>/`).
   - GitLab: download the job's artifact zip and unpack it in the checkout;
     it holds `results/id-<n>/`.
2. Look at what differs:

   ```sh
   python3 test/check_id.py --all       # every id that has a results/id-<n>/
   python3 test/check_id.py 2 3 10 11   # or only these
   ```

   It prints, per id, every test that is in another state than the config
   says, e.g. `floats:137-139: expected passed, got failed_to_run`. The
   logs are `results/id-<n>/<file>.run.log` (TAP) and `<file>.build.log`.
3. Accept, once you agree that those are the expected results:

   ```sh
   python3 test/check_id.py --all --accept-and-update-config
   ```

   This rewrites those rows of `test/configs.json` (and `test/tests.json`,
   the number of tests of each file) from the results.
4. Review `git diff test/configs.json` and commit. Every added entry is a
   test that does not pass: do not accept one to avoid fixing it.

Accepting is refused for results from an uncommitted tree, for results of
a commit whose files differ from the checkout's (apart from
`test/configs.json` and `test/tests.json`, so that several accepts can
follow one run), and for anything a state cannot express: a missing log, an
interrupted run, a crash outside a test, a negative test that compiles, a
{fmt} that is not the pinned one.

`python3 test/check_id.py --summary` prints which ids share which failures.

## CI

Never relax the CI to make it pass: do not skip, loosen or delete a check,
lower the coverage gate (region coverage of `base.hpp` must not drop), or
accept a new failure into `test/configs.json` just to avoid implementing a fix.

- GitHub runs the host ids (`goal` test: built and run against {fmt}) and
  the GPU ids (`goal` build, no `-O0`), which only build, as GitHub has no
  GPU: green there means the device code builds, not that it runs
  correctly. Their setups make the compiler report each test kernel's
  registers, stack or scratch, spills and shared local memory
  (`FFMT_TEST_FLAGS`; for icpx, `IGC_ShaderDumpEnable`); `test/resource_usage.py` reads the build and compares it with
  the base branch. That job fails when a test's kernel grows, or a file
  stops building unless `test/configs.json` expects it: do not take a
  regression there for noise, and do not accept one without a reason.
- GitLab (JLSE) runs every id, on real hardware.

GitLab's existing pull mirror imports every GitHub branch; do not add branch
synchronization to Actions. Only mirrored default-branch pushes may launch
JLSE, and they run the full standard 50-id suite. `.github/workflows/jlse.yml`
uses a pipeline trigger token in the reviewer-protected `jlse` Environment to
submit an existing mirrored ref, optionally with required H100 coverage. Push
and trigger pipelines keep their summary artifacts but do not publish the wiki.
