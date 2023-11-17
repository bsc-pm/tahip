/*
	This file is part of Task-Aware HIP and is licensed under the terms contained in the COPYING and COPYING.LESSER files.

	Copyright (C) 2021 Barcelona Supercomputing Center (BSC)
*/

#include <cassert>

#include <TAHIP.h>

#include "common/RequestManager.hpp"

using namespace tahip;

#pragma GCC visibility push(default)

extern "C" {

hipError_t
tahipLaunchKernel(
    const void* func, dim3 gridDim, dim3 blockDim, 
    void** args, size_t sharedMem, hipStream_t stream,
    tahipRequest *requestPtr)
{

	hipError_t eret;
    
	eret = hipLaunchKernel(func, gridDim, blockDim, args,
		sharedMem, stream);
	if (eret != hipSuccess)
		return eret;

	Request *request = RequestManager::generateRequest(stream, (requestPtr == nullptr));
	assert(request != nullptr);

	if (requestPtr != nullptr)
		*requestPtr = request;

	return hipSuccess;
}

} // extern C

#pragma GCC visibility pop
