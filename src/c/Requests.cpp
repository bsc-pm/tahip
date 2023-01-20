/*
	This file is part of Task-Aware CUDA and is licensed under the terms contained in the COPYING and COPYING.LESSER files.

	Copyright (C) 2021 Barcelona Supercomputing Center (BSC)
*/

#include <TAHIP.h>

#include "common/Environment.hpp"
#include "common/TaskingModel.hpp"
#include "common/util/ErrorHandler.hpp"

using namespace tahip;

#pragma GCC visibility push(default)

extern "C" {

hipError_t
tahipWaitRequestAsync(tahipRequest *request)
{
	assert(request != nullptr);

	if (*request != TAHIP_REQUEST_NULL)
		RequestManager::processRequest((Request *) *request);

	*request = TAHIP_REQUEST_NULL;

	return hipSuccess;
}

hipError_t
tahipWaitallRequestsAsync(size_t count, tahipRequest requests[])
{
	if (count == 0)
		return hipSuccess;

	assert(requests != nullptr);

	RequestManager::processRequests(count, (Request * const *) requests);

	for (size_t r = 0; r < count; ++r) {
		requests[r] = TAHIP_REQUEST_NULL;
	}

	return hipSuccess;
}

} // extern C

#pragma GCC visibility pop
