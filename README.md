# ffmt

- `fmt::print`-like formatting from device code: SYCL, OpenMP target, CUDA, HIP.
- Required C++20

This project would not exist without [{fmt}](https://github.com/fmtlib/fmt)
by Victor Zverovich and [Dragonbox](https://github.com/jk-jeon/dragonbox)
by Junekey Jeon. The float formatting paths (default `{}`, `{:g}`, `{:e}`,
`{:f}`, hex `{:a}`) are a direct port of {fmt}'s Dragonbox integration;
the spec parser and many formatting decisions follow {fmt}'s precedent.
The output matches `fmt::format` exactly, including where {fmt} and
`std::format` disagree (see [Spec target](#spec-target-fmt) below).

## Supported backends

| Backend                  | Compiler                            | Status | Notes |
|--------------------------|-------------------------------------|--------|-------|
| SYCL (Intel SPIR-V)      | `icpx -fsycl`                       | ✅ | Specifiers path (printf). At `-O0` a libdpcpp string-literal bug breaks a handful of `%s` tests. |
| SYCL (Intel LLVM)        | intel/llvm `clang++ -fsycl`         | ✅ | Specifiers path, as `icpx -fsycl`. |
| SYCL (AdaptiveCpp)       | `acpp --acpp-targets=generic`       | ✅ | Buffer path. |
| OpenMP target → SPIR-V   | `icpx -fiopenmp -fopenmp-targets=spir64` | ✅ | Specifiers path (shares icpx-SYCL backend). |
| OpenMP target → NVPTX    | `clang -fopenmp --offload-arch=sm_XX` | ✅ | Buffer path. At `-O0` more than half the tests fail, or never run after a crash; the sweep inputs are `static` at `-O2` to avoid an LLVM bug ([llvm#245290](https://github.com/llvm/llvm-project/issues/245290)). |
| CUDA                     | `clang++ -x cuda --cuda-gpu-arch=sm_XX` | ✅ | Buffer path. Auto-detected via `__CUDACC__`. |
| OpenMP target → AMDGPU   | ROCm `clang -fopenmp --offload-arch=gfx942` | ✅ | Buffer path. At `-O0` the same tests break as on NVPTX (precision floats: an LLVM OpenMP shared-stack bug, [llvm#245328](https://github.com/llvm/llvm-project/issues/245328)); `-O2` is clean. |
| HIP                      | `hipcc -x hip --offload-arch=gfx942` | ✅ | Buffer path. Auto-detected via `__HIPCC__`. |
| HIP (chipStar, Level Zero) | chipStar `hipcc -x hip`           | ⚠️ | Buffer path. Clean at `-O0`; at `-O2` half the test files fail to compile (LLVM 23's SPIR-V backend asks for `SPV_ALTERA_arbitrary_precision_integers`). |
| Kokkos                   | Kokkos over Serial, CUDA, HIP, SYCL | ✅ | The path of the backend underneath (SYCL: specifiers path). |
| CUDA                     | `nvcc`                              | ❌ | Fails: `libnvvm` can't emit the `print_string` consteval ctor for device code. |
| CUDA                     | `nvc++ -cuda`                       | ❌ | nvc++'s libstdc++ wiring blocks C++20 headers. |

Auto-detection is purely preprocessor — `#include <ffmt/base.hpp>` and the right emit hook installs itself based on the active compiler/backend macros.

### Toolchain bugs found with ffmt

Each has a small reproducer in `toolchain_bugs/`.

| Reproducer | Bug | Reported |
|---|---|---|
| `clang_based_omp_O2_const_array.c` | clang OpenMP → NVPTX, `-O2`: a local constant array read at a run-time index faults | [llvm#245290](https://github.com/llvm/llvm-project/issues/245290) |
| `clang_based_omp_O0_shared_stack_overlap.c` | clang-based OpenMP (NVPTX, ROCm), `-O0`: two live locals get the same address | [llvm#245328](https://github.com/llvm/llvm-project/issues/245328) |
| `icpx_sycl_O0_printf_char_pointer.cpp` | icpx SYCL `-O0`: `printf("%s")` of a string literal through a local pointer segfaults | [AuroraBugTracking#132](https://github.com/argonne-lcf/AuroraBugTracking/issues/132) |
| `icpx_sycl_printf_bool.cpp` | icpx SYCL: a `bool` passed to `printf` crashes IGC when it builds the kernel (`-O0` and `-O2`) | [AuroraBugTracking#189](https://github.com/argonne-lcf/AuroraBugTracking/issues/189) |
| `icpx_omp_pragma_in_dowhile.cpp` | icpx OpenMP: a target region inside a `do`/`while` fails to load its kernel | [AuroraBugTracking#144](https://github.com/argonne-lcf/AuroraBugTracking/issues/144) |
| `nvcc_constexpr_array_value_init.cpp` | nvcc: a `constexpr` array of aggregates value-initialized with `{}` is rejected when the aggregate has a member with a default initializer | not yet |

## Quick example

### SYCL

> Source: [`examples/readme1_sycl.cpp`](examples/readme1_sycl.cpp)

```cpp
#include <ffmt/base.hpp>
#include <sycl/sycl.hpp>

int main() {
  sycl::queue q;
  q.parallel_for(4, [=](sycl::id<1> i) {
    FFMT_PRINTLN("work-item {} says {}", i, "hello");
  }).wait();
}
```

### OpenMP target offload

> Source: [`examples/readme1_omp.cpp`](examples/readme1_omp.cpp)

```cpp
#include <ffmt/base.hpp>

int main() {
  #pragma omp target teams distribute parallel for num_teams(1) thread_limit(4)
  for (int i = 0; i < 4; i++) {
    FFMT_PRINTLN("work-item {} says {}", i, "hello");
  }
}
```

### CUDA (clang)

> Source: [`examples/readme1_cuda.cu`](examples/readme1_cuda.cu)

```cpp
#include <ffmt/base.hpp>

__global__ void hello() {
  FFMT_PRINTLN("work-item {} says {}", threadIdx.x, "hello");
}

int main() {
  hello<<<1, 4>>>();
  cudaDeviceSynchronize();
}
```

One possible ordering of the output (same for all three):
```
work-item 0 says hello
work-item 2 says hello
work-item 1 says hello
work-item 3 says hello
```

`sycl::id` is printed by the built-in formatter (no cast needed). See
[Custom types](#custom-types) below for the full list of supported types
and how to add your own.

## Advanced example

> Source: [`examples/readme2_sycl.cpp`](examples/readme2_sycl.cpp)

```cpp
#include <ffmt/base.hpp>
#include <sycl/sycl.hpp>

int main() {
  sycl::queue q;
  q.parallel_for(4, [=](sycl::id<1> i) {
    int id = static_cast<int>(i);
    float v = 3.14159f * (id + 1);
    FFMT_PRINTLN("format used: 'id: {{0}}, v2dp={{1:6.2f}}, v={{1:8.5f}}' -> id: {0}, v2dp={1:6.2f}, v={1:8.5f}", id, v);
  }).wait();
}
```

One possible ordering of the output:
```bash
format used: 'id: {0}, v2dp={1:6.2f}, v={1:8.5f}' -> id: 0, v2dp=  3.14, v= 3.14159
format used: 'id: {0}, v2dp={1:6.2f}, v={1:8.5f}' -> id: 2, v2dp=  9.42, v= 9.42477
format used: 'id: {0}, v2dp={1:6.2f}, v={1:8.5f}' -> id: 1, v2dp=  6.28, v= 6.28318
format used: 'id: {0}, v2dp={1:6.2f}, v={1:8.5f}' -> id: 3, v2dp= 12.57, v=12.56636
```

## API

```cpp
FFMT_PRINT("format string", args...);    // no trailing newline
FFMT_PRINTLN("format string", args...);  // appends \n
```

The macros expand to `ffmt::print` / `ffmt::println`, whose call shape
depends on the formatting path (see [Backend differences](#backend-differences)):
`ffmt::println<"format string">(args...)` on the specifiers path,
`ffmt::println("format string", args...)` on the buffer path. The macros
work on both. Arguments can be primitives or any type with a
`ffmt::formatter<T>` specialization.

## Custom types

`FFMT_PRINT` / `FFMT_PRINTLN` accept any type for which a
`ffmt::formatter<T>` specialization is in scope. The library
ships built-in formatters for the following SYCL types:

| Type | Output |
|------|--------|
| `sycl::range<N>` (N=1,2,3) | `4`, `4x8`, `4x8x16` |
| `sycl::id<N>` (N=1,2,3) | `(0)`, `(0, 1)`, `(0, 1, 2)` |
| `sycl::item<N>` (N=1,2,3) | `item(global=(0, 1), range=2x3)` |
| `sycl::nd_item<N>` (N=1,2,3) | `nd_item(global=(0, 1), local=(0, 0), range=2x3)` |

### Adding your own

A formatter exposes a single static `format(v)` returning a
`formatted<Fmt, Args...>` that bundles a compile-time format string with
the runtime values to splice in. The library recursively expands these
until everything is a primitive, so a formatter can reference other
formattable types.

```cpp
struct vec3 { float x, y, z; };

template <>
struct ffmt::formatter<vec3> {
  static constexpr auto format(vec3 v) {
    return formatted<"({}, {}, {})", float, float, float>{{v.x, v.y, v.z}};
  }
};

// Now usable directly:
FFMT_PRINTLN("position = {}", vec3{1.0f, 2.0f, 3.0f});
// → position = (1, 2, 3)
```

### Restrictions

A format spec (`{:>5}`) on a custom-formatter argument is rejected at compile
time, as {fmt} does for a formatter without a spec parser. Positional
indices (`{1} {0}`, also inside a formatter's own string) are supported.

### Floating-point: FTZ/DAZ depends on the runtime

Arithmetic on subnormals depends on the runtime. Some toolchain CRTs
enable flush-to-zero (FTZ) and denormals-as-zero (DAZ) at startup —
notably `icpx` does whenever any TU is compiled at `-O1` or higher — while
others (glibc + g++, and most SYCL device runtimes) leave subnormals
intact. On the buffer path, ffmt formats from the bits: in the fast-math
ids (`-ffast-math` with g++ on x86, clang CUDA and OpenMP offload) a
subnormal that reaches it prints correctly (`{:e}` of `5e-324` gives
`4.940656e-324`, where {fmt} under DAZ prints `0.000000e+00`), also when
FTZ/DAZ is set at run time or by another TU's startup code. On the
specifiers path the device printf formats the value. Neither
can undo arithmetic that flushed the value before the call. To keep
subnormals under `icpx`, compile with
`-fno-fast-math` (or `-fp-model=precise`); the test ids do, except the
fast-math ones, which check the above (`test/test_fast_math.cpp`).

### Backend differences

Two emit paths share the same front-end:

- **Buffer path** (host, AdaptiveCpp, clang OpenMP offload, CUDA, HIP)
  formats the full output into a fixed-size buffer, then emits it once:
  `printf("%s", buf)` with clang OpenMP, CUDA and HIP (host pass too),
  `__acpp_sscp_print` on AdaptiveCpp devices, `fputs` otherwise. Supports
  every {fmt} spec except one inside a custom formatter's own string,
  which is ignored (issue #39).
- **Specifiers path** (DPC++, icpx-OMP-SPIR64) translates the format
  string into a `printf` format and lets the device runtime format
  args directly. Atomic but limited to printf-compatible specs.

DPC++ rejects unsupported features at compile time:

```
error: static assertion failed:
  This format string uses features the specifiers path (DPC++, icpx OpenMP)
  cannot express with printf: {:b}, {:a}, {:^}, custom fill, {:x}/{:o} with
  signed int, {:#x}, dynamic width/precision.
```

Features only available on the buffer path:
- Binary format (`{:b}`, `{:B}`)
- Hex float (`{:a}`, `{:A}`)
- Center alignment (`{:^}`)
- Custom fill characters (`{:*>10}`, or any UTF-8 code point: `{:─^10}`)
- Signed integers with hex/oct (`{:x}` with `int`)
- Alternate hex (`{:#x}` with signed int)
- Dynamic width/precision (`{:{}}`, `{:.{}}`)
- {fmt}'s shortest float form: on the specifiers path a float with no type
  (`{}`, `{:10}`, `{:+}`) prints like `{:g}` (6 significant digits), since
  printf has no shortest conversion and Intel GPU printf supports neither
  `*` widths nor `%s` on computed strings. Explicit `{:g}`/`{:e}`/`{:f}`
  match {fmt} on both paths.
- Unicode-aware string width: the buffer path counts width and string
  precision in display columns like {fmt} (1 per code point, 2 for East
  Asian wide and emoji), so `{:^10}` with `"静夜思"` adds 4 fill. printf's
  `%10s` / `%.3s` count bytes, so on the specifiers path non-ASCII string
  arguments are under-padded (and precision can cut a code point). The
  argument is only known at runtime, so this cannot be diagnosed at
  compile time. ASCII strings match on both paths.

### Spec target: `{fmt}`

The output reference is [{fmt}](https://github.com/fmtlib/fmt): ffmt prints
exactly what `fmt::format` prints, and the test suite diffs against a pinned
`{fmt}` (`FMT_REF` in `Makefile.id`, fetched into `third_party/fmt`, or point
`FMT_DIR` at a checkout). The system `std::format` is not used as a
reference because implementations disagree with each other and with
`{fmt}`. Where `{fmt}` and `std::format` differ, ffmt follows `{fmt}`:

| | `{fmt}` / ffmt | `std::format` |
|---|---|---|
| `{}` of `1e15` | `1000000000000000` (fixed while exponent < 16 for double, < 7 for float) | `1e+15` (shorter form) |
| `{:a}` of `1.5` | `0x1.8p+0` | `1.8p+0` |
| `{:.1a}` of `1.03125` | `0x1.1p+0` (ties round up) | `1.0p+0` (ties to even) |
| `{:a}` of a subnormal | `0x0.0000000000001p-1022` | libc++ and libstdc++ differ |
| `{:+}` on `5u` (any sign on an unsigned or `bool`) | compile error | `+5` (valid per the standard) |

One deliberate difference: a null `const char*`. `fmt::format` reports it
as an error (throws `format_error`, or aborts without exceptions), which a
device can do neither of. ffmt prints it on both paths as the string
`(null)`, as `fmt::printf` does, spec included: `{:>10}` gives
`    (null)` and `{:.3}` gives `(nu`.

### Buffer-path size limit

The buffer-path output buffer defaults to 128 characters. Output longer
than that per `FFMT_PRINT` call is silently truncated. Override with:

```cpp
#define FFMT_BUFFER_SIZE 512
#include <ffmt/base.hpp>
```

### Buffer-path Unicode width

By default the buffer path counts string width and precision in display
columns, like {fmt} (see Backend differences). That brings in a UTF-8
decoder and a ~800-byte table of wide characters wherever a string
argument is formatted with a spec. To drop both from device code, count
bytes instead (as printf does, and as {fmt}'s `FMT_UNICODE=0`):

```cpp
#define FFMT_UNICODE 0
#include <ffmt/base.hpp>
```

ASCII output is identical either way; fill characters such as `{:─^10}`
stay UTF-8-aware.

## Build

```bash
# DPC++ (Intel)
icpx -fsycl -std=c++20 my_kernel.cpp -o my_kernel

# AdaptiveCpp (generic/SSCP backend)
acpp --acpp-targets=generic -std=c++20 my_kernel.cpp -o my_kernel

# OpenMP target offload (clang → NVPTX)
clang++ -fopenmp --offload-arch=sm_80 -std=c++20 my_kernel.cpp -o my_kernel

# CUDA (clang)
clang++ -x cuda --cuda-gpu-arch=sm_80 -std=c++20 my_kernel.cu -lcudart -o my_kernel
```

## Tests

Each test configuration has an id in [`test/configs.json`](test/configs.json):
a compiler, its flags, a device, the modules to load, all set by
`test/setup/setup_<id>.sh`. Ids 0 and 1 are the host references (g++, the
specifiers path and the buffer path).

```bash
test/run_id.sh 1              # build and run id 1; results in results/id-1/
test/run_id.sh 40 build       # build only, for a device this machine lacks
python3 test/check_id.py 1    # compare with what test/configs.json expects of id 1
python3 test/check_id.py --summary   # which ids share which failures
```

Every id builds every test file, each as its own binary. A test is
`<file>:<number>` (`floats:104` is the 104th `RUN` of
`test/test_floats.cpp`), and `test/configs.json` lists, per id, every test
that is not expected to pass:

- `failed_to_compile`: its file does not build;
- `failed_to_run`: it prints something else than `{fmt}`, or crashes;
- `compiled_but_not_run`: an earlier test of its file crashed the binary.

A row also says which CI job runs the id (`ci`) and on which device its
tests ran when its results were last accepted (`device`).

`test/check_id.py` fails when a test is in another state than listed, in
either direction, or when the tests ran on another device. `test/check_id.py <id> --accept-and-update-config`
rewrites the id's lists from its results instead: review the diff.
[`test/tests.json`](test/tests.json), the number of tests of each file, is
generated the same way.

`PASS — results match config` requires complete artifacts and matching
observed states, even when the expected failure-state counts are nonzero.
Per-ID summaries separate matched expected states from unexpected state
differences, whose test IDs remain visible in both directions. Missing or
interrupted artifacts are hard failures, and `compiled_but_not_run` keeps tests
after a crash explicit rather than dropping them as coverage.

Output is TAP: the plan `1..N`, then per test `# start N` and `ok N - file`
or `not ok N - file`. Raw `not ok` lines describe observed failures, including
expected ones; checker success means those failures match the config.
Negative compile rejections are passing behavior, not XFAILs. The `--table`
report repeats this legend with its PASS/FAIL/incomplete totals, and the same
report is published to the JLSE wiki.

CI runs the ids whose `ci` is `github:<runner>`
([`.github/workflows/test.yml`](.github/workflows/test.yml)) and
`jlse:<job>` ([`.gitlab-ci.yml`](.gitlab-ci.yml)).

- GitHub, on every push and pull request, in minutes:
  - the host ids (`goal` test), built and run against {fmt};
  - the GPU ids (`goal` build), built without a GPU and never at `-O0`. The
    `GPU resources non-regression` job compares each test kernel's registers,
    stack or scratch, spills and shared local memory with the base branch
    (one table in the run's summary), and fails when a test's kernel grows,
    or when a file stops building and `test/configs.json` does not expect
    it. A test with no reference, such as a new one, is listed, not failed.
    Unchanged `RUN` blocks keep their reference when they move between test
    files. Intel dump directories are per test file, so identical module
    metadata cannot attach another file's register counts. The resource job
    uses the candidate `Makefile.id` for both sides' dump layouts while
    compiling each side's own test sources.
- GitLab (JLSE): every id, on real hardware.

The six H100 ids belong to
the optional manual `h100-extended` job in that same GitLab pipeline because a
node can take hours to get. Without it, a successful pipeline reports the other
52 ids. For all 58, run `h100-extended` and, after it finishes, retry
`jlse-summary`, then `jlse-wiki`. The retried summary fetches the H100 artifacts;
completed downstream jobs do not rerun automatically.

Basic and extended JLSE jobs share `FFMT_RUNTIME_ROOT`; its `modulefiles`
directory is available alongside `/soft/modulefiles`. The generic upstream
Clang CUDA/HIP/OpenMP jobs load `llvm/release-23.1.6`, whose modulefile selects the
x86-64 or AArch64 installation by architecture. Intel LLVM, ROCm,
AdaptiveCpp, ChipStar, NVCC, and GCC jobs retain their separate toolchains.

GitLab's existing pull mirror imports every GitHub branch. A mirrored push to
the default branch runs the complete standard 50-id suite and leaves a summary
artifact; a mirrored feature-branch push does not launch the hardware suite.
Push and Actions-triggered pipelines never replace the wiki, which remains
limited to web and scheduled pipelines.

### On-demand JLSE from GitHub

The [`Dispatch JLSE`](.github/workflows/jlse.yml) Action submits a pipeline for
a ref that the existing mirror has already imported. It does not synchronize
branches or poll the hardware result. After submission, its job summary links
to the created GitLab pipeline ID and URL; eventual results remain in GitLab.

A Maintainer must configure the integration once:

1. In the GitLab project, create a **pipeline trigger token** under Settings >
   CI/CD > Pipeline trigger tokens. Do not use a personal access token.
2. In GitHub, create the protected Environment `jlse`, require reviewer
   approval, and add `GITLAB_TRIGGER_TOKEN` as its secret. Never commit the
   token.
3. Configure `GITLAB_PROJECT_ID` as a non-secret repository Actions variable
   with value `230`, separately from the trigger secret.

Then use Actions > Dispatch JLSE > Run workflow. Enter the mirrored GitLab
`ref` and leave `run_h100` off for the usual 52-id report and optional manual
H100. Enable it to make H100 automatic and required, producing the complete
58-id summary through the same normal stage ordering. Either request creates a
summary artifact without changing the wiki. The Action submits asynchronous
CI: without a separate read token, it deliberately does not report eventual
hardware success.

Rejected submissions report a bounded, sanitized GitLab error and any
request/correlation ID in the Action log and summary; raw response bodies and
headers are not logged. If a recently merged GitHub `ref` still has old rules on
GitLab, wait for or refresh the existing pull mirror before retrying. A
`workflow:rules` rejection does not mean the trigger token needs rotation.
