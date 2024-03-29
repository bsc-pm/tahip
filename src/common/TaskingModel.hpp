/*
	This file is part of Task-Aware HIP and is licensed under the terms contained in the COPYING and COPYING.LESSER files.

	Copyright (C) 2021-2023 Barcelona Supercomputing Center (BSC)
*/

#ifndef TASKING_MODEL_HPP
#define TASKING_MODEL_HPP

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>

#include "ALPI.hpp"
#include "util/EnvironmentVariable.hpp"
#include "Symbol.hpp"

namespace tahip {

//! Class that gives access to the tasking model features
class TaskingModel {
public:
	typedef struct alpi_task *task_handle_t;
	typedef void (*polling_function_t)(void *args);

	//! Structure that stores information regarding a polling instance
	struct PollingInstance {
		std::string _name;
		polling_function_t _function;
		void *_args;
		uint64_t _pollingFrequency;
		std::atomic<bool> _mustFinish;
		std::atomic<bool> _finished;

		PollingInstance(const std::string &name, polling_function_t function, void *args, uint64_t pollingFrequency) :
			_name(name), _function(function), _args(args), _pollingFrequency(pollingFrequency),
			_mustFinish(false), _finished(false)
		{
		}
	};

private:
	//! The symbol declarations of the tasking model functions
	using alpi_error_string_t = SymbolDecl<const char *, int>;
	using alpi_version_check_t = SymbolDecl<int, int, int>;
	using alpi_version_get_t = SymbolDecl<int, int *, int *>;
	using alpi_task_self_t = SymbolDecl<int, struct alpi_task **>;
	using alpi_task_events_increase_t = SymbolDecl<int, struct alpi_task *, uint64_t>;
	using alpi_task_events_decrease_t = SymbolDecl<int, struct alpi_task *, uint64_t>;
	using alpi_task_waitfor_ns_t = SymbolDecl<int, uint64_t, uint64_t *>;
	using alpi_task_spawn_t = SymbolDecl<int, void (*)(void *), void *, void (*)(void *), void *, const char *, const struct alpi_attr *>;
	using alpi_cpu_count_t = SymbolDecl<int, uint64_t *>;
	using alpi_cpu_logical_id_t = SymbolDecl<int, uint64_t *>;

	//! The symbols of the tasking model functions
	static Symbol<alpi_error_string_t> _alpi_error_string;
	static Symbol<alpi_version_check_t> _alpi_version_check;
	static Symbol<alpi_version_get_t> _alpi_version_get;
	static Symbol<alpi_task_self_t> _alpi_task_self;
	static Symbol<alpi_task_events_increase_t> _alpi_task_events_increase;
	static Symbol<alpi_task_events_decrease_t> _alpi_task_events_decrease;
	static Symbol<alpi_task_waitfor_ns_t> _alpi_task_waitfor_ns;
	static Symbol<alpi_task_spawn_t> _alpi_task_spawn;
	static Symbol<alpi_cpu_count_t> _alpi_cpu_count;
	static Symbol<alpi_cpu_logical_id_t> _alpi_cpu_logical_id;

public:
	//! \brief Initialize and load the symbols of the tasking model
	static void initialize();

	//! \brief Register a polling instance
	//!
	//! This function registers a polling instance, which will call the
	//! specified function with its argument periodically. The polling
	//! function must follow the type polling_function_t
	//!
	//! \param name The name of the polling instance
	//! \param function The function to be called periodically
	//! \param args The arguments of the function
	//!
	//! \returns A polling handle to unregister the instance once
	//!          the polling should finish
	static PollingInstance *registerPolling(
		const std::string &name,
		polling_function_t function,
		void *args,
		uint64_t pollingFrequency
	) {
		PollingInstance *instance = new PollingInstance(name, function, args, pollingFrequency);
		assert(instance != nullptr);

		// Spawn a task that will do the periodic polling
		int err = _alpi_task_spawn(
			genericPolling, static_cast<void *>(instance),
			genericCompleted, static_cast<void *>(instance),
			name.data(), nullptr);
		if (err)
			ErrorHandler::fail("Failed alpi_task_spawn: ", getError(err));

		return instance;
	}

	//! \brief Unregister a polling instance
	//!
	//! This function unregisters a polling instance, which will
	//! prevent the associated polling function from being further
	//! called. The polling function is guaranteed to not be called
	//! after returing from this function. Note that other registered
	//! polling instances may continue calling that function
	//!
	//! \param handle The handle of the polling instance to unregister
	static void unregisterPolling(PollingInstance *instance)
	{
		assert(instance != nullptr);

		// Notify that the polling should stop
		instance->_mustFinish = true;

		// Wait until the spawned task completes
		while (!instance->_finished) {
			// Task yield to avoid consuming a CPU for waiting. Otherwise, in
			// the case of a single CPU, the execution could hang
			if (int err = _alpi_task_waitfor_ns(1000000, nullptr))
				ErrorHandler::fail("Failed alpi_task_waitfor_ns: ", getError(err));
		}
		delete instance;
	}

	//! \brief Get the current task handle
	static task_handle_t getCurrentTask()
	{
		struct alpi_task *task;
		if (int err = _alpi_task_self(&task))
			ErrorHandler::fail("Failed alpi_task_self: ", getError(err));

		return task;
	}

	//! \brief Increase the events of the current task
	//!
	//! \param task The current task's handle
	//! \param increment The amount of events to increase
	static void increaseCurrentTaskEvents(task_handle_t task, uint64_t increment)
	{
		if (int err = _alpi_task_events_increase(task, increment))
			ErrorHandler::fail("Failed alpi_task_events_increase: ", getError(err));
	}

	//! \brief Decrease the events of a task
	//!
	//! \param task The task's handle to decrease events
	//! \param decrement The amount of events to decrease
	static void decreaseTaskEvents(task_handle_t task, uint64_t decrement)
	{
		if (int err = _alpi_task_events_decrease(task, decrement))
			ErrorHandler::fail("Failed alpi_task_events_decrease: ", getError(err));
	}

	//! \brief Get the number of CPUs present in the system
	//!
	//! \returns The maximum number of avalable CPUs
	static uint64_t getNumCPUs()
	{
		uint64_t cpus;
		if (int err = _alpi_cpu_count(&cpus))
			ErrorHandler::fail("Failed alpi_cpu_count: ", getError(err));

		return cpus;
	}

	//! \brief Get the current logical (0..n) CPU id
	//!
	//! \returns The current cpu id
	static uint64_t getCurrentCPU()
	{
		uint64_t currentCPU;
		if (int err = _alpi_cpu_logical_id(&currentCPU))
			ErrorHandler::fail("Failed alpi_cpu_count: ", getError(err));

		return currentCPU;
	}


private:
	//! \brief Wrapper function called by all polling tasks
	//!
	//! This function is called once for each polling instance and is
	//! executed by a task. This function runs on a loop until the
	//! instance is unregistered. The body of the loop performs a call
	//! to the polling instance function and then blocks the task for
	//! a time specified by that function as its return value
	//!
	//! \param args An opaque pointer to the polling instance
	static void genericPolling(void *args)
	{
		PollingInstance *instance = static_cast<PollingInstance *>(args);
		assert(instance != nullptr);

		// Poll until it is externally notified to stop
		while (!instance->_mustFinish) {
			// Call the actual polling function
			instance->_function(instance->_args);

			// Pause the polling task for some microseconds
			if (int err = _alpi_task_waitfor_ns(instance->_pollingFrequency * 1000, nullptr))
				ErrorHandler::fail("Failed task_waitfor_ns: ", getError(err));
		}
	}

	//! \brief Function called by a polling task is completed
	//!
	//! \param args An opaque pointer to the polling instance
	static void genericCompleted(void *args)
	{
		PollingInstance *instance = static_cast<PollingInstance *>(args);
		assert(instance != nullptr);

		// The polling task has completed
		instance->_finished = true;
	}

	//! \brief Get the string describing the alpi error
	//!
	//! \param error The error code
	static const char *getError(int error)
	{
		return _alpi_error_string(error);
	}
};

} // namespace tahip

#endif // TASKING_MODEL_HPP
