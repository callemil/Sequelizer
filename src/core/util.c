// **********************************************************************
// core/util.c - Shared Utility Functions
// **********************************************************************
// S Magierowski Aug 23 2025
//

#include "util.h"
#include <stdio.h>

// **********************************************************************
// Progress Display Functions
// **********************************************************************

void display_progress_simple(int completed, int total, bool verbose, const char *operation) {
  if (total == 0) return;
  
  int percent = (completed * 100) / total;
  int bar_width = 40;
  int filled = (completed * bar_width) / total;
  
  printf("\r[");
  for (int i = 0; i < bar_width; i++) {
    printf(i < filled ? "█" : "░");
  }
  printf("] %d%% (%d/%d)", percent, completed, total);
  
  if (verbose && operation) {
    printf(" %s", operation);
  }
  
  fflush(stdout);
}