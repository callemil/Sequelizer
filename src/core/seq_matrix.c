// **********************************************************************
// core/seq_matrix.c - Matrix Implementation for Signal Processing
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 9 2025

#include "seq_matrix.h"

// **********************************************************************
// Matrix Creation Functions
// **********************************************************************

/**
 * seq_matrix_create: Create matrix with default column-major layout
 *
 * Column-major is the default because it's optimal for:
 * - BLAS/LAPACK compatibility
 * - Column-wise operations common in signal processing
 * - Vectorized operations on signal chunks
 */
seq_matrix* seq_matrix_create(size_t nr, size_t nc) {
  return seq_matrix_create_cm(nr, nc);
}

/**
 * seq_matrix_create_cm: Create column-major matrix
 *
 * Column-major layout: data stored column-by-column in memory
 * Memory pattern: [col0_row0, col0_row1, ..., col0_rowN, col1_row0, ...]
 * Access: data[col * stride + row] where stride = nrq
 *
 * SIMD optimization: nrq is rounded up to multiple of 4, allowing
 * efficient vectorized operations on columns without alignment issues.
 */
seq_matrix* seq_matrix_create_cm(size_t nr, size_t nc) {
  // Validate dimensions
  if (nr == 0 || nc == 0) {
    return NULL;
  }

  // Allocate matrix structure
  seq_matrix *mat = (seq_matrix*)calloc(1, sizeof(seq_matrix));
  if (mat == NULL) {
    return NULL;
  }

  // Set dimensions
  mat->nr = nr;
  mat->nc = nc;

  // Quantize rows to next multiple of 4 for SIMD alignment
  // Example: nr=10 -> nrq=12, nr=8 -> nrq=8, nr=1 -> nrq=4
  mat->nrq = ((nr + 3) / 4) * 4;

  // Column-major: stride equals quantized row count
  mat->stride = mat->nrq;

  // Allocate data array with SIMD-aligned dimensions
  // Total size: nrq * nc elements (includes padding rows)
  mat->data.f = (float*)calloc(mat->nrq * nc, sizeof(float));
  if (mat->data.f == NULL) {
    free(mat);
    return NULL;
  }

  return mat;
}

/**
 * seq_matrix_create_rm: Create row-major matrix
 *
 * Row-major layout: data stored row-by-row in memory
 * Memory pattern: [row0_col0, row0_col1, ..., row0_colN, row1_col0, ...]
 * Access: data[row * stride + col] where stride = nc
 *
 * Note: Row-major doesn't use nrq padding in allocation since
 * SIMD operations typically work across rows, not down columns.
 * However, nrq is still calculated for consistency.
 */
seq_matrix* seq_matrix_create_rm(size_t nr, size_t nc) {
  // Validate dimensions
  if (nr == 0 || nc == 0) {
    return NULL;
  }

  // Allocate matrix structure
  seq_matrix *mat = (seq_matrix*)calloc(1, sizeof(seq_matrix));
  if (mat == NULL) {
    return NULL;
  }

  // Set dimensions
  mat->nr = nr;
  mat->nc = nc;

  // Calculate quantized rows (for consistency, even if not used in allocation)
  mat->nrq = ((nr + 3) / 4) * 4;

  // Row-major: stride equals column count
  mat->stride = nc;

  // Allocate data array (row-major uses actual dimensions, no padding)
  mat->data.f = (float*)calloc(nr * nc, sizeof(float));
  if (mat->data.f == NULL) {
    free(mat);
    return NULL;
  }

  return mat;
}

// **********************************************************************
// Matrix Cleanup
// **********************************************************************

/**
 * seq_matrix_free: Safely deallocate matrix and its data
 *
 * Handles NULL pointers gracefully. Frees data array first, then
 * the matrix structure itself.
 */
void seq_matrix_free(seq_matrix *mat) {
  if (mat == NULL) {
    return;
  }

  // Free data array if it exists
  if (mat->data.f != NULL) {
    free(mat->data.f);
  }

  // Free matrix structure
  free(mat);
}
