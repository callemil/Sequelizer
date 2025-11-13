// **********************************************************************
// kmer_model_loader.c - Load Oxford Nanopore k-mer models
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 12 2025

#include "kmer_model_loader.h"
#include "../../include/sequelizer.h"  // for RETURN_NULL_IF, warnx
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>     // for pow()
#include <stdbool.h>

kmer_model_t* load_kmer_model(const char *models_dir, const char *model_name) {
    // Try each filename in order
    const char *filenames[] = {
        "9mer_levels_v1.txt",
        "5mer_levels_v1.txt",
        "template_median68pA.model",
        "template_median69pA.model",
        NULL
    };

    char filepath[1024];
    FILE *fp = NULL;

    for (int i = 0; filenames[i] != NULL; i++) {
        snprintf(filepath, sizeof(filepath), "%s/%s/%s",
                 models_dir, model_name, filenames[i]);
        fp = fopen(filepath, "r");
        if (fp) break;
    }

    if (!fp) {
        warnx("Could not find k-mer model in %s/%s", models_dir, model_name);
        return NULL;
    }

    // Allocate struct
    kmer_model_t *model = calloc(1, sizeof(kmer_model_t));
    if (!model) {
        fclose(fp);
        return NULL;
    }

    model->name = strdup(model_name);
    if (!model->name) {
        free(model);
        fclose(fp);
        return NULL;
    }

    // Detect format by checking first line
    char line[1024];
    if (!fgets(line, sizeof(line), fp)) {
        warnx("Empty k-mer model file");
        free_kmer_model(model);
        fclose(fp);
        return NULL;
    }

    bool is_legacy = (strncmp(line, "kmer", 4) == 0);

    // Read first k-mer to determine size
    char first_kmer[32];
    if (is_legacy) {
        // Skip header, read first data line
        if (!fgets(line, sizeof(line), fp)) {
            warnx("No data after header in k-mer model");
            free_kmer_model(model);
            fclose(fp);
            return NULL;
        }
        sscanf(line, "%s", first_kmer);
    } else {
        // First line is data
        sscanf(line, "%s", first_kmer);
    }

    int kmer_size = strlen(first_kmer);
    size_t num_kmers = (size_t)pow(4, kmer_size);

    model->kmer_size = kmer_size;
    model->num_kmers = num_kmers;

    // Rewind to start of data
    rewind(fp);
    if (is_legacy) {
        fgets(line, sizeof(line), fp); // Skip header again
    }

    // Parse based on format
    if (!is_legacy) {
        // Modern format: kmer level_mean
        model->level_mean = calloc(num_kmers, sizeof(float));
        if (!model->level_mean) {
            free_kmer_model(model);
            fclose(fp);
            return NULL;
        }

        model->level_stddev = NULL;
        model->default_stddev = 1.5f;
        model->sd_mean = NULL;
        model->sd_stdv = NULL;
        model->ig_lambda = NULL;
        model->weight = NULL;

        // Parse each line
        char kmer_str[32];
        size_t i = 0;
        while (fgets(line, sizeof(line), fp) && i < num_kmers) {
            if (sscanf(line, "%s %f", kmer_str, &model->level_mean[i]) != 2) {
                warnx("Parse error at line %zu", i + 1);
                free_kmer_model(model);
                fclose(fp);
                return NULL;
            }
            i++;
        }

        if (i != num_kmers) {
            warnx("Expected %zu k-mers, found %zu", num_kmers, i);
            free_kmer_model(model);
            fclose(fp);
            return NULL;
        }

    } else {
        // Legacy format: kmer level_mean level_stdv [sd_mean sd_stdv ig_lambda weight]
        model->level_mean = calloc(num_kmers, sizeof(float));
        model->level_stddev = calloc(num_kmers, sizeof(float));
        if (!model->level_mean || !model->level_stddev) {
            free_kmer_model(model);
            fclose(fp);
            return NULL;
        }

        model->default_stddev = 0.0f;  // Not used since we have level_stddev

        // Allocate optional arrays
        model->sd_mean = calloc(num_kmers, sizeof(float));
        model->sd_stdv = calloc(num_kmers, sizeof(float));
        model->ig_lambda = calloc(num_kmers, sizeof(float));
        model->weight = calloc(num_kmers, sizeof(float));

        // Parse each line
        char kmer_str[32];
        size_t i = 0;
        while (fgets(line, sizeof(line), fp) && i < num_kmers) {
            // Try full 7-column parse
            int n = sscanf(line, "%s %f %f %f %f %f %f",
                           kmer_str,
                           &model->level_mean[i],
                           &model->level_stddev[i],
                           &model->sd_mean[i],
                           &model->sd_stdv[i],
                           &model->ig_lambda[i],
                           &model->weight[i]);

            if (n < 3) {  // Minimum: kmer, level_mean, level_stddev
                warnx("Parse error at legacy line %zu (got %d columns)", i + 1, n);
                free_kmer_model(model);
                fclose(fp);
                return NULL;
            }
            i++;
        }

        if (i != num_kmers) {
            warnx("Expected %zu k-mers, found %zu", num_kmers, i);
            free_kmer_model(model);
            fclose(fp);
            return NULL;
        }
    }

    fclose(fp);
    return model;
}

void free_kmer_model(kmer_model_t *model) {
    if (!model) return;

    free(model->name);
    free(model->level_mean);
    free(model->level_stddev);
    free(model->sd_mean);
    free(model->sd_stdv);
    free(model->ig_lambda);
    free(model->weight);
    free(model);
}
