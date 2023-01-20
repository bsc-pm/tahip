/*
	This file is part of Task-Aware HIP and is licensed under the terms contained in the COPYING and COPYING.LESSER files.

	Copyright (C) 2021 Barcelona Supercomputing Center (BSC)
*/

#ifndef STREAM_POOL_HPP
#define STREAM_POOL_HPP

#include <hip/hip_runtime_api.h>

#include <cassert>
#include <vector>

#include "TaskingModel.hpp"
#include "util/ErrorHandler.hpp"


namespace tahip {

//! Class that manages the TAHIP streams
class StreamPool {
private:
	//! Array of streams
	static std::vector<hipStream_t> _streams;

	//! Context of the streams
	static hipCtx_t _context;

public:
	//! \brief Initialize the pool of streams
	//!
	//! \param nstreams The number of streams to create
	static inline void initialize(size_t nstreams)
	{
		assert(nstreams > 0);

		hipError_t eret = hipCtxGetCurrent(&_context);
		if (eret != hipSuccess)
			ErrorHandler::fail("Failed in hipCtxGetCurrent: ", eret);

		_streams.resize(nstreams);
		for (size_t s = 0; s < nstreams; ++s) {
			hipError_t eret2 = hipStreamCreate(&_streams[s]);
			if (eret2 != hipSuccess)
				ErrorHandler::fail("Failed in hipStreamCreate: ", eret2);
		}
	}

	//! \brief Finalize the pool of streams
	static inline void finalize()
	{
		for (size_t s = 0; s < _streams.size(); ++s) {
			hipError_t eret = hipStreamDestroy(_streams[s]);
			if (eret != hipSuccess)
				ErrorHandler::fail("Failed in hipStreamDestroy: ", eret);
		}
	}

	//! \brief Get stream within pool
	//!
	//! \param streamId The stream identifier within the pool
	static inline hipStream_t getStream(size_t streamId)
	{
		assert(streamId < _streams.size());

		hipError_t eret = hipCtxSetCurrent(_context);
		if (eret != hipSuccess)
			ErrorHandler::fail("Failed in hipCtxSetCurrent: ", eret);

		return _streams[streamId];
	}
};

} // namespace tahip

#endif // STREAM_POOL_HPP
