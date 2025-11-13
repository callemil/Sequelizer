// **********************************************************************
// test_kmer_loader.c - Test k-mer model loader
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 12 2025

#include "../src/core/kmer_model_loader.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int tests_passed = 0;
    int tests_failed = 0;

    printf("Testing k-mer model loader...\n\n");

    // Test 1: Modern 5-mer model
    printf("Test 1: Modern 5-mer model (rna_r9.4_180mv_70bps)...\n");
    kmer_model_t *model1 = load_kmer_model("kmer_models", "rna_r9.4_180mv_70bps");
    if (!model1) {
        printf("✗ Failed to load model\n");
        tests_failed++;
    } else {
        if (model1->kmer_size != 5 || model1->num_kmers != 1024) {
            printf("✗ Wrong size: got %d-mer, %zu kmers (expected 5-mer, 1024 kmers)\n",
                   model1->kmer_size, model1->num_kmers);
            tests_failed++;
        } else if (!model1->level_mean || model1->level_stddev != NULL ||
                   model1->default_stddev != 1.5f) {
            printf("✗ Wrong modern format structure\n");
            tests_failed++;
        } else {
            printf("✓ Loaded %s: %d-mer, %zu kmers\n",
                   model1->name, model1->kmer_size, model1->num_kmers);
            printf("✓ First 3 means: %.4f, %.4f, %.4f\n",
                   model1->level_mean[0], model1->level_mean[1], model1->level_mean[2]);
            printf("✓ default_stddev: %.1f\n", model1->default_stddev);
            tests_passed++;
        }
        free_kmer_model(model1);
    }
    printf("\n");

    // Test 2: Modern 9-mer model
    printf("Test 2: Modern 9-mer model (dna_r10.4.1_e8.2_260bps)...\n");
    kmer_model_t *model2 = load_kmer_model("kmer_models", "dna_r10.4.1_e8.2_260bps");
    if (!model2) {
        printf("✗ Failed to load model\n");
        tests_failed++;
    } else {
        if (model2->kmer_size != 9 || model2->num_kmers != 262144) {
            printf("✗ Wrong size: got %d-mer, %zu kmers (expected 9-mer, 262144 kmers)\n",
                   model2->kmer_size, model2->num_kmers);
            tests_failed++;
        } else {
            printf("✓ Loaded %s: %d-mer, %zu kmers\n",
                   model2->name, model2->kmer_size, model2->num_kmers);
            printf("✓ First 3 means: %.4f, %.4f, %.4f\n",
                   model2->level_mean[0], model2->level_mean[1], model2->level_mean[2]);
            tests_passed++;
        }
        free_kmer_model(model2);
    }
    printf("\n");

    // Test 3: Legacy 6-mer model
    printf("Test 3: Legacy 6-mer model (legacy_r9.4_180mv_450bps_6mer)...\n");
    kmer_model_t *model3 = load_kmer_model("kmer_models",
                                            "legacy/legacy_r9.4_180mv_450bps_6mer");
    if (!model3) {
        printf("✗ Failed to load model\n");
        tests_failed++;
    } else {
        if (model3->kmer_size != 6 || model3->num_kmers != 4096) {
            printf("✗ Wrong size: got %d-mer, %zu kmers (expected 6-mer, 4096 kmers)\n",
                   model3->kmer_size, model3->num_kmers);
            tests_failed++;
        } else if (!model3->level_mean || !model3->level_stddev) {
            printf("✗ Missing level_mean or level_stddev arrays\n");
            tests_failed++;
        } else {
            printf("✓ Loaded %s: %d-mer, %zu kmers\n",
                   model3->name, model3->kmer_size, model3->num_kmers);
            printf("✓ First 3 means: %.2f, %.2f, %.2f\n",
                   model3->level_mean[0], model3->level_mean[1], model3->level_mean[2]);
            printf("✓ First 3 stddevs: %.2f, %.2f, %.2f\n",
                   model3->level_stddev[0], model3->level_stddev[1], model3->level_stddev[2]);
            tests_passed++;
        }
        free_kmer_model(model3);
    }
    printf("\n");

    // Test 4: Error handling - non-existent model
    printf("Test 4: Error handling (non-existent model)...\n");
    kmer_model_t *model4 = load_kmer_model("kmer_models", "fake_model_does_not_exist");
    if (model4 != NULL) {
        printf("✗ Should have returned NULL for non-existent model\n");
        free_kmer_model(model4);
        tests_failed++;
    } else {
        printf("✓ Non-existent model correctly returned NULL\n");
        tests_passed++;
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
