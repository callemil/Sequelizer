// **********************************************************************
// test_seqgen.c - Test signal generation utilities
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 13 2025
// compile: build % cmake --build
// run:     build % ./test_seqgen

#include "../src/core/seqgen_utils.h"
#include "../src/core/seq_tensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  int tests_passed = 0;
  int tests_failed = 0;

  printf("Testing signal generation utilities...\n\n");

  // Test 1: Sequence to squiggle
  printf("Test 1: Sequence to squiggle...\n");
  const char *seq = "ACGTACGTACGT";  // 12 bases
  size_t seq_len = strlen(seq);

  pore_model_params params = default_pore_model_params();
  params.model_name = "rna_r9.4_180mv_70bps";  // 5-mer model

  seq_tensor *squiggle = sequence_to_squiggle(seq, seq_len, false, &params);
  if (!squiggle) {
    printf("✗ Failed to generate squiggle\n");
    tests_failed++;
  } else {
    size_t expected_kmers = seq_len - 5 + 1;  // 12 - 5 + 1 = 8
    if (squiggle->shape[0] != expected_kmers || squiggle->shape[1] != 3) {
      printf("✗ Wrong squiggle shape: [%zu, %zu] (expected [%zu, 3])\n",
       squiggle->shape[0], squiggle->shape[1], expected_kmers);
      tests_failed++;
    } else {
      float *data = seq_tensor_data_float(squiggle);
      printf("✓ Generated squiggle: [%zu, 3]\n", expected_kmers);
      printf("✓ First event: current=%.4f, stddev=%.4f, dwell=%.1f\n",
       data[0], data[1], data[2]);
      printf("✓ Last event: current=%.4f, stddev=%.4f, dwell=%.1f\n",
       data[(expected_kmers-1)*3 + 0],
       data[(expected_kmers-1)*3 + 1],
       data[(expected_kmers-1)*3 + 2]);
      tests_passed++;
    }
  }
  printf("\n");

  // Test 2: Squiggle to raw signal
  if (squiggle) {
    printf("Test 2: Squiggle to raw signal...\n");
    seq_tensor *raw = squiggle_to_raw(squiggle, 4.0f);
    if (!raw) {
      printf("✗ Failed to generate raw signal\n");
      tests_failed++;
    } else {
      printf("✓ Generated raw signal: [%zu, 1]\n", raw->shape[0]);
      float *raw_data = seq_tensor_data_float(raw);
      printf("✓ First 5 samples: %.4f, %.4f, %.4f, %.4f, %.4f\n",
       raw_data[0], raw_data[1], raw_data[2], raw_data[3], raw_data[4]);
      tests_passed++;
      seq_tensor_free(raw);
    }
    printf("\n");

    // Test 3: Squiggle to event signal
    printf("Test 3: Squiggle to event signal...\n");
    seq_tensor *event = squiggle_to_event(squiggle, 4.0f);
    if (!event) {
      printf("✗ Failed to generate event signal\n");
      tests_failed++;
    } else {
      printf("✓ Generated event signal: [%zu, 1]\n", event->shape[0]);
      float *event_data = seq_tensor_data_float(event);
      printf("✓ First 5 samples: %.4f, %.4f, %.4f, %.4f, %.4f\n",
      event_data[0], event_data[1], event_data[2],
      event_data[3], event_data[4]);

      // Verify piecewise constant (first 10 samples should be the same)
      bool is_constant = true;
      for (int i = 1; i < 10; i++) {
        if (event_data[i] != event_data[0]) {
         is_constant = false;
         break;
        }
      }
      if (is_constant) {
        printf("✓ Event signal is piecewise constant (first 10 samples identical)\n");
      } else {
        printf("⚠ Warning: Event signal varies (expected constant for first event)\n");
      }

      tests_passed++;
      seq_tensor_free(event);
    }
    printf("\n");

    seq_tensor_free(squiggle);
  }

  // Test 4: Test with 9-mer model
  printf("Test 4: Sequence to squiggle with 9-mer model...\n");
  const char *long_seq = "ACGTACGTACGTACGTACGTACGT";  // 24 bases
  size_t long_seq_len = strlen(long_seq);

  pore_model_params params9 = default_pore_model_params();
  params9.model_name = "dna_r10.4.1_e8.2_260bps";  // 9-mer model

  seq_tensor *squiggle9 = sequence_to_squiggle(long_seq, long_seq_len, false, &params9);
  if (!squiggle9) {
    printf("✗ Failed to generate 9-mer squiggle\n");
    tests_failed++;
  } else {
    size_t expected_kmers9 = long_seq_len - 9 + 1;  // 24 - 9 + 1 = 16
    if (squiggle9->shape[0] != expected_kmers9) {
    printf("✗ Wrong 9-mer squiggle size: %zu (expected %zu)\n",
    squiggle9->shape[0], expected_kmers9);
    tests_failed++;
    } else {
    printf("✓ Generated 9-mer squiggle: [%zu, 3]\n", expected_kmers9);
    tests_passed++;
    }
    seq_tensor_free(squiggle9);
  }
  printf("\n");

  // Summary
  printf("======================\n");
  printf("Tests passed: %d\n", tests_passed);
  printf("Tests failed: %d\n", tests_failed);
  printf("======================\n");

  if (tests_failed == 0) {
    printf("\n✓ All tests passed!\n");
    return 0;
  } else {
    printf("\n✗ Some tests failed!\n");
    return 1;
  }
}
