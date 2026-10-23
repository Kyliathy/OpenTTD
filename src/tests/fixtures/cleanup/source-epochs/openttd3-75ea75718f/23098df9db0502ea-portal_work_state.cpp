/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file portal_work_state.cpp Canonical two-end queries, bounded pockets and real typed-reference restoration. @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 2 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../train.h"
#include "../landscape.h"
#include "../vehicle_func.h"
#include "../tile_cmd.h"
#include "../station_base.h"
#include "../command_func.h"
#include "../landscape_cmd.h"
#include "../rail_cmd.h"
#include "../rail_map.h"
#include "../rail.h"
#include "../pathfinder/follow_track.hpp"

#include "../safeguards.h"

/** Real map/company/geology owners; train allocation is a reference fixture, not a shipped TBM model or movement scenario. */
class PortalStateFixture {
public:
	ExcavationFixture world; ///< Actual profile and geological authority.
	TileIndex source = TileXYL(20, 20, MapLayer::Surface); ///< Canonical lower housing.
	TileIndex target = TileXYL(20, 20, MapLayer::Underground); ///< Destination rock housing.
	Train *reference_train = nullptr; ///< Actual pool-owned articulated reference part; no controller role admission is claimed.
	/** Initialize reciprocal native housing geometry without invoking the later sealed-build command. */
	PortalStateFixture()
	{
		ResetLayerPortalWorkState();
		MakeRailLayerPortal(source, _current_company, DiagDirection::SE, MapLayer::Underground, RAILTYPE_RAIL);
		MakeRailLayerPortal(target, _current_company, DiagDirection::NW, MapLayer::Surface, RAILTYPE_RAIL);
	}
	/** Release only fixture-owned reference/state before its actual company/world teardown. */
	~PortalStateFixture()
	{
		ResetLayerPortalWorkState();
		if (reference_train != nullptr) {
			// This fixture verified the pool was empty and owns its sole reference object; real pool cleanup avoids gameplay UI destruction in a headless unit test.
			assert(Vehicle::GetNumItems() == 1);
			_vehicle_pool.CleanPool();
			ResetVehicleHash(); // Real world initialization resets derived spatial hashes after pool cleanup; otherwise a later fixture can see a released actual part.
		}
	}
	/**
	 * Construct canonical saved scalar authority for one requested phase.
	 * @param phase Exact saved work phase.
	 * @return Owned empty-state record; commissioning references are attached explicitly by BindProject.
	 */
	LayerPortalWorkRecord Record(LayerPortalWorkState phase) const
	{
		LayerPortalWorkRecord record;
		record.pair_key = source; record.company = _current_company; record.state = phase;
		return record;
	}
	/** Open a real starter floor through the producer's validated fixture transport, preserving the housings. */
	void OpenFloor()
	{
		std::vector<TileIndex> floor;
		// Every initial cutter/approach cell is actual explicit open support; this is absent from production command paths.
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) floor.push_back(TileAddWrap(target, x, y));
		}
		world.Open(floor);
	}
	/**
	 * Bind a real geometric reservation and actual train-pool reference for saved-reference validation only.
	 * @param record Commissioning transport receiving owner references.
	 */
	void BindProject(LayerPortalWorkRecord &record)
	{
		OpenFloor();
		ExcavationJob job;
		job.company = _current_company; job.model_id = 0; job.commissioning_anchor = target;
		const auto next = TileAddWrap(target, 1, 0);
		job.requested_tiles = {next};
		ExcavationCutGeometry cut{next, Direction::SW, {}, {target}};
		// The typed project owns only its explicit conservative forward sweep, never an invented commissioning vehicle model.
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) cut.swept_tiles.push_back(TileAddWrap(next, x, y));
		}
		job.authorized_tiles = cut.swept_tiles; job.route.push_back(std::move(cut));
		const auto reserved = ReserveExcavationJob(job, true);
		REQUIRE(reserved.error == ExcavationError::None);
		REQUIRE(Train::CanAllocateItem());
		REQUIRE(Vehicle::GetNumItems() == 0);
		reference_train = Train::Create();
		reference_train->owner = _current_company;
		reference_train->SetArticulatedPart(); // A reference-only pool object participates in no engine/group stock accounting.
		reference_train->vehstatus.Set(VehState::Hidden);
		record.project = reserved.id; record.cutter = reference_train->index;
	}
	/**
	 * Validate and publish one copied saved-state candidate through its actual owner boundaries.
	 * @param record Explicit owned candidate.
	 */
	void Restore(const LayerPortalWorkRecord &record)
	{
		const std::array rows{record};
		REQUIRE(ValidateLayerPortalWorkRecords(rows, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
		StageLoadedLayerPortalWork({record});
		AcceptLoadedLayerPortalWork();
	}
};

/** Both ends query one actual record in all three phases; copied-state round trips preserve explicit typed assignment. */
TEST_CASE("Portal work state round trips one record from either endpoint", "[portal-work-state]")
{
	PortalStateFixture fixture;
	LayerPortalWorkState phase = LayerPortalWorkState::Sealed;
	SECTION("Sealed") {}
	SECTION("Connected") { phase = LayerPortalWorkState::Connected; fixture.OpenFloor(); }
	SECTION("Commissioning typed references") { phase = LayerPortalWorkState::Commissioning; }
	auto record = fixture.Record(phase);
	if (phase == LayerPortalWorkState::Commissioning) fixture.BindProject(record);
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	fixture.Restore(record);
	const auto work_digest = ComputeLayerPortalWorkDigest();
	CHECK(GetLayerPortalPairKey(fixture.source) == fixture.source);
	CHECK(GetLayerPortalPairKey(fixture.target) == fixture.source);
	CHECK(GetLayerPortalWorkState(fixture.source) == phase);
	CHECK(GetLayerPortalWorkState(fixture.target) == phase);
	CHECK(GetLayerPortalWorkRecord(fixture.source) == GetLayerPortalWorkRecord(fixture.target));
	const auto snapshot = CaptureLayerPortalWorkRecords();
	REQUIRE(snapshot.size() == 1);
	CHECK(snapshot[0].project == record.project);
	CHECK(snapshot[0].cutter == record.cutter);
	ResetLayerPortalWorkState();
	CHECK(GetLayerPortalWorkState(fixture.source) == LayerPortalWorkState::Sealed);
	fixture.Restore(snapshot[0]);
	CHECK(ComputeLayerPortalWorkDigest() == work_digest);
	CHECK(GetLayerPortalWorkState(fixture.target) == phase);
	const auto pocket = GetLayerPortalWorkPocket(fixture.source);
	if (phase == LayerPortalWorkState::Commissioning) {
		REQUIRE(pocket);
		CHECK(pocket->assigned_cutter == record.cutter);
		CHECK(pocket->tiles.size() == 9);
		CHECK(pocket->anchor == fixture.target);
	} else {
		CHECK_FALSE(pocket);
	}
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
}

/** Invalid/missing/duplicate physical keys and saved references refuse without mutating accepted state or packed terrain. */
TEST_CASE("Portal work rejects orphan state and preserves implicit Classic connection", "[portal-work-state]")
{
	PortalStateFixture fixture;
	CHECK_FALSE(GetLayerPortalPairKey(TileXYL(30, 20, MapLayer::Underground)));
	CHECK(GetLayerPortalWorkState(INVALID_TILE) == LayerPortalWorkState::Invalid);
	auto record = fixture.Record(LayerPortalWorkState::Sealed);
	const auto profile = GetCurrentMultiLayerProfileSelection();
	CHECK(ValidateLayerPortalWorkRecords(std::array{record, record}, profile, GetGeologyWorldState()) == LayerPortalWorkError::InvalidPair);
	record.pair_key = fixture.target;
	CHECK(ValidateLayerPortalWorkRecords(std::array{record}, profile, GetGeologyWorldState()) == LayerPortalWorkError::InvalidPair);
	record = fixture.Record(LayerPortalWorkState::Connected);
	CHECK(ValidateLayerPortalWorkRecords(std::array{record}, profile, GetGeologyWorldState()) == LayerPortalWorkError::MissingExcavation);
	record = fixture.Record(LayerPortalWorkState::Commissioning);
	CHECK(ValidateLayerPortalWorkRecords(std::array{record}, profile, GetGeologyWorldState()) == LayerPortalWorkError::InvalidReference);
	const auto edge = TileXYL(30, 0, MapLayer::Surface), edge_pair = MultiLayerMap::TileInLayer(edge, MapLayer::Underground);
	MakeRailLayerPortal(edge, _current_company, DiagDirection::SE, MapLayer::Underground, RAILTYPE_RAIL);
	MakeRailLayerPortal(edge_pair, _current_company, DiagDirection::NW, MapLayer::Surface, RAILTYPE_RAIL);
	CHECK(GetLayerPortalDoorTile(edge) == INVALID_TILE);
	record = fixture.Record(LayerPortalWorkState::Connected); record.pair_key = edge;
	CHECK(ValidateLayerPortalWorkRecords(std::array{record}, profile, GetGeologyWorldState()) == LayerPortalWorkError::MissingExcavation);
	ResetMultiLayerProfileSelection();
	CHECK(GetLayerPortalWorkState(fixture.source) == LayerPortalWorkState::Connected);
	CHECK(GetLayerPortalWorkState(fixture.target) == LayerPortalWorkState::Connected);
	CHECK(CaptureLayerPortalWorkRecords().empty());
	CHECK_FALSE(PrepareLayerPortalWorkPair(fixture.source, _current_company).Ready());
}

/** Saved phases hide ordinary paths without destroying physical pairing; real reference trains gain no unsupported TBM role. */
TEST_CASE("Portal sealed routing and actual movement refuse ordinary traffic", "[portal-routing]")
{
	PortalStateFixture fixture;
	ResetRailTypes();
	auto record = fixture.Record(LayerPortalWorkState::Sealed);
	SECTION("Sealed") {}
	SECTION("Commissioning remains assigned and controller gated") {
		record.state = LayerPortalWorkState::Commissioning;
		fixture.BindProject(record);
	}
	fixture.Restore(record);
	CHECK(GetTileTrackStatus(fixture.source, TransportType::Rail, RoadTramType::Invalid).trackdirs.None());
	CHECK(GetTileTrackStatus(fixture.target, TransportType::Rail, RoadTramType::Invalid).trackdirs.None());
	CHECK(GetLayerPortalNextTile(fixture.source, DiagDirection::SE) == fixture.target);
	CHECK(GetLayerPortalNextTile(fixture.target, DiagDirection::NW) == fixture.source);
	const auto door = TileAddByDiagDir(fixture.source, DiagDirection::NW);
	MakeRailNormal(door, _current_company, Track::Y, RAILTYPE_RAIL);
	CFollowTrackRail follower(_current_company, RailTypes{RAILTYPE_RAIL});
	CHECK_FALSE(follower.Follow(door, DiagDirToDiagTrackdir(DiagDirection::SE)));
	const auto work = ComputeLayerPortalWorkDigest();
	CHECK(CheckLayerPortalWorkAccess(fixture.source, VehicleID::Invalid(), record.project) != LayerPortalWorkError::None);
	if (fixture.reference_train != nullptr) {
		CHECK(CheckLayerPortalWorkAccess(fixture.source, fixture.reference_train->index, record.project) == LayerPortalWorkError::ControllerUnavailable);
		CHECK(CheckLayerPortalWorkAccess(fixture.source, fixture.reference_train->index, ExcavationJobID{UINT32_MAX}) == LayerPortalWorkError::InvalidReference);
		CHECK(VehicleEnterTile(fixture.reference_train, fixture.source, TileX(fixture.source) * TILE_SIZE, TileY(fixture.source) * TILE_SIZE).Test(VehicleEnterTileState::CannotEnter));
		CHECK(VehicleEnterTile(fixture.reference_train, fixture.target, TileX(fixture.target) * TILE_SIZE, TileY(fixture.target) * TILE_SIZE).Test(VehicleEnterTileState::CannotEnter));
		REQUIRE(Train::CanAllocateItem());
		auto *unrelated = Train::Create();
		unrelated->owner = _current_company; unrelated->SetArticulatedPart(); unrelated->vehstatus.Set(VehState::Hidden);
		CHECK(CheckLayerPortalWorkAccess(fixture.source, unrelated->index, record.project) == LayerPortalWorkError::InvalidReference);
		CHECK(VehicleEnterTile(unrelated, fixture.source, TileX(fixture.source) * TILE_SIZE, TileY(fixture.source) * TILE_SIZE).Test(VehicleEnterTileState::CannotEnter));
		// Both actual reference parts are fixture-owned and the pool was initially empty; cleanup claims no TBM model or runtime consist scenario.
		REQUIRE(Vehicle::GetNumItems() == 2);
		_vehicle_pool.CleanPool(); ResetVehicleHash(); fixture.reference_train = nullptr;
	}
	CHECK(ComputeLayerPortalWorkDigest() == work);
}

/** A cached route loses its edge only after the actual visible PBS reservations have been released. */
TEST_CASE("Portal connection drains reservations before sealed routing hides the axis", "[portal-routing]")
{
	PortalStateFixture fixture;
	ResetRailTypes(); fixture.OpenFloor();
	auto record = fixture.Record(LayerPortalWorkState::Connected);
	fixture.Restore(record);
	REQUIRE(GetTileTrackStatus(fixture.source, TransportType::Rail, RoadTramType::Invalid).trackdirs.Any());
	const auto door = TileAddByDiagDir(fixture.source, DiagDirection::NW);
	MakeRailNormal(door, _current_company, Track::Y, RAILTYPE_RAIL);
	CFollowTrackRail cached_follower(_current_company, RailTypes{RAILTYPE_RAIL});
	REQUIRE(cached_follower.Follow(door, DiagDirToDiagTrackdir(DiagDirection::SE)));
	REQUIRE(GetTileTrackStatus(fixture.target, TransportType::Rail, RoadTramType::Invalid).trackdirs.Any());
	SetLayerPortalReservation(fixture.source, true); SetLayerPortalReservation(fixture.target, true);
	const auto geo = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(ChangeLayerPortalConnection(fixture.target, _current_company, false, false) == LayerPortalWorkError::None);
	CHECK(GetLayerPortalWorkState(fixture.source) == LayerPortalWorkState::Connected);
	CHECK(HasLayerPortalReservation(fixture.source)); CHECK(HasLayerPortalReservation(fixture.target));
	REQUIRE(ChangeLayerPortalConnection(fixture.target, _current_company, false, true) == LayerPortalWorkError::None);
	CHECK_FALSE(HasLayerPortalReservation(fixture.source)); CHECK_FALSE(HasLayerPortalReservation(fixture.target));
	CHECK(GetTileTrackStatus(fixture.source, TransportType::Rail, RoadTramType::Invalid).trackdirs.None());
	CHECK_FALSE(cached_follower.Follow(door, DiagDirToDiagTrackdir(DiagDirection::SE)));
	CHECK(GetLayerPortalWorkState(fixture.target) == LayerPortalWorkState::Sealed);
	CHECK(GetLayerPortalWorkRecord(fixture.source)->revision == record.revision + 1);
	CHECK(GetLayerPortalPairTile(fixture.source) == fixture.target);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geo);
}

/** Downstream producers compile the actual signature and receive explicit no-allocation controller refusals. */
TEST_CASE("Portal public commissioning contract preserves real owner handoff", "[portal-contract]")
{
	STATIC_REQUIRE(std::is_invocable_r_v<LayerPortalWorkError, decltype(&CheckLayerPortalWorkAccess), TileIndex, VehicleID, ExcavationJobID>);
	STATIC_REQUIRE(std::is_invocable_r_v<LayerPortalCommissionResult, decltype(&CommissionLayerPortalTBM), const LayerPortalCommissionRequest &, bool>);
	PortalStateFixture fixture;
	fixture.Restore(fixture.Record(LayerPortalWorkState::Sealed));
	LayerPortalCommissionRequest request;
	request.portal = fixture.source; request.proposal.company = _current_company;
	request.proposal.model_id = 0; request.proposal.commissioning_anchor = fixture.target;
	request.proposal.pass_terms.emplace();
	REQUIRE(Station::GetNumItems() == 0); REQUIRE(Station::CanAllocateItem());
	auto *origin = Station::Create(TileXYL(10, 10, MapLayer::Surface));
	origin->owner = _current_company; origin->facilities.Set(StationFacility::Train);
	request.proposal.origin = origin->index; // An actual owned Surface railway station reference validates the portal-side handoff; no wagon/exchange runtime is claimed.
	const auto work = ComputeLayerPortalWorkDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto funds = Company::Get(_current_company)->money;
	const auto quote = CommissionLayerPortalTBM(request, false), command = CommissionLayerPortalTBM(request, true);
	CHECK(quote.error == LayerPortalWorkError::ControllerUnavailable); CHECK(command.error == quote.error);
	// Catalogue tube/capsule and ordinary wagon entries cannot acquire the portal-only TBM commissioning role, even with otherwise valid actual owner/station references.
	for (uint16_t model : {uint16_t{2}, uint16_t{9}, uint16_t{10}, uint16_t{11}, uint16_t{12}, uint16_t{13}}) {
		request.proposal.model_id = model;
		CHECK(CommissionLayerPortalTBM(request, true).error == LayerPortalWorkError::InvalidReference);
	}
	request.proposal.model_id = 0;
	request.proposal.pass_terms->rock_capacity_t = 0;
	CHECK(CommissionLayerPortalTBM(request, true).error == LayerPortalWorkError::InvalidReference);
	request.proposal.pass_terms->rock_capacity_t = 128;
	CHECK(quote.pair_key == fixture.source); CHECK(quote.pair_revision == GetLayerPortalWorkRecord(fixture.target)->revision);
	CHECK(command.project == ExcavationJobID{UINT32_MAX}); CHECK(command.cutter == VehicleID::Invalid());
	request.portal = fixture.target;
	CHECK(CommissionLayerPortalTBM(request, true).error == LayerPortalWorkError::InvalidPair);
	request.portal = fixture.source; request.proposal.company = CompanyID::Invalid();
	CHECK(CommissionLayerPortalTBM(request, true).error == LayerPortalWorkError::InvalidOwner);
	CHECK(ComputeLayerPortalWorkDigest() == work); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == funds);
	REQUIRE(Station::GetNumItems() == 1); _station_pool.CleanPool();
}

/** A separate actual command connects compatible open approaches once; occupied parts preserve position and reservation on refusal. */
TEST_CASE("Portal connection validates both approaches and actual occupied references", "[portal-connect]")
{
	PortalStateFixture fixture;
	ResetRailTypes();
	fixture.Restore(fixture.Record(LayerPortalWorkState::Sealed));
	CHECK(CmdChangeLayerPortalConnection({DoCommandFlag::Execute}, fixture.source, true).Failed());
	fixture.OpenFloor();
	CHECK(CmdChangeLayerPortalConnection({}, fixture.target, true).Failed());
	// Floor alone is not an ordinary railway connection: both ends need owned track that actually joins the door.
	for (TileIndex end : {fixture.source, fixture.target}) MakeRailNormal(TileAddByDiagDir(end, GetLayerPortalDoorDirection(end)), _current_company, Track::Y, RAILTYPE_RAIL);
	const TileIndex approach = TileAddByDiagDir(fixture.source, GetLayerPortalDoorDirection(fixture.source));
	MakeRailNormal(approach, _current_company, Track::X, RAILTYPE_RAIL);
	CHECK(CmdChangeLayerPortalConnection({}, fixture.source, true).Failed());
	auto curves = DiagdirReachesTracks(GetLayerPortalDoorDirection(fixture.source));
	curves.Reset(Track::Y);
	REQUIRE(curves.Any());
	MakeRailNormal(approach, _current_company, *curves.begin(), RAILTYPE_RAIL);
	CHECK(CmdChangeLayerPortalConnection({}, fixture.target, true).Succeeded());
	CHECK(GetLayerPortalWorkState(fixture.source) == LayerPortalWorkState::Sealed);
	REQUIRE(CmdChangeLayerPortalConnection({DoCommandFlag::Execute}, fixture.target, true).Succeeded());
	const auto revision = GetLayerPortalWorkRecord(fixture.source)->revision;
	CHECK(GetTileTrackStatus(fixture.source, TransportType::Rail, RoadTramType::Invalid).trackdirs.Any());
	CHECK(CmdChangeLayerPortalConnection({DoCommandFlag::Execute}, fixture.source, true).Succeeded());
	CHECK(GetLayerPortalWorkRecord(fixture.target)->revision == revision);
	REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(Train::CanAllocateItem());
	fixture.reference_train = Train::Create();
	fixture.reference_train->owner = _current_company; fixture.reference_train->SetArticulatedPart(); fixture.reference_train->vehstatus.Set(VehState::Hidden);
	fixture.reference_train->tile = fixture.target; fixture.reference_train->z_pos = 0;
	fixture.reference_train->x_pos = TileX(fixture.target) * TILE_SIZE; fixture.reference_train->y_pos = TileY(fixture.target) * TILE_SIZE;
	fixture.reference_train->UpdatePosition(); // Real spatial owner registers the actual part; no shadow vehicle-on-tile cache is supplied.
	SetLayerPortalReservation(fixture.source, true); SetLayerPortalReservation(fixture.target, true);
	const auto position = fixture.reference_train->tile;
	CHECK(CmdChangeLayerPortalConnection({DoCommandFlag::Execute}, fixture.source, false).Failed());
	CHECK(fixture.reference_train->tile == position); CHECK(GetLayerPortalWorkRecord(fixture.source)->revision == revision);
	CHECK(HasLayerPortalReservation(fixture.source)); CHECK(HasLayerPortalReservation(fixture.target));
	CHECK(GetLayerPortalWorkState(fixture.source) == LayerPortalWorkState::Connected);
}

/** Actual assigned project references stay recoverable through refused removal, conversion and owner callbacks. */
TEST_CASE("Assigned portal lifecycle waits for its actual recovery controller", "[portal-lifecycle]")
{
	PortalStateFixture fixture;
	ResetRailTypes(); Company::Get(_current_company)->avail_railtypes.Set(RAILTYPE_ELECTRIC);
	auto record = fixture.Record(LayerPortalWorkState::Commissioning);
	fixture.BindProject(record); fixture.Restore(record);
	const auto work = ComputeLayerPortalWorkDigest(), geo = ComputeGeologyStateDigest(GetGeologyWorldState());
	CHECK(CheckLayerPortalLifecycle(fixture.source) == LayerPortalWorkError::Conflict);
	CHECK(CheckLayerPortalLifecycle(fixture.target) == LayerPortalWorkError::Conflict);
	CHECK(Command<Commands::LandscapeClear>::Do({DoCommandFlag::Execute}, fixture.target).Failed());
	CHECK(CmdConvertRail({DoCommandFlag::Execute}, fixture.source, fixture.source, RAILTYPE_ELECTRIC, false).Failed());
	_tile_type_procs[TileType::LayerPortal]->change_tile_owner_proc(fixture.target, _current_company, INVALID_OWNER);
	CHECK(GetTileOwner(fixture.source) == _current_company); CHECK(GetTileOwner(fixture.target) == _current_company);
	CHECK(GetRailType(fixture.source) == RAILTYPE_RAIL); CHECK(GetRailType(fixture.target) == RAILTYPE_RAIL);
	CHECK(GetExcavationJob(record.project) != nullptr);
	CHECK(ComputeLayerPortalWorkDigest() == work); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geo);
}
