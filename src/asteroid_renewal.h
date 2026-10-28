/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_renewal.h Pure ticket-local replacement geometry under the original field/ledger owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups11/27-28.
 */
#ifndef ASTEROID_RENEWAL_H
#define ASTEROID_RENEWAL_H
#include "asteroid.h"
#include "space_departure.h"

/** Named read-only result, independent of actual insertion, credit debit or timer rearming. */
enum class AsteroidRenewalProposalStatus : uint8_t { Ready, InvalidInput, NotDue, NoEligibleSize, NoClearEntry, AllocationRefused };
/** Copied ticket and optional unissued body. It owns no live identity, knowledge, lease, credit or random stream. */
struct AsteroidRenewalProposal {
	AsteroidRenewalProposalStatus status = AsteroidRenewalProposalStatus::InvalidInput;
	AsteroidRenewalState source_renewal; ///< Exact copied ledger, including both streams and latest metadata; a ticket alone cannot prove unchanged credit/history.
	uint32_t source_next_body_id = 0; ///< Original issuance cursor; a quote cannot outlive another successful birth.
	std::optional<AsteroidRenewalAttempt> attempt; ///< Exactly one saved-stream ticket, even when no size is eligible; the live stream is unchanged.
	std::optional<AsteroidBody> body; ///< Ready-only canonical description with INVALID_ORBITAL_BODY; actual issuance belongs to commit.
	uint8_t candidates_examined = 0; ///< At most two inward sides times eight deterministic coordinates, with no random retries.
	/** @param other Copied proposal from the same explicit inputs. @return Whether reason, ticket/RNG, unissued body and bounded work agree. */
	bool operator==(const AsteroidRenewalProposal &other) const = default;
};
/** Generate one bounded entry proposal without changing any original owner.
 * @param field Valid original/copied due field with its existing saved ledger.
 * @param obstacles Same world's original static footprints and physical approach points, never a GUI selection.
 * @param craft_positions Copied actual Space hull centres, excluding carried passengers; no native pool is consulted by this pure function.
 * @return Named refusal or one unissued finite body and Group11's copied next candidate stream. Size uses the saved ticket; six local draws supply class, reserve, heading, speed, inward side and coordinate. At most16 fixed alternatives consume no further randomness. No ID, debit, insertion, timer or global RNG change occurs.
 */
AsteroidRenewalProposal PlanAsteroidRenewal(const AsteroidField &field, std::span<const SpaceAttachmentObstacle> obstacles,
	std::span<const SpacePoint> craft_positions);
#endif /* ASTEROID_RENEWAL_H */
