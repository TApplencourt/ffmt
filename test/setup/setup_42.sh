# Setup for id 42: GitHub icpx OpenMP offload, AOT for PVC, -O2, no fast-math, build only (no GPU).
# Sourced by test/run_id.sh; must export CXX, FFMT_FLAGS, FFMT_MAIN, FFMT_SYNTAX_FLAGS
# and FFMT_FMT_FLAGS.

# GitHub runner: icpx from Intel's oneAPI apt repo (oneAPI 2025.2) and a
# current ocloc for the ahead-of-time PVC build, so IGC's codegen is checked
# at build time; there is no GPU to run on. Installed only if missing. ocloc
# comes from Intel's Ubuntu 24.04 graphics PPA: noble's own is 23.43, older
# than the SPIR-V current icpx emits. add-apt-repository also refreshes the
# package lists, the oneAPI repo's included.
if [ ! -f /opt/intel/oneapi/setvars.sh ]; then
  wget -qO- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
    | sudo gpg --dearmor -o /usr/share/keyrings/oneapi-archive-keyring.gpg
  echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" \
    | sudo tee /etc/apt/sources.list.d/oneAPI.list
  sudo add-apt-repository -y ppa:kobuk-team/intel-graphics
  sudo apt-get install -y intel-oneapi-compiler-dpcpp-cpp-2025.2 intel-ocloc
fi
set --   # setvars.sh reads the caller's "$@" as its own arguments
source /opt/intel/oneapi/setvars.sh
export CXX=icpx
export FFMT_FLAGS='-std=c++20 -Wall -Werror -fiopenmp -fopenmp-targets=spir64_gen -Xopenmp-target-backend "-device pvc" -O2 -fno-fast-math'
export FFMT_MAIN=test/main_omp.cpp
# Only for the negative tests' first pass: NEG_SYNTAX_CHECK turns the
# harness's format call into a no-op, so a negative file that still fails to
# compile has a bug of its own and its rejection would mean nothing.
export FFMT_SYNTAX_FLAGS="-DNEG_SYNTAX_CHECK"
# Only for the negative tests' {fmt} cross-check: NEG_CHECK_FMT makes the
# harness call fmt::format instead of ffmt, and {fmt} must reject the case
# too, or the test is a restriction we invented that {fmt} would accept.
export FFMT_FMT_FLAGS="-std=c++20 -Wall -Werror -DNEG_CHECK_FMT"
