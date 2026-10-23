/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_town_service_fixture.h Actual native generator, funded risers, cable commands and Surface terminal for connection boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups15BBB/15BBC.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Excavated floor, connected portal approaches, native town position and receiver-boundary materials are explicit fixture inputs.
 * The separate paid first-plant scenario owns TBM excavation, driven supply and populated-town acceptance.
 */
#ifndef TESTS_GEOTHERMAL_TOWN_SERVICE_FIXTURE_H
#define TESTS_GEOTHERMAL_TOWN_SERVICE_FIXTURE_H

#include "geothermal_generator_operating_fixture.h"
#include "../geothermal_riser.h"
#include "../geothermal_service_cmd.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../rail.h"
#include "../rail_cmd.h"
#include "../road.h"
#include "../road_cmd.h"
#include "../track_func.h"
#include "../station_cmd.h"
#include "../station_base.h"
#include "../station_map.h"
#include "../newgrf_station.h"
#include "../safeguards.h"

/** One actual shared-source native world with complete paid recipient machinery; no private power, stock or participant IDs. */
class GeothermalTownServiceRig : public GeothermalGeneratorOperatingRig {
	AutoRestoreBackup<decltype(_settings_game.economy.dist_local_authority)> town_authority{
		_settings_game.economy.dist_local_authority, 20}; ///< Canonical economy_settings.ini authority default; isolated fixture teardown restores the caller's setting.

	/** @param from Actual source port/housing. @param to Actual next machinery port on that same plane. @return Paid installed explicit fixture corridor, retaining existing cells. */
	std::vector<TileIndex> BuildCable(TileIndex from, TileIndex to)
	{
		std::vector<TileIndex> cells{from}; auto tile = from;
		const int outside = static_cast<int>(std::min(TileX(from), TileX(to))) - 1;
		// Leave the actual port on its outside column, follow the next sender's
		// row, then enter its declared port through the owned housing. These two
		// fixture corridors avoid both real machinery floors and public doors.
		while (static_cast<int>(TileX(tile)) != outside) { tile = TileAddWrap(tile, -1, 0); cells.push_back(tile); }
		while (TilePlaneY(tile) != TilePlaneY(to)) { tile = TileAddWrap(tile, 0, TilePlaneY(tile) < TilePlaneY(to) ? 1 : -1); cells.push_back(tile); }
		while (TileX(tile) != TileX(to)) { tile = TileAddWrap(tile, 1, 0); cells.push_back(tile); }
		this->Open(cells);
		for (const auto cell : cells) {
			if (!GetGeothermalUtilityLink(cell, UtilityEnergyDomain::Electricity).has_value()) this->Build(cell, UtilityEnergyDomain::Electricity); // Each new physical cell pays once; an existing original port is retained.
		}
		return cells;
	}

	/** @param source Actual sending rock plane. @param destination Actual reciprocal receiving plane. @param y Explicit fixture row. @param road Direct Deep/Surface road elevator instead of a rail pair. @return Actual funded native riser identity, with its complete bill received but no fabricated work. */
	IndustryID BuildRiser(MapLayer source, MapLayer destination, uint y, bool road)
	{
		const TileIndex portal = TileXYL(44, y, source), upper = TileXYL(44, y, destination), anchor = TileXYL(45, y, source);
		std::vector<TileIndex> floor;
		// The actual command participants receive complete declared endpoint,
		// floor and outward approach geometry. This is not paid boring evidence.
		for (const auto layer : {source, destination}) {
			if (layer == MapLayer::Surface) continue;
			for (int dy = -2; dy <= 3; ++dy) {
				for (int x = 42; x <= 48; ++x) floor.push_back(TileXYL(x, y + dy, layer));
			}
		}
		this->Open(floor);
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, upper, road ? TransportType::Road : TransportType::Rail,
			source, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
		// Actual native approach commands supply the accepted boundary geometry;
		// no fake Connected record or guessed portal revision is installed.
		for (const auto endpoint : {portal, upper}) {
			const auto door = GetLayerPortalDoorTile(endpoint);
			if (road) {
				REQUIRE(CmdBuildRoad(DoCommandFlag::Execute, door, AxisToRoadBits(DiagDirToAxis(GetLayerPortalShaftDirection(endpoint))),
					ROADTYPE_ROAD, DisallowedRoadDirections{}, TownID::Invalid()).Succeeded());
			} else {
				REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, door, RAILTYPE_RAIL,
					AxisToTrack(DiagDirToAxis(GetLayerPortalShaftDirection(endpoint))), false).Succeeded());
			}
		}
		REQUIRE(ChangeLayerPortalConnection(portal, _current_company, true, true) == LayerPortalWorkError::None);
		REQUIRE(Command<Commands::BuildGeothermalRiser>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, anchor,
			std::make_tuple(anchor, portal, anchor, DiagDirection::SE, uint32_t{123})).Succeeded());
		const auto id = FindGeothermalRiserForPortal(portal); REQUIRE(Industry::IsValidID(id));
		for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Riser)->inputs) {
			REQUIRE(ReceiveGeothermalRiserCargo(*Industry::Get(id), GetCargoTypeByLabel(input.label), input.quantity) == input.quantity); // Each real native accepted slot owns its declared boundary bill once.
		}
		return id;
	}

public:
	GeothermalTownServiceRequest service; ///< Explicit actual native hardware/recipient request, never preallocated ledger IDs.
	std::vector<TileIndex> deep_cable; ///< Actual command-installed first leg, for physical-loss tests.
	std::vector<TileIndex> relay_cable; ///< Actual Underground link; empty for direct export.
	TileIndex platform = INVALID_TILE; ///< Actual occupied Surface station tile.

	/** @return Actual newly built/rebuilt owned Surface terminal, paying its native station invoice. */
	StationID BuildTerminal()
	{
		REQUIRE(Command<Commands::BuildRailStation>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, this->platform,
			std::make_tuple(this->platform, RAILTYPE_RAIL, Axis::Y, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, NEW_STATION, false)).Succeeded());
		return GetStationIndex(this->platform);
	}

	/**
	 * Fund and finish another actual model on separately disclosed fixture floor.
	 * @param model Actual dated replacement, funded from the fixture's remaining original cash.
	 * @return Actual newly issued machinery; no operating output is fabricated.
	 */
	IndustryID BuildReplacementGenerator(GeothermalGeneratorModel model)
	{
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{2000}, 0, 1);
		const auto *operating = GetMultiLayerGeothermalGeneratorTerms(model); REQUIRE(operating != nullptr);
		const auto *terms = GetMultiLayerGeothermalConstruction(operating->construction);
		auto request = this->request; request.kind = operating->construction;
		request.tile = request.endpoint = TileXYL(36, 32, MapLayer::DeepUnderground);
		std::vector<TileIndex> floor;
		// Supply only explicit second-building floor/rim as boundary input;
		// ordinary funding, native accepted stock and296/592/888 work stay real.
		for (int y = -1; y <= terms->height; ++y) {
			for (int x = -1; x <= terms->width; ++x) floor.push_back(TileAddWrap(request.tile, x, y));
		}
		this->Open(floor); const auto claim = ClaimGeothermalSite(request); REQUIRE(claim.error == GeothermalError::None);
		REQUIRE(Command<Commands::BuildGeothermalGenerator>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, request.tile,
			std::make_tuple(request.tile, claim.id, uint32_t{123})).Succeeded());
		IndustryID result = IndustryID::Invalid();
		for (const auto &state : CaptureGeothermalGenerators()) { // Resolve the actual consumed original claim, never an assumed native pool slot.
			if (state.site == claim.id) result = state.industry;
		}
		REQUIRE(Industry::IsValidID(result));
		for (const auto &input : terms->inputs) { // This second native owner receives its independent whole receiver-boundary bill once.
			REQUIRE(ReceiveGeothermalGeneratorCargo(*Industry::Get(result), GetCargoTypeByLabel(input.label), input.quantity) == input.quantity);
		}
		this->Tick(static_cast<uint16_t>(terms->work_ticks)); REQUIRE(IsGeothermalGeneratorConstructionComplete(result));
		std::vector<TileIndex> branch;
		// Branch from the original collector's existing outside column, below
		// the first machinery, into the second model's original boundary port.
		for (int x = 49; x >= 35; --x) branch.push_back(TileXYL(x, 31, MapLayer::DeepUnderground));
		branch.push_back(TileXYL(35, 32, MapLayer::DeepUnderground)); branch.push_back(request.endpoint);
		this->Open(branch);
		for (const auto tile : branch) {
			if (!GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Heat).has_value()) this->Build(tile, UtilityEnergyDomain::Heat); // Reuse only existing original pipes; every new domain cell pays its own native invoice.
		}
		this->BuildCable(request.endpoint, GetGeothermalRiserState(this->service.first_riser)->site.port);
		return result;
	}

	/** @param direct Select a real direct road elevator, leaving Underground solid. @param model Actual dated generator model. */
	explicit GeothermalTownServiceRig(bool direct = false, GeothermalGeneratorModel model = GeothermalGeneratorModel::VentTap) :
		GeothermalGeneratorOperatingRig(false, true, 1000000, model)
	{
		REQUIRE(Station::GetNumItems() == 0); ResetLayerPortalWorkState(); ResetGeothermalRisers();
		ResetRailTypes(); ResetRoadTypes(); StationClass::Reset(); RebuildStationKdtree();
		REQUIRE(InitialiseGeothermalRiserDefinitions(this->ActivatedContent()));
		this->service.generator = this->generator;
		this->service.first_riser = this->BuildRiser(MapLayer::DeepUnderground, direct ? MapLayer::Surface : MapLayer::Underground, 12, direct);
		if (!direct) this->service.second_riser = this->BuildRiser(MapLayer::Underground, MapLayer::Surface, 8, false);
		this->Tick(296);
		REQUIRE(IsGeothermalRiserOperational(this->service.first_riser));
		if (!direct) REQUIRE(IsGeothermalRiserOperational(this->service.second_riser));
		this->deep_cable = this->BuildCable(this->plant_port, GetGeothermalRiserState(this->service.first_riser)->site.port);
		if (!direct) this->relay_cable = this->BuildCable(GetGeothermalRiserState(this->service.first_riser)->destination,
			GetGeothermalRiserState(this->service.second_riser)->site.port);
		// Reposition this fixture's actual already-created empty native town;
		// populated-town construction remains the separate paid-world scenario.
		REQUIRE(Town::GetNumItems() == 1);
		for (Town *town : Town::Iterate()) { // Resolve the sole base fixture town's actual issued identity, without assuming a numeric pool slot.
			this->service.town = town->index; town->xy = TileXYL(43, direct ? 11 : 7, MapLayer::Surface);
		}
		RebuildTownKdtree();
		this->platform = TileXYL(43, direct ? 12 : 8, MapLayer::Surface);
		this->service.terminal = this->BuildTerminal();
		this->Operate(); // One real shared opportunity registers and publishes the actual coupled output before connection.
	}

	/** Retire actual fixture machinery/stations before their map/company owners; no supplied file or private ledger is replaced. */
	~GeothermalTownServiceRig()
	{
		UpdateSignalsInBuffer();
		for (Industry *industry : Industry::Iterate()) delete industry; // Native destruction releases each exact typed source/riser before numeric pool reuse.
		_station_pool.CleanPool(); RebuildStationKdtree(); ResetLayerPortalWorkState(); ResetGeothermalRisers();
	}

	/** @param flags Pure quote or actual native execution. @return Whole selected connection command result. */
	CommandCost Connect(DoCommandFlags flags = {})
	{
		return Command<Commands::ConnectGeothermalTownService>::Do(flags, this->service.generator, this->service.first_riser,
			this->service.second_riser, this->service.terminal, this->service.town);
	}
};

#endif /* TESTS_GEOTHERMAL_TOWN_SERVICE_FIXTURE_H */
