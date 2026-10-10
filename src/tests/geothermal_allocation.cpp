/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_allocation.cpp Actual paid collector/path/shared reservation and whole-expense allocation boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group3
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.3; actual operating plants retain10.6
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_collector_fixture.h"
#include "utility_power_fixture.h"
#include "../geothermal_heat.h"
#include "../geothermal_utility.h"
#include "../geothermal_utility_cmd.h"
#include "../safeguards.h"

/** Native paid source/path with explicit unit-test floor and delivered construction inputs, without an operating generator or copied stock. */
class AllocationRig : public GeothermalCollectorRig {
	AutoRestoreBackup<PauseModes> pause{_pause_mode, PauseModes{}}; ///< Actual unpaused native source policy, restored on teardown.
public:
	TileIndex endpoint = INVALID_TILE; ///< Explicit installed consumer port beyond the actual collector floor.
	UtilityProducerID producer{UINT32_MAX}; ///< Actual completed source's shared identity after publication.
	/** Complete actual funded machinery and install a paid cardinal source-to-consumer path; no heat is published yet. */
	AllocationRig()
	{
		ResetUtilityPowerState(); REQUIRE(this->Fund().Succeeded());
		for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector)->inputs) REQUIRE(this->Deliver(input.label, input.quantity) == input.quantity); // Exactly the frozen delivered bill precedes paid work.
		this->Tick(296); REQUIRE(IsGeothermalCollectorOperational(this->Collector().index));
		this->endpoint = TileAddWrap(this->request.tile, 1, -1);
		for (const auto tile : {this->request.endpoint, TileAddWrap(this->request.tile, 0, -1), this->endpoint}) this->Build(tile); // Every physical source/consumer path cell is paid through the ordinary synchronized kernel.
	}
	/** Reset shared/physical references before their actual native company and machinery leave the fixture. */
	~AllocationRig() { ResetUtilityPowerState(); }
	/** @param tile Actual fixture-open floor cell. Install one actual paid Heat cell without changing its native metadata. */
	void Build(TileIndex tile)
	{
		REQUIRE(Command<Commands::BuildGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile, UtilityEnergyDomain::Heat)).Succeeded());
	}
	/** @param ticks Actual common interval duration. Publish once from the original completed collector. */
	void Publish(uint32_t ticks = 1)
	{
		REQUIRE(PublishGeothermalCollectorHeat(this->Collector().index, GetUtilityPowerInterval(), ticks) == UtilityPowerError::None);
		this->producer = GetGeothermalCollectorState(this->Collector().index)->heat_producer;
	}
	/** @param use Immutable complete benefit. @param quantity Feasible requested joules. @param port Explicit installed port, or the fixture's connected endpoint. @return Actual issued thermal consumer request. */
	GeothermalHeatRequest Consumer(UtilityHeatUse use, uint64_t quantity, TileIndex port = INVALID_TILE)
	{
		if (port == INVALID_TILE) port = this->endpoint;
		const auto id = RegisterGeothermalHeatConsumer(_current_company, port, use, 100000); REQUIRE(id.has_value());
		return {_current_company, *id, port, use, quantity};
	}
	/** @return A second actually funded/completed collector and explicit installed source port; the additional natural field/floor/materials are declared isolated unit inputs. */
	std::pair<IndustryID, TileIndex> SecondSource()
	{
		auto snapshot = CaptureGeologyState(); auto field = snapshot.vents.front();
		field.id = GeothermalVentID{snapshot.next_vent_id++};
		field.x = field.x < 32 ? 45 : 10; field.y = field.y < 32 ? 45 : 10;
		snapshot.vents.push_back(field); GeologyWorldState candidate;
		REQUIRE(PrepareGeologyRestoration(snapshot, candidate) == GeologyError::None); StageLoadedGeologyState(std::move(candidate)); AcceptLoadedGeologyState();
		GeothermalSiteRequest request{field.id, _current_company, MultiLayerGeothermalConstructionKind::Collector, TileXYL(field.x, field.y, field.layer), TileXYL(field.x, field.y, field.layer), DiagDirection::SE};
		std::vector<TileIndex> floor;
		// The second source receives a complete explicitly supplied fixture
		// floor and a separate real bill/work cycle; no duplicate meter grants
		// another building free output from the original vent.
		for (int y = -1; y <= 3; ++y) {
			for (int x = -1; x <= 3; ++x) floor.push_back(TileAddWrap(request.tile, x, y));
		}
		this->Open(floor); const auto claim = ClaimGeothermalSite(request); REQUIRE(claim.error == GeothermalError::None);
		REQUIRE(Command<Commands::BuildGeothermalCollector>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, request.tile, std::make_tuple(request.tile, claim.id, uint32_t{123})).Succeeded());
		const auto industry = FindGeothermalCollectorForVent(field.id); REQUIRE(Industry::IsValidID(industry));
		for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector)->inputs) REQUIRE(ReceiveGeothermalCollectorCargo(*Industry::Get(industry), GetCargoTypeByLabel(input.label), input.quantity) == input.quantity); // Independent actual native input stock earns readiness, without geothermal bootstrap.
		this->Tick(296); REQUIRE(IsGeothermalCollectorOperational(industry));
		std::vector<TileIndex> path{request.endpoint, TileAddWrap(request.endpoint, -1, 0)};
		auto tile = path.back(); const int target_y = static_cast<int>(TilePlaneY(this->endpoint));
		// Exit the new machinery's sole port before walking its outside column.
		// Join the original paid consumer row through explicit cardinal cells;
		// no plane sweep or coordinate catchment supplies the shared bottleneck.
		while (static_cast<int>(TilePlaneY(tile)) != target_y) { tile = TileAddWrap(tile, 0, static_cast<int>(TilePlaneY(tile)) < target_y ? 1 : -1); path.push_back(tile); }
		const int target_x = static_cast<int>(TileX(this->endpoint));
		while (static_cast<int>(TileX(tile)) != target_x) { tile = TileAddWrap(tile, static_cast<int>(TileX(tile)) < target_x ? 1 : -1, 0); path.push_back(tile); }
		this->Open(path);
		for (const auto cell : path) {
			if (!GetGeothermalUtilityLink(cell, UtilityEnergyDomain::Heat).has_value()) this->Build(cell); // Shared already-paid cells are reused, while every new path cell gets its own real installation charge.
		}
		return {industry, request.endpoint};
	}
};

/** A real complete company expense implements the thermal participant contract; it asserts no generator/recipe output before its later native producer. */
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
		return this->feasible && request.owner == this->owner && request.use == this->use && Company::Get(this->owner)->money >= Money{5};
	}
	/** @param proposal Exact original native source reservation. @return Whole actual work admission without mutation. */
	bool Validate(const UtilityMutationProposal &proposal) const override { return this->work.Validate(proposal); }
	/** @param proposal Exact already checked original quantity. @return Complete actual expense or unchanged refusal. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override { return this->work.Commit(proposal); }
};

/** @param left First actual demand. @param right Second actual demand. @return Original shared identity order for exhaustive bounded request permutations. */
static bool DemandOrder(const GeothermalHeatDemand &left, const GeothermalHeatDemand &right) { return left.request.consumer < right.request.consumer; }

/** Stable native identity priority applies across both roles, with exact path/source limits and complete consumed-plus-reserved conservation. */
TEST_CASE("Connected two generators and direct consumer share one actual vent budget", "[geothermal-heat-allocation]")
{
	AllocationRig rig; rig.Publish();
	RoutedExpense first(_current_company, UtilityHeatUse::ElectricalGenerator), second(_current_company, UtilityHeatUse::ElectricalGenerator), direct(_current_company, UtilityHeatUse::DirectRecipe);
	std::array demands{GeothermalHeatDemand{rig.Consumer(UtilityHeatUse::ElectricalGenerator, 10000), &first},
		GeothermalHeatDemand{rig.Consumer(UtilityHeatUse::ElectricalGenerator, 10000), &second}, GeothermalHeatDemand{rig.Consumer(UtilityHeatUse::DirectRecipe, 10000), &direct}};
	const auto native = ComputeUtilityPowerDigest(), physical = ComputeGeothermalHeatRouteDigest(); const auto cash = Company::Get(_current_company)->money;
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		// All request permutations produce the same three source slices, exact
		// installed paths and priority; preparing never debits live owners.
		do {
			const auto plan = PrepareVentHeatAllocation(1, demands); REQUIRE(plan.error == UtilityPowerError::None); REQUIRE(plan.shares.size() == 3);
			CHECK(plan.shares[0].quantity_j == 10000); CHECK(plan.shares[1].quantity_j == 10000); CHECK(plan.shares[2].quantity_j == 7000);
			CHECK(plan.routes.size() == 3); CHECK(plan.routes[0].cells == plan.routes[1].cells); CHECK(plan.routes[1].cells == plan.routes[2].cells);
		} while (std::next_permutation(demands.begin(), demands.end(), DemandOrder));
		CHECK(ComputeUtilityPowerDigest() == native); CHECK(ComputeGeothermalHeatRouteDigest() == physical);
		REQUIRE(AllocateVentHeat(1, demands, false) == UtilityPowerError::None); CHECK(ComputeUtilityPowerDigest() == native);
		REQUIRE(AllocateVentHeat(1, demands) == UtilityPowerError::None);
		CHECK(AllocateVentHeat(1, demands) != UtilityPowerError::None);
	}
	CHECK(Company::Get(_current_company)->money == cash);
	REQUIRE(ConsumeGeothermalHeat({demands[0].request.consumer, 1}, 10000, first).error == UtilityPowerError::None);
	REQUIRE(ConsumeGeothermalHeat({demands[1].request.consumer, 1}, 10000, second).error == UtilityPowerError::None);
	CHECK(ConsumeGeothermalHeat({demands[2].request.consumer, 1}, 10000, direct).error != UtilityPowerError::None);
	REQUIRE(ConsumeGeothermalHeat({demands[2].request.consumer, 1}, 7000, direct).error == UtilityPowerError::None);
	const auto *source = GetUtilityPowerProducer(rig.producer); CHECK(source->consumed_lifetime_j == 27000); CHECK(source->reserved_j == 0);
	CHECK(source->consumed_lifetime_j + source->reserved_j <= source->generated_lifetime_j); CHECK(Company::Get(_current_company)->money == cash - Money{15});
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Feasible zero/partial/disconnected demand is distinct from refusing whole work; none can gain energy from endpoint proximity or output animation. */
TEST_CASE("Connected heat respects zero partial disconnected and infeasible whole demand", "[geothermal-heat-allocation]")
{
	AllocationRig rig; RoutedExpense expense(_current_company, UtilityHeatUse::ElectricalGenerator);
	auto request = rig.Consumer(UtilityHeatUse::ElectricalGenerator, 20000);
	uint64_t expected = 20000;
	SECTION("zero supply") { expected = 0; }
	SECTION("partial request") { rig.Publish(); request.needed_j = expected = 1234; }
	SECTION("zero request") { rig.Publish(); request.needed_j = expected = 0; }
	SECTION("disconnected installed port") {
		rig.Publish(); const auto port = TileAddWrap(rig.request.tile, 3, 3); rig.Build(port);
		request = rig.Consumer(UtilityHeatUse::ElectricalGenerator, 20000, port); expected = 0;
	}
	SECTION("whole input output or auxiliary refusal") { rig.Publish(); expense.feasible = false; expected = 0; }
	SECTION("missing whole participant") { rig.Publish(); expected = 0; }
	const auto before = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	GeothermalHeatDemand demand{request, &expense};
	if (expected == 0 && rig.producer.base() != UINT32_MAX && expense.feasible && request.endpoint == rig.endpoint && request.needed_j != 0) demand.participant = nullptr; // This section models an absent full consumer owner, with no useful work admission.
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		const auto plan = PrepareVentHeatAllocation(1, std::array{demand}); REQUIRE(plan.error == UtilityPowerError::None); CHECK(ComputeUtilityPowerDigest() == before);
		REQUIRE(AllocateVentHeat(1, std::array{demand}) == UtilityPowerError::None);
	}
	CHECK(GetUtilityPowerConsumer(request.consumer)->meter.granted_energy_j == expected); CHECK(Company::Get(_current_company)->money == cash);
	if (!expense.feasible) CHECK(GetUtilityPowerConsumer(request.consumer)->meter.demand_energy_j == 0);
}

/** Complete owner refusal and role mismatch cannot settle any part of an already reserved physical benefit. */
TEST_CASE("Physical heat use refuses whole work and retains exact unused source lineage", "[geothermal-heat-allocation]")
{
	AllocationRig rig; rig.Publish(); RoutedExpense expense(_current_company, UtilityHeatUse::ElectricalGenerator);
	const auto request = rig.Consumer(UtilityHeatUse::ElectricalGenerator, 20000);
	{ AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(AllocateVentHeat(1, std::array{GeothermalHeatDemand{request, &expense}}) == UtilityPowerError::None); }
	const auto before = ComputeUtilityPowerDigest(); const auto cash = Company::Get(_current_company)->money;
	expense.work.commit = false; CHECK(ConsumeGeothermalHeat({request.consumer, 1}, 10000, expense).error != UtilityPowerError::None);
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(Company::Get(_current_company)->money == cash);
	expense.work.commit = true; expense.work.admit = false;
	CHECK(ConsumeGeothermalHeat({request.consumer, 1}, 10000, expense).error != UtilityPowerError::None); CHECK(ComputeUtilityPowerDigest() == before);
	expense.work.admit = true; RoutedExpense wrong(_current_company, UtilityHeatUse::DirectRecipe);
	CHECK(ConsumeGeothermalHeat({request.consumer, 1}, 10000, wrong).error != UtilityPowerError::None); CHECK(ComputeUtilityPowerDigest() == before);
	REQUIRE(ConsumeGeothermalHeat({request.consumer, 1}, 10000, expense).error == UtilityPowerError::None);
	CHECK(GetUtilityPowerProducer(rig.producer)->reserved_j == 10000); CHECK(GetUtilityPowerProducer(rig.producer)->consumed_lifetime_j == 10000);
}

/** Independent copied validation refuses lost source/path references and interval capacity inflation without borrowing live geometry or quantities. */
TEST_CASE("Exact physical heat paths stage atomically against original shared reservations", "[geothermal-heat-route-stage]")
{
	AllocationRig rig; rig.Publish(); RoutedExpense expense(_current_company, UtilityHeatUse::DirectRecipe); const auto request = rig.Consumer(UtilityHeatUse::DirectRecipe, 20000);
	{ AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(AllocateVentHeat(1, std::array{GeothermalHeatDemand{request, &expense}}) == UtilityPowerError::None); }
	REQUIRE(ConsumeGeothermalHeat({request.consumer, 1}, 1000, expense).error == UtilityPowerError::None);
	const auto native = CaptureUtilityPowerState(); const auto bindings = CaptureGeothermalHeatConsumers(); auto routes = CaptureGeothermalHeatRoutes();
	const auto before = ComputeGeothermalHeatRouteDigest(); const std::map ports{std::pair{rig.producer, rig.request.endpoint}};
	REQUIRE(ValidateGeothermalHeatRouteState(bindings, routes, 1, native, 64, 64, 4, ports));
	SECTION("wrong copied geometry") { CHECK_FALSE(ValidateGeothermalHeatRouteState(bindings, routes, 1, native, 128, 64, 4, ports)); return; }
	SECTION("exact complete reload") {
		StageLoadedGeothermalHeatState(bindings, routes, 1); REQUIRE(AcceptLoadedGeothermalHeatState()); CHECK(ComputeGeothermalHeatRouteDigest() == before); return;
	}
	SECTION("missing source path") { routes.clear(); }
	SECTION("missing cell") { routes.front().cells.erase(routes.front().cells.begin() + 1); }
	SECTION("repeated cell") { routes.front().cells.push_back(routes.front().cells.back()); }
	SECTION("foreign source") { routes.front().producer = UtilityProducerID{UINT32_MAX}; }
	SECTION("reversed path") { std::ranges::reverse(routes.front().cells); }
	CHECK_FALSE(ValidateGeothermalHeatRouteState(bindings, routes, 1, native, 64, 64, 4, ports));
	StageLoadedGeothermalHeatState(bindings, routes, 1); CHECK_FALSE(AcceptLoadedGeothermalHeatState()); CHECK(ComputeGeothermalHeatRouteDigest() == before);
}

/** Installed alternate routes remain deterministic when the shortest interior is unavailable; excluded cells never alter paid geometry. */
TEST_CASE("Residual heat routing selects an installed alternative around a saturated interior", "[geothermal-heat-routing]")
{
	AllocationRig rig; const auto middle = TileAddWrap(rig.request.tile, 0, -1);
	const auto alternate = TileAddWrap(rig.request.tile, -1, -1);
	// Explicit fixture-open extra floor supplies only test geometry. All extra
	// physical capacity is separately installed and paid, with no source grant.
	std::array extra{TileAddWrap(rig.request.tile, -1, -2), TileAddWrap(rig.request.tile, 0, -2), TileAddWrap(rig.request.tile, 1, -2)}; rig.Open(extra);
	rig.Build(TileAddWrap(rig.request.tile, -1, 0)); rig.Build(alternate);
	for (const auto tile : extra) rig.Build(tile); // Cardinal bypass requires each paid cell, including the extra explicitly opened floor.
	const auto id = GetGeothermalUtilityLink(middle, UtilityEnergyDomain::Heat)->id;
	const auto native = ComputeUtilityPowerDigest();
	const auto path = FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, rig.request.endpoint, rig.endpoint, std::array{id}); REQUIRE(path.has_value());
	CHECK(std::ranges::find(path->cells, id) == path->cells.end()); CHECK(path->cells.size() == 7); CHECK(ComputeUtilityPowerDigest() == native);
	CHECK_FALSE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, rig.request.endpoint, rig.endpoint, std::array{GetGeothermalUtilityLink(rig.request.endpoint, UtilityEnergyDomain::Heat)->id}).has_value());
}

/** Two actual natural/constructed sources cannot multiply a shared inlet's capacity, while their separate ports can serve independent work in the same interval. */
TEST_CASE("Two native collectors conserve shared path residual capacity and interval duration", "[geothermal-heat-routing]")
{
	AllocationRig rig; const auto [industry, port] = rig.SecondSource(); rig.Publish();
	uint32_t second_ticks = 1;
	SECTION("shared pipe capacity") {}
	SECTION("mixed interval durations refuse") { second_ticks = 2; }
	REQUIRE(PublishGeothermalCollectorHeat(industry, 1, second_ticks) == UtilityPowerError::None);
	const auto second_source = GetGeothermalCollectorState(industry)->heat_producer;
	RoutedExpense generator(_current_company, UtilityHeatUse::ElectricalGenerator), direct(_current_company, UtilityHeatUse::DirectRecipe);
	const auto first = rig.Consumer(UtilityHeatUse::ElectricalGenerator, 60000), second = rig.Consumer(UtilityHeatUse::DirectRecipe, 27000, port);
	std::array demands{GeothermalHeatDemand{second, &direct}, GeothermalHeatDemand{first, &generator}};
	const auto before = ComputeUtilityPowerDigest();
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		if (second_ticks != 1) {
			CHECK(AllocateVentHeat(1, demands) == UtilityPowerError::InvalidInterval); CHECK(ComputeUtilityPowerDigest() == before); return;
		}
		REQUIRE(AllocateVentHeat(1, demands) == UtilityPowerError::None);
	}
	CHECK(GetUtilityPowerConsumer(first.consumer)->meter.granted_energy_j == 27000); CHECK(GetUtilityPowerConsumer(first.consumer)->meter.unmet_energy_j == 33000);
	CHECK(GetUtilityPowerConsumer(second.consumer)->meter.granted_energy_j == 27000);
	CHECK(GetUtilityPowerProducer(rig.producer)->reserved_j == 27000); CHECK(GetUtilityPowerProducer(second_source)->reserved_j == 27000);
	REQUIRE(ConsumeGeothermalHeat({first.consumer, 1}, 27000, generator).error == UtilityPowerError::None);
	REQUIRE(ConsumeGeothermalHeat({second.consumer, 1}, 27000, direct).error == UtilityPowerError::None);
	CHECK(GetUtilityPowerProducer(rig.producer)->consumed_lifetime_j + GetUtilityPowerProducer(second_source)->consumed_lifetime_j == 54000);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}
