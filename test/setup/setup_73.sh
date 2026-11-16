# Setup for id 73: AdaptiveCpp 25.10.0 generic/SSCP on MI300X (gfx942), LLVM 20.1.8, -O2, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

case ${FFMT_RUNTIME_ROOT:-} in
  /*) ;;
  *) echo "setup_73.sh: FFMT_RUNTIME_ROOT must be an absolute path" >&2; return 1 ;;
esac
type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load gcc/13.3.0
module load rocm/7.0.2
export HIP_VISIBLE_DEVICES=0
export PATH=$FFMT_RUNTIME_ROOT/software/mi300x/install/adaptivecpp/bin:$FFMT_RUNTIME_ROOT/software/x86_64/install/llvm20-shared/bin:$PATH
export LD_LIBRARY_PATH=$FFMT_RUNTIME_ROOT/software/mi300x/install/adaptivecpp/lib:$FFMT_RUNTIME_ROOT/software/mi300x/install/adaptivecpp/lib64:$FFMT_RUNTIME_ROOT/software/x86_64/install/llvm20-shared/lib:$FFMT_RUNTIME_ROOT/software/x86_64/install/llvm20-shared/lib/x86_64-unknown-linux-gnu:$FFMT_RUNTIME_ROOT/software/mi300x/install/host-runtime/lib:${LD_LIBRARY_PATH:-}
export CXX=$FFMT_RUNTIME_ROOT/software/mi300x/install/adaptivecpp/bin/acpp
export FFMT_ORACLE_CXX=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/bin/g++
export FMT_DIR=$FFMT_RUNTIME_ROOT/third_party/fmt-main
export FFMT_FLAGS="-std=c++20 -Wall -Werror --acpp-targets=generic -O2 -fno-fast-math"
export FFMT_EXTRA_FILES="examples/readme1_sycl.cpp examples/readme2_sycl.cpp"
export FFMT_LDFLAGS=-no-pie
export FFMT_RUN_WRAPPER='timeout --signal=TERM --kill-after=10s 1800s'
