SHELL    := /bin/bash
# -fno-fast-math: keep float subnormals intact. icpx's CRT enables
# FTZ/DAZ at startup whenever any TU is compiled at -O1+, which makes
# std::format/printf flush subnormals to zero on the host while the
# SYCL device runtime does not — breaking the host-reference diff for
# values like float denorm_min. Disabling fast-math keeps host and
# device agreeing across icpx/gcc/clang.
CXXFLAGS := -std=c++20 -Wall -Werror -fno-fast-math
# CI / sanitizer injection: append to CXXFLAGS without disturbing the base
# flags above (e.g. EXTRA_CXXFLAGS="-fsanitize=address,undefined").
CXXFLAGS += $(EXTRA_CXXFLAGS)

# Opt level for host build (test-host + coverage). Overridable from CI
# so the matrix can exercise -O0 / -O2 (or -O3, etc.). Don't put this
# in CXXFLAGS — the SYCL build has its own per-test opt loop.
HOST_OPT ?= -O2

ifdef USE_ACPP
  export PATH := $(HOME)/projet/p26.02/install/bin:$(PATH)
  CXX             := acpp
  BACKEND_FLAGS   := --acpp-targets=generic
  OPT_LEVELS      := O0 O2
else ifdef USE_OMP_CLANG
  # Mainline LLVM clang offloading to NVIDIA via NVPTX. The header's
  # __clang__ && _OPENMP block auto-installs the buffer-path emit hook
  # (printf("%s", out.data)) — no extra defines needed.
  # `origin` check (not `?=`) because Make's built-in default `CXX=g++`
  # counts as "set" for `?=`, which would leave gcc in place and fail on
  # `--offload-arch`. Env- or command-line-supplied CXX still wins.
  ifeq ($(origin CXX),default)
    CXX           := clang++
  endif
  OFFLOAD_ARCH    ?= sm_80
  OMP_OPT         ?= -O0
  BACKEND_FLAGS   := -fopenmp --offload-arch=$(OFFLOAD_ARCH) $(OMP_OPT)
  # FMT_PTX_CLANG_O0: see capture.hpp for the symptom catalog. Verified
  # clean at -O2, so gate it only when OMP_OPT == -O0. Run
  # `make ... OMP_OPT=-O2` to confirm.
  ifeq ($(OMP_OPT),-O0)
    BACKEND_FLAGS += -DFMT_PTX_CLANG_O0
  endif
  OPT_LEVELS      := O0 O2
else ifdef USE_OMP_ICPX
  # icpx OpenMP-target on SPIR64. Shares the SPIR backend with icpx-SYCL,
  # so this hits the specifiers path; the header's __INTEL_LLVM_COMPILER
  # && _OPENMP block auto-installs the omp_printf → __spirv_ocl_printf
  # emit hook.
  # `origin` check (not `?=`) — see USE_OMP_CLANG branch above.
  ifeq ($(origin CXX),default)
    CXX           := icpx
  endif
  OMP_OPT         ?= -O0
  BACKEND_FLAGS   := -fiopenmp -fopenmp-targets=spir64 $(OMP_OPT)
  # Same SPIRV-O0 string-literal-through-pointer bug the SYCL path hits.
  ifeq ($(OMP_OPT),-O0)
    BACKEND_FLAGS += -DFMT_SPIRV_O0
  endif
  OPT_LEVELS      := O0 O2
else
  # `origin` check so an env- or command-line-supplied CXX (e.g. CXX=g++
  # from CI) wins, while Make's built-in default `g++` gets replaced by
  # icpx. With plain `?=` the built-in default counted as set and icpx
  # never took effect; with `:=` env/command-line was silently overridden.
  ifeq ($(origin CXX),default)
    CXX           := icpx
  endif
  BACKEND_FLAGS   := -fsycl
  OPT_LEVELS      := O0 O1 O2 O3
  BUFFER_PATH     :=
  # FMT_SPIRV_O0 gates tests broken by an icpx-SYCL-on-SPIR64 bug at -O0:
  # a string-literal accessed through a pointer segfaults on device.
  WA_O0           := -DFMT_SPIRV_O0
endif

# ── Source files ────────────────────────────────────────────
TEST_DIR    := test
TEST_NAMES  := integers floats strings layout misc formatter
ifdef USE_ACPP
  TEST_NAMES += buffer_path escape_percent
endif
TEST_HDRS   := $(wildcard $(TEST_DIR)/*.hpp $(TEST_DIR)/*.inc)

# Derived binary names (all in build/)
TEST_BINS := $(foreach t,$(TEST_NAMES),$(foreach o,$(OPT_LEVELS),build/test_$(t)_$(o)))

ALL_BINS := $(TEST_BINS)

# Per-test names that share an aggregating main (test_main_host.cpp + the
# coverage host build). The standalone TEST_NAMES set above also includes
# these but adds escape_percent which is GPU-only.
#
# COV_TESTS_COMMON: compiles for both FMT_SYCL_BUFFER_PATH values.
# COV_TESTS_BUFFER_ONLY: buffer-path-only specs ({:a}, {:b}, {:^}, ...) —
#   the specifiers path rejects these at compile time via static_assert.
COV_TESTS             := integers floats strings layout misc formatter
COV_TESTS_BUFFER_ONLY := buffer_path

.PHONY: all build test test-format test-omp readme-examples test-host coverage clean

# USE_OMP_CLANG / USE_OMP_ICPX share the SYCL header but not the SYCL
# examples (those `#include <sycl/sycl.hpp>`). Default to the OMP test rig
# only — building TEST_BINS / readme-examples would try to compile SYCL
# sources with OMP-only flags and fail.
ifneq (,$(or $(USE_OMP_CLANG),$(USE_OMP_ICPX)))
all: test-omp
else
all: test
endif

# ── Build directory ─────────────────────────────────────────

build/:
	mkdir -p build

# ── Test binaries (one binary per test × opt level) ─────────

define TEST_template
build/test_$(1)_$(2): $(TEST_DIR)/test_$(1).cpp $(TEST_HDRS) sycl_khx_print.hpp | build/
	@echo "$$(CXX) $$(CXXFLAGS) $$(BACKEND_FLAGS) -$(2) $$(BUFFER_PATH) $$(WA_$(2)) $$< -o $$@"
	@TIMEFORMAT="  compile test_$(1)_$(2): %Rs"; time \
	$$(CXX) $$(CXXFLAGS) $$(BACKEND_FLAGS) -$(2) $$(BUFFER_PATH) $$(WA_$(2)) $$< -o $$@
endef

$(foreach t,$(TEST_NAMES),$(foreach o,$(OPT_LEVELS),$(eval $(call TEST_template,$(t),$(o)))))

# README examples (SYCL-only — they #include <sycl/sycl.hpp>; skipped
# under USE_OMP_CLANG / USE_OMP_ICPX where BACKEND_FLAGS is OMP-only).
build/example_sycl_readme%: example_sycl_readme%.cpp sycl_khx_print.hpp | build/
	@echo "$(CXX) $(CXXFLAGS) $(BACKEND_FLAGS) $< -o $@"
	@TIMEFORMAT="  compile example_sycl_readme$*: %Rs"; time \
	$(CXX) $(CXXFLAGS) $(BACKEND_FLAGS) $< -o $@

readme-examples: build/example_sycl_readme1 build/example_sycl_readme2
	@t0=$$(date +%s%N); \
	./build/example_sycl_readme1 >/dev/null && ./build/example_sycl_readme2 >/dev/null; rc=$$?; \
	ms=$$(( ($$(date +%s%N) - t0) / 1000000 )); \
	if [ $$rc -eq 0 ]; then echo "readme-examples: PASS ($${ms}ms)"; \
	else echo "readme-examples: FAIL ($${ms}ms)"; false; fi

# ── Test targets ─────────────────────────────────────────────

build: $(ALL_BINS)

test: test-format readme-examples
	@echo "==============================="
	@echo "All tests passed."
	@echo "==============================="

test-format: $(TEST_BINS)
	@fail=0; \
	for t in $(TEST_NAMES); do \
	  for opt in $(OPT_LEVELS); do \
	    t0=$$(date +%s%N); \
	    ./build/test_$${t}_$$opt; rc=$$?; \
	    ms=$$(( ($$(date +%s%N) - t0) / 1000000 )); \
	    if [ $$rc -eq 0 ]; then echo "test_$$t -$$opt: PASS ($${ms}ms)"; \
	    else echo "test_$$t -$$opt: FAIL ($${ms}ms)"; fail=1; fi; \
	  done; \
	done; \
	exit $$fail

# ── OMP test rig (opt-in: USE_OMP_CLANG=1 or USE_OMP_ICPX=1) ──────
# Builds test_main_omp + per-test sources into one binary, then runs
# it. Single-binary single-rule (vs the per-opt-level loop of test-format)
# because the OMP smoke harness just needs to prove the header still
# matches std::format on a real device — not multiply GPU launches.
#
# Buffer path (clang-OMP-CUDA) compiles the buffer-only tests too;
# specifiers path (icpx-OMP-SPIR64) skips them because they'd hit
# static_asserts.
ifdef USE_OMP_CLANG
  TEST_NAMES_OMP := integers floats strings layout misc formatter buffer_path
endif
ifdef USE_OMP_ICPX
  TEST_NAMES_OMP := integers floats strings layout misc formatter
endif

build/test_main_omp: $(TEST_DIR)/test_main_omp.cpp $(TEST_HDRS) sycl_khx_print.hpp \
                     $(foreach t,$(TEST_NAMES_OMP),$(TEST_DIR)/test_$(t).cpp) | build/
	@echo "$(CXX) $(CXXFLAGS) $(BACKEND_FLAGS) -DTEST_NO_MAIN -o $@ test_main_omp.cpp + per-test sources"
	@TIMEFORMAT="  compile test_main_omp: %Rs"; time \
	$(CXX) $(CXXFLAGS) $(BACKEND_FLAGS) -DTEST_NO_MAIN -o $@ \
	  $(TEST_DIR)/test_main_omp.cpp \
	  $(foreach t,$(TEST_NAMES_OMP),$(TEST_DIR)/test_$(t).cpp)

test-omp: build/test_main_omp
	@t0=$$(date +%s%N); \
	./build/test_main_omp; rc=$$?; \
	ms=$$(( ($$(date +%s%N) - t0) / 1000000 )); \
	if [ $$rc -eq 0 ]; then echo "test_main_omp: PASS ($${ms}ms)"; \
	else echo "test_main_omp: FAIL ($${ms}ms)"; false; fi

# ── Host tests + coverage (host-only, no SYCL device, no OpenMP) ──
# Both targets build test_main_host.cpp + per-test .o files twice — once
# with FMT_SYCL_BUFFER_PATH=0 (specifiers path) and once with =1 (buffer
# path). Emit on both sides is ::printf via libc, so the std vs sycl
# diff inside each test_X() is meaningful.
#
# `test-host` is the plain pass/fail variant. `coverage` is the same
# build with -fprofile-instr-generate -fcoverage-mapping added (and
# an llvm-cov report at the end). Pattern rules below are templatized
# on a prefix (test_main_host_X / cov_X) and a flag string ($(COV_FLAGS)
# for coverage, empty for test-host).
#
# HOST_TEMPLATE args:
#   $(1) = file prefix      ("test_main_host" or "cov")
#   $(2) = extra flags      ("" or "$(COV_FLAGS)")

define HOST_TEMPLATE
$(1)_OBJS_SPECIFIERS := $$(foreach t,$$(COV_TESTS),build/$(1)_specifiers_$$(t).o)
$(1)_OBJS_BUFFER     := $$(foreach t,$$(COV_TESTS) $$(COV_TESTS_BUFFER_ONLY),build/$(1)_buffer_$$(t).o)

build/$(1)_specifiers_%.o: $$(TEST_DIR)/test_%.cpp $$(TEST_HDRS) sycl_khx_print.hpp | build/
	$$(CXX) $$(CXXFLAGS) -DFMT_SYCL_BUFFER_PATH=0 -DTEST_NO_MAIN $(HOST_OPT) $(2) -c $$< -o $$@

build/$(1)_buffer_%.o: $$(TEST_DIR)/test_%.cpp $$(TEST_HDRS) sycl_khx_print.hpp | build/
	$$(CXX) $$(CXXFLAGS) -DFMT_SYCL_BUFFER_PATH=1 -DTEST_NO_MAIN $(HOST_OPT) $(2) -c $$< -o $$@

build/$(1)_specifiers_main.o: $$(TEST_DIR)/test_main_host.cpp $$(TEST_DIR)/capture.hpp sycl_khx_print.hpp | build/
	$$(CXX) $$(CXXFLAGS) -DFMT_SYCL_BUFFER_PATH=0 $(HOST_OPT) $(2) -c $$< -o $$@

build/$(1)_buffer_main.o: $$(TEST_DIR)/test_main_host.cpp $$(TEST_DIR)/capture.hpp sycl_khx_print.hpp | build/
	$$(CXX) $$(CXXFLAGS) -DFMT_SYCL_BUFFER_PATH=1 $(HOST_OPT) $(2) -c $$< -o $$@

build/$(1)_specifiers: build/$(1)_specifiers_main.o $$($(1)_OBJS_SPECIFIERS)
	$$(CXX) $$(CXXFLAGS) $(2) $$^ -o $$@

build/$(1)_buffer: build/$(1)_buffer_main.o $$($(1)_OBJS_BUFFER)
	$$(CXX) $$(CXXFLAGS) $(2) $$^ -o $$@
endef

$(eval $(call HOST_TEMPLATE,test_main_host,))

test-host: build/test_main_host_specifiers build/test_main_host_buffer
	@fail=0; \
	for variant in specifiers buffer; do \
	  t0=$$(date +%s%N); \
	  ./build/test_main_host_$$variant; rc=$$?; \
	  ms=$$(( ($$(date +%s%N) - t0) / 1000000 )); \
	  if [ $$rc -eq 0 ]; then echo "test_main_host_$$variant: PASS ($${ms}ms)"; \
	  else echo "test_main_host_$$variant: FAIL ($${ms}ms)"; fail=1; fi; \
	done; \
	exit $$fail

# Coverage: same build + -fprofile-instr-generate -fcoverage-mapping.
LLVM_PROFDATA = $(shell $(CXX) -print-prog-name=llvm-profdata)
LLVM_COV      = $(shell $(CXX) -print-prog-name=llvm-cov)
COV_FLAGS     := -fprofile-instr-generate -fcoverage-mapping

$(eval $(call HOST_TEMPLATE,cov,$(COV_FLAGS)))

COV_ALL := build/cov_specifiers build/cov_buffer

# Run instrumented binaries and merge into one .profdata. Split out so both
# the human-readable `coverage` target and CI's `coverage-json` can share it.
build/coverage.profdata: $(COV_ALL)
	@rm -f build/cov_*.profraw
	@for bin in $(COV_ALL); do \
	  LLVM_PROFILE_FILE="$$bin.profraw" ./$$bin > /dev/null; \
	done
	@$(LLVM_PROFDATA) merge -o $@ build/cov_*.profraw
	@rm -f build/cov_*.profraw

COV_OBJS := $(firstword $(COV_ALL)) \
            $(addprefix -object ,$(wordlist 2,$(words $(COV_ALL)),$(COV_ALL)))

coverage: build/coverage.profdata
	@$(LLVM_COV) report $(COV_OBJS) -instr-profile=$< -sources sycl_khx_print.hpp

# Machine-readable summary for CI non-regression check. The stderr redirect
# silences llvm-cov's "N functions have mismatched data" warning that would
# otherwise land on the same stream and break naive `> coverage.json` capture.
coverage-json: build/coverage.profdata
	@$(LLVM_COV) export $(COV_OBJS) -instr-profile=$< \
	  -sources sycl_khx_print.hpp -summary-only --format=text 2>/dev/null

clean:
	rm -rf build/
