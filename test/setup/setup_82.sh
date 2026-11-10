# Setup for id 82: Kokkos 5.2.2 SYCL on Chiatta Max 1550, Intel LLVM 7.2.0 clang++, UMD 1032.21, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

case ${FFMT_RUNTIME_ROOT:-} in
  /*) ;;
  *) echo "setup_82.sh: FFMT_RUNTIME_ROOT must be an absolute path" >&2; return 1 ;;
esac
type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load intel_compute_runtime/release/821.36
export LD_LIBRARY_PATH=/soft/libraries/intel-gpu-umd/1032.21/lib64:/soft/libraries/intel-gpu-umd/1032.21/lib64/intel-opencl:$FFMT_RUNTIME_ROOT/software/chiatta/install/intel-llvm/lib:${LD_LIBRARY_PATH:-}
export PATH=$FFMT_RUNTIME_ROOT/software/chiatta/install/intel-llvm/bin:$PATH
export ONEAPI_DEVICE_SELECTOR=level_zero:gpu
export ZE_AFFINITY_MASK=0.0
kokkos_prefix=$FFMT_RUNTIME_ROOT/software/chiatta/install/kokkos-sycl
kokkos_libdir=lib
if [ ! -d "$kokkos_prefix/lib" ]; then kokkos_libdir=lib64; fi
export LD_LIBRARY_PATH=$kokkos_prefix/$kokkos_libdir:$LD_LIBRARY_PATH
export CXX=$FFMT_RUNTIME_ROOT/software/chiatta/install/intel-llvm/bin/clang++
export FMT_DIR=$FFMT_RUNTIME_ROOT/third_party/fmt-main
export FFMT_FLAGS="-std=c++20 -Wall -Werror -fsycl -include sycl/sycl.hpp -O0 -fno-fast-math -DFFMT_TEST_KOKKOS=1 -DFFMT_TEST_KOKKOS_SPACE=Kokkos::SYCL -isystem$kokkos_prefix/include"
export FFMT_EXTRA_FILES="examples/readme1_sycl.cpp examples/readme2_sycl.cpp"
export FFMT_LDFLAGS="-L$kokkos_prefix/$kokkos_libdir -lkokkoscontainers -lkokkoscore -ldl -pthread"
unset kokkos_prefix kokkos_libdir
