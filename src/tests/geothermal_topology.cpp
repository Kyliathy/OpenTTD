/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_topology.cpp Actual paid link loss/repaired allocation and native company lifecycle preserve original consumed heat.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-master-plan.md checkpoint10.3; operating plants remain later producers
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_collector_fixture.h"
#include "utility_power_fixture.h"
#include "../geothermal_heat.h"
#include "../geothermal_utility.h"
#include "../geothermal_utility_cmd.h"
#include "../safeguards.h"

/** @param tile Actual fixture-open cell. @param remove Retire instead of install. @param estimate Quote without cash/geometry/energy changes. @param domain Immutable thermal/electrical network. @return Ordinary native command-kernel result, including once-only complete capital/removal charge. */
static CommandCost TopologyCell(TileIndex tile, bool remove = false, bool estimate = false, UtilityEnergyDomain domain = UtilityEnergyDomain::Heat)
{
	if (remove) return Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate, tile, std::make_tuple(tile, domain));
	return Command<Commands::BuildGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate, tile, std::make_tuple(tile, domain));
}

/** Actual completed native collector and paid four-cell path; supplied floor/materials remain explicit unit inputs rather than a bootstrap claim. */
class TopologyRig : public GeothermalCollectorRig {
	AutoRestoreBackup<PauseModes> pause{_pause_mode, PauseModes{}}; ///< Actual unpaused source policy, restored after isolated checks.
public:
	TileIndex bridge = INVALID_TILE; ///< Actual paid interior cell whose loss splits the source path.
	TileIndex endpoint = INVALID_TILE; ///< Actual paid consumer port outside the native collector floor.
	UtilityProducerID producer{UINT32_MAX}; ///< Actual original collector's shared Heat source.
	/** Fund and complete real machinery, install every paid path cell and publish its single original shared source offer. */
	TopologyRig()
	{
		ResetUtilityPowerState(); REQUIRE(this->Fund().Succeeded());
		for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector)->inputs) REQUIRE(this->Deliver(input.label, input.quantity) == input.quantity); // Exactly the complete delivered native bill earns paid synchronized work.
		this->Tick(296); REQUIRE(IsGeothermalCollectorOperational(this->Collector().index));
		this->bridge = TileAddWrap(this->request.tile, -1, 1); this->endpoint = TileAddWrap(this->request.tile, -1, 2);
		for (const auto tile : {this->request.endpoint, TileAddWrap(this->request.tile, -1, 0), this->bridge, this->endpoint}) REQUIRE(TopologyCell(tile).Succeeded()); // No endpoint proximity or free path capacity replaces an actual native installation.
		this->Publish(); this->producer = GetGeothermalCollectorState(this->Collector().index)->heat_producer;
	}
	/** Clear fixture utility/physical references before the actual native machinery/company are removed. */
	~TopologyRig() { ResetUtilityPowerState(); }
	/** Publish one actual synchronized tick from the original completed collector in the current open native interval. */
	void Publish() { REQUIRE(PublishGeothermalCollectorHeat(this->Collector().index, GetUtilityPowerInterval(), 1) == UtilityPowerError::None); }
	/** @param quantity Positive feasible requested joules. @return Real registered shared generator-role demand at the installed consumer port. */
	GeothermalHeatRequest Consumer(uint64_t quantity = 20000)
	{
		const auto id = RegisterGeothermalHeatConsumer(_current_company, this->endpoint, UtilityHeatUse::ElectricalGenerator, 100000); REQUIRE(id.has_value());
		return {_current_company, *id, this->endpoint, UtilityHeatUse::ElectricalGenerator, quantity};
	}
	/** @param request Actual registered owned demand. @param participant Complete actual work owner. Close each domain's native phase once, preserving independent electrical service. */
	void Allocate(const GeothermalHeatRequest &request, GeothermalHeatParticipant &participant)
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateVentHeat(GetUtilityPowerInterval(), std::array{GeothermalHeatDemand{request, &participant}}) == UtilityPowerError::None);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), {}) == UtilityPowerError::None);
	}
};

/** Complete real company expense demonstrates atomic thermal benefit and lifecycle custody, without asserting operating plant output. */
class TopologyExpense final : public GeothermalHeatParticipant {
	CompanyID owner; ///< Actual paying company, never copied money authority.
	UtilityCompanyWork expense; ///< Actual whole native expense owner.
public:
	/** @param owner Actual company paying the complete five-unit action. */
	explicit TopologyExpense(CompanyID owner) : owner(owner), expense(owner) {}
	/** @return Immutable generator-role thermal benefit. */
	std::optional<UtilityHeatUse> ThermalUse() const override { return UtilityHeatUse::ElectricalGenerator; }
	/** @param request Exact owned current demand. @return Actual whole funding readiness before heat reservation. */
	bool ValidateDemand(const GeothermalHeatRequest &request) const override { return request.owner == this->owner && Company::Get(this->owner)->money >= Money{5}; } // Scarce heat cannot reserve ahead of an unfunded whole action.
	/** @param proposal Exact original native source reservation. @return Complete actual work admission. */
	bool Validate(const UtilityMutationProposal &proposal) const override { return this->expense.Validate(proposal); }
	/** @param proposal Exact already checked native quantity. @return Complete actual expense or unchanged refusal. */
	bool Commit(const UtilityMutationProposal &proposal) noexcept override { return this->expense.Commit(proposal); }
};

/** Pure removal quotes preserve grants, while actual retirement invalidates an outstanding whole-use proposal and releases only unused original slices once. */
TEST_CASE("Paid heat bridge removal expires uncommitted native grants once", "[geothermal-heat-topology]")
{
	TopologyRig rig; TopologyExpense expense(_current_company); const auto request = rig.Consumer(); rig.Allocate(request, expense);
	const auto proposal = PrepareUtilityHeatMutation(UtilityMutationKind::Consume, {request.consumer, 1}, 5000); REQUIRE(proposal.error == UtilityPowerError::None);
	const auto before = ComputeUtilityPowerDigest(), physical = ComputeGeothermalHeatRouteDigest(); const auto cash = Company::Get(_current_company)->money;
	REQUIRE(TopologyCell(rig.bridge, true, true).Succeeded()); CHECK(ComputeUtilityPowerDigest() == before); CHECK(ComputeGeothermalHeatRouteDigest() == physical); CHECK(Company::Get(_current_company)->money == cash);
	REQUIRE(TopologyCell(rig.bridge, true).Succeeded()); CHECK(Company::Get(_current_company)->money == cash - Money{250});
	CHECK(GetUtilityPowerProducer(rig.producer)->reserved_j == 0); CHECK(GetUtilityPowerProducer(rig.producer)->consumed_lifetime_j == 0);
	CHECK(GetUtilityPowerConsumer(request.consumer)->shares.front().released_j == 20000); CHECK(GetUtilityHeatAllocation(request.consumer)->granted_energy_j == 0);
	CHECK(CommitUtilityMutation(proposal, &expense).error == UtilityPowerError::StaleMutation); CHECK(ComputeGeothermalHeatRouteDigest() == physical);
	const auto retired = ComputeUtilityPowerDigest(); CHECK(TopologyCell(rig.bridge, true).Failed()); CHECK(ComputeUtilityPowerDigest() == retired); CHECK(Company::Get(_current_company)->money == cash - Money{250});
	CHECK_FALSE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, rig.request.endpoint, rig.endpoint).has_value());
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Repaired fresh geometry cannot revive old rights; the next native allocation replans a new interval while original consumed history survives. */
TEST_CASE("Paid heat repair preserves consumed lineage and replans only fresh interval rights", "[geothermal-heat-topology]")
{
	TopologyRig rig; TopologyExpense expense(_current_company); const auto request = rig.Consumer(); rig.Allocate(request, expense);
	REQUIRE(ConsumeGeothermalHeat({request.consumer, 1}, 5000, expense).error == UtilityPowerError::None);
	const auto old = GetGeothermalUtilityLink(rig.bridge, UtilityEnergyDomain::Heat)->id; const auto physical = ComputeGeothermalHeatRouteDigest();
	REQUIRE(TopologyCell(rig.bridge, true).Succeeded()); CHECK(GetUtilityPowerProducer(rig.producer)->reserved_j == 0); CHECK(GetUtilityPowerProducer(rig.producer)->consumed_lifetime_j == 5000);
	CHECK(GetUtilityPowerConsumer(request.consumer)->shares.front().consumed_j == 5000); CHECK(GetUtilityPowerConsumer(request.consumer)->shares.front().released_j == 15000); CHECK(ComputeGeothermalHeatRouteDigest() == physical);
	const auto native = CaptureUtilityPowerState(); const auto bindings = CaptureGeothermalHeatConsumers(); const auto routes = CaptureGeothermalHeatRoutes();
	StageLoadedUtilityPowerState(native); REQUIRE(AcceptLoadedUtilityPowerState()); StageLoadedGeothermalHeatState(bindings, routes, 1); REQUIRE(AcceptLoadedGeothermalHeatState()); CHECK(ComputeGeothermalHeatRouteDigest() == physical);
	REQUIRE(TopologyCell(rig.bridge).Succeeded()); const auto replacement = GetGeothermalUtilityLink(rig.bridge, UtilityEnergyDomain::Heat)->id; CHECK(replacement != old);
	CHECK(ConsumeGeothermalHeat({request.consumer, 1}, 1, expense).error != UtilityPowerError::None); CHECK(ComputeGeothermalHeatRouteDigest() == physical);
	{ AutoRestoreBackup system(_current_company, OWNER_DEITY); REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None); }
	rig.Publish(); rig.Allocate(request, expense); REQUIRE(CaptureGeothermalHeatRoutes().size() == 1);
	const auto replanned = CaptureGeothermalHeatRoutes(); const auto &path = replanned.front().cells;
	CHECK(std::ranges::find(path, replacement) != path.end()); CHECK(std::ranges::find(path, old) == path.end());
	CHECK(GetUtilityPowerProducer(rig.producer)->reserved_j == 20000); CHECK(GetUtilityPowerProducer(rig.producer)->generated_lifetime_j == 54000); CHECK(GetUtilityPowerProducer(rig.producer)->consumed_lifetime_j == 5000);
	REQUIRE(ConsumeGeothermalHeat({request.consumer, 2}, 3000, expense).error == UtilityPowerError::None); CHECK(GetUtilityPowerProducer(rig.producer)->consumed_lifetime_j == 8000);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Retiring a coexisting electrical cell has no thermal lineage; physical domains remain independent even with identical coordinates. */
TEST_CASE("Electrical topology loss leaves separate physical heat rights intact", "[geothermal-heat-topology]")
{
	TopologyRig rig; TopologyExpense expense(_current_company); const auto request = rig.Consumer(); rig.Allocate(request, expense);
	REQUIRE(TopologyCell(rig.bridge, false, false, UtilityEnergyDomain::Electricity).Succeeded());
	REQUIRE(TopologyCell(rig.bridge, true, false, UtilityEnergyDomain::Electricity).Succeeded());
	CHECK(GetUtilityPowerProducer(rig.producer)->reserved_j == 20000); CHECK(GetUtilityPowerConsumer(request.consumer)->shares.front().released_j == 0);
	REQUIRE(ConsumeGeothermalHeat({request.consumer, 1}, 5000, expense).error == UtilityPowerError::None); CHECK(GetUtilityPowerProducer(rig.producer)->reserved_j == 15000);
}

/** A valid pre-invalidation393 compatibility input can retain a lost-cell promise; actual load settlement expires only unused source lineage and is idempotent. */
TEST_CASE("Legacy lost heat path reconciliation preserves original consumed quantities", "[geothermal-heat-topology-load]")
{
	TopologyRig rig; TopologyExpense expense(_current_company); const auto request = rig.Consumer(); rig.Allocate(request, expense);
	REQUIRE(ConsumeGeothermalHeat({request.consumer, 1}, 5000, expense).error == UtilityPowerError::None);
	auto legacy = CaptureUtilityPowerState(); const auto bindings = CaptureGeothermalHeatConsumers(); const auto routes = CaptureGeothermalHeatRoutes();
	const auto id = GetGeothermalUtilityLink(rig.bridge, UtilityEnergyDomain::Heat)->id;
	// Explicit compatibility input reproduces the previously valid393 overlay
	// retirement without its missing unused-right hook. No generation, grant,
	// expense, consumption or original path reference is fabricated or altered.
	legacy.links[id.base()].active = false; ++legacy.local_link_revision; ++legacy.operation_sequence;
	REQUIRE(ValidateUtilityPowerState(legacy, GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
	StageLoadedUtilityPowerState(legacy); REQUIRE(AcceptLoadedUtilityPowerState()); StageLoadedGeothermalHeatState(bindings, routes, 1); REQUIRE(AcceptLoadedGeothermalHeatState());
	const auto physical = ComputeGeothermalHeatRouteDigest(); RevalidateUtilityHeatRoutes();
	CHECK(GetUtilityPowerProducer(rig.producer)->reserved_j == 0); CHECK(GetUtilityPowerProducer(rig.producer)->consumed_lifetime_j == 5000);
	CHECK(GetUtilityPowerConsumer(request.consumer)->shares.front().released_j == 15000); CHECK(ComputeGeothermalHeatRouteDigest() == physical);
	CHECK(CaptureUtilityPowerState().operation_sequence == legacy.operation_sequence + 1);
	const auto settled = ComputeUtilityPowerDigest(); RevalidateUtilityHeatRoutes(); CHECK(ComputeUtilityPowerDigest() == settled);
}

/** Native whole-company takeover moves original source/consumer/cell control together; subsequent removal expires free rights once and keeps paid work history. */
TEST_CASE("Native company topology lifecycle preserves used heat without reviving unused grants", "[geothermal-heat-topology-owner]")
{
	TopologyRig rig; TopologyExpense original(_current_company); const auto request = rig.Consumer(); rig.Allocate(request, original);
	REQUIRE(Company::CanAllocateItem()); auto *successor = Company::Create(); successor->money = 1000000; const auto next = successor->index;
	ChangeOwnershipOfCompanyItems(rig.request.owner, next);
	CHECK(GetUtilityPowerProducer(rig.producer)->owner == next); CHECK(GetUtilityPowerConsumer(request.consumer)->owner == next); CHECK(GetUtilityPowerProducer(rig.producer)->reserved_j == 20000);
	{
		AutoRestoreBackup actor(_current_company, next); TopologyExpense transferred(next);
		REQUIRE(ConsumeGeothermalHeat({request.consumer, 1}, 5000, transferred).error == UtilityPowerError::None);
		ChangeOwnershipOfCompanyItems(next, INVALID_OWNER); CHECK(GetUtilityPowerProducer(rig.producer)->reserved_j == 0); CHECK(GetUtilityPowerProducer(rig.producer)->consumed_lifetime_j == 5000);
		CHECK(GetUtilityPowerConsumer(request.consumer)->shares.front().released_j == 15000); CHECK_FALSE(GetUtilityPowerConsumer(request.consumer)->active);
		CHECK_FALSE(FindGeothermalUtilityPath(next, UtilityEnergyDomain::Heat, rig.request.endpoint, rig.endpoint).has_value());
	}
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None); delete successor;
}

/** Derived component/path caches depend only on committed installed topology; residual exclusions and resource queries cannot poison unchanged canonical routes. */
TEST_CASE("Installed heat components and canonical paths rebuild only on committed topology", "[geothermal-heat-topology-cache]")
{
	TopologyRig rig;
	const auto path = FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, rig.request.endpoint, rig.endpoint); REQUIRE(path.has_value());
	const auto builds = GetGeothermalUtilityCacheBuildCount(), searches = GetGeothermalUtilityPathSearchCount(); const auto native = ComputeUtilityPowerDigest();
	for (uint query = 0; query < 5; ++query) REQUIRE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, rig.request.endpoint, rig.endpoint)->cells == path->cells); // Repeated interval/query consumers reuse unchanged canonical geometry, independent of meter mutations.
	CHECK(GetGeothermalUtilityCacheBuildCount() == builds); CHECK(GetGeothermalUtilityPathSearchCount() == searches); CHECK(ComputeUtilityPowerDigest() == native);
	const auto isolated = TileAddWrap(rig.request.tile, 3, 3); REQUIRE(TopologyCell(isolated).Succeeded());
	CHECK_FALSE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, rig.request.endpoint, isolated).has_value()); CHECK(GetGeothermalUtilityCacheBuildCount() == builds + 1); CHECK(GetGeothermalUtilityPathSearchCount() == searches);
	REQUIRE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, rig.request.endpoint, rig.endpoint).has_value()); const auto refreshed = GetGeothermalUtilityPathSearchCount();
	const auto excluded = GetGeothermalUtilityLink(rig.bridge, UtilityEnergyDomain::Heat)->id;
	CHECK_FALSE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, rig.request.endpoint, rig.endpoint, std::array{excluded}).has_value()); CHECK(GetGeothermalUtilityPathSearchCount() == refreshed + 1);
	REQUIRE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, rig.request.endpoint, rig.endpoint)->cells == path->cells); CHECK(GetGeothermalUtilityPathSearchCount() == refreshed + 1);
	REQUIRE(TopologyCell(rig.bridge, true).Succeeded()); const auto before_split = GetGeothermalUtilityPathSearchCount();
	CHECK_FALSE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, rig.request.endpoint, rig.endpoint).has_value()); CHECK(GetGeothermalUtilityPathSearchCount() == before_split);
	REQUIRE(TopologyCell(rig.bridge).Succeeded()); REQUIRE(FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Heat, rig.request.endpoint, rig.endpoint).has_value());
	CHECK(GetGeothermalUtilityPathSearchCount() == before_split + 1);
}
