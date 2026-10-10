/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_heat.cpp Actual collector/shared-meter/clock boundaries; supplied floor and delivered materials remain explicit unit inputs.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group2
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.3; transported operating plant retains10.6
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_collector_fixture.h"
#include "utility_power_fixture.h"
#include "../geothermal_heat.h"
#include "../transport_tube.h"
#include "../multilayer_state.h"
#include "../safeguards.h"

/** Real native machinery plus isolated shared utility/driver state, without a second energy or cargo store. */
class GeothermalHeatRig : public GeothermalCollectorRig {
	AutoRestoreBackup<PauseModes> pause{_pause_mode, PauseModes{}}; ///< Restore the caller's pause policy after native simulation checks.
public:
	/** Start empty actual shared meters and the canonical common clock before acquiring any thermal identity. */
	GeothermalHeatRig() { ResetUtilityPowerState(); ResetTubePowerVehicles(); }
	/** Clear fixture utility/driver history while the actual company and machinery still exist. */
	~GeothermalHeatRig() { ResetUtilityPowerState(); ResetTubePowerVehicles(); }
	/** @param rate_w Explicit isolated natural-source input for fractional and malformed-binding tests; restore through geology's public staging boundary without casting away live immutability. */
	void SeedRate(uint64_t rate_w)
	{
		auto snapshot = CaptureGeologyState(); snapshot.vents.front().thermal_capacity_w = rate_w;
		GeologyWorldState candidate; REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None);
		StageLoadedGeologyState(std::move(candidate)); AcceptLoadedGeologyState();
	}
	/** Complete the actual funded native collector through the whole delivered bill and synchronized construction dispatcher. */
	void Complete()
	{
		REQUIRE(this->Fund().Succeeded());
		// Construction receives exactly the frozen bill through Industry stock.
		// No free completion flag or geothermal-powered bootstrap is used.
		for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector)->inputs) {
			REQUIRE(this->Deliver(input.label, input.quantity) == input.quantity);
		}
		this->Tick(296); REQUIRE(IsGeothermalCollectorOperational(this->Collector().index));
	}
};

/** Typed actual company expense participates in shared heat use; it claims no generator output or operating recipe. */
class GeothermalHeatExpense : public UtilityCompanyWork {
public:
	/** @param owner Actual company paying the whole boundary expense. */
	explicit GeothermalHeatExpense(CompanyID owner) : UtilityCompanyWork(owner) {}
	/** @return Thermal quantity domain of the actual whole expense. */
	UtilityEnergyDomain EnergyDomain() const override { return UtilityEnergyDomain::Heat; }
	/** @return This test's immutable whole generator-role expense. */
	std::optional<UtilityHeatUse> ThermalUse() const override { return UtilityHeatUse::ElectricalGenerator; }
};

/** Cold/unfinished claims and pure estimates never create meters; only actual completed machinery publishes its original sustainable rate once. */
TEST_CASE("Actual collector publishes one shared bounded offer after completed construction", "[geothermal-heat-source]")
{
	GeothermalHeatRig rig;
	CHECK_FALSE(HasGeothermalHeatSources());
	const auto empty = ComputeUtilityPowerDigest();
	CHECK(PublishGeothermalCollectorHeat(IndustryID::Invalid(), 1, 74) == UtilityPowerError::InvalidReference);
	REQUIRE(rig.Fund().Succeeded()); const auto id = rig.Collector().index;
	CHECK(PublishGeothermalCollectorHeat(id, 1, 74) == UtilityPowerError::Refused); CHECK(ComputeUtilityPowerDigest() == empty);
	// The real construction owner retains stock/work throughout shortage.
	// Delivering the bill then dispatching296 ticks earns actual readiness.
	for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector)->inputs) REQUIRE(rig.Deliver(input.label, input.quantity) == input.quantity);
	rig.Tick(296); REQUIRE(HasGeothermalHeatSources());
	const auto natural = ComputeGeologyStateDigest(GetGeologyWorldState()), construction = ComputeGeothermalCollectorDigest();
	const auto cash = Company::Get(_current_company)->money;
	CHECK(PublishGeothermalCollectorHeat(id, 2, 74) == UtilityPowerError::InvalidInterval);
	CHECK(PublishGeothermalCollectorHeat(id, 1, 0) == UtilityPowerError::InvalidQuantity);
	CHECK(PublishGeothermalCollectorHeat(id, 1, MAX_UTILITY_INTERVAL_TICKS + 1) == UtilityPowerError::InvalidQuantity);
	CHECK(PublishGeothermalCollectorHeat(id, 1, 74, false) == UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == empty); CHECK(ComputeGeothermalCollectorDigest() == construction);
	REQUIRE(PublishGeothermalCollectorHeat(id, 1, 74) == UtilityPowerError::None);
	const auto producer = GetGeothermalCollectorState(id)->heat_producer; const auto published = ComputeUtilityPowerDigest();
	REQUIRE(GetUtilityPowerProducer(producer) != nullptr); CHECK(GetUtilityPowerProducer(producer)->meter.generation_energy_j == 1998000);
	CHECK(GetUtilityPowerProducer(producer)->meter.remainder_mj == 0); CHECK(GetUtilityPowerProducer(producer)->meter.accounting_interval == 1);
	CHECK(PublishGeothermalCollectorHeat(id, 1, 74) == UtilityPowerError::DuplicatePublication);
	CHECK(PublishGeothermalCollectorHeat(id, 1, 74, false) == UtilityPowerError::DuplicatePublication);
	CHECK(ComputeUtilityPowerDigest() == published); CHECK(CaptureUtilityPowerState().producers.size() == 1);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural);
}

/** One common driver rotates both domains once, with native pause/date waits and no capsule prerequisite or thermal consumer inferred from proximity. */
TEST_CASE("Collector only simulation shares saved utility tick and curtails unused heat", "[geothermal-heat-source]")
{
	GeothermalHeatRig rig; rig.Complete(); const auto id = rig.Collector().index;
	CHECK(CaptureTubePowerVehicles().empty()); PrepareTubePowerTick();
	const auto producer = GetGeothermalCollectorState(id)->heat_producer; REQUIRE(GetUtilityPowerProducer(producer) != nullptr);
	CHECK(IsUtilityHeatAllocated()); CHECK(IsUtilityPowerAllocated()); CHECK(CaptureUtilityPowerState().consumers.empty());
	const auto digest = ComputeUtilityPowerDigest(); PrepareTubePowerTick(); CHECK(ComputeUtilityPowerDigest() == digest);
	// Seventy-four actual synchronized one-tick opportunities equal the frozen
	// whole interval offer; unrequested joules are curtailed on each rotation.
	for (uint tick = 1; tick < 74; ++tick) { ++TimerGameTick::counter; PrepareTubePowerTick(); }
	CHECK(GetUtilityPowerProducer(producer)->generated_lifetime_j == 1998000); CHECK(GetUtilityPowerProducer(producer)->reserved_j == 0);
	CHECK(GetUtilityPowerProducer(producer)->meter.generation_energy_j == 27000); CHECK(GetUtilityPowerInterval() == 74);
	const auto before_pause = ComputeUtilityPowerDigest();
	{
		AutoRestoreBackup paused(_pause_mode, PauseModes{PauseMode::Normal}); ++TimerGameTick::counter; PrepareTubePowerTick();
		CHECK(ComputeUtilityPowerDigest() == before_pause); CHECK(PublishGeothermalCollectorHeat(id, 74, 1) == UtilityPowerError::Refused);
	}
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31);
	PrepareTubePowerTick(); CHECK(GetUtilityPowerProducer(producer)->meter.generation_energy_j == 0);
	CHECK(GetUtilityPowerProducer(producer)->generated_lifetime_j == 1998000);
	TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1);
	++TimerGameTick::counter; PrepareTubePowerTick(); CHECK(GetUtilityPowerProducer(producer)->generated_lifetime_j == 2025000);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	CHECK(ValidateGeothermalCollectors(CaptureGeothermalCollectors()));
}

/** Explicit physical proposals use actual shared identities and immutable roles; their ports do not create heat eligibility. */
TEST_CASE("Generator and direct heat request admission preserves shared role owner date and endpoint", "[geothermal-heat-request]")
{
	GeothermalHeatRig rig; rig.Complete();
	const auto direct = RegisterUtilityHeatConsumer(_current_company, UtilityHeatUse::DirectRecipe, 100, {}); REQUIRE(direct.has_value());
	const auto generator = RegisterUtilityHeatConsumer(_current_company, UtilityHeatUse::ElectricalGenerator, 100, {}); REQUIRE(generator.has_value());
	const auto electric = RegisterUtilityPowerConsumer(_current_company, MapLayer::DeepUnderground, UtilityConsumerEffect::TubeWork, 100, {}); REQUIRE(electric.has_value());
	const auto endpoint = TileAddWrap(rig.request.tile, -1, 0);
	GeothermalHeatRequest request{_current_company, *direct, endpoint, UtilityHeatUse::DirectRecipe, 70};
	const auto before = ComputeUtilityPowerDigest(); REQUIRE(PrepareGeothermalHeatRequest(request).has_value());
	request.consumer = *generator; request.use = UtilityHeatUse::ElectricalGenerator; REQUIRE(PrepareGeothermalHeatRequest(request).has_value());
	SECTION("wrong benefit") { request.use = UtilityHeatUse::DirectRecipe; }
	SECTION("electric identity") { request.consumer = *electric; }
	SECTION("missing identity") { request.consumer = UtilityConsumerID{UINT32_MAX}; }
	SECTION("foreign owner") { request.owner = CompanyID::Invalid(); }
	SECTION("quantity exceeds capacity") { request.needed_j = 101; }
	SECTION("Surface port") { request.endpoint = TileXYL(10, 10, MapLayer::Surface); }
	SECTION("solid Deep port") { request.endpoint = TileXYL(2, 2, MapLayer::DeepUnderground); }
	SECTION("locked date") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	CHECK_FALSE(PrepareGeothermalHeatRequest(request).has_value()); CHECK(ComputeUtilityPowerDigest() == before);
}

/** Shared reservations, actual whole expense, unused release and reconstruction preserve original source identity and consumed history. */
TEST_CASE("Collector removal expires unused grants and rebuild uses a fresh shared source", "[geothermal-heat-accounting]")
{
	GeothermalHeatRig rig; rig.Complete(); const auto id = rig.Collector().index;
	REQUIRE(PublishGeothermalCollectorHeat(id, 1, 1) == UtilityPowerError::None);
	const auto producer = GetGeothermalCollectorState(id)->heat_producer;
	const auto consumer = RegisterUtilityHeatConsumer(_current_company, UtilityHeatUse::ElectricalGenerator, 20000, std::array{producer}); REQUIRE(consumer.has_value());
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityHeat(1, std::array{UtilityPowerRequest{*consumer, 20000}}) == UtilityPowerError::None);
		REQUIRE(AllocateUtilityPower(1, {}) == UtilityPowerError::None);
	}
	GeothermalHeatExpense expense(_current_company);
	const auto proposal = PrepareUtilityHeatMutation(UtilityMutationKind::Consume, {*consumer, 1}, 5000);
	REQUIRE(CommitUtilityMutation(proposal, &expense).error == UtilityPowerError::None);
	const auto release = PrepareUtilityHeatMutation(UtilityMutationKind::ReleaseUnused, {*consumer, 1}, 3000);
	REQUIRE(CommitUtilityMutation(release).error == UtilityPowerError::None); CHECK(GetUtilityHeatAllocation(*consumer)->granted_energy_j == 12000);
	const auto natural = ComputeGeologyStateDigest(GetGeologyWorldState());
	delete &rig.Collector();
	CHECK_FALSE(GetUtilityPowerProducer(producer)->active); CHECK(GetUtilityPowerProducer(producer)->consumed_lifetime_j == 5000);
	CHECK(GetUtilityHeatAllocation(*consumer)->granted_energy_j == 0); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == natural);
	const auto claim = ClaimGeothermalSite(rig.request); REQUIRE(claim.error == GeothermalError::None); rig.site = claim.id;
	rig.Complete();
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	REQUIRE(PublishGeothermalCollectorHeat(rig.Collector().index, 2, 1) == UtilityPowerError::None);
	CHECK(GetGeothermalCollectorState(rig.Collector().index)->heat_producer != producer);
	CHECK(GetUtilityPowerProducer(producer)->consumed_lifetime_j == 5000); CHECK(GetUtilityPowerConsumer(*consumer)->linked_producers.empty());
	CHECK(ValidateGeothermalCollectors(CaptureGeothermalCollectors()));
}

/** Restored associations cannot reinterpret electrical, missing, unfinished or inflated meters as usable completed collector output. */
TEST_CASE("Collector shared heat references validate without changing live authority", "[geothermal-heat-stage]")
{
	GeothermalHeatRig rig; rig.Complete(); const auto id = rig.Collector().index;
	REQUIRE(PublishGeothermalCollectorHeat(id, 1, 1) == UtilityPowerError::None);
	const auto rows = CaptureGeothermalCollectors(); const auto digest = ComputeGeothermalCollectorDigest();
	StageLoadedGeothermalCollectors(rows); REQUIRE(AcceptLoadedGeothermalCollectors()); CHECK(ComputeGeothermalCollectorDigest() == digest);
	auto bad = rows;
	SECTION("absent meter") { bad[0].heat_producer = UtilityProducerID{3000}; }
	SECTION("electrical meter") {
		const auto electric = RegisterUtilityPowerProducer(_current_company, MapLayer::DeepUnderground, UtilityGenerationKind::External); REQUIRE(electric.has_value());
		bad[0].heat_producer = *electric;
	}
	SECTION("unfinished machinery") { bad[0].work_ticks = 295; }
	SECTION("inflated natural rate") { rig.SeedRate(1000001); }
	CHECK_FALSE(ValidateGeothermalCollectors(bad)); StageLoadedGeothermalCollectors(bad); CHECK_FALSE(AcceptLoadedGeothermalCollectors());
	CHECK(ComputeGeothermalCollectorDigest() == digest);
}

/** Fractional natural capacities use the same saved shared meter across subdivision; no collector-local remainder can multiply a small offer. */
TEST_CASE("Actual collector source preserves the shared millijoule remainder", "[geothermal-heat-source]")
{
	GeothermalHeatRig rig; rig.Complete(); const auto id = rig.Collector().index;
	// Explicitly seed a valid small natural source before its first meter exists.
	// This isolates fractional integration; normal native geology remains1MW.
	rig.SeedRate(17);
	const auto integrated = IntegrateUtilityGeneration(17, 37, 0); REQUIRE(integrated.has_value());
	UtilityProducerID producer{UINT32_MAX};
	// Close both shared phases and rotate every one-tick subdivision, retaining
	// only the source's fractional carry while curtailing unrequested whole heat.
	for (uint interval = 1; interval <= 37; ++interval) {
		REQUIRE(PublishGeothermalCollectorHeat(id, interval, 1) == UtilityPowerError::None);
		producer = GetGeothermalCollectorState(id)->heat_producer;
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityHeat(interval, {}) == UtilityPowerError::None);
		REQUIRE(AllocateUtilityPower(interval, {}) == UtilityPowerError::None);
		if (interval < 37) REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
	}
	CHECK(GetUtilityPowerProducer(producer)->generated_lifetime_j == integrated->energy_j);
	CHECK(GetUtilityPowerProducer(producer)->meter.remainder_mj == integrated->remainder_mj);
	CHECK(integrated->energy_j == 16); CHECK(integrated->remainder_mj == 983);
	CHECK(ValidateGeothermalCollectors(CaptureGeothermalCollectors()));
}
