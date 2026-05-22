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
  SYCLFLAGS       := --acpp-targets=generic
  OPT_LEVELS      := O0 O2
else
  # `?=` so an env-supplied CXX (e.g. CXX=g++ from CI) wins. With `:=` the
  # env was silently overridden and CI tried to call icpx unconditionally.
  CXX             ?= icpx
  SYCLFLAGS       := -fsycl
  OPT_LEVELS      := O0 O1 O2 O3
  BUFFER_PATH     :=
  # Work around DPC++ bug (string literal through pointer segfaults at O0)
  WA_O0           := -DFMT_SYCL_WA_STR
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
FUZZ_BINS := $(addprefix build/fuzz_,$(OPT_LEVELS))
FUZZ_FM   := $(addprefix build/fuzz_ffast_,$(OPT_LEVELS))
ifdef USE_ACPP
  FUZZ_PCT := $(addprefix build/fuzz_escape_percent_,$(OPT_LEVELS))
endif

ALL_BINS := $(TEST_BINS) $(FUZZ_BINS) $(FUZZ_FM) $(FUZZ_PCT)

# Per-test names that share an aggregating main (test_main_host.cpp + the
# coverage host build). The standalone TEST_NAMES set above also includes
# these but adds escape_percent which is GPU-only.
#
# COV_TESTS_COMMON: compiles for both FMT_SYCL_BUFFER_PATH values.
# COV_TESTS_BUFFER_ONLY: buffer-path-only specs ({:a}, {:b}, {:^}, ...) —
#   the specifiers path rejects these at compile time via static_assert.
COV_TESTS             := integers floats strings layout misc formatter
COV_TESTS_BUFFER_ONLY := buffer_path

.PHONY: all build test test-format test-fuzz test-fuzz-pct test-ffast \
        readme-examples test-host coverage clean

all: test

# ── Build directory ─────────────────────────────────────────

build/:
	mkdir -p build

# ── Test binaries (one binary per test × opt level) ─────────

define TEST_template
build/test_$(1)_$(2): $(TEST_DIR)/test_$(1).cpp $(TEST_HDRS) sycl_khx_print.hpp | build/
	@echo "$$(CXX) $$(CXXFLAGS) $$(SYCLFLAGS) -$(2) $$(BUFFER_PATH) $$(WA_$(2)) $$< -o $$@"
	@TIMEFORMAT="  compile test_$(1)_$(2): %Rs"; time \
	$$(CXX) $$(CXXFLAGS) $$(SYCLFLAGS) -$(2) $$(BUFFER_PATH) $$(WA_$(2)) $$< -o $$@
endef

$(foreach t,$(TEST_NAMES),$(foreach o,$(OPT_LEVELS),$(eval $(call TEST_template,$(t),$(o)))))

# ── Fuzz targets (single binary per opt level) ──────────────

build/fuzz_%: $(TEST_DIR)/fuzz.cpp $(TEST_DIR)/capture.hpp sycl_khx_print.hpp | build/
	@echo "$(CXX) $(CXXFLAGS) $(SYCLFLAGS) -$* $(BUFFER_PATH) $(WA_$*) $< -o $@"
	@TIMEFORMAT="  compile fuzz_$*: %Rs"; time \
	$(CXX) $(CXXFLAGS) $(SYCLFLAGS) -$* $(BUFFER_PATH) $(WA_$*) $< -o $@

build/fuzz_ffast_%: $(TEST_DIR)/fuzz.cpp $(TEST_DIR)/capture.hpp sycl_khx_print.hpp | build/
	@echo "$(CXX) $(CXXFLAGS) $(SYCLFLAGS) -$* -ffast-math $(BUFFER_PATH) $(WA_$*) $< -o $@"
	@TIMEFORMAT="  compile fuzz_ffast_$*: %Rs"; time \
	$(CXX) $(CXXFLAGS) $(SYCLFLAGS) -$* -ffast-math $(BUFFER_PATH) $(WA_$*) $< -o $@

build/fuzz_escape_percent_%: $(TEST_DIR)/fuzz_escape_percent.cpp $(TEST_DIR)/capture.hpp sycl_khx_print.hpp | build/
	@echo "$(CXX) $(CXXFLAGS) $(SYCLFLAGS) -$* $(WA_$*) $< -o $@"
	@TIMEFORMAT="  compile fuzz_escape_percent_$*: %Rs"; time \
	$(CXX) $(CXXFLAGS) $(SYCLFLAGS) -$* $(WA_$*) $< -o $@

# acpp/clang heap-corrupts ("malloc(): invalid next size") when several heavy
# fuzz.cpp instantiations link concurrently. Chain fuzz binaries so each
# waits for the previous one — make -j still parallelizes everything else.
# Use order-only prereqs (after `|`) so timestamps don't trigger unnecessary
# rebuilds; we only want serialized build order, not a real dependency.
ifdef USE_ACPP
ALL_FUZZ := $(FUZZ_BINS) $(FUZZ_FM) $(FUZZ_PCT)
PREV_FUZZ := $(wordlist 1,$(words $(ALL_FUZZ)),x $(ALL_FUZZ))
$(foreach i,$(shell seq 2 $(words $(ALL_FUZZ))),\
  $(eval $(word $(i),$(ALL_FUZZ)): | $(word $(i),$(PREV_FUZZ))))
endif

# README examples
build/example_readme%: example_readme%.cpp sycl_khx_print.hpp | build/
	@echo "$(CXX) $(CXXFLAGS) $(SYCLFLAGS) $< -o $@"
	@TIMEFORMAT="  compile example_readme$*: %Rs"; time \
	$(CXX) $(CXXFLAGS) $(SYCLFLAGS) $< -o $@

readme-examples: build/example_readme1 build/example_readme2
	@t0=$$(date +%s%N); \
	./build/example_readme1 >/dev/null && ./build/example_readme2 >/dev/null; rc=$$?; \
	ms=$$(( ($$(date +%s%N) - t0) / 1000000 )); \
	if [ $$rc -eq 0 ]; then echo "readme-examples: PASS ($${ms}ms)"; \
	else echo "readme-examples: FAIL ($${ms}ms)"; false; fi

# ── Test targets ─────────────────────────────────────────────

build: $(ALL_BINS)

test: test-format test-fuzz test-ffast readme-examples
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

test-fuzz: $(FUZZ_BINS)
	@fail=0; \
	for opt in $(OPT_LEVELS); do \
	  t0=$$(date +%s%N); \
	  ./build/fuzz_$$opt; rc=$$?; \
	  ms=$$(( ($$(date +%s%N) - t0) / 1000000 )); \
	  if [ $$rc -eq 0 ]; then echo "fuzz -$$opt: PASS ($${ms}ms)"; \
	  else echo "fuzz -$$opt: FAIL ($${ms}ms)"; fail=1; fi; \
	done; \
	exit $$fail

ifdef USE_ACPP
test-fuzz-pct: $(FUZZ_PCT)
	@fail=0; \
	for opt in $(OPT_LEVELS); do \
	  t0=$$(date +%s%N); \
	  ./build/fuzz_escape_percent_$$opt; rc=$$?; \
	  ms=$$(( ($$(date +%s%N) - t0) / 1000000 )); \
	  if [ $$rc -eq 0 ]; then echo "fuzz_escape_percent -$$opt: PASS ($${ms}ms)"; \
	  else echo "fuzz_escape_percent -$$opt: FAIL ($${ms}ms)"; fail=1; fi; \
	done; \
	exit $$fail
endif

test-ffast: $(FUZZ_FM)
	@fail=0; \
	for opt in $(OPT_LEVELS); do \
	  t0=$$(date +%s%N); \
	  ./build/fuzz_ffast_$$opt; rc=$$?; \
	  ms=$$(( ($$(date +%s%N) - t0) / 1000000 )); \
	  if [ $$rc -eq 0 ]; then echo "fuzz -ffast-math -$$opt: PASS ($${ms}ms)"; \
	  else echo "fuzz -ffast-math -$$opt: FAIL ($${ms}ms)"; fail=1; fi; \
	done; \
	exit $$fail

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
$(1)_OBJS_SPECIFIERS := $$(foreach t,$$(COV_TESTS),build/$(1)_specifiers_$$(t).o) build/$(1)_specifiers_fuzz.o
$(1)_OBJS_BUFFER     := $$(foreach t,$$(COV_TESTS) $$(COV_TESTS_BUFFER_ONLY),build/$(1)_buffer_$$(t).o) build/$(1)_buffer_fuzz.o

build/$(1)_specifiers_%.o: $$(TEST_DIR)/test_%.cpp $$(TEST_HDRS) sycl_khx_print.hpp | build/
	$$(CXX) $$(CXXFLAGS) -DFMT_SYCL_BUFFER_PATH=0 -DTEST_NO_MAIN $(HOST_OPT) $(2) -c $$< -o $$@

build/$(1)_buffer_%.o: $$(TEST_DIR)/test_%.cpp $$(TEST_HDRS) sycl_khx_print.hpp | build/
	$$(CXX) $$(CXXFLAGS) -DFMT_SYCL_BUFFER_PATH=1 -DTEST_NO_MAIN $(HOST_OPT) $(2) -c $$< -o $$@

build/$(1)_specifiers_fuzz.o: $$(TEST_DIR)/fuzz.cpp $$(TEST_DIR)/capture.hpp sycl_khx_print.hpp | build/
	$$(CXX) $$(CXXFLAGS) -DFMT_SYCL_BUFFER_PATH=0 -DTEST_NO_MAIN $(HOST_OPT) $(2) -c $$< -o $$@

build/$(1)_buffer_fuzz.o: $$(TEST_DIR)/fuzz.cpp $$(TEST_DIR)/capture.hpp sycl_khx_print.hpp | build/
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
