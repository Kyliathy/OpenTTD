/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file allocation_failure_fixture.h Test-executable-only scoped failure of actual C++ preparation allocations.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups28/28B.
 */
#ifndef TESTS_ALLOCATION_FAILURE_FIXTURE_H
#define TESTS_ALLOCATION_FAILURE_FIXTURE_H
#include <cstdint>

/** Fail one actual ordinary C++ allocation on this thread, then disarm before exception reporting. Never linked into the normal game. */
class ScopedAllocationFailure {
public:
	/** @param successful_allocations Number of real allocation calls admitted before the single bad_alloc. Nested armed scopes are deliberately unsupported. */
	explicit ScopedAllocationFailure(uint32_t successful_allocations) noexcept;
	/** Disarm this thread before assertions, diagnostics or subsequent independent cases allocate. */
	~ScopedAllocationFailure();
	/** @return Whether this scope actually injected its one allocation failure. Reads only thread-local test state and allocates nothing. */
	bool Injected() const noexcept;
	/** @param other Existing scope cannot be copied or lend failure ownership to another lifetime. */
	ScopedAllocationFailure(const ScopedAllocationFailure &other) = delete;
	/** @param other Existing scope cannot be assigned over another active lifetime. */
	ScopedAllocationFailure &operator=(const ScopedAllocationFailure &other) = delete;
};
#endif /* TESTS_ALLOCATION_FAILURE_FIXTURE_H */
