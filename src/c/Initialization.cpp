/*
	This file is part of Task-Aware HIP and is licensed under the terms contained in the COPYING and COPYING.LESSER files.

	Copyright (C) 2021 Barcelona Supercomputing Center (BSC)
*/

#include <TAHIP.h>

#include "common/Environment.hpp"

using namespace tahip;

#pragma GCC visibility push(default)

extern "C" {

hipError_t
tahipInit(unsigned int flags)
{
	hipError_t eret = hipInit(flags);
	if (eret == hipSuccess) {
		Environment::initialize();
	}
	return eret;
}

hipError_t
tahipFinalize()
{
	Environment::finalize();
	return hipSuccess;
}

} // extern C

#pragma GCC visibility pop
