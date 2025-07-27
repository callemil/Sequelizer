#ifndef SEQUELIZER_H
#define SEQUELIZER_H

// Sequelizer - Open Source DNA Sequence Analysis Toolkit
// Public API definitions

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

// Version information
#define SEQUELIZER_VERSION_MAJOR 0
#define SEQUELIZER_VERSION_MINOR 1
#define SEQUELIZER_VERSION_PATCH 0

// Subcommand modes
typedef enum {
  SEQUELIZER_MODE_UNKNOWN = 0,
  SEQUELIZER_MODE_SEQGEN,
  SEQUELIZER_MODE_HELP
} sequelizer_mode_t;

// Main function declarations
int main_help_short(void);
int main_help_long(void);
int main_seqgen(int argc, char *argv[]);

// Utility functions
sequelizer_mode_t get_sequelizer_mode(const char *mode_string);

#ifdef __cplusplus
}
#endif

#endif // SEQUELIZER_H