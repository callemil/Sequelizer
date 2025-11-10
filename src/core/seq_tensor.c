// **********************************************************************
// seq_tensor.c - N-Dimensional Tensor Implementation
// **********************************************************************
// Sebastian Claudiusz Magierowski Nov 10 2025

#include "seq_tensor.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// **********************************************************************
// Internal Helper Functions
// **********************************************************************

/**
 * Calculate C-order strides from shape
 * C-order (row-major): stride[i] = product of shape[i+1] to shape[ndim-1]
 * Example: shape=[2,3,4] → stride=[12,4,1]
 */
static void calculate_c_strides(size_t ndim, const size_t *shape, size_t *stride) {
    if (ndim == 0) return;

    // Last dimension has stride 1
    stride[ndim - 1] = 1;

    // Work backwards, each stride is the product of subsequent dimensions
    for (int i = (int)ndim - 2; i >= 0; i--) {
        stride[i] = stride[i + 1] * shape[i + 1];
    }
}

/**
 * Calculate total number of elements from shape
 */
static size_t calculate_total_size(size_t ndim, const size_t *shape) {
    if (ndim == 0) return 0;

    size_t total = 1;
    for (size_t i = 0; i < ndim; i++) {
        total *= shape[i];
    }
    return total;
}

/**
 * Allocate aligned memory for tensor data
 * Returns NULL on failure
 */
static void* allocate_aligned_data(size_t num_elements, size_t element_size) {
    size_t total_bytes = num_elements * element_size;
    if (total_bytes == 0) return NULL;

    void *ptr = NULL;

#ifdef _POSIX_C_SOURCE
    // Try 16-byte aligned allocation for SIMD
    if (posix_memalign(&ptr, 16, total_bytes) != 0) {
        return NULL;
    }
    // Zero-initialize the memory
    memset(ptr, 0, total_bytes);
#else
    // Fallback to calloc (zero-initialized)
    ptr = calloc(num_elements, element_size);
#endif

    return ptr;
}

// **********************************************************************
// Core Creation Functions
// **********************************************************************

/**
 * Create a float32 tensor with specified shape
 */
seq_tensor* seq_tensor_create_float(size_t ndim, const size_t *shape) {
    if (ndim == 0 || shape == NULL) {
        return NULL;
    }

    // Validate shape (all dimensions must be > 0)
    for (size_t i = 0; i < ndim; i++) {
        if (shape[i] == 0) {
            return NULL;
        }
    }

    // Allocate tensor structure
    seq_tensor *t = (seq_tensor*)calloc(1, sizeof(seq_tensor));
    if (t == NULL) {
        return NULL;
    }

    // Set basic properties
    t->ndim = ndim;
    t->dtype = SEQ_TENSOR_FLT32;
    t->element_size = sizeof(float);
    t->scale = 1.0f;
    t->zero_point = 0;

    // Allocate and copy shape array
    t->shape = (size_t*)malloc(ndim * sizeof(size_t));
    if (t->shape == NULL) {
        free(t);
        return NULL;
    }
    memcpy(t->shape, shape, ndim * sizeof(size_t));

    // Allocate and calculate stride array
    t->stride = (size_t*)malloc(ndim * sizeof(size_t));
    if (t->stride == NULL) {
        free(t->shape);
        free(t);
        return NULL;
    }
    calculate_c_strides(ndim, shape, t->stride);

    // Calculate total size
    t->size = calculate_total_size(ndim, shape);

    // Allocate data array (16-byte aligned for SIMD)
    t->data = allocate_aligned_data(t->size, t->element_size);
    if (t->data == NULL) {
        free(t->stride);
        free(t->shape);
        free(t);
        return NULL;
    }

    // Set flags
    t->flags = SEQ_TENSOR_OWNS_DATA | SEQ_TENSOR_ALIGNED_16;

    return t;
}

/**
 * Create an int8 quantized tensor
 */
seq_tensor* seq_tensor_create_int8(size_t ndim, const size_t *shape,
                                   float scale, int32_t zero_point) {
    if (ndim == 0 || shape == NULL) {
        return NULL;
    }

    // Validate shape
    for (size_t i = 0; i < ndim; i++) {
        if (shape[i] == 0) {
            return NULL;
        }
    }

    // Allocate tensor structure
    seq_tensor *t = (seq_tensor*)calloc(1, sizeof(seq_tensor));
    if (t == NULL) {
        return NULL;
    }

    // Set basic properties
    t->ndim = ndim;
    t->dtype = SEQ_TENSOR_INT8;
    t->element_size = sizeof(int8_t);
    t->scale = scale;
    t->zero_point = zero_point;

    // Allocate and copy shape array
    t->shape = (size_t*)malloc(ndim * sizeof(size_t));
    if (t->shape == NULL) {
        free(t);
        return NULL;
    }
    memcpy(t->shape, shape, ndim * sizeof(size_t));

    // Allocate and calculate stride array
    t->stride = (size_t*)malloc(ndim * sizeof(size_t));
    if (t->stride == NULL) {
        free(t->shape);
        free(t);
        return NULL;
    }
    calculate_c_strides(ndim, shape, t->stride);

    // Calculate total size
    t->size = calculate_total_size(ndim, shape);

    // Allocate data array
    t->data = allocate_aligned_data(t->size, t->element_size);
    if (t->data == NULL) {
        free(t->stride);
        free(t->shape);
        free(t);
        return NULL;
    }

    // Set flags
    t->flags = SEQ_TENSOR_OWNS_DATA | SEQ_TENSOR_ALIGNED_16;

    return t;
}

/**
 * Create an int32 accumulator tensor
 */
seq_tensor* seq_tensor_create_int32(size_t ndim, const size_t *shape,
                                    float scale, int32_t zero_point) {
    if (ndim == 0 || shape == NULL) {
        return NULL;
    }

    // Validate shape
    for (size_t i = 0; i < ndim; i++) {
        if (shape[i] == 0) {
            return NULL;
        }
    }

    // Allocate tensor structure
    seq_tensor *t = (seq_tensor*)calloc(1, sizeof(seq_tensor));
    if (t == NULL) {
        return NULL;
    }

    // Set basic properties
    t->ndim = ndim;
    t->dtype = SEQ_TENSOR_INT32;
    t->element_size = sizeof(int32_t);
    t->scale = scale;
    t->zero_point = zero_point;

    // Allocate and copy shape array
    t->shape = (size_t*)malloc(ndim * sizeof(size_t));
    if (t->shape == NULL) {
        free(t);
        return NULL;
    }
    memcpy(t->shape, shape, ndim * sizeof(size_t));

    // Allocate and calculate stride array
    t->stride = (size_t*)malloc(ndim * sizeof(size_t));
    if (t->stride == NULL) {
        free(t->shape);
        free(t);
        return NULL;
    }
    calculate_c_strides(ndim, shape, t->stride);

    // Calculate total size
    t->size = calculate_total_size(ndim, shape);

    // Allocate data array
    t->data = allocate_aligned_data(t->size, t->element_size);
    if (t->data == NULL) {
        free(t->stride);
        free(t->shape);
        free(t);
        return NULL;
    }

    // Set flags
    t->flags = SEQ_TENSOR_OWNS_DATA | SEQ_TENSOR_ALIGNED_16;

    return t;
}

/**
 * Convenience: Create 2D float matrix with column-major layout
 *
 * Column-major: data stored column-by-column
 * stride = [1, rows] so element (i,j) is at data[j*rows + i]
 */
seq_tensor* seq_tensor_create_2d_float_cm(size_t rows, size_t cols) {
    if (rows == 0 || cols == 0) {
        return NULL;
    }

    size_t shape[2] = {rows, cols};
    seq_tensor *t = seq_tensor_create_float(2, shape);

    if (t != NULL) {
        // Override strides for column-major layout
        // stride[0] = 1 (move one row = one element)
        // stride[1] = rows (move one column = rows elements)
        t->stride[0] = 1;
        t->stride[1] = rows;
    }

    return t;
}

/**
 * Convenience: Create 2D float matrix with row-major layout
 *
 * Row-major: data stored row-by-row
 * stride = [cols, 1] so element (i,j) is at data[i*cols + j]
 */
seq_tensor* seq_tensor_create_2d_float_rm(size_t rows, size_t cols) {
    if (rows == 0 || cols == 0) {
        return NULL;
    }

    size_t shape[2] = {rows, cols};
    seq_tensor *t = seq_tensor_create_float(2, shape);

    if (t != NULL) {
        // Strides are already C-order (row-major) from create_float
        // stride[0] = cols (move one row = cols elements)
        // stride[1] = 1 (move one column = one element)
        // No need to override
    }

    return t;
}

/**
 * Free tensor and all associated memory
 */
void seq_tensor_free(seq_tensor *tensor) {
    if (tensor == NULL) {
        return;
    }

    // Free shape array
    if (tensor->shape != NULL) {
        free(tensor->shape);
    }

    // Free stride array
    if (tensor->stride != NULL) {
        free(tensor->stride);
    }

    // Free data array if we own it
    if (tensor->data != NULL && (tensor->flags & SEQ_TENSOR_OWNS_DATA)) {
        free(tensor->data);
    }

    // Free tensor structure
    free(tensor);
}

// **********************************************************************
// Data Access Functions
// **********************************************************************

/**
 * Get typed pointer to float32 data
 */
float* seq_tensor_data_float(seq_tensor *t) {
    assert(t != NULL);
    assert(t->dtype == SEQ_TENSOR_FLT32);
    return (float*)t->data;
}

/**
 * Get typed pointer to int8 data
 */
int8_t* seq_tensor_data_int8(seq_tensor *t) {
    assert(t != NULL);
    assert(t->dtype == SEQ_TENSOR_INT8);
    return (int8_t*)t->data;
}

/**
 * Get typed pointer to int32 data
 */
int32_t* seq_tensor_data_int32(seq_tensor *t) {
    assert(t != NULL);
    assert(t->dtype == SEQ_TENSOR_INT32);
    return (int32_t*)t->data;
}

// **********************************************************************
// Dimension Query Functions
// **********************************************************************

/**
 * Get size of specific dimension
 */
size_t seq_tensor_dim(const seq_tensor *t, size_t axis) {
    assert(t != NULL);
    assert(axis < t->ndim);
    return t->shape[axis];
}

/**
 * Get total number of elements
 */
size_t seq_tensor_total_size(const seq_tensor *t) {
    assert(t != NULL);
    return t->size;
}

// **********************************************************************
// Debug/Display Functions
// **********************************************************************

/**
 * Print tensor shape and first few elements
 */
void seq_tensor_print(const seq_tensor *t) {
    if (t == NULL) {
        printf("seq_tensor: NULL\n");
        return;
    }

    // Print dtype name
    const char *dtype_name;
    switch (t->dtype) {
        case SEQ_TENSOR_INT8:  dtype_name = "int8";   break;
        case SEQ_TENSOR_INT32: dtype_name = "int32";  break;
        case SEQ_TENSOR_FLT32: dtype_name = "float32"; break;
        default:               dtype_name = "unknown"; break;
    }

    // Print shape
    printf("seq_tensor dtype=%s shape=[", dtype_name);
    for (size_t i = 0; i < t->ndim; i++) {
        printf("%zu", t->shape[i]);
        if (i < t->ndim - 1) printf(",");
    }
    printf("] size=%zu\n", t->size);

    // Print first 10 elements
    size_t num_to_print = (t->size < 10) ? t->size : 10;
    printf("  First %zu elements: ", num_to_print);

    switch (t->dtype) {
        case SEQ_TENSOR_FLT32: {
            float *data = (float*)t->data;
            for (size_t i = 0; i < num_to_print; i++) {
                printf("%.4f ", data[i]);
            }
            break;
        }
        case SEQ_TENSOR_INT8: {
            int8_t *data = (int8_t*)t->data;
            for (size_t i = 0; i < num_to_print; i++) {
                printf("%d ", data[i]);
            }
            break;
        }
        case SEQ_TENSOR_INT32: {
            int32_t *data = (int32_t*)t->data;
            for (size_t i = 0; i < num_to_print; i++) {
                printf("%d ", data[i]);
            }
            break;
        }
    }
    printf("\n");

    // Print quantization info if applicable
    if (t->dtype == SEQ_TENSOR_INT8 || t->dtype == SEQ_TENSOR_INT32) {
        printf("  Quantization: scale=%.6f zero_point=%d\n", t->scale, t->zero_point);
    }
}
