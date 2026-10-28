/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_lifecycle.h Atomic publication through the original asteroid field/index/ledger owners.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups28/28B; Group29 owns the sole normal-loop call.
 */
#ifndef ASTEROID_LIFECYCLE_H
#define ASTEROID_LIFECYCLE_H
#include "asteroid_renewal.h"

/** Explicit native publication outcome; a failed due attempt can close its ledger without creating a body. */
enum class AsteroidLifecycleStatus : uint8_t {
	Ready, Retired, Spawned, NoChange, InvalidField, InvalidBatch, ParticipantsActive, NotDue, StaleAttempt,
	ProposalChanged, NoEligibleSize, NoClearEntry, Capacity, IssuanceExhausted, AllocationRefused,
};
/** Publication receipt only, with no separate body, credit, cargo or timer owner. */
struct AsteroidLifecycleResult {
	AsteroidLifecycleStatus status = AsteroidLifecycleStatus::InvalidField;
	uint32_t retired = 0;
	std::optional<OrbitalBodyID> spawned;
	bool attempt_closed = false; ///< True means the defined one-ticket/timer closure already published, even when no body was created.
};
/** Retire a complete prevalidated outward batch from the installed native world.
 * @param bodies Strictly ascending original live IDs whose next straight step actually exits. Actual native mining pursuit/leases must already be released.
 * @return Explicit receipt; refusal retains field/index/knowledge/credit/native work. Fallible copies and one-pass knowledge filtering precede native Survey/Visit release and nonthrowing owner publication. Successful mineral notifications happen afterward. The caller must not retain any borrowed body pointer across this transaction.
 */
AsteroidLifecycleResult RetireAsteroidBatch(std::span<const OrbitalBodyID> bodies);
/** Inspect the original installed field and bounded native hull/static/reserved geometry without changing any owner.
 * @return One copied unissued proposal, or a named refusal. Allocation refusal retains the exact recognized due attempt when the original field passed validation; inspection never closes it.
 */
AsteroidRenewalProposal PlanNativeAsteroidRenewal();
/** Inspect creation admission using the same predicate as native commit.
 * @param field Explicit complete field; it is validated without allocating or consulting another world.
 * @param attempt Exact current due ticket to inspect.
 * @return Ready, or a named invalid/stale/ineligible/capacity/exhaustion refusal. No physical arrival, insertion, debit, stream or timer change occurs.
 */
AsteroidLifecycleStatus CheckAsteroidRenewalAdmission(const AsteroidField &field, const AsteroidRenewalAttempt &attempt);
/** Close one recognized due interval and, when admitted, publish one actual replacement.
 * @param proposal Exact copied original ledger/cursor/ticket and unissued body. Current native geometry is independently revalidated.
 * @return Named receipt. A stale ledger/ticket changes nothing. A recognized placement/capacity/allocation refusal publishes exactly its failed-attempt stream/timer, retaining all credit and issuance. Success prepares field/index storage before nonthrowing publication and notification. No borrowed body pointer survives this transaction.
 */
AsteroidLifecycleResult CommitAsteroidRenewal(const AsteroidRenewalProposal &proposal);
#endif /* ASTEROID_LIFECYCLE_H */
