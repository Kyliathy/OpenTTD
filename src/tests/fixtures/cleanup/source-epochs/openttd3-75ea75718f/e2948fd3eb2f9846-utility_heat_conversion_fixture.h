/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_heat_conversion_fixture.h Actual company/shared numerical identities and whole expense boundary; physical plant operation remains a separate producer.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8DB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_UTILITY_HEAT_CONVERSION_FIXTURE_H
#define TESTS_UTILITY_HEAT_CONVERSION_FIXTURE_H

#include "excavation_fixture.h"
#include "utility_power_fixture.h"
#include "../economy_func.h"
#include "../core/backup_type.hpp"
#include "../town.h"
#include "../strings_type.h"

/** Real company/world and original shared identities; these are numerical core boundaries rather than installed physical plant ports. */
class UtilityConversionRig {
public:
	ExcavationFixture world; ///< Actual Deep/calendar/company admission.
	UtilityPowerReset ledger; ///< Clear actual shared history before its world/company retires.
	UtilityProducerID heat{UINT32_MAX}; ///< Issued original thermal source, never a test-local energy store.
	UtilityConsumerID input{UINT32_MAX}; ///< Actual typed generator reservation owner.
	UtilityProducerID output{UINT32_MAX}; ///< Conversion-only exact electrical source.
	std::unique_ptr<Town> town; ///< Ordinary game loading requires one real town; pure numerical fixtures do not create one.
	/**
	 * Register actual shared participants after pure prediction with zero initial energy/carry.
	 * @param efficiency Canonical numerical ratio; physical model/date admission remains the later native facility's responsibility.
	 * @param serialized_native_slots Use actual shipped cargo slots for genuine ordinary-file fixtures.
	 * @param capacity_w Optional explicit core output ceiling; zero selects the ratio-rated1MW thermal reference.
	 */
	explicit UtilityConversionRig(uint8_t efficiency = 15, bool serialized_native_slots = false, uint64_t capacity_w = 0) : world(serialized_native_slots)
	{
		const auto source = RegisterUtilityHeatProducer(_current_company); REQUIRE(source.has_value()); this->heat = *source;
		const auto consumer = RegisterUtilityHeatConsumer(_current_company, UtilityHeatUse::ElectricalGenerator, 27000, std::array{this->heat});
		REQUIRE(consumer.has_value()); this->input = *consumer;
		const auto before = ComputeUtilityPowerDigest();
		const uint64_t rating = capacity_w == 0 ? uint64_t{efficiency} * 10000 : capacity_w;
		const auto estimate = RegisterUtilityHeatConversionProducer(_current_company, this->input, efficiency, rating, false);
		REQUIRE(estimate.has_value()); CHECK(ComputeUtilityPowerDigest() == before);
		const auto producer = RegisterUtilityHeatConversionProducer(_current_company, this->input, efficiency, rating);
		REQUIRE(producer == estimate); this->output = *producer;
		if (serialized_native_slots) {
			// An ordinary game needs an actual native town before afterload can
			// accept its world. This supplies no heat, electricity or company cash.
			REQUIRE(Town::CanAllocateItem()); this->town.reset(Town::Create(TileXYL(10, 10, MapLayer::Surface)));
			this->town->townnametype = SPECSTR_TOWNNAME_START; RebuildTownKdtree();
		}
	}
	/** Retire the fixture-owned native town and its spatial reference before the surrounding actual world/company disappears. */
	~UtilityConversionRig() { this->town.reset(); RebuildTownKdtree(); }
	/** @param amount Exact bounded demand from this original generator. @param rate_w Actual shared numerical supply, including a valid unavailable zero. Publish and close only the Heat allocation phase. */
	void Allocate(uint64_t amount, uint64_t rate_w = 1000000)
	{
		REQUIRE(PublishUtilityHeatGeneration(this->heat, GetUtilityPowerInterval(), rate_w, 1) == UtilityPowerError::None);
		AutoRestoreBackup system{_current_company, OWNER_DEITY};
		REQUIRE(AllocateUtilityHeat(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{this->input, amount}}) == UtilityPowerError::None);
	}
	/** @param amount Exact positive original Heat use. @return Pure whole conversion proposal in the current epoch. */
	UtilityHeatConversionProposal Prepare(uint64_t amount) const
	{
		return PrepareUtilityHeatConversion({{this->input, GetUtilityPowerInterval()}, this->output, amount, 1});
	}
	/** Close the electrical pass and rotate through the actual shared owner; no prior whole Heat or electrical quantity is renewed. */
	void Rotate()
	{
		AutoRestoreBackup system{_current_company, OWNER_DEITY};
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), {}) == UtilityPowerError::None);
		REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
};

/** Actual five-Money company expense accompanies each complete core benefit; the eventual physical plant owns its own service tariff. */
class UtilityConversionBenefit : public UtilityMutationParticipant {
	CompanyID owner; ///< Actual invoice owner, never copied cash.
public:
	bool admit = true; ///< Explicit complete validation refusal.
	bool accept = true; ///< Explicit unchanged commit refusal.
	UtilityHeatUse role = UtilityHeatUse::ElectricalGenerator; ///< Explicit wrong-role injection at the core participant boundary.
	std::optional<UtilityProducerID> recursive_source; ///< Optional ordinary source used to prove existing recursive mutation refusal.
	UtilityPowerError recursive_result = UtilityPowerError::None; ///< Test observation only, never saved gameplay authority.
	uint64_t completed_heat = 0; ///< Core benefit receipt, not a spendable thermal store.
	/** @param owner Actual company controlling this complete invoice. */
	explicit UtilityConversionBenefit(CompanyID owner) : owner(owner) {}
	/** @return Exact original thermal benefit domain. */
	UtilityEnergyDomain EnergyDomain() const override { return UtilityEnergyDomain::Heat; }
	/** @return Immutable generator role, distinct from a direct recipe. */
	std::optional<UtilityHeatUse> ThermalUse() const override { return this->role; }
	/** @param proposal Actual original Heat consumption. @return Whether this complete real company invoice still admits the same whole transaction. */
	bool Validate(const UtilityMutationProposal &proposal) const override
	{
		// A foreign, refused or insolvent benefit cannot receive output while
		// leaving its complete actual company expense unsettled.
		return this->admit && proposal.owner == this->owner && Company::IsValidID(this->owner) && Company::Get(this->owner)->money >= Money{5};
	}
	/** @param proposal Exact already checked original use. @return Complete invoice/benefit success or unchanged refusal, with recursive ledger writes rejected. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override
	{
		if (!this->accept) return false;
		if (this->recursive_source.has_value()) this->recursive_result = PublishUtilityGeneration(*this->recursive_source, proposal.grant.interval, 1000, 1);
		SubtractMoneyFromCompany(this->owner, CommandCost(ExpensesType::Other, Money{5})); this->completed_heat += proposal.quantity_j; return true;
	}
};

#endif /* TESTS_UTILITY_HEAT_CONVERSION_FIXTURE_H */
