# Setup for id 75: chipStar 1.3.0 HIP over Level Zero on Chiatta Max 1550, typed SPIR-V translator, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

case ${FFMT_RUNTIME_ROOT:-} in
  /*) ;;
  *) echo "setup_75.sh: FFMT_RUNTIME_ROOT must be an absolute path" >&2; return 1 ;;
esac
type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load gcc/13.3.0
module load intel_compute_runtime/release/821.36
export LD_LIBRARY_PATH=/soft/libraries/intel-gpu-umd/1032.21/lib64:/soft/libraries/intel-gpu-umd/1032.21/lib64/intel-opencl:$FFMT_RUNTIME_ROOT/software/chiatta/install/chipstar/lib:$FFMT_RUNTIME_ROOT/software/chiatta/install/chipstar/lib64:$FFMT_RUNTIME_ROOT/software/chiatta/install/spirv-translator/lib:$FFMT_RUNTIME_ROOT/software/x86_64/install/llvm20-shared/lib:$FFMT_RUNTIME_ROOT/software/chiatta/install/host-runtime/lib:${LD_LIBRARY_PATH:-}
export PATH=$FFMT_RUNTIME_ROOT/software/chiatta/install/chipstar/bin:$FFMT_RUNTIME_ROOT/software/x86_64/install/llvm20-shared/bin:$PATH
export CHIP_BE=level0
export CHIP_DEVICE_TYPE=gpu
export CXX=$FFMT_RUNTIME_ROOT/software/chiatta/install/chipstar/bin/hipcc
export FFMT_ORACLE_CXX=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/bin/g++
export FMT_DIR=$FFMT_RUNTIME_ROOT/third_party/fmt-main
export FFMT_FLAGS="-std=c++20 -Wall -Werror -x hip --gcc-toolchain=/soft/compilers/gcc/13.3.0/x86_64-suse-linux -B$FFMT_RUNTIME_ROOT/software/chiatta/install/spirv-translator-typed/bin -O2 -fno-fast-math"
