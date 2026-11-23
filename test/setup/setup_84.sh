# Setup for id 84: GitHub ROCm 10.1.0 HIP (gfx942), -O2, no fast-math, build only (no GPU).
# Sourced by test/run_id.sh; must export CXX and FFMT_FLAGS.

# GitHub runner: ROCm 10.1.0 from AMD's stable wheel index, in a virtual
# environment (no root, no apt; about 4 GB, cached by the workflow). Its
# hipcc builds for MI300X (gfx942), as id 49 does on JLSE; there is no GPU
# to run on. Installed only if missing.
rocm_venv=${FFMT_ROCM_VENV:-$HOME/rocm-10.1.0}
if [ ! -x "$rocm_venv/bin/rocm-sdk" ]; then
  python3 -m venv "$rocm_venv" &&
    "$rocm_venv/bin/pip" install -q --index-url https://stable.repo.amd.com/rocm/whl-next/ \
      "rocm[devel]==10.1.0" || exit 1
fi
export ROCM_PATH=$("$rocm_venv/bin/rocm-sdk" path --root)
export PATH=$ROCM_PATH/bin:$PATH
export CXX=$ROCM_PATH/bin/hipcc
export FFMT_FLAGS="-std=c++20 -Wall -Werror -x hip --offload-arch=gfx942 -O2 -fno-fast-math"
unset rocm_venv
# For test/resource_usage.py: the compiler reports each kernel's registers, scratch and spills.
export FFMT_TEST_FLAGS='-Rpass-analysis=kernel-resource-usage'
