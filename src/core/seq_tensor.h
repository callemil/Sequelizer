// **********************************************************************
// seq_tensor.h - N-Dimensional Tensor for Signal Processing
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 10 2025
//
// Unified tensor structure supporting:
// - N-dimensional arrays (matrices are 2D tensors)
// - Multiple data types (int8, int32, float32)
// - Quantization metadata for ML inference
// - SIMD-aligned memory allocation
//
// Design Philosophy:
// Provides a single abstraction for both simple 2D signal matrices
// and complex N-dimensional neural network tensors. Starts simple
// but scales to support quantized inference and hardware acceleration.

#ifndef SEQUELIZER_SEQ_TENSOR_H
#define SEQUELIZER_SEQ_TENSOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// **********************************************************************
// Data Type Enumeration
// **********************************************************************
typedef enum {
    SEQ_TENSOR_INT8,      // 8-bit signed integer (quantized inference)
    SEQ_TENSOR_INT32,     // 32-bit signed integer (accumulators)
    SEQ_TENSOR_FLT32      // 32-bit floating point (standard)
} seq_tensor_dtype;

// **********************************************************************
// Core Tensor Structure
// **********************************************************************
typedef struct seq_tensor {
    // Dimensionality
    size_t ndim;            // num of dim (2 for matrix, 3+ for tensors)
    size_t *shape;          // dim sizes [malloc'd]   (2D [C,R] C: col size;           R: row size)
    size_t *stride;         // mem strides [malloc'd] (2D [c,r] c: strd to nxt C elem; r: strd to nxt R elem)
    size_t size;            // total number of elements

    // Data type information
    seq_tensor_dtype dtype; // elem data type flag (let's you know what data type was set)
    size_t element_size;    // sizeof(element) in bytes

    // Quantization parameters (only used for INT8/INT32 types)
    float scale;            // Quantization scale factor
    int32_t zero_point;     // Quantization zero point

    // Data storage
    void *data;             // Pointer to actual data (malloc'd)

    // Memory metadata
    uint32_t flags;         // Memory properties (alignment, ownership)
} seq_tensor;

// **********************************************************************
// Memory Layout Flags
// **********************************************************************
#define SEQ_TENSOR_OWNS_DATA  0x8000   // Tensor owns data pointer
#define SEQ_TENSOR_ALIGNED_16 0x1000   // Data is 16-byte aligned (SIMD)

// **********************************************************************
// Construction Functions
// **********************************************************************

/**
 * Create a float32 tensor with specified shape
 *
 * @param ndim Number of dimensions
 * @param shape Array of dimension sizes (e.g., [batch, channels, length])
 * @return Newly allocated tensor or NULL on failure
 *
 * Example: seq_tensor_create_float(2, (size_t[]){10, 5}) creates 10x5 matrix
 */
seq_tensor* seq_tensor_create_float(size_t ndim, const size_t *shape);

/**
 * Create an int8 quantized tensor
 *
 * @param ndim Number of dimensions
 * @param shape Array of dimension sizes
 * @param scale Quantization scale (real_value = scale * (quantized - zero_point))
 * @param zero_point Quantization zero point
 * @return Newly allocated tensor or NULL on failure
 */
seq_tensor* seq_tensor_create_int8(size_t ndim, const size_t *shape,
                                   float scale, int32_t zero_point);

/**
 * Create an int32 accumulator tensor
 *
 * @param ndim Number of dimensions
 * @param shape Array of dimension sizes
 * @param scale Quantization scale
 * @param zero_point Quantization zero point
 * @return Newly allocated tensor or NULL on failure
 */
seq_tensor* seq_tensor_create_int32(size_t ndim, const size_t *shape,
                                    float scale, int32_t zero_point);

/**
 * Convenience: Create 2D float matrix with column-major layout
 *
 * Column-major: data stored column-by-column
 * Element at (row, col) is at: data[col * stride[0] + row]
 */
seq_tensor* seq_tensor_create_2d_float_cm(size_t rows, size_t cols);

/**
 * Convenience: Create 2D float matrix with row-major layout
 *
 * Row-major: data stored row-by-row
 * Element at (row, col) is at: data[row * stride[0] + col]
 */
seq_tensor* seq_tensor_create_2d_float_rm(size_t rows, size_t cols);

/**
 * Free tensor and all associated memory
 */
void seq_tensor_free(seq_tensor *tensor);

// **********************************************************************
// Data Access Functions
// **********************************************************************

/**
 * Get typed pointer to float32 data
 * Asserts that tensor dtype is FLT32 in debug builds
 */
float* seq_tensor_data_float(seq_tensor *t);

/**
 * Get typed pointer to int8 data
 * Asserts that tensor dtype is INT8 in debug builds
 */
int8_t* seq_tensor_data_int8(seq_tensor *t);

/**
 * Get typed pointer to int32 data
 * Asserts that tensor dtype is INT32 in debug builds
 */
int32_t* seq_tensor_data_int32(seq_tensor *t);

// **********************************************************************
// Dimension Query Functions
// **********************************************************************

/**
 * Get size of specific dimension
 * @param axis Dimension index (0-indexed)
 */
size_t seq_tensor_dim(const seq_tensor *t, size_t axis);

/**
 * Get total number of elements
 */
size_t seq_tensor_total_size(const seq_tensor *t);

// **********************************************************************
// Debug/Display Functions
// **********************************************************************

/**
 * Print tensor shape and first few elements
 * Useful for debugging
 */
void seq_tensor_print(const seq_tensor *t);

#endif // SEQUELIZER_SEQ_TENSOR_H
