# Setup for id 78: Kokkos 5.2.2 CUDA on A40 (sm_86), clang 22.1.8 + cuda 12.9.1, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

case ${FFMT_RUNTIME_ROOT:-} in
  /*) ;;
  *) echo "setup_78.sh: FFMT_RUNTIME_ROOT must be an absolute path" >&2; return 1 ;;
esac
type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load gcc/13.3.0
module load llvm/release-22.1.8
module load cuda/12.9.1
export CUDA_VISIBLE_DEVICES=0
kokkos_prefix=$FFMT_RUNTIME_ROOT/software/h100/install/kokkos-cuda-sm86
kokkos_libdir=lib
if [ ! -d "$kokkos_prefix/lib" ]; then kokkos_libdir=lib64; fi
export LD_LIBRARY_PATH=$kokkos_prefix/$kokkos_libdir:$FFMT_RUNTIME_ROOT/software/h100/install/host-runtime/lib:${LD_LIBRARY_PATH:-}
export CXX=/soft/compilers/llvm/release-22.1.8/bin/clang++
export FMT_DIR=$FFMT_RUNTIME_ROOT/third_party/fmt-main
export FFMT_FLAGS="-std=c++20 -Wall -Werror -Wno-unknown-cuda-version -x cuda --cuda-path=$CUDA_PATH --cuda-gpu-arch=sm_86 -L$CUDA_PATH/lib64 -lcudart -O0 -fno-fast-math -DFFMT_TEST_KOKKOS=1 -DFFMT_TEST_KOKKOS_SPACE=Kokkos::Cuda -isystem$kokkos_prefix/include"
export FFMT_EXTRA_FILES="examples/readme1_cuda.cu examples/readme2_cuda.cu"
export FFMT_LDFLAGS="-L$kokkos_prefix/$kokkos_libdir -lkokkoscontainers -lkokkoscore -ldl -pthread -lcuda"
unset kokkos_prefix kokkos_libdir
