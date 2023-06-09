/*
	This file is part of Task-Aware HIP and is licensed under the terms contained in the COPYING and COPYING.LESSER files.

	Copyright (C) 2021 Barcelona Supercomputing Center (BSC)
*/

#ifndef SPIN_LOCK_HPP
#define SPIN_LOCK_HPP

#include <atomic>

#include "Utils.hpp"


namespace tahip {

//! Class that implements a ticket array spinlock that focuses on
//! avoiding the overhead when there are many threads trying to
//! acquire the lock at the same time
class SpinLock {
private:
	const static size_t Size = MAX_SYSTEM_CPUS;

	alignas(CACHELINE_SIZE) util::Padded<std::atomic<size_t> > _buffer[Size];
	alignas(CACHELINE_SIZE) std::atomic<size_t> _head;
	alignas(CACHELINE_SIZE) size_t _next;

public:
	inline SpinLock() :
		_head(0),
		_next(0)
	{
		for (size_t i = 0; i < Size; ++i) {
			std::atomic_init(_buffer[i].ptr_to_basetype(), (size_t) 0);
		}
	}

	//! \brief Aquire the spinlock
	inline void lock()
	{
		const size_t head = _head.fetch_add(1, std::memory_order_relaxed);
		const size_t idx = head % Size;
		while (_buffer[idx].load(std::memory_order_acquire) != head) {
			util::spinWait();
		}
	}

	//! \brief Try to acquire the spinlock
	//!
	//! \returns Whether the spinlock was acquired
	inline bool try_lock()
	{
		size_t head = _head.load(std::memory_order_relaxed);
		const size_t idx = head % Size;
		if (_buffer[idx].load(std::memory_order_relaxed) != head)
			return false;

		return std::atomic_compare_exchange_strong_explicit(
			&_head, &head, head + 1,
			std::memory_order_acquire,
			std::memory_order_relaxed);
	}

	//! \brief Release the spinlock
	inline void unlock()
	{
		const size_t idx = ++_next % Size;
		_buffer[idx].store(_next, std::memory_order_release);
	}
};

} // namespace tahip

#endif // SPIN_LOCK_HPP
