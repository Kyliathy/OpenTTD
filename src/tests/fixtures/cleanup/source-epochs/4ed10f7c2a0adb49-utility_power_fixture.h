/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file utility_power_fixture.h Actual company expense participant shared by utility owner and native file tests.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group 8
 * @see docs/zz-reach2/upgrades/2026-10/portal-tbm/r2mlvs3mp-v-utility-settlement-contract.md
 */

#ifndef TESTS_UTILITY_POWER_FIXTURE_H
#define TESTS_UTILITY_POWER_FIXTURE_H

#include "../utility_power.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_type.h"

/** Scoped utility isolation clears live/staged accounts before the surrounding real company/world fixture is destroyed. */
class UtilityPowerReset {
public:
	/** Begin a fresh utility owner while its actual world/company already exists. */
	UtilityPowerReset() { ResetUtilityPowerState(); }
	/** Clear only this fixture's utility authority before actual world teardown. */
	~UtilityPowerReset() { ResetUtilityPowerState(); }
};

/** Actual company expense participant exercises whole-owner admission and failure without copying material or money authority. */
class UtilityCompanyWork : public UtilityMutationParticipant {
	CompanyID owner; ///< Actual fixture company; its own money and expense counters remain authoritative.
public:
	bool admit = true; ///< Explicit whole-owner validation refusal.
	bool commit = true; ///< Explicit unchanged whole-owner commit refusal.
	/** @param owner Actual company whose complete expense action accompanies this test's metered use. */
	explicit UtilityCompanyWork(CompanyID owner) : owner(owner) {}
	/** @param proposal Exact owned utility proposal. @return Whether the real company can still pay this whole five-unit action. */
	bool Validate(const UtilityMutationProposal &proposal) const override
	{
		// One actual owner must admit the complete expense; a foreign proposal
		// or unavailable funds cannot consume energy ahead of a failed action.
		return this->admit && proposal.owner == this->owner && Company::IsValidID(this->owner) && Company::Get(this->owner)->money >= Money{5};
	}
	/** @param proposal Exact already validated action. @return Complete real expense success or unchanged refusal. */
	bool Commit([[maybe_unused]] const UtilityMutationProposal &proposal) noexcept override
	{
		if (!this->commit) return false;
		SubtractMoneyFromCompany(this->owner, CommandCost(ExpensesType::Other, Money{5}));
		return true;
	}
};

#endif /* TESTS_UTILITY_POWER_FIXTURE_H */
