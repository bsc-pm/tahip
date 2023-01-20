/*
	This file is part of Task-Aware HIP and is licensed under the terms contained in the COPYING and COPYING.LESSER files.

	Copyright (C) 2021 Barcelona Supercomputing Center (BSC)
*/

#ifndef REQUEST_MANAGER_HPP
#define REQUEST_MANAGER_HPP

#include <hip/hip_runtime_api.h>

#include <boost/intrusive/list.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include <cassert>

#include "util/ErrorHandler.hpp"


namespace tahip {

//! Struct that represents a TAHIP request
struct Request {
	typedef boost::intrusive::link_mode<boost::intrusive::normal_link> link_mode_t;
	typedef boost::intrusive::list_member_hook<link_mode_t> links_t;

	//! The event of the request
    hipEvent_t _event;

	//! The stream where the operation was posted
    hipStream_t _stream;

	//! The event counter of the calling task
	void *_eventCounter;

	//! Support for Boost intrusive lists
	links_t _listLinks;

	inline Request() :
		_eventCounter(nullptr)
	{
	}
};


//! Class that manages the TAHIP requests
class RequestManager {
private:
	typedef boost::lockfree::spsc_queue<Request*, boost::lockfree::capacity<63*1024> > add_queue_t;
	typedef boost::intrusive::member_hook<Request, typename Request::links_t, &Request::_listLinks> hook_option_t;
	typedef boost::intrusive::list<Request, hook_option_t> list_t;

	//! Fast queues to add requests concurrently
	static add_queue_t _addQueue;

	//! Lock to add requests
	static SpinLock _addQueueLock;

	//! List of pending requests
	static list_t _pendingRequests;

	//! \brief Add a TAHIP request
	//!
	//! \param request The request to add
	static void addRequest(Request *request)
	{
		_addQueueLock.lock();
		while (!_addQueue.push(request)) {
			util::spinWait();
		}
		_addQueueLock.unlock();
	}

	//! \brief Add multiple TAHIP requests
	//!
	//! \param requests The requests to add
	static void addRequests(size_t count, Request *const requests[])
	{
		size_t added = 0;
		_addQueueLock.lock();
		do {
			added += _addQueue.push(requests+added, count-added);
		} while (added < count);
		_addQueueLock.unlock();
	}

public:
	//! \brief Generate a TAHIP request waiting for stream completion
	//!
	//! \param stream The stream to wait
	//! \param bind Whether should be bound to the current task
	static Request *generateRequest(hipStream_t stream, bool bind)
	{
		Request *request = Allocator<Request>::allocate();
		assert(request != nullptr);

		hipError_t eret;
		eret = hipEventCreate(&request->_event);
		if (eret != hipSuccess) {
			ErrorHandler::fail("Failed in hipEventCreate: ", eret);
		}

		eret = hipEventRecord(request->_event, stream);
		if (eret != hipSuccess) {
			ErrorHandler::fail("Failed in hipEventRecord: ", eret);
		}

		// Bind the request to the calling task if needed
		if (bind) {
			void *counter = TaskingModel::getCurrentEventCounter();
			assert(counter != nullptr);

			request->_eventCounter = counter;

			TaskingModel::increaseCurrentTaskEventCounter(counter, 1);

			RequestManager::addRequest(request);
		}
		return request;
	}

	static void processRequest(Request *request)
	{
		assert(request != nullptr);
		assert(request->_eventCounter == nullptr);

		void *counter = TaskingModel::getCurrentEventCounter();
		assert(counter != nullptr);

		request->_eventCounter = counter;

		TaskingModel::increaseCurrentTaskEventCounter(counter, 1);

		addRequest(request);
	}

	static void processRequests(size_t count, Request *const requests[])
	{
		assert(count > 0);
		assert(requests != nullptr);

		void *counter = TaskingModel::getCurrentEventCounter();
		assert(counter != nullptr);

		size_t nactive = 0;
		for (size_t r = 0; r < count; ++r) {
			if (requests[r] != nullptr) {
				assert(requests[r]->_eventCounter == nullptr);
				requests[r]->_eventCounter = counter;
				++nactive;
			}
		}
		TaskingModel::increaseCurrentTaskEventCounter(counter, nactive);

		addRequests(count, requests);
	}

	static void checkRequests()
	{
		if (!_addQueue.empty()) {
			_addQueue.consume_all(
				[&](Request *request) {
					if (request != nullptr)
						_pendingRequests.push_back(*request);
				}
			);
		}

		hipError_t eret;

		auto it = _pendingRequests.begin();
		while (it != _pendingRequests.end()) {
			Request &request = *it;

			eret = hipEventQuery(request._event);
			if (eret != hipSuccess && eret != hipErrorNotReady) {
				ErrorHandler::fail("Failed in hipEventQuery: ", eret);
			} else if (eret == hipSuccess) {
                assert(request._eventCounter != nullptr);
				TaskingModel::decreaseTaskEventCounter(request._eventCounter, 1);

				eret = hipEventDestroy(request._event);
				if (eret != hipSuccess) {
					ErrorHandler::fail("Failed in hipEventDestroy: ", eret);
				}

				Allocator<Request>::free(&request);

				it = _pendingRequests.erase(it);
				continue;
            }
			++it;
		}
	}
};

} // namespace tahip

#endif // REQUEST_MANAGER_HPP
