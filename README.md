# fmt-sycl

- `std::print`-like formatting for SYCL device kernels.
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
| SYCL (AdaptiveCpp)       | `acpp --acpp-targets=generic`       | ✅ | Buffer path. Full `std::format` spec. |
| OpenMP target → SPIR-V   | `icpx -fiopenmp -fopenmp-targets=spir64` | ✅ | Specifiers path (shares icpx-SYCL backend). |
| OpenMP target → NVPTX    | `clang -fopenmp --offload-arch=sm_XX` | ✅ | Buffer path. At `-O0` ~50 NVPTX-printf codegen bugs break tests; `-O2` is clean. |
| CUDA                     | `clang++ -x cuda --cuda-gpu-arch=sm_XX` | ✅ | Buffer path. Auto-detected via `__CUDACC__`. |
| CUDA                     | `nvcc`                              | ❌ | Fails: `libnvvm` can't emit the `print_string` consteval ctor for device code. |
| CUDA                     | `nvc++ -cuda`                       | ❌ | nvc++'s libstdc++ wiring blocks C++20 headers. |

Auto-detection is purely preprocessor — `#include <ffmt/base.hpp>` and the right emit hook installs itself based on the active compiler/backend macros.

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

Printed values reflect whatever the runtime does with subnormals. Some
toolchain CRTs enable flush-to-zero (FTZ) and denormals-as-zero (DAZ) at
startup — notably `icpx` does whenever any TU is compiled at `-O1` or
higher — while others (glibc + g++, and most SYCL device runtimes) leave
subnormals intact. The library does not paper over this: a value like
`std::numeric_limits<float>::denorm_min()` may print as `1.4013e-45` in
one environment and `0` in another, matching the local
`printf`/`std::format`. To get the unflushed result under `icpx`, compile
with `-fno-fast-math` (or `-fp-model=precise`); the host test suite in
this repo does exactly that for portability across icpx/gcc/clang.

### Backend differences

Two emit paths share the same front-end:

- **Buffer path** (plain host builds, ACPP, clang-OMP-CUDA, CUDA-clang) formats the full
  output into a fixed-size buffer, then emits it with a single
  `printf("%s", buf)`. Supports the full `std::format` spec.
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
`    (null)` and `{:.3}` gives `(nu`. On Intel GPUs (SPIR-V) the null goes
to the device printf, which also prints `(null)` but nothing with a
precision below 6: `{:.3}` gives an empty string there.

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
specifiers path and the buffer path); every other id behaves like one of
the two, apart from the failures it lists.

```bash
test/run_id.sh 1              # build and run id 1; results in results/id-1/
test/run_id.sh 39 build       # build only, for a device this machine lacks
python3 test/check_id.py 1    # compare with id 1's expected failures
```

Every id builds every test. `test/check_id.py` passes when the failures are
exactly the ones listed under the id's `expected_fail`: `compile` for files
that do not build, `runtime` for tests whose output differs from `{fmt}`.
It reports anything else in either direction: an unexpected failure, or an
expected one that did not happen.

Output is TAP-style: one line per test, `ok N - file` or `not ok N - file`.

CI runs the ids a GitHub runner can handle
([`.github/workflows/test.yml`](.github/workflows/test.yml)) and the JLSE
GPU ids ([`.gitlab-ci.yml`](.gitlab-ci.yml)).
