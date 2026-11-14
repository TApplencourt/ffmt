# Setup for id 64: Intel LLVM 7.2.0 clang++ SYCL on Chiatta Max 1550, intel_compute_runtime/release/821.36 + UMD 1032.21, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

case ${FFMT_RUNTIME_ROOT:-} in
  /*) ;;
  *) echo "setup_64.sh: FFMT_RUNTIME_ROOT must be an absolute path" >&2; return 1 ;;
esac
type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load intel_compute_runtime/release/821.36
export LD_LIBRARY_PATH=/soft/libraries/intel-gpu-umd/1032.21/lib64:/soft/libraries/intel-gpu-umd/1032.21/lib64/intel-opencl:$FFMT_RUNTIME_ROOT/software/chiatta/install/intel-llvm/lib:${LD_LIBRARY_PATH:-}
export PATH=$FFMT_RUNTIME_ROOT/software/chiatta/install/intel-llvm/bin:$PATH
export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
unset ZE_AFFINITY_MASK
export CXX=$FFMT_RUNTIME_ROOT/software/chiatta/install/intel-llvm/bin/clang++
export FMT_DIR=$FFMT_RUNTIME_ROOT/third_party/fmt-main
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fsycl -include sycl/sycl.hpp -O0 -fno-fast-math"
export FFMT_EXTRA_FILES="examples/readme1_sycl.cpp examples/readme2_sycl.cpp"
