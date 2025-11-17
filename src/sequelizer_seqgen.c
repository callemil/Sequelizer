// **********************************************************************
// sequelizer_seqgen.c
// **********************************************************************
// Sebastian Claudiusz Magierowski Oct 15 2025
/*
Generates signals from read sequences: squiggle, raw, and event sequences.
 squiggle: pos (position along ref), base (from ref at position), current (normalized current), sd (standard deviation of normalized current), dwell (number of samples over which current dwells)
 raw:      is just a model of squiggle in terms of samples with a normal distribution;
 event:    is just a piecewise-constant (i.e., no normal distribution) version of raw

 Example uses:
 Produce squiggle:               sequelizer seqgen ../reads/sebastian_squiggles.fa
 Squiggle to file:               sequelizer seqgen ../reads/sebastian_squiggles.fa -o seb.out
 Produce raw:                    sequelizer seqgen --raw ../reads/sebastian_squiggles.fa
 Alternate sample rate (in kHz): sequelizer seqgen --raw --srate 3 ./reads/sebastian_squiggles.fa
 Produce event:                  sequelizer seqgen --event ./reads/sebastian_squiggles.fa
 Generate raw:                   sequelizer seqgen -g -r -L 200 -o raw_data200.txt
 Generate 5 squiggle:            sequelizer seqgen -g --num-sequences 5 --seq-length 100
 Generate 10 squiggle to file:   sequelizer seqgen --generate --num-sequences 10 --seq-length 50 -o output.txt
 Multiple FASTA to one o/p:      sequelizer seqgen file1.fa file2.fa file3.fa -o combined_output.txt
 Run a size 3 kmer model:        sequelizer seqgen --generate --model rna_r9.4_180mv_70bps --kmer-size 3 --seq-length 10

 Fast5/HDF5 output examples (requires --raw, --fast5, -o):
 Generate single synthetic read:     sequelizer seqgen --raw --fast5 --generate --seq-length 100 -o synthetic.fast5
 Generate multiple synthetic reads:  sequelizer seqgen --raw --fast5 --generate --num-sequences 5 --seq-length 200 -o multi_synthetic.fast5
 Generate data w/ reference:         sequelizer seqgen --raw --fast5 --generate --seq-length=1000 --num-sequences=3 --reference=reference.fa -o signals.fast5
 Convert FASTA to Fast5:             sequelizer seqgen --raw --fast5 ../reads/test_squiggles.fa -o converted.fast5
 Multiple FASTA to Fast5:            sequelizer seqgen --raw --fast5 file1.fa file2.fa file3.fa -o combined.fast5
 Consolidate multiple FASTA in ref:  sequelizer seqgen --raw --fast5 file1.fa file2.fa file3.fa --reference=all_sequences.fa -o combined.fast5
 Fast5 with custom sample rate:      sequelizer seqgen --raw --fast5 --srate 8 ../reads/input.fa -o high_rate.fast5
 Fast5 with read limit:              sequelizer seqgen --raw --fast5 --limit 3 ../reads/many_reads.fa -o limited.fast5
 Limit and multiple i/p files:       sequelizer seqgen --raw --fast5 --limit 100 file1.fa file2.fa file3.fa -o limited.fast5
 Fast5 with reproducible generation: sequelizer seqgen --raw --fast5 --generate --seed 42 --num-sequences 10 -o reproducible.fast5
 Fast5 with kmer model:              sequelizer seqgen --raw --fast5 --generate --model dna_r10.4.1_e8.2_260bps --kmer-size 9 -o kmer.fast5
 Save BOTH fast5 & txt:              sequelizer seqgen --raw --fast5 --save-text --generate --seq-length 50 --num-sequences 1 --reference debug_ref.fa -o debug_signals.fast5

 Notes:
  - read names preserved: each read keeps its original FASTA header name
  - limit enforcement: --limit applies across ALL files, not per file

 Current limitation:
  Cannot generate multiple separate output files - the system currently only supports:
  - One output destination (stdout or single file via -o)
  - One output format per run (squiggle OR raw OR event)
*/
#include <stdio.h>
#include <unistd.h> // on macOS POSIX read() lives in <unistd.h>, not in <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <err.h>
#include <argp.h>

#include "core/seqgen_models.h"
#include "core/seqgen_utils.h"
#include "core/seq_utils.h"
#include "core/seq_tensor.h"
#include "core/kseq.h"         // lightweight FASTA/FASTQ parser from klib
#include "core/fast5_io.h"     // Fast5 file writing functions

KSEQ_INIT(int, read) // create a kseq parser that reads from an int fd using the standard C read() system call

// **********************************************************************
// Helper functions for kseq synthetic sequences
// **********************************************************************

// Initialize a kseq_t structure for manual population (without file I/O)
static kseq_t* kseq_init_synthetic(void) {
  kseq_t *seq = calloc(1, sizeof(kseq_t));
  if (seq == NULL) return NULL;

  // Initialize the kstring_t structures
  seq->name.l = seq->name.m = 0;
  seq->name.s = NULL;
  seq->comment.l = seq->comment.m = 0;
  seq->comment.s = NULL;
  seq->seq.l = seq->seq.m = 0;
  seq->seq.s = NULL;
  seq->qual.l = seq->qual.m = 0;
  seq->qual.s = NULL;

  return seq;
}

// Set the sequence name in a kseq_t structure
static void kseq_set_name(kseq_t *seq, const char *name) {
  if (seq == NULL || name == NULL) return;

  size_t len = strlen(name);
  if (seq->name.m < len + 1) {
    seq->name.m = len + 1;
    seq->name.s = realloc(seq->name.s, seq->name.m);
  }
  strcpy(seq->name.s, name);
  seq->name.l = len;
}

// Set the sequence data in a kseq_t structure (takes ownership of sequence string)
static void kseq_set_seq(kseq_t *seq, char *sequence, size_t length) {
  if (seq == NULL) return;

  // Free existing sequence if any
  if (seq->seq.s != NULL) {
    free(seq->seq.s);
  }

  // Take ownership of the provided sequence
  seq->seq.s = sequence;
  seq->seq.l = length;
  seq->seq.m = length + 1;
}

// Clean up a synthetic kseq_t (simpler than full kseq_destroy since no file stream)
static void kseq_destroy_synthetic(kseq_t *seq) {
  if (seq == NULL) return;

  if (seq->name.s) free(seq->name.s);
  if (seq->comment.s) free(seq->comment.s);
  if (seq->seq.s) free(seq->seq.s);
  if (seq->qual.s) free(seq->qual.s);
  free(seq);
}

// **********************************************************************
// Helper function to count sequences across files
// **********************************************************************

static int count_total_sequences(char **files, int nfile) {
  int total = 0;

  for (int i = 0; i < nfile; i++) {
    FILE *fp = fopen(files[i], "r");
    if (fp == NULL) {
      warnx("Failed to open file for counting: %s", files[i]);
      continue;
    }

    kseq_t *seq = kseq_init(fileno(fp));
    while (kseq_read(seq) >= 0) {
      total++;
    }
    kseq_destroy(seq);
    fclose(fp);
  }

  return total;
}

// **********************************************************************
// Argument Parsing
// **********************************************************************
static char doc[] = "sequelizer seqgen -- Signal generation from DNA sequence reads\v"
"EXAMPLES:\n"
"  sequelizer seqgen reads.fa\n"
"  sequelizer seqgen -g --num-sequences 5 --seq-length 100";

static char args_doc[] = "fasta [fasta ...]";

static struct argp_option options[] = {
  {"model",         'm', "name",       0, "K-mer model name (e.g., 'rna_r9.4_180mv_70bps', 'dna_r10.4.1_e8.2_260bps')"},
  {"models-dir",    'd', "path",       0, "K-mer models directory (default: 'kmer_models')"},
  {"kmer-size",     'k', "size",       0, "K-mer size for k-mer model (default: 5)"},
  {"limit",         'l', "nreads",     0, "Maximum number of reads to call (0 is unlimited)"},
  {"output",        'o', "filename",   0, "Write to file rather than stdout"},
  {"prefix",        'p', "string",     0, "Prefix to append to name of each read"},
  {"rescale",        1,  0,            0, "Rescale network output"},
  {"no-rescale",     2,  0, OPTION_ALIAS, "Don't rescale network output"},
  {"raw",           'r', 0,            0, "Generate raw signal from squiggle events"},
  {"event",         'e', 0,            0, "Generate event signal from squiggle"},
  {"fast5",         'f', 0,            0, "Output signals in Fast5/HDF5 format (requires --raw and -o)"},
  {"srate",         's', "rate",       0, "Sampling rate in kHz (default: 4.0)"},
  {"generate",      'g', 0,            0, "Generate synthetic sequences instead of reading from file"},
  {"seq-length",    'L', "length",     0, "Length of generated sequences in bases (default: 100)"},
  {"num-sequences", 'N', "count",      0, "Number of sequences to generate (default: 1)"},
  {"seed",          'S', "seed",       0, "Random seed for reproducible generation (optional)"},
  {"reference",     'R', "filename",   0, "Save sequences to reference FASTA file (use with --generate or multiple inputs)"},
  {"save-text",     'T', 0,            0, "Also save text format when using --fast5 (creates .txt companion file)"},
  {0}
};

struct arguments {
  char *model_name;
  char *models_dir;
  int kmer_size;
  int limit;
  FILE *output;
  char *output_filename;
  char *prefix;
  bool rescale;
  bool generate_raw;
  bool generate_event;
  bool generate_sequences;
  bool output_fast5;
  int seq_length;
  int num_sequences;
  unsigned int seed;
  bool use_seed;
  float sample_rate_khz;
  char *reference_filename;
  FILE *reference_file;
  bool save_text;
  char **files;
};

static int parse_arg(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
    case 'm':
      arguments->model_name = arg;
      break;
    case 'd':
      arguments->models_dir = arg;
      break;
    case 'k':
      arguments->kmer_size = atoi(arg);
      if (arguments->kmer_size <= 0 || arguments->kmer_size > 9) {
        errx(EXIT_FAILURE, "K-mer size must be between 1 and 9, got %d", arguments->kmer_size);
      }
      break;
    case 'l':
      arguments->limit = atoi(arg);
      if (arguments->limit < 0) {
        errx(EXIT_FAILURE, "Limit must be non-negative, got %d", arguments->limit);
      }
      break;
    case 'o':
      arguments->output_filename = arg;
      arguments->output = fopen(arg, "w");
      if (NULL == arguments->output) {
        errx(EXIT_FAILURE, "Failed to open \"%s\" for output.", arg);
      }
      break;
    case 'p':
      arguments->prefix = arg;
      break;
    case 1:
      arguments->rescale = true;
      break;
    case 2:
      arguments->rescale = false;
      break;
    case 'r':
      arguments->generate_raw = true;
      break;
    case 'e':
      arguments->generate_event = true;
      break;
    case 'f':
      arguments->output_fast5 = true;
      break;
    case 's':
      arguments->sample_rate_khz = atof(arg);
      if (arguments->sample_rate_khz <= 0.0) {
        errx(EXIT_FAILURE, "Sampling rate must be positive, got %f", arguments->sample_rate_khz);
      }
      break;
    case 'g':
      arguments->generate_sequences = true;
      break;
    case 'L':
      arguments->seq_length = atoi(arg);
      if (arguments->seq_length <= 0) {
        errx(EXIT_FAILURE, "Sequence length must be positive, got %d", arguments->seq_length);
      }
      break;
    case 'N':
      arguments->num_sequences = atoi(arg);
      if (arguments->num_sequences <= 0) {
        errx(EXIT_FAILURE, "Number of sequences must be positive, got %d", arguments->num_sequences);
      }
      break;
    case 'S':
      arguments->seed = (unsigned int)atoi(arg);
      arguments->use_seed = true;
      break;
    case 'R':
      arguments->reference_filename = arg;
      arguments->reference_file = fopen(arg, "w");
      if (NULL == arguments->reference_file) {
        errx(EXIT_FAILURE, "Failed to open reference file \"%s\" for writing", arg);
      }
      break;
    case 'T':
      arguments->save_text = true;
      break;
    case ARGP_KEY_NO_ARGS:
      if (!arguments->generate_sequences) {
        argp_usage(state);
      }
      break;
    case ARGP_KEY_ARG:
      arguments->files = &state->argv[state->next - 1];
      state->next = state->argc;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { options, parse_arg, args_doc, doc };

// **********************************************************************
// Main Function
// **********************************************************************
int main_seqgen(int argc, char *argv[]) {

  // ========================================================================
  // STEP 1: Initialize arguments structure with defaults
  // ========================================================================
  struct arguments arguments;

  // Set sensible defaults for all configuration options
  arguments.model_name = "rna_r9.4_180mv_70bps";  // Small 5-mer default for testing
  arguments.models_dir = "kmer_models";
  arguments.kmer_size = 5;
  arguments.limit = 0;
  arguments.output = NULL;
  arguments.output_filename = NULL;
  arguments.prefix = "";
  arguments.rescale = true;
  arguments.generate_raw = false;
  arguments.generate_event = false;
  arguments.generate_sequences = false;
  arguments.output_fast5 = false;
  arguments.seq_length = 100;
  arguments.num_sequences = 1;
  arguments.seed = 0;
  arguments.use_seed = false;
  arguments.sample_rate_khz = 4.0;
  arguments.reference_filename = NULL;
  arguments.reference_file = NULL;
  arguments.save_text = false;
  arguments.files = NULL;

  // ========================================================================
  // STEP 2: Parse command line arguments
  // ========================================================================
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  // Validate --fast5 constraints
  if (arguments.output_fast5) {
    if (!arguments.generate_raw) {
      errx(EXIT_FAILURE, "--fast5 flag requires --raw flag (cannot output squiggle or event modes to Fast5)");
    }
    if (NULL == arguments.output_filename) {
      errx(EXIT_FAILURE, "--fast5 flag requires -o output filename (cannot output Fast5 to stdout)");
    }
  }

  // Validate --save-text constraints
  if (arguments.save_text && !arguments.output_fast5) {
    errx(EXIT_FAILURE, "--save-text flag requires --fast5 flag (text output is automatic without --fast5)");
  }

  // Set up text output file for --save-text mode
  if (arguments.save_text) {
    // Create companion text file by replacing .fast5 extension with .txt
    char *text_filename = malloc(strlen(arguments.output_filename) + 10);
    strcpy(text_filename, arguments.output_filename);
    char *ext = strrchr(text_filename, '.');
    if (ext && strcmp(ext, ".fast5") == 0) {
      strcpy(ext, ".txt");
    } else {
      strcat(text_filename, ".txt");
    }
    arguments.output = fopen(text_filename, "w");
    if (NULL == arguments.output) {
      errx(EXIT_FAILURE, "Failed to open text output file \"%s\"", text_filename);
    }
    free(text_filename);
  } else if (NULL == arguments.output) {
    // Default to stdout if no output file specified
    arguments.output = stdout;
  }

  // ========================================================================
  // STEP 3: Initialize counters and pointers, determine loop parameters
  // ========================================================================
  int reads_started = 0;
  const int reads_limit = arguments.limit;

  // Determine loop count and Fast5 capacity based on mode
  int num_iterations;  // Total number of sequences to process
  int nfile = 0;       // Number of input files (file-based mode only)
  int fast5_capacity;  // Initial capacity for Fast5 arrays

  // File-based mode: need to track current file and sequence parser
  FILE **file_handles = NULL;
  kseq_t **file_parsers = NULL;
  int current_file_idx = 0;
  kseq_t *current_seq = NULL;

  if (arguments.generate_sequences) {
    // SYNTHETIC MODE: loop count = number of sequences to generate
    num_iterations = arguments.num_sequences;
    fast5_capacity = (reads_limit > 0) ? reads_limit : num_iterations;
  } else {
    // FILE-BASED MODE: count files and total sequences
    for (; arguments.files[nfile]; nfile++); // count files

    // Pre-count total sequences across all files
    int total_sequences = count_total_sequences(arguments.files, nfile);
    num_iterations = total_sequences;
    fast5_capacity = (reads_limit > 0) ? reads_limit : total_sequences;

    // Open all files and initialize parsers
    file_handles = calloc(nfile, sizeof(FILE*));
    file_parsers = calloc(nfile, sizeof(kseq_t*));

    // Open files & create kseq parsers
    for (int i = 0; i < nfile; i++) {
      file_handles[i] = fopen(arguments.files[i], "r");
      if (NULL == file_handles[i]) {
        warnx("Failed to open \"%s\" for input", arguments.files[i]);
        continue;
      }
      file_parsers[i] = kseq_init(fileno(file_handles[i]));
    }

    // Find first valid file & select its kseq_t parser
    current_file_idx = 0;
    while (current_file_idx < nfile && file_parsers[current_file_idx] == NULL) {
      current_file_idx++;
    }
    if (current_file_idx < nfile) {
      current_seq = file_parsers[current_file_idx];
    }
  }

  // Add dwell time tracking variables
  float total_dwell_time = 0.0f;
  size_t total_positions = 0;

  // Initialize random seed if requested
  if (arguments.use_seed) {
    srand(arguments.seed);
  }

  // Fast5 mode: prepare arrays to collect data
  seq_tensor **fast5_raw_signals = NULL;
  const char **fast5_read_names = NULL;
  int fast5_read_count = 0;

  if (arguments.output_fast5) {
    fast5_raw_signals = calloc(fast5_capacity, sizeof(seq_tensor*));
    fast5_read_names = calloc(fast5_capacity, sizeof(char*));
    if (NULL == fast5_raw_signals || NULL == fast5_read_names) {
      errx(EXIT_FAILURE, "Failed to allocate memory for Fast5 data collection");
    }
  }

  // ========================================================================
  // STEP 4: FLAT UNIFIED PROCESSING LOOP (handles both synthetic and file-based modes)
  // ========================================================================
  for (int i = 0; i < num_iterations; i++) {
    // Respect user-specified read limit
    if (reads_limit > 0 && reads_started >= reads_limit) {
      break;
    }
    reads_started += 1;

    // ======================================================================
    // STEP 4.1: Obtain next sequence (either synthetic or from file)
    // ======================================================================
    kseq_t *seq = NULL;
    bool is_synthetic = false;

    if (arguments.generate_sequences) {
      // SYNTHETIC MODE: Create a new sequence
      seq = kseq_init_synthetic();
      if (NULL == seq) {
        errx(EXIT_FAILURE, "Failed to allocate kseq_t for synthetic sequence %d", i + 1);
      }
      is_synthetic = true;

      // Generate synthetic DNA sequence and populate kseq_t
      char *generated_sequence = random_str(arguments.seq_length);
      if (NULL == generated_sequence) {
        kseq_destroy_synthetic(seq);
        errx(EXIT_FAILURE, "Failed to generate synthetic sequence %d", i + 1);
      }

      // Populate kseq_t fields
      char seq_name[32];
      snprintf(seq_name, sizeof(seq_name), "generated_%03d", i + 1);
      kseq_set_name(seq, seq_name);
      kseq_set_seq(seq, generated_sequence, arguments.seq_length);
    } else {
      // FILE-BASED MODE: Read next sequence from current file
      // Find next file with sequences if current file is exhausted
      while (current_file_idx < nfile) {
        if (current_seq != NULL && kseq_read(current_seq) >= 0) {
          seq = current_seq;
          break;
        }
        // Current file exhausted, move to next file
        current_file_idx++;
        if (current_file_idx < nfile) {
          current_seq = file_parsers[current_file_idx];
        }
      }

      if (seq == NULL) {
        // No more sequences available
        break;
      }
    }

    // Write sequence to reference file if requested
    if (arguments.reference_file != NULL) {
      if (seq->name.l > 0) {
        fprintf(arguments.reference_file, ">%s\n%s\n", seq->name.s, seq->seq.s);
      } else {
        fprintf(arguments.reference_file, ">sequence_%d\n%s\n", reads_started, seq->seq.s);
      }
    }

    // Debug output: show sequence length
    printf("seq length %zu\n", seq->seq.l);

    // ======================================================================
    // STEP 4.2: Set up model parameters for k-mer dispatcher
    // ======================================================================
    struct seqgen_model_params model_params = {
      .model_type = SEQGEN_MODEL_KMER,
      .params = {
        .kmer = {
          .model_name = arguments.model_name,
          .models_dir = arguments.models_dir,
          .kmer_size = arguments.kmer_size,
          .sample_rate_khz = arguments.sample_rate_khz
        }
      }
    };

    // ======================================================================
    // STEP 4.3: Generate squiggle data from sequence
    // This calls sequence_to_squiggle() -> dispatcher -> squiggle_kmer()
    // ======================================================================
    seq_tensor *squiggle = sequence_to_squiggle(
      seq->seq.s,
      seq->seq.l,
      arguments.rescale,
      &model_params
    );

    if (NULL != squiggle) {
      // Write sequence identifier to output (skip for Fast5 mode unless save_text is enabled)
      if (!arguments.output_fast5 || arguments.save_text) {
        fprintf(arguments.output, "#%s\n", seq->name.s);
      }

      // ====================================================================
      // STEP 4.4: Generate final output based on user's requested format
      // ====================================================================
      if (arguments.generate_raw) {
        // RAW MODE: Convert squiggle events to time-series samples with Gaussian noise
        seq_tensor *raw_signal = squiggle_to_raw(squiggle, arguments.sample_rate_khz);
        if (NULL != raw_signal) {
          if (arguments.output_fast5) {
            // Fast5 mode: check capacity and expand if needed
            if (fast5_read_count >= fast5_capacity) {
              fast5_capacity *= 2;
              fast5_raw_signals = realloc(fast5_raw_signals, fast5_capacity * sizeof(seq_tensor*));
              fast5_read_names = realloc(fast5_read_names, fast5_capacity * sizeof(char*));
              if (NULL == fast5_raw_signals || NULL == fast5_read_names) {
                errx(EXIT_FAILURE, "Failed to expand Fast5 data arrays");
              }
            }

            // Store the tensor and read name (we own these now)
            fast5_raw_signals[fast5_read_count] = raw_signal;
            fast5_read_names[fast5_read_count] = strdup(seq->name.s);
            fast5_read_count++;

            // Also output text if save_text is enabled
            if (arguments.save_text) {
              fprintf(arguments.output, "sample_index\traw_value\n");
              float *raw_data = seq_tensor_data_float(raw_signal);
              size_t num_samples = seq_tensor_dim(raw_signal, 0);
              for (size_t j = 0; j < num_samples; j++) {
                fprintf(arguments.output, "%zu\t%3.6f\n", j, raw_data[j]);
              }
            }
          } else {
            // Text mode: output to file/stdout then free
            fprintf(arguments.output, "sample_index\traw_value\n");
            float *raw_data = seq_tensor_data_float(raw_signal);
            size_t num_samples = seq_tensor_dim(raw_signal, 0);
            for (size_t j = 0; j < num_samples; j++) {
              fprintf(arguments.output, "%zu\t%3.6f\n", j, raw_data[j]);
            }
            seq_tensor_free(raw_signal);
          }
        }
      } else if (arguments.generate_event) {
        // EVENT MODE: Convert squiggle to piecewise-constant signal (no noise)
        seq_tensor *event_signal = squiggle_to_event(squiggle, arguments.sample_rate_khz);
        if (NULL != event_signal) {
          fprintf(arguments.output, "sample_index\tevent_value\n");
          float *event_data = seq_tensor_data_float(event_signal);
          size_t num_samples = seq_tensor_dim(event_signal, 0);
          for (size_t j = 0; j < num_samples; j++) {
            fprintf(arguments.output, "%zu\t%3.6f\n", j, event_data[j]);
          }
          seq_tensor_free(event_signal);
        }
      } else {
        // SQUIGGLE MODE (default): Output the three squiggle features
        fprintf(arguments.output, "pos\tbase\tcurrent\tsd\tdwell\n");
        float *data = seq_tensor_data_float(squiggle);
        size_t num_positions = seq_tensor_dim(squiggle, 0);

        for (size_t j = 0; j < num_positions; j++) {
          float current = data[j * 3 + 0];
          float stddev = data[j * 3 + 1];
          float dwell_time = data[j * 3 + 2];

          fprintf(arguments.output, "%zu\t%c\t%3.6f\t%3.6f\t%3.6f\n",
                  j,
                  seq->seq.s[j],
                  current,
                  stddev,
                  dwell_time);

          // Accumulate dwell time statistics
          total_dwell_time += dwell_time;
          total_positions++;
        }
      }

      seq_tensor_free(squiggle);
    }

    // Clean up sequence structure (only for synthetic sequences)
    if (is_synthetic) {
      kseq_destroy_synthetic(seq);
    }
  } // end for loop

  // ========================================================================
  // STEP 5: CLEANUP AND FINALIZATION (common to both modes)
  // ========================================================================

  // Clean up file-based mode resources
  if (!arguments.generate_sequences && file_parsers != NULL) {
    for (int i = 0; i < nfile; i++) {
      if (file_parsers[i] != NULL) {
        kseq_destroy(file_parsers[i]);
      }
      if (file_handles[i] != NULL) {
        fclose(file_handles[i]);
      }
    }
    free(file_parsers);
    free(file_handles);
  }

  // Print average dwell time statistics if we processed sequences in SQUIGGLE mode
  if (!arguments.generate_raw && !arguments.generate_event) {
    if (total_positions > 0) {
      float average_dwell_time = total_dwell_time / total_positions;
      size_t total_samples = (size_t)ceil(average_dwell_time * total_positions);
      printf("Average dwell time: %.6f (across %zu positions and %zu samples)\n",
             average_dwell_time, total_positions, total_samples);
    } else {
      printf("No sequences processed - no dwell time data available\n");
    }
  }

  // Report reference file writing if enabled
  if (arguments.reference_file != NULL) {
    fflush(arguments.reference_file);
    if (arguments.generate_sequences) {
      printf("Wrote %d generated sequences to reference file: %s\n",
             reads_started, arguments.reference_filename);
    } else {
      printf("Wrote %d sequences from input files to reference file: %s\n",
             reads_started, arguments.reference_filename);
    }
  }

  // Write Fast5 file if requested
  if (arguments.output_fast5 && fast5_read_count > 0) {
    if (fast5_read_count == 1) {
      seq_write_fast5_single(arguments.output_filename,
                            fast5_raw_signals,
                            fast5_read_names,
                            fast5_read_count,
                            arguments.sample_rate_khz);
    } else {
      seq_write_fast5_multi(arguments.output_filename,
                           fast5_raw_signals,
                           fast5_read_names,
                           fast5_read_count,
                           arguments.sample_rate_khz);
    }

    printf("Wrote %d reads to Fast5 file: %s\n",
           fast5_read_count, arguments.output_filename);

    // Clean up Fast5 data
    for (int i = 0; i < fast5_read_count; i++) {
      if (fast5_raw_signals[i]) {
        seq_tensor_free(fast5_raw_signals[i]);
      }
      if (fast5_read_names[i]) {
        free((void*)fast5_read_names[i]);
      }
    }
    free(fast5_raw_signals);
    free(fast5_read_names);
  }

  // Close reference file if it was opened
  if (arguments.reference_file != NULL) {
    fclose(arguments.reference_file);
    printf("Closed reference file: %s\n", arguments.reference_filename);
  }

  // Close text output file if we opened it for --save-text
  if (arguments.save_text && arguments.output != stdout) {
    fclose(arguments.output);
  }

  return EXIT_SUCCESS;
}
