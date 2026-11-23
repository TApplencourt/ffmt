# Setup for id 40: GitHub icpx SYCL, AOT for PVC, -O2, no fast-math, build only (no GPU).
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

# GitHub runner: icpx from Intel's oneAPI apt repo (oneAPI 2025.2) and a
# current ocloc for the ahead-of-time PVC build, so IGC's codegen is checked
# at build time; there is no GPU to run on. Installed only if missing. ocloc
# and the IGC it loads are pinned release .debs from Intel's GitHub, not
# Intel's Ubuntu PPA, to avoid PPA outages. noble's own ocloc is too old for
# the SPIR-V current icpx emits.
if [ ! -f /opt/intel/oneapi/setvars.sh ]; then
  wget -qO- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
    | sudo gpg --dearmor -o /usr/share/keyrings/oneapi-archive-keyring.gpg
  echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" \
    | sudo tee /etc/apt/sources.list.d/oneAPI.list
  wget -q \
    https://github.com/intel/intel-graphics-compiler/releases/download/v2.42.6/intel-igc-core-2_2.42.6+23070_amd64.deb \
    https://github.com/intel/intel-graphics-compiler/releases/download/v2.42.6/intel-igc-opencl-2_2.42.6+23070_amd64.deb \
    https://github.com/intel/compute-runtime/releases/download/26.40.40196.8/intel-ocloc_26.40.40196.8-0_amd64.deb || exit 1
  sudo apt-get update
  sudo apt-get install -y intel-oneapi-compiler-dpcpp-cpp-2025.2 ./intel-*.deb
fi
set --   # setvars.sh reads the caller's "$@" as its own arguments
source /opt/intel/oneapi/setvars.sh
export CXX=icpx
export FFMT_FLAGS='-std=c++20 -Wall -Werror -fsycl -fsycl-targets=spir64_gen -Xsycl-target-backend "-device pvc" -O2 -fno-fast-math'
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_sycl.cpp examples/readme2_sycl.cpp"
