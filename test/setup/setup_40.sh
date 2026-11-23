# Setup for id 40: GitHub icpx SYCL, AOT for PVC, -O2, no fast-math, build only (no GPU).
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

# GitHub runner: icpx from Intel's oneAPI apt repo (oneAPI 2025.2) and a
# current ocloc for the ahead-of-time PVC build, so IGC's codegen is checked
# at build time; there is no GPU to run on. Installed only if missing. ocloc
# and the IGC it loads are pinned release .debs from Intel's GitHub, checked
# against the sums the releases publish: noble's own ocloc is 23.43, older
# than the SPIR-V current icpx emits, and a pinned version is the same on
# every run (Intel's Ubuntu PPA moves, and Launchpad can be down).
if [ ! -f /opt/intel/oneapi/setvars.sh ]; then
  wget -qO- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
    | sudo gpg --dearmor -o /usr/share/keyrings/oneapi-archive-keyring.gpg
  echo "deb [signed-by=/usr/share/keyrings/oneapi-archive-keyring.gpg] https://apt.repos.intel.com/oneapi all main" \
    | sudo tee /etc/apt/sources.list.d/oneAPI.list
  igc=https://github.com/intel/intel-graphics-compiler/releases/download/v2.42.6
  neo=https://github.com/intel/compute-runtime/releases/download/26.40.40196.8
  debs=$(mktemp -d)
  if ! (cd "$debs" &&
        wget -q "$igc/intel-igc-core-2_2.42.6+23070_amd64.deb" \
                "$igc/intel-igc-opencl-2_2.42.6+23070_amd64.deb" \
                "$neo/intel-ocloc_26.40.40196.8-0_amd64.deb" &&
        printf '%s  %s\n' \
          e20539632f2d99875d8f8b225d5da58b31f98baa54bb47c6328aa98ec473ab9d intel-igc-core-2_2.42.6+23070_amd64.deb \
          7f5593cf828503ffb8450967768bde9209e22753bc576296e846b5a751011fa3 intel-igc-opencl-2_2.42.6+23070_amd64.deb \
          b2868d6d7958cc5ca28fa9b48a7d39961760ee39ef07673d5b58894dd53d6b4e intel-ocloc_26.40.40196.8-0_amd64.deb \
          | sha256sum -c --quiet); then
    echo "could not download or verify the ocloc and IGC .debs" >&2
    exit 1
  fi
  sudo apt-get update
  sudo apt-get install -y intel-oneapi-compiler-dpcpp-cpp-2025.2 "$debs"/*.deb
fi
set --   # setvars.sh reads the caller's "$@" as its own arguments
source /opt/intel/oneapi/setvars.sh
export CXX=icpx
export FFMT_FLAGS='-std=c++20 -Wall -Werror -fsycl -fsycl-targets=spir64_gen -Xsycl-target-backend "-device pvc" -O2 -fno-fast-math'
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_sycl.cpp examples/readme2_sycl.cpp"
