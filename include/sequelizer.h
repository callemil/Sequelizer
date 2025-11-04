// **********************************************************************
// include/sequelizer.h
// **********************************************************************
// Sebastian Claudiusz Magierowski Jul 31 2025
// 
#ifndef SEQUELIZER_H
#define SEQUELIZER_H

// Sequelizer - Open Source DNA Sequence Analysis Toolkit
// Public API definitions

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h> // size_t
#include <string.h> // memcopy
#include <err.h>

// Version information
#define SEQUELIZER_VERSION_MAJOR 0
#define SEQUELIZER_VERSION_MINOR 1
#define SEQUELIZER_VERSION_PATCH 0

# ifdef ABORT_ON_NULL
#   define RETURN_NULL_IF(A, B) \
    if (A) {    \
      warnx("Failure at %s : %d", __FILE__, __LINE__);    \
      abort(); \
    }
# else
#   define RETURN_NULL_IF(A, B) if (A) { return B; }
# endif

#ifdef __cplusplus
}
#endif

#endif // SEQUELIZER_H