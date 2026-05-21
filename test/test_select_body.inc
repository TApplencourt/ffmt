// SYCL/ACPP TUs use the queue-based body; everything else (host test rig,
// OpenMP) uses the no-queue body. Check compiler predefines directly — the
// header that defines FMT_SYCL_COMPILER_ACPP isn't included yet at this
// point (test files include this _before_ pulling in sycl_khx_print.hpp).
#if defined(SYCL_LANGUAGE_VERSION) \
    || defined(__ADAPTIVECPP__) || defined(__HIPSYCL__) || defined(__ACPP__)
#include "test_body.inc"
#else
#include "test_body_host.inc"
#endif
