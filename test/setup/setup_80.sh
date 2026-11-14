# Setup for id 80: Kokkos 5.2.2 HIP on MI300X (gfx942), ROCm 7.0.2 hipcc, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

case ${FFMT_RUNTIME_ROOT:-} in
  /*) ;;
  *) echo "setup_80.sh: FFMT_RUNTIME_ROOT must be an absolute path" >&2; return 1 ;;
esac
type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load gcc/13.3.0
module load rocm/7.0.2
export HIP_VISIBLE_DEVICES=0
kokkos_prefix=$FFMT_RUNTIME_ROOT/software/mi300x/install/kokkos-hip
kokkos_libdir=lib
if [ ! -d "$kokkos_prefix/lib" ]; then kokkos_libdir=lib64; fi
export LD_LIBRARY_PATH=$kokkos_prefix/$kokkos_libdir:$FFMT_RUNTIME_ROOT/software/mi300x/install/host-runtime/lib:${LD_LIBRARY_PATH:-}
export CXX=$ROCM_PATH/bin/hipcc
export FFMT_ORACLE_CXX=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/bin/g++
export FMT_DIR=$FFMT_RUNTIME_ROOT/third_party/fmt-main
export FFMT_FLAGS="-std=c++20 -Wall -Werror -x hip --offload-arch=gfx942 -O0 -fno-fast-math -DFFMT_TEST_KOKKOS=1 -DFFMT_TEST_KOKKOS_SPACE=Kokkos::HIP -isystem$kokkos_prefix/include"
export FFMT_LDFLAGS="-L$kokkos_prefix/$kokkos_libdir -lkokkoscontainers -lkokkoscore -ldl -pthread"
unset kokkos_prefix kokkos_libdir
