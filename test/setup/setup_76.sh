# Setup for id 76: Kokkos 5.2.2 Serial on Rome, g++ 13.3.0, buffer path, -O0, no fast-math.
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

case ${FFMT_RUNTIME_ROOT:-} in
  /*) ;;
  *) echo "setup_76.sh: FFMT_RUNTIME_ROOT must be an absolute path" >&2; return 1 ;;
esac
type module >/dev/null 2>&1 || source /etc/profile >/dev/null 2>&1
module use /soft/modulefiles
module load gcc/13.3.0
kokkos_prefix=$FFMT_RUNTIME_ROOT/software/rome/install/kokkos-serial
kokkos_libdir=lib
if [ ! -d "$kokkos_prefix/lib" ]; then kokkos_libdir=lib64; fi
export LD_LIBRARY_PATH=$kokkos_prefix/$kokkos_libdir:$FFMT_RUNTIME_ROOT/software/rome/install/host-runtime/lib:${LD_LIBRARY_PATH:-}
export CXX=/soft/compilers/gcc/13.3.0/x86_64-suse-linux/bin/g++
export FMT_DIR=$FFMT_RUNTIME_ROOT/third_party/fmt-main
export FFMT_FLAGS="-std=c++20 -Wall -Werror -O0 -fno-fast-math -DFFMT_BUFFER_PATH=1 -DFFMT_TEST_KOKKOS=1 -DFFMT_TEST_KOKKOS_SPACE=Kokkos::Serial -isystem$kokkos_prefix/include"
export FFMT_EXTRA_FILES="examples/ccpreference_print.cpp"
export FFMT_LDFLAGS="-L$kokkos_prefix/$kokkos_libdir -lkokkoscontainers -lkokkoscore -ldl -pthread"
unset kokkos_prefix kokkos_libdir
