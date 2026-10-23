/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_heat_participant_fixture.h Existing whole native company-expense Heat participant shared by physical allocation and actual powerhouse acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups3/8DD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * This real five-Money expense proves complete physical use, without claiming an operating smelter or manufactured cargo.
 */
#ifndef TESTS_GEOTHERMAL_HEAT_PARTICIPANT_FIXTURE_H
#define TESTS_GEOTHERMAL_HEAT_PARTICIPANT_FIXTURE_H

#include "utility_power_fixture.h"
#include "../geothermal_heat.h"

/** A real complete company expense implements the thermal participant contract; its five-Money benefit is separate from actual generator output or manufactured cargo. */
class RoutedExpense final : public GeothermalHeatParticipant {
	CompanyID owner; ///< Actual company funding the complete five-unit action.
	UtilityHeatUse use; ///< Immutable generator/direct expense role.
public:
	UtilityCompanyWork work; ///< Existing actual expense owner; no money/input inventory is copied.
	bool feasible = true; ///< Whole-owner demand admission switch, used to exercise full/starved input/output/auxiliary refusal.
	/** @param owner Actual paying company. @param use Immutable complete role. */
	RoutedExpense(CompanyID owner, UtilityHeatUse use) : owner(owner), use(use), work(owner) {}
	/** @return This actual complete benefit's immutable thermal role. */
	std::optional<UtilityHeatUse> ThermalUse() const override { return this->use; }
	/** @param request Exact owned feasible demand. @return Whole expense readiness without any cash or heat mutation. */
	bool ValidateDemand(const GeothermalHeatRequest &request) const override
	{
		// Full/starved whole-owner admission and actual funding precede scarce
		// reservations; a refused work action must leave heat available to peers.
		return this->feasible && request.owner == this->owner && request.use == this->use && Company::IsValidID(this->owner) && Company::Get(this->owner)->money >= Money{5};
	}
	/** @param proposal Exact original native source reservation. @return Whole actual work admission without mutation. */
	bool Validate(const UtilityMutationProposal &proposal) const override { return this->work.Validate(proposal); }
	/** @param proposal Exact already checked original quantity. @return Complete actual expense or unchanged refusal. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override { return this->work.Commit(proposal); }
};

#endif /* TESTS_GEOTHERMAL_HEAT_PARTICIPANT_FIXTURE_H */
