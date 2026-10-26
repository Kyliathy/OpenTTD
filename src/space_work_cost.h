/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_work_cost.h Opt-in native operation measurements without simulation or resource authority.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group21
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group33; lifecycle/forecast/retry measurements remain disposable observations.
 * Normal gameplay has no observer. These disposable thread-local diagnostics are never saved, synchronized or consulted for admission.
 */
#ifndef SPACE_WORK_COST_H
#define SPACE_WORK_COST_H

#include <array>
#include <cstddef>
#include <cstdint>

/** Actual native operations; counts describe work and cannot authorize cargo, money, motion or another simulation opportunity. */
enum class SpaceWorkKind : uint8_t {
	GenerationCells, GenerationStarterCandidates, GenerationShuffleSwaps, GenerationBodies,
	ValidationBodies, ValidationVelocityTests, BodySteps,
	IndexUpdates, IndexUnlinks, IndexLinks, QueryCalls, QueryBuckets, QueryCandidates, QueryOrderTests,
	BodyLookups, CraftTicks, AttachmentVisits, ApproachCraftVisits, ApproachOccupantChecks, ApproachTargetChecks,
	PlannerRequests, PlannerSlots, PlannerSlotTests, PlannerOwnerTests, PlannerOrderTests,
	StagingBodyTests, ExtractionAttempts, ExtractionUnits,
	ExitForecasts, InterceptForecasts, MiningSelections, RenewalAttempts, RenewalEntryCandidates, RetirementParticipants, End,
};

/** Caller-owned diagnostic totals; accumulating them creates no second gameplay ledger or persistent field owner. */
struct SpaceWorkSample {
	std::array<uint64_t, static_cast<size_t>(SpaceWorkKind::End)> counts{}; ///< Native operations observed in this caller's explicit scope.
	/** @param kind Native operation being inspected. @return Its accumulated diagnostic count. */
	uint64_t Get(SpaceWorkKind kind) const { return this->counts[static_cast<size_t>(kind)]; }
};

/** Internal observation routing only; another drawing thread cannot mutate a game-thread measurement. */
namespace SpaceWorkDiagnostics {
	inline thread_local SpaceWorkSample *current = nullptr; ///< Null during normal gameplay; never part of save/network state.
}

/** Record an actual native operation only while explicitly observed.
 * @param kind Operation just performed by the original producer.
 * @param amount Actual operations or native emitted units, allowing a contiguous bucket copy to be counted without another scan.
 */
inline void RecordSpaceWork(SpaceWorkKind kind, uint64_t amount = 1)
{
	if (SpaceWorkDiagnostics::current != nullptr) SpaceWorkDiagnostics::current->counts[static_cast<size_t>(kind)] += amount;
}

/** Temporarily observe native work on this thread, preserving any outer diagnostic scope without changing simulation inputs. */
class ScopedSpaceWork {
	SpaceWorkSample *previous; ///< Previous observer restored exactly once at scope exit.
public:
	/** @param sample Caller-owned totals; existing counts remain available for deliberate cumulative component measurements. */
	explicit ScopedSpaceWork(SpaceWorkSample &sample) : previous(SpaceWorkDiagnostics::current) { SpaceWorkDiagnostics::current = &sample; }
	/** Restore the previous observer; no gameplay owner, interval or admission changes. */
	~ScopedSpaceWork() { SpaceWorkDiagnostics::current = this->previous; }
	/** @param other Observer lifetime that cannot be duplicated or restore routing twice. */
	ScopedSpaceWork(const ScopedSpaceWork &other) = delete;
	/** @param other Observer lifetime that cannot replace this scope's restoration duty. @return Unavailable: observer assignment is forbidden. */
	ScopedSpaceWork &operator=(const ScopedSpaceWork &other) = delete;
};

#endif /* SPACE_WORK_COST_H */
