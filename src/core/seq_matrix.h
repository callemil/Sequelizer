// **********************************************************************
// core/seq_matrix.h - Foundational Matrix Type for Signal Processing
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 9 2025
//
// Defines seq_matrix: a flexible 2D array structure optimized for
// sequence signal processing and numerical computation.
//
// Key Features:
// - SIMD-friendly memory alignment (rows quantized to multiples of 4)
// - Support for both row-major and column-major layouts
// - Union type for float or integer data
// - Efficient stride-based memory access patterns
//
// Design Philosophy:
// This is a lightweight, performance-oriented matrix type designed for
// nanopore signal processing where vectorized operations are critical.
// The nrq (quantized rows) field ensures memory alignment for SIMD
// instructions while nr tracks the logical matrix dimensions.

#ifndef SEQUELIZER_SEQ_MATRIX_H
#define SEQUELIZER_SEQ_MATRIX_H

#include <stdlib.h>
#include <stdbool.h>

// **********************************************************************
// Matrix Data Structure
// **********************************************************************

/**
 * seq_matrix: 2D matrix structure for sequence signal processing
 *
 * Memory Layout:
 * - Column-major (default): data stored column-by-column, stride = nrq
 *   Access: data[col * stride + row]
 * - Row-major: data stored row-by-row, stride = nc
 *   Access: data[row * stride + col]
 *
 * SIMD Alignment:
 * - nr: logical number of rows (actual matrix dimension)
 * - nrq: rows quantized to multiple of 4 for SIMD vectorization
 *   Example: if nr=10, then nrq=12 (rounded up to nearest multiple of 4)
 *   This ensures aligned memory access for SSE/AVX instructions
 *
 * Data Storage:
 * - Union allows reinterpretation as float or int arrays
 * - Total allocation: nrq * nc * sizeof(type)
 * - Padding rows (nrq - nr) are allocated but not logically used
 */
typedef struct {
  size_t nr;      // Number of rows (logical dimension)
  size_t nrq;     // Number of rows quantized for SIMD (multiple of 4)
  size_t nc;      // Number of columns
  size_t stride;  // Memory stride: nrq (col-major) or nc (row-major)

  union {
    float *f;     // Float data pointer (for signal values, probabilities)
    int *iv;      // Integer data pointer (for discrete states, indices)
  } data;
} seq_matrix;

// **********************************************************************
// Function Prototypes
// **********************************************************************

/**
 * seq_matrix_create: Create matrix with default column-major layout
 *
 * @param nr  Number of rows (logical dimension)
 * @param nc  Number of columns
 * @return    Pointer to newly allocated seq_matrix, or NULL on failure
 *
 * Note: Rows are automatically quantized to next multiple of 4 for SIMD
 */
seq_matrix* seq_matrix_create(size_t nr, size_t nc);

/**
 * seq_matrix_create_rm: Create matrix with row-major layout
 *
 * @param nr  Number of rows
 * @param nc  Number of columns
 * @return    Pointer to newly allocated seq_matrix, or NULL on failure
 *
 * Row-major layout: consecutive memory locations store row elements
 * Access pattern: data[row * stride + col] where stride = nc
 */
seq_matrix* seq_matrix_create_rm(size_t nr, size_t nc);

/**
 * seq_matrix_create_cm: Create matrix with column-major layout
 *
 * @param nr  Number of rows
 * @param nc  Number of columns
 * @return    Pointer to newly allocated seq_matrix, or NULL on failure
 *
 * Column-major layout: consecutive memory locations store column elements
 * Access pattern: data[col * stride + row] where stride = nrq
 */
seq_matrix* seq_matrix_create_cm(size_t nr, size_t nc);

/**
 * seq_matrix_free: Deallocate matrix and its data
 *
 * @param mat  Pointer to seq_matrix to free
 *
 * Safely handles NULL pointers. Frees both the data array and the
 * seq_matrix structure itself.
 */
void seq_matrix_free(seq_matrix *mat);

#endif // SEQUELIZER_SEQ_MATRIX_H
