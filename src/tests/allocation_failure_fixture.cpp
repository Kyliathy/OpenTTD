/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file allocation_failure_fixture.cpp Actual allocation faults restricted to this test executable and an explicit thread-local scope.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups28/28B.
 * Ordinary allocation/deallocation retains malloc/free ownership and the C++ new_handler retry contract. Over-aligned operators keep their paired standard implementation. The normal game never links this file.
 */
#include "../stdafx.h"
#include "allocation_failure_fixture.h"
#include <cstdlib>
#include <new>
#include "../safeguards.h"

// This translation unit implements the test executable's lowest-level standard
// allocation operators themselves. Higher-level engine/test code still uses
// new/vector; the ordinary malloc safeguard remains enabled in every other TU.
#undef malloc

/** Per-thread transient fault control; it is never gameplay state or serialized. */
struct AllocationFailureControl {
	bool armed = false, injected = false;
	uint32_t remaining = 0;
};
static thread_local AllocationFailureControl _allocation_failure;

/** @copydoc ScopedAllocationFailure::ScopedAllocationFailure(uint32_t) */
ScopedAllocationFailure::ScopedAllocationFailure(uint32_t successful_allocations) noexcept
{
	assert(!_allocation_failure.armed);
	_allocation_failure = {true, false, successful_allocations};
}
/** @copydoc ScopedAllocationFailure::~ScopedAllocationFailure */
ScopedAllocationFailure::~ScopedAllocationFailure() { _allocation_failure.armed = false; }
/** @copydoc ScopedAllocationFailure::Injected */
bool ScopedAllocationFailure::Injected() const noexcept { return _allocation_failure.injected; }

/** @param size Standard requested byte count; zero still receives a distinct nonnull allocation. @return Ordinary suitably aligned storage, or bad_alloc after the explicitly scoped fault/standard handler refuses recovery. */
void *operator new(size_t size)
{
	bool fault_eligible = true;
#if defined(_MSC_VER) && _ITERATOR_DEBUG_LEVEL != 0
	// MSVC allocates iterator proxies even in noexcept default constructors.
	// Injecting there terminates inside the standard library before the engine
	// can observe bad_alloc. Debug therefore skips proxy-sized allocations;
	// Release still faults every ordinary size, including any real allocation
	// sharing that size. The remaining Debug faults exercise native preparation.
	fault_eligible = size != sizeof(std::_Container_proxy);
#endif
	// Only the explicit test scope may fail an eligible real allocation. Debug
	// bookkeeping exclusion does not consume its preparation-failure countdown.
	if (_allocation_failure.armed && fault_eligible) {
		// Admit the requested real preparation calls, then fail exactly once.
		// Disarm before throwing so Catch/reporting and native refusal receipts
		// can allocate normally instead of recursively failing diagnostics.
		if (_allocation_failure.remaining == 0) {
			_allocation_failure.armed = false; _allocation_failure.injected = true; throw std::bad_alloc{};
		}
		--_allocation_failure.remaining;
	}
	// Retain the standard new_handler contract during a genuine host allocation
	// failure: the handler may free memory, throw, or permit another attempt.
	for (;;) {
		if (void *memory = std::malloc(size == 0 ? 1 : size)) return memory;
		const auto handler = std::get_new_handler();
		if (handler == nullptr) throw std::bad_alloc{};
		handler();
	}
}
/** @param size Ordinary array storage byte count. @return The same scoped ordinary allocation semantics as scalar new. */
void *operator new[](size_t size) { return ::operator new(size); }
/** @param memory Ordinary storage returned by this executable's paired new; null is accepted. */
void operator delete(void *memory) noexcept { std::free(memory); }
/** @param memory Ordinary array storage from the paired new[]. */
void operator delete[](void *memory) noexcept { std::free(memory); }
/** @param memory Ordinary storage from paired scalar new. @param size Compiler-supplied size, which does not change storage ownership. */
void operator delete(void *memory, [[maybe_unused]] size_t size) noexcept { std::free(memory); }
/** @param memory Ordinary storage from paired array new[]. @param size Compiler-supplied size, which does not change storage ownership. */
void operator delete[](void *memory, [[maybe_unused]] size_t size) noexcept { std::free(memory); }
