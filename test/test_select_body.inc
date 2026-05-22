// SYCL/ACPP TUs use the queue-based body. OpenMP TUs use the omp-target body
// (no queue, work wrapped in `#pragma omp target`). Everything else (host
// test rig, plain CPU) uses the no-queue body that just calls KHX_PRINT
// from a serial loop. Check compiler predefines directly — the header that
// defines FMT_SYCL_COMPILER_ACPP isn't included yet at this point (test
// files include this _before_ pulling in sycl_khx_print.hpp).
#if defined(SYCL_LANGUAGE_VERSION) \
    || defined(__ADAPTIVECPP__) || defined(__HIPSYCL__) || defined(__ACPP__)
#include "test_body.inc"
#elif defined(_OPENMP)
#include "test_body_omp.inc"
#else
#include "test_body_host.inc"
#endif
