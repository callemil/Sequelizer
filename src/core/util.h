// **********************************************************************
// core/util.h - Shared Utility Functions
// **********************************************************************
// S Magierowski Aug 23 2025
//
#ifndef SEQUELIZER_UTIL_H
#define SEQUELIZER_UTIL_H

#include <stdbool.h>

// **********************************************************************
// Progress Display Functions
// **********************************************************************

// Simple progress bar display with customizable operation text
void display_progress_simple(int completed, int total, bool verbose, const char *operation);

#endif // SEQUELIZER_UTIL_H