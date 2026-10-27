/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_fixture.h Shared real native engine definitions and a sealed rail pair for TBM site and acquisition tests.
 * The engine fixture moved here from tbm_engine.cpp; the acquisition rig (hoppers available, train limits, a
 * door-exit proposal), the published-project rig and the face-work rig are shared by the Group 2B-8 tests.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Groups 2, 2B and 3-8
 * @see docs/zz-reach2/upgrades/2026-10/r2lbf-linux-build-fixes.md Group 15 (the engine fixture releases the cargo packets its trains and stations held)
 */

#ifndef TESTS_TBM_FIXTURE_H
#define TESTS_TBM_FIXTURE_H

#include "excavation_fixture.h"
#include "../tbm.h"
#include "../engine_base.h"
#include "../newgrf_config.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../station_base.h"
#include "../train.h"
#include "../vehicle_func.h"
#include "../rail.h"
#include "../road_type.h"
#include "../group.h"
#include "../company_base.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "mock_environment.h"
#include "../order_base.h"
#include "../order_cmd.h"
#include "../cargopacket.h"
#include "../direction_func.h"

/**
 * Scoped choice of the rock that the next TBM acquisition rig declares over its whole rock plane, before it issues the
 * portal proof (ExcavationFixture::DeclareRockSeam). Without a guard the rig keeps the legacy all-Soft Underground.
 * Declare it before the rig, for example `const TBMRigRock mixed_rock{GeologyHardness::Mixed};`, so the choice stays
 * visible at the test that needs a capacity refusal or a mid-route exchange (r2mlvs3mp master task 10R.1).
 */
class TBMRigRock {
	static inline std::optional<GeologyHardness> current; ///< Rock the next rig declares; nothing keeps the legacy plane.
	std::optional<GeologyHardness> saved; ///< Enclosing choice, restored when this guard ends.
public:
	/** @param hardness Rock that every rig built while this guard lives declares. */
	explicit TBMRigRock(GeologyHardness hardness) : saved(current) { current = hardness; }
	/** Restore the enclosing choice, so no later test case inherits this one. */
	~TBMRigRock() { current = saved; }
	/** @param other Scoped choice that must not be copied or extend its lifetime. */
	TBMRigRock(const TBMRigRock &other) = delete;
	/** @param other Scoped choice that cannot replace another guard. @return Assignment is forbidden. */
	TBMRigRock &operator=(const TBMRigRock &other) = delete;
	/** @return The rock a rig built now declares, or nothing for the legacy plane. */
	static std::optional<GeologyHardness> Get() { return current; }
};

/** Actual native definitions and physical portal owners, isolated without replacing gameplay engine/cargo registries. */
class TBMEngineFixture {
	decltype(_engine_mngr.mappings) original_mapping = _engine_mngr.mappings; ///< Preserve the caller's existing real EIDS configuration.
public:
	ExcavationFixture world; ///< Real accepted profile, company and cargo definitions.
	GRFConfigList package; ///< Exact producer package transport for definition initialization.
	Engine *stock = nullptr; ///< Actual original engine definition retained alongside appended native models.
	Station *origin = nullptr; ///< Actual pool-owned Surface station reference.
	LayerPortalCommissionRequest request; ///< Actual source-site request; no geological route or controller is fabricated.
	/** Install real native engine definitions and sealed geometry through their public owners. */
	TBMEngineFixture()
	{
		REQUIRE(Engine::GetNumItems() == 0); REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(Station::GetNumItems() == 0);
		const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
		auto config = std::make_unique<GRFConfig>();
		config->ident = required.package; config->version = required.version; config->status = GRFStatus::Activated;
		package.push_back(std::move(config));
		REQUIRE(Engine::CanAllocateItem());
		stock = Engine::Create(VehicleType::Train, 0);
		_engine_mngr.SetID(VehicleType::Train, 0, INVALID_GRFID, 0, stock->index);
		REQUIRE(InitialiseTBMEngineDefinitions(package));
		ResetLayerPortalWorkState(); ResetTBMProjectState(); ResetRailTypes();
		request.portal = TileXYL(20, 20, MapLayer::Surface);
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, request.portal, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
		REQUIRE(Station::CanAllocateItem());
		origin = Station::Create(TileXYL(18, 20, MapLayer::Surface));
		origin->owner = _current_company; origin->facilities.Set(StationFacility::Train);
		request.proposal.company = _current_company; request.proposal.origin = origin->index;
		request.proposal.model_id = 0; request.proposal.commissioning_anchor = GetLayerPortalPairTile(request.portal);
		request.proposal.pass_terms.emplace();
	}
	/** Release only fixture-owned pools before content/company teardown, restoring the actual mapping configuration. */
	~TBMEngineFixture()
	{
		_vehicle_pool.CleanPool(); ResetVehicleHash();
		_station_pool.CleanPool(); ResetLayerPortalWorkState(); ResetTBMProjectState();
		// Cleaning a vehicle or station pool only forgets their packet lists (CargoList::OnCleanPool), because a whole-game
		// clean also empties the packet pool. Release those orphaned packets too, so no later case inherits the spoil.
		_cargopacket_pool.CleanPool();
		_engine_pool.CleanPool(); _engine_mngr.mappings = std::move(original_mapping);
	}
};

/** Every world quantity acquisition may touch, captured without mutation. */
struct AcquisitionDigest {
	decltype(ComputeGeologyStateDigest(GetGeologyWorldState())) geology; ///< Packed terrain, deposits and jobs.
	decltype(ComputeLayerPortalWorkDigest()) portal; ///< Pair work records.
	Money money; ///< Company cash.
	uint32_t random0, random1; ///< Game random state.
	size_t vehicles; ///< Vehicle pool items.
	UnitID next_unit; ///< Next free train unit number.
	uint16_t trains; ///< Counted primary trains.
	std::array<uint16_t, 3> engines; ///< Counted cutter, Earth and Rock engines.
	/** @param other Digest to compare. @return Whether every quantity matches. */
	bool operator==(const AcquisitionDigest &other) const = default;
};

/** Real fixture world with available hoppers, train limits and a door-exit proposal. */
class TBMAcquisitionRig {
public:
	TBMEngineFixture fixture; ///< Real engines, sealed pair and origin; destroyed last.
	AutoRestoreBackup<UnitID> trains{_settings_game.vehicle.max_trains, UnitID{500}}; ///< Unit tests start with zero limits.
	AutoRestoreBackup<uint8_t> length{_settings_game.vehicle.max_train_length, uint8_t{7}}; ///< Seven tiles, the normal default.
	ExcavationJob base; ///< Fixture proposal before any route is added.
	LayerPortalBootstrapProof proof; ///< Issued portal proof.

	/** Make both hoppers available to the company and propose one straight cut out of the door. */
	TBMAcquisitionRig()
	{
		MockEnvironment::Instance(); // Coupling marks the train dirty, which reads vehicle sprite bounds from the sprite cache.
		for (uint16_t model : {uint16_t{2}, uint16_t{3}}) Engine::Get(GetTBMEngine(model))->company_avail.Set(_current_company);
		this->base = this->fixture.request.proposal;
		// Declare the chosen finite rock before proof/route admission so quotes and actual paid cuts read the same setup.
		if (const auto rock = TBMRigRock::Get(); rock.has_value()) this->fixture.world.DeclareRockSeam(GetTileLayer(this->base.commissioning_anchor), *rock);
		this->proof = IssueLayerPortalBootstrap(this->fixture.request);
		REQUIRE(this->proof.error == LayerPortalWorkError::None);
		this->fixture.request.proposal = this->Proposal(1, 3);
	}

	/**
	 * Proposal advancing straight out of the door.
	 * @param segments Number of straight advances.
	 * @param points Planning history the route is planned with.
	 * @return Proposal whose authorized floor is the union of every sweep.
	 */
	ExcavationJob Proposal(size_t segments, size_t points) const
	{
		return this->Proposal(std::vector<Direction>(segments, this->proof.exit_heading), points);
	}

	/**
	 * Proposal advancing out of the door along given headings, through the canonical planner.
	 * @param headings Heading of each advance; the first must be the exit heading.
	 * @param points Planning history the route is planned with.
	 * @return Proposal whose authorized floor is the union of every sweep, so turn space is accepted work.
	 */
	ExcavationJob Proposal(std::span<const Direction> headings, size_t points) const
	{
		auto proposal = this->base;
		ExcavationPlanningPose pose{this->proof.destination, this->proof.exit_heading, std::vector<TileIndex>(points, this->proof.destination)};
		// Each advance appends its cut; the authorized floor collects every swept cell once.
		for (Direction heading : headings) {
			const auto step = AdvanceExcavationPlanningPose(pose, heading);
			REQUIRE(step.error == ExcavationError::None);
			proposal.route.push_back(step.cut);
			proposal.authorized_tiles.insert(proposal.authorized_tiles.end(), step.cut.swept_tiles.begin(), step.cut.swept_tiles.end());
			pose = step.next;
		}
		std::ranges::sort(proposal.authorized_tiles);
		proposal.authorized_tiles.erase(std::unique(proposal.authorized_tiles.begin(), proposal.authorized_tiles.end()), proposal.authorized_tiles.end());
		proposal.requested_tiles = {pose.position};
		return proposal;
	}

	/** @return Current digest of every quantity acquisition may touch. */
	AcquisitionDigest Digest() const
	{
		const auto &stats = Company::Get(_current_company)->group_all[VehicleType::Train];
		/**
		 * @param engine Engine to look up.
		 * @return Its counted number, zero when never counted.
		 */
		const auto count = [&stats](EngineID engine) {
			const auto it = stats.num_engines.find(engine);
			return it == stats.num_engines.end() ? uint16_t{0} : it->second;
		};
		return {ComputeGeologyStateDigest(GetGeologyWorldState()), ComputeLayerPortalWorkDigest(), Company::Get(_current_company)->money, _random.state[0], _random.state[1], Vehicle::GetNumItems(), GetFreeUnitNumber(VehicleType::Train), stats.num_vehicle, {count(GetTBMEngine(0)), count(GetTBMEngine(2)), count(GetTBMEngine(3))}};
	}
};

/** One published project on the rig's pair: acquired consist, reserved job, Commissioning pair, store entry. */
class TBMProjectRig : public TBMAcquisitionRig {
public:
	TBMAcquisitionPlan plan; ///< Accepted preparation.
	TBMProject project; ///< Published project.
	Train *front = nullptr; ///< Actual cutter.

	/**
	 * Publish through the real owners, in publication order.
	 * @param segments Straight cuts out of the door; more than one is planned with the consist's own four points.
	 */
	explicit TBMProjectRig(size_t segments = 1) : TBMProjectRig(segments == 1 ? std::vector<DirDiff>{} : std::vector<DirDiff>(segments, DirDiff::Same)) {}

	/**
	 * Publish a project whose route turns as given, planned with the consist's own four points.
	 * @param turns Change of heading at each advance, starting from the door's exit heading; an empty list keeps the
	 *              rig's single straight cut.
	 */
	explicit TBMProjectRig(const std::vector<DirDiff> &turns)
	{
		std::vector<Direction> headings;
		Direction heading = this->proof.exit_heading;
		// Each turn is relative to the previous advance, so the list reads like driving instructions.
		for (DirDiff turn : turns) {
			heading = ChangeDir(heading, turn);
			headings.push_back(heading);
		}
		if (!headings.empty()) this->fixture.request.proposal = this->Proposal(headings, 4);
		this->Publish();
	}

	/**
	 * Publish a project on a proposal built once the sealed pair exists, such as an accepted area preview.
	 * @param propose Builds the proposal from the rig, whose proof and fixture are ready.
	 */
	explicit TBMProjectRig(const std::function<ExcavationJob(const TBMAcquisitionRig &)> &propose)
	{
		this->fixture.request.proposal = propose(*this);
		this->Publish();
	}

	/** Give the cutter one ordinary travel order to its Surface origin. */
	void AddTravelOrder() const
	{
		Order order;
		order.MakeGoToStation(this->fixture.origin->index);
		REQUIRE(CmdInsertOrder(DoCommandFlag::Execute, this->front->index, 0, order).Succeeded());
		REQUIRE(this->front->GetNumOrders() == 1);
	}

private:
	/** Prepare, acquire, reserve and publish the current proposal through the real owners, in publication order. */
	void Publish()
	{
		this->plan = PrepareTBMAcquisition(this->fixture.request, {2, 2});
		REQUIRE(this->plan.error == TBMAcquisitionError::None);
		TBMAcquisition held;
		REQUIRE(AcquireTBMConsist(this->fixture.request, this->plan, held) == TBMAcquisitionError::None);
		this->front = held.GetFront();
		const auto job = ReserveExcavationJob(this->plan.job, true);
		REQUIRE(job.error == ExcavationError::None);
		// The pair becomes Commissioning this job and cutter through the real loader boundary, as publication will do.
		auto record = *GetLayerPortalWorkRecord(this->fixture.request.portal);
		record.state = LayerPortalWorkState::Commissioning;
		record.project = job.id;
		record.cutter = this->front->index;
		record.origin = this->fixture.origin->index;
		++record.revision;
		const std::array rows{record};
		REQUIRE(ValidateLayerPortalWorkRecords(rows, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
		StageLoadedLayerPortalWork({record});
		AcceptLoadedLayerPortalWork();
		this->project = held.Release();
		this->project.job = job.id;
		REQUIRE(PublishTBMProject(this->project, true) == TBMProjectError::None);
	}
};

/**
 * Published project made to work at its face. The consist cannot yet drive into the pocket (master Group 7), so the
 * rig reaches Bore the way a save made while boring does: through the staged-load boundary.
 */
class TBMWorkRig : public TBMProjectRig {
public:
	/**
	 * Publish the project and enter Bore with a started train.
	 * @param segments Straight cuts out of the door.
	 */
	explicit TBMWorkRig(size_t segments = 1) : TBMProjectRig(segments)
	{
		this->Reload(TBMPhase::Bore);
		this->front->vehstatus.Reset(VehState::Stopped);
	}

	/**
	 * Publish and enter Bore on a route that turns as given.
	 * @param turns Change of heading at each advance (see TBMProjectRig).
	 */
	explicit TBMWorkRig(const std::vector<DirDiff> &turns) : TBMProjectRig(turns)
	{
		this->Reload(TBMPhase::Bore);
		this->front->vehstatus.Reset(VehState::Stopped);
	}

	/**
	 * Publish and enter Bore on a proposal built once the sealed pair exists.
	 * @param propose Builds the proposal (see TBMProjectRig).
	 */
	explicit TBMWorkRig(const std::function<ExcavationJob(const TBMAcquisitionRig &)> &propose) : TBMProjectRig(propose)
	{
		this->Reload(TBMPhase::Bore);
		this->front->vehstatus.Reset(VehState::Stopped);
	}

	/**
	 * Work the current cut to its commit, every tick without a wait.
	 * @return The estimate of the committed cut.
	 */
	ExcavationCutResult BoreOneCut() const
	{
		const auto estimate = EstimateExcavationCut(this->project.job);
		REQUIRE(estimate.error == ExcavationError::None);
		const uint32_t cursor = this->Job().cursor;
		const auto &model = *GetTBMCutterModel(this->Project().model_id);
		const uint64_t remaining = estimate.work_ticks - this->Job().partial_work_ticks;
		const uint64_t ticks = remaining == 0 ? 1 : CeilDiv(remaining * model.work_rate_denominator - this->Project().work_remainder, uint64_t{model.work_rate_numerator});
		// Drive the actual controller for its remaining fixed-point duration;
		// a zero-material overlap still needs one atomic controller commit.
		for (uint64_t tick = 0; tick < ticks; ++tick) REQUIRE(this->Tick().blocked == TBMBlockedReason::None);
		REQUIRE(this->Job().cursor == cursor + 1);
		return estimate;
	}

	/**
	 * Empty every wagon and return to Bore, standing in for an exchange and the drive back onto the new support,
	 * which do not exist yet (Group 4.2B, master Group 7).
	 */
	void ExchangeAndReturn() const
	{
		for (const auto role : {TBMConsistRole::EarthWagon, TBMConsistRole::RockWagon}) {
			for (Train *wagon : this->Wagons(role)) wagon->cargo.Truncate();
		}
		this->Reload(TBMPhase::Bore);
	}

	/**
	 * Save and reload projects and support through their staging boundary.
	 * @param phase Phase to give the project in the reloaded copy; none keeps the saved phase.
	 * @param edit Further change to the saved copy, as a save edited between sessions would carry.
	 */
	void Reload(std::optional<TBMPhase> phase = std::nullopt, const std::function<void(TBMProject &)> &edit = {}) const
	{
		auto projects = CaptureTBMProjects();
		auto support = CaptureTBMSupport();
		const auto next = GetNextTBMProjectID();
		// Only a requested change edits the copy; it then gets a new revision like any controller change.
		if (phase.has_value()) {
			projects.back().phase = *phase;
			projects.back().phase_ticks = 0;
		}
		if (edit) edit(projects.back());
		if (phase.has_value() || edit) ++projects.back().revision;
		ResetTBMProjectState();
		StageLoadedTBMProjects(std::move(projects), next);
		StageLoadedTBMSupport(std::move(support));
		REQUIRE(AcceptLoadedTBMProjects());
	}

	/** @return The live project. */
	const TBMProject &Project() const { return *GetTBMProject(this->project.id); }

	/** @return The live job. */
	const ExcavationJob &Job() const { return *GetExcavationJob(this->project.job); }

	/**
	 * Run one controller tick, which must succeed.
	 * @return The live project after it.
	 */
	const TBMProject &Tick() const
	{
		REQUIRE(AdvanceTBMWork(this->project.id) == TBMProjectError::None);
		return this->Project();
	}

	/**
	 * @param role Earth or Rock.
	 * @return Every wagon unit of that role, in chain order.
	 */
	std::vector<Train *> Wagons(TBMConsistRole role) const
	{
		std::vector<Train *> result;
		for (const auto &unit : InspectTBMConsist(*this->front, this->Project()).exchangeable_units) {
			if (unit.role == role) result.push_back(Train::Get(unit.vehicle));
		}
		return result;
	}

	/**
	 * @param role Earth or Rock.
	 * @return Tonnes aboard all wagons of that role.
	 */
	uint Spoil(TBMConsistRole role) const
	{
		uint total = 0;
		for (const Train *wagon : this->Wagons(role)) total += wagon->cargo.TotalCount();
		return total;
	}

	/**
	 * Put existing spoil into a wagon, as a previous cut would have.
	 * @param wagon Wagon to load.
	 * @param tonnes Whole tonnes, at most its free space.
	 */
	void Preload(Train *wagon, uint16_t tonnes) const
	{
		REQUIRE(CargoPacket::CanAllocateItem());
		CargoPacket *packet = CargoPacket::Create(StationID::Invalid(), tonnes, Source{Source::Invalid, SourceType::Industry});
		packet->UpdateLoadingTile(this->proof.destination);
		wagon->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
	}
};

#endif /* TESTS_TBM_FIXTURE_H */
