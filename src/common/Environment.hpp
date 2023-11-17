/*
	This file is part of Task-Aware HIP and is licensed under the terms contained in the COPYING and COPYING.LESSER files.

	Copyright (C) 2021 Barcelona Supercomputing Center (BSC)
*/

#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include "Allocator.hpp"
#include "TaskingModel.hpp"
#include "RequestManager.hpp"
#include "StreamPool.hpp"


namespace tahip {

//! Class that represents the environment
class Environment {
private:
	//! The polling instance that periodically checks the completion of the TAHIP
	//! requests and events
	static TaskingModel::PollingInstance *_pollingInstance;

	//! Determine the polling frequency when the TAHIP polling is
	//! implemented with tasks that are paused periodically. That is
	//! the frequency in time (microseconds) at which the in-flight
	//! TAHIP requests and events are checked in TAHIP. This environment
	//! variable is called TAHIP_POLLING_FREQUENCY and the default value
	//! is 500 microseconds
	static EnvironmentVariable<uint64_t> _pollingFrequency;

public:
	Environment() = delete;
	Environment(const Environment &) = delete;
	const Environment& operator= (const Environment &) = delete;

	//! \brief Initialize the environment of TAHIP
	//!
	//! This function should be called after a successful call to
	//! the original cuInit function
	static void initialize()
	{
		TaskingModel::initialize();

		Allocator<Request>::initialize();

		assert(!_pollingInstance);
		_pollingInstance = TaskingModel::registerPolling("TAHIP", Environment::polling, nullptr, _pollingFrequency);
	}

	//! \brief Finalize the environment of TAHIP
	//!
	//! This function should be called before finalizing
	//! the program and after any HIP function
	static void finalize()
	{
		TaskingModel::unregisterPolling(_pollingInstance);

		Allocator<Request>::finalize();
	}

private:
	//! \brief Polling function that checks the requests and events
	//!
	//! This function is periodically called by the tasking runtime
	//! system and should check for the TAHIP requests and events
	//!
	//! \param args An opaque pointer to the arguments
	static void polling(void *)
	{
		RequestManager::checkRequests();
	}
};

} // namespace tahip

#endif // ENVIRONMENT_HPP
