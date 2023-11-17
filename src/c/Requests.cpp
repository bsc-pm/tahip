/*
	This file is part of Task-Aware HIP and is licensed under the terms contained in the COPYING and COPYING.LESSER files.

	Copyright (C) 2021-2023 Barcelona Supercomputing Center (BSC)
*/

#include <cassert>

#include <TAHIP.h>

#include "common/RequestManager.hpp"

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
