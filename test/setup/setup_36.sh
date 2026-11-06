# Setup for id 36: GitHub clang-22 CUDA (sm_80), CUDA 12.6.3, -O2, no fast-math, build only (no GPU).
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

# GitHub runner: clang-22 from apt.llvm.org, with libomp for -fopenmp, and
# the CUDA 12.6 pieces clang needs for NVPTX: nvcc's package (headers,
# libdevice, ptxas, nvlink), cudart, and libcurand-dev, whose header clang's
# CUDA wrapper includes even when cuRAND is unused. Installed only if
# missing; no GPU or driver. clang 22 is the first release that fully
# supports CUDA 12.6: 20 and 21 stop at 12.3 and warn, which -Werror rejects.
if ! command -v clang++-22 >/dev/null 2>&1; then
  wget -q https://apt.llvm.org/llvm.sh
  sudo bash llvm.sh 22
  rm -f llvm.sh
  sudo apt-get install -y libomp-22-dev
fi
export CUDA_PATH=/usr/local/cuda-12.6
if [ ! -x "$CUDA_PATH/bin/ptxas" ]; then
  wget -q https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-keyring_1.1-1_all.deb
  sudo dpkg -i cuda-keyring_1.1-1_all.deb
  rm -f cuda-keyring_1.1-1_all.deb
  sudo apt-get update
  sudo apt-get install -y cuda-nvcc-12-6 cuda-cudart-dev-12-6 libcurand-dev-12-6
fi
export CXX=clang++-22
export FFMT_FLAGS="-std=c++20 -Wall -Werror --cuda-path=${CUDA_PATH} -x cuda --cuda-gpu-arch=sm_80 -L${CUDA_PATH}/lib64 -lcudart -O2 -fno-fast-math"
# The README examples for this programming model: built with the flags
# above and run by the test goal, but not tests (Makefile.id).
export FFMT_EXTRA_FILES="examples/readme1_cuda.cu examples/readme2_cuda.cu"
