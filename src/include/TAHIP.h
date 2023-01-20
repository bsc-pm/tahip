/*
	This file is part of Task-Aware HIP and is licensed under the terms contained in the COPYING and COPYING.LESSER files.

	Copyright (C) 2021 Barcelona Supercomputing Center (BSC)
*/

#ifndef TAHIP_H
#define TAHIP_H

#include <hipblas.h>
#include <hipsparse.h>
#include <hip/hip_runtime_api.h>

#include <stddef.h>
#include <stdint.h>

#pragma GCC visibility push(default)

#ifdef __cplusplus
extern "C" {
#endif

typedef void *tahipRequest;

static const tahipRequest TAHIP_REQUEST_NULL = NULL;
static const size_t TAHIP_STREAMS_AUTO = 0;

//! \brief Initialization
//!
//! Initializes the TAHIP environment and the HIP driver API
hipError_t
tahipInit(unsigned int flags);

//! \brief Finalization
//!
//! Finalizes the TAHIP environment
hipError_t
tahipFinalize();

//! \brief Initialization of the pool of streams
//!
//! Initializes the pool of stream with count streams
hipError_t
tahipCreateStreams(size_t count);

//! \brief Finalization of the pool of streams
hipError_t
tahipDestroyStreams();

//! \brief Getting a stream
//!
//!  Gets a stream from the pool by passing a pointer to a previously declarated stream
hipError_t
tahipGetStream(hipStream_t *stream);

//! \brief Returning a stream to the pool
hipError_t
tahipReturnStream(hipStream_t stream);

//! \brief Binding the calling task to a stream
//!
//! Asynchronous function, binds the completion of the calling task
//! to the finalization of the submitted operations on the stream
hipError_t
tahipSynchronizeStreamAsync(hipStream_t stream);


//! The following four functions are wrapper functions for some HIP asynchronous operations.
//! Apart form their standard behaviour, they also return a TAHIP request.
//! 	- If the request pointer parameter is not NULL, TAHIP generates a tahipRequest and saves a
//! 	pointer to it in the output parameter after executing the corresponding operation.
//! 	- If the parameter is NULL, TAHIP directly binds the desired operation to the calling task without generating any request.

//! \brief Copying of data between host and device
//!
//! Asynchronous wrapper function, copies data between host and device
__host__ __device__ hipError_t
tahipMemcpyAsync(
	void *dst, const void *src, size_t sizeBytes,
	enum hipMemcpyKind kind, hipStream_t stream,
	tahipRequest *request);

//! \brief Initialization of device memory
//!
//! Asynchronous wrapper function, initializes or sets device memory to a value
__host__ __device__ hipError_t
tahipMemsetAsync(
	void *devPtr, int value, size_t sizeBytes,
	hipStream_t stream,
	tahipRequest *request);

//! \brief Launching a device function
//!
//! Asynchronous wrapper function (from the CPU view), launches a kernel or device function "func"
__host__ hipError_t
tahipLaunchKernel(
    const void* func, dim3 gridDim, dim3 blockDim,
    void** args, size_t sharedMem, hipStream_t stream,
    tahipRequest *request);

//! \brief Performs a matrix-matrix multiplication
//!
//! Asynchronous wrapper function (from the CPU view) from the cuBLAS library,
//! performs the following matrix-matrix multiplication: C := alpha*op(A)*op(B) + beta*C
__device__ hipblasStatus_t
tahipblasGemmEx(hipblasHandle_t handle,
            hipblasOperation_t transa, hipblasOperation_t transb, int m,
            int n, int k, const void *alpha, const void *matA, hipblasDatatype_t Atype,
            int lda, const void *matB, hipblasDatatype_t Btype, int ldb,
            const void *beta, void *matC, hipblasDatatype_t Ctype, int ldc, hipblasDatatype_t computeType,
            hipblasGemmAlgo_t algo, hipStream_t stream, tahipRequest *requestPtr);

//! \brief Binding a request
//!
//! Asynchronous and non-blocking operation, binds a request to the calling task
hipError_t
tahipWaitRequestAsync(tahipRequest *request);

//! \brief Bindig multiple requests
//!
//! Asynchronous and non-blocking operation, binds the calling task
//! to count requests, all of them stored in "requests"
hipError_t
tahipWaitallRequestsAsync(size_t count, tahipRequest requests[]);

#ifdef __cplusplus
}
#endif

#pragma GCC visibility pop

#endif /* TAHIP_H */
