// SYCL/ACPP TUs use the queue-based body. OpenMP TUs use the omp-target body
// (no queue, work wrapped in `#pragma omp target`). CUDA TUs (.cu compiled
// with nvcc or `clang++ -x cuda`) use the cuda body that launches each test
// as a __global__ kernel. Everything else (host test rig, plain CPU) uses
// the no-queue body that just calls FFMT_PRINT from a serial loop. Check
// compiler predefines directly — the header that defines
// FFMT_COMPILER_ACPP isn't included yet at this point (test files
// include this _before_ pulling in ffmt/base.hpp).
#if defined(SYCL_LANGUAGE_VERSION) \
    || defined(__ADAPTIVECPP__) || defined(__HIPSYCL__) || defined(__ACPP__)
#include "test_body.inc"
#elif defined(_OPENMP)
#include "test_body_omp.inc"
#elif defined(__CUDACC__)
#include "test_body_cuda.inc"
#else
#include "test_body_host.inc"
#endif
