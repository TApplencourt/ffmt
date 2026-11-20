# Setup for id 74: chipStar 4b91fc6 HIP over Level Zero on Chiatta Max 1550, LLVM 23.1.2 SPIR-V backend, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

case ${FFMT_RUNTIME_ROOT:-} in
  /*) ;;
  *) echo "setup_74.sh: FFMT_RUNTIME_ROOT must be an absolute path" >&2; return 1 ;;
esac
type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load intel/oneapi/release/2026.2.0 compiler
export LD_LIBRARY_PATH=$FFMT_RUNTIME_ROOT/software/chiatta/install/chipstar-20261119-23-Release/lib:$FFMT_RUNTIME_ROOT/software/chiatta/install/chipstar-20261119-23-Release/lib64:$FFMT_RUNTIME_ROOT/software/x86_64/install/clang-chip-spv-23-20261119-23-Release/lib:${LD_LIBRARY_PATH:-}
export PATH=$FFMT_RUNTIME_ROOT/software/chiatta/install/chipstar-20261119-23-Release/bin:$FFMT_RUNTIME_ROOT/software/x86_64/install/clang-chip-spv-23-20261119-23-Release/bin:$PATH
export CHIP_BE=level0
export CHIP_DEVICE_TYPE=gpu
export CXX=$FFMT_RUNTIME_ROOT/software/chiatta/install/chipstar-20261119-23-Release/bin/hipcc
export FFMT_ORACLE_CXX=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/bin/g++
export FMT_DIR=$FFMT_RUNTIME_ROOT/third_party/fmt-main
export FFMT_FLAGS="-std=c++20 -Wall -Werror -x hip -O0 -fno-fast-math"
