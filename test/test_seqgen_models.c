// **********************************************************************
// test_seqgen_models.c - Test signal generation model dispatcher
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 14 2025

#include "../src/core/seqgen_models.h"
#include "../src/core/seq_tensor.h"
#include "../src/core/seq_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  int tests_passed = 0;
  int tests_failed = 0;

  printf("Testing signal generation model dispatcher...\n\n");

  // Test 1: Model string conversion
  printf("Test 1: Model string to enum conversion...\n");
  seqgen_model_type type_kmer = get_seqgen_model("squiggle_kmer");
  seqgen_model_type type_r94 = get_seqgen_model("squiggle_r94");
  seqgen_model_type type_invalid = get_seqgen_model("invalid_model");

  if (type_kmer != SEQGEN_MODEL_KMER) {
    printf("✗ Failed to get KMER model type\n");
    tests_failed++;
  } else if (type_r94 != SEQGEN_MODEL_R9_4) {
    printf("✗ Failed to get R9.4 model type\n");
    tests_failed++;
  } else if (type_invalid != SEQGEN_MODEL_INVALID) {
    printf("✗ Invalid model should return SEQGEN_MODEL_INVALID\n");
    tests_failed++;
  } else {
    printf("✓ Model string conversion works correctly\n");
    tests_passed++;
  }
  printf("\n");

  // Test 2: Function dispatcher
  printf("Test 2: Function dispatcher...\n");
  seqgen_func_ptr func = get_seqgen_func(SEQGEN_MODEL_KMER);
  if (!func) {
    printf("✗ Failed to get function pointer for KMER model\n");
    tests_failed++;
  } else {
    printf("✓ Got function pointer for KMER model\n");
    tests_passed++;
  }
  printf("\n");

  // Test 3: K-mer model with 5-mer
  printf("Test 3: K-mer model (5-mer)...\n");
  const char *seq = "ACGTACGTACGT";

  // Encode as individual bases (A=0, C=1, G=2, T=3)
  int *encoded = calloc(strlen(seq), sizeof(int));
  for (size_t i = 0; i < strlen(seq); i++) {
    encoded[i] = base_to_int(seq[i], true);
  }

  struct seqgen_model_params params = {
    .model_type = SEQGEN_MODEL_KMER,
    .params.kmer = {
      .model_name = "rna_r9.4_180mv_70bps",
      .models_dir = "kmer_models",
      .kmer_size = 5,
      .sample_rate_khz = 4.0f
    }
  };

  seq_tensor *squiggle = squiggle_kmer(encoded, strlen(seq), false, &params);
  if (!squiggle) {
    printf("✗ Failed to generate squiggle with 5-mer model\n");
    tests_failed++;
  } else {
    size_t expected = strlen(seq) - 5 + 1;
    if (squiggle->shape[0] != expected || squiggle->shape[1] != 3) {
      printf("✗ Wrong shape: [%zu, %zu] (expected [%zu, 3])\n",
             squiggle->shape[0], squiggle->shape[1], expected);
      tests_failed++;
    } else {
      printf("✓ Generated 5-mer squiggle: [%zu, 3]\n", expected);
      tests_passed++;
    }
    seq_tensor_free(squiggle);
  }
  free(encoded);
  printf("\n");

  // Test 4: K-mer decimation (9-mer model, request 5-mer)
  printf("Test 4: K-mer decimation (9-mer → 5-mer)...\n");
  const char *long_seq = "ACGTACGTACGTACGTACGTACGT";

  int *encoded_long = calloc(strlen(long_seq), sizeof(int));
  for (size_t i = 0; i < strlen(long_seq); i++) {
    encoded_long[i] = base_to_int(long_seq[i], true);
  }

  struct seqgen_model_params params_dec = {
    .model_type = SEQGEN_MODEL_KMER,
    .params.kmer = {
      .model_name = "dna_r10.4.1_e8.2_260bps",  // 9-mer model
      .models_dir = "kmer_models",
      .kmer_size = 5,  // Request 5-mer (should decimate)
      .sample_rate_khz = 4.0f
    }
  };

  seq_tensor *squiggle_dec = squiggle_kmer(encoded_long, strlen(long_seq), false, &params_dec);
  if (!squiggle_dec) {
    printf("✗ Failed to generate decimated squiggle\n");
    tests_failed++;
  } else {
    size_t expected_dec = strlen(long_seq) - 5 + 1;
    if (squiggle_dec->shape[0] != expected_dec) {
      printf("✗ Wrong decimated shape: [%zu] (expected [%zu])\n",
             squiggle_dec->shape[0], expected_dec);
      tests_failed++;
    } else {
      printf("✓ Generated decimated squiggle: [%zu, 3]\n", expected_dec);
      printf("✓ Decimation: 9-mer model → 5-mer output\n");
      tests_passed++;
    }
    seq_tensor_free(squiggle_dec);
  }
  free(encoded_long);
  printf("\n");

  // Test 5: Sample rate scaling
  printf("Test 5: Sample rate scaling...\n");
  const char *test_seq = "ACGTACGTACGTACGT";

  int *encoded_test = calloc(strlen(test_seq), sizeof(int));
  for (size_t i = 0; i < strlen(test_seq); i++) {
    encoded_test[i] = base_to_int(test_seq[i], true);
  }

  struct seqgen_model_params params_8k = {
    .model_type = SEQGEN_MODEL_KMER,
    .params.kmer = {
      .model_name = "rna_r9.4_180mv_70bps",
      .models_dir = "kmer_models",
      .kmer_size = 5,
      .sample_rate_khz = 8.0f  // 2x sample rate
    }
  };

  seq_tensor *squiggle_8k = squiggle_kmer(encoded_test, strlen(test_seq), false, &params_8k);
  if (!squiggle_8k) {
    printf("✗ Failed to generate 8kHz squiggle\n");
    tests_failed++;
  } else {
    float *data = seq_tensor_data_float(squiggle_8k);
    float dwell_4k = 10.0f * (4.0f / 4.0f);  // 10.0
    float dwell_8k = 10.0f * (8.0f / 4.0f);  // 20.0

    if (data[2] == dwell_8k) {
      printf("✓ Sample rate scaling works: dwell at 8kHz = %.1f (vs %.1f at 4kHz)\n",
             dwell_8k, dwell_4k);
      tests_passed++;
    } else {
      printf("✗ Dwell time incorrect: %.1f (expected %.1f)\n", data[2], dwell_8k);
      tests_failed++;
    }
    seq_tensor_free(squiggle_8k);
  }
  free(encoded_test);
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
