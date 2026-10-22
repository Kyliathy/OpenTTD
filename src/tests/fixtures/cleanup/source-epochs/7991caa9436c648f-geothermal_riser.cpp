/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file geothermal_riser.cpp Real-map two-endpoint riser admission; fixture-open floors do not prove paid construction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group5B
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "excavation_fixture.h"
#include "../geothermal_riser.h"
#include "../utility_power.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../rail_map.h"
#include "../core/random_func.hpp"

#include "../safeguards.h"

/** Actual profile, company, geometry and accepted portal owner with explicitly declared open test floor. */
class GeothermalRiserRig : public ExcavationFixture {
public:
	GeothermalRiserRequest request; ///< Pure quote input, never a funded native building.
	TileIndex upper = INVALID_TILE; ///< Actual reciprocal receiving housing.
	/**
	 * Prepare accepted native Connected geometry without claiming that this fixture excavated or funded it.
	 * @param direct_elevator Select Deep/Surface road geometry; otherwise use the actual Deep/Underground rail pair.
	 * @param source_layer Actual rock plane containing export machinery; Underground uses its reciprocal Surface rail pair.
	 */
	explicit GeothermalRiserRig(bool direct_elevator = false, MapLayer source_layer = MapLayer::DeepUnderground)
	{
		ResetUtilityPowerState(); ResetLayerPortalWorkState();
		this->request.owner = _current_company;
		this->request.portal = TileXYL(20, 20, source_layer);
		this->request.tile = this->request.port = TileXYL(21, 20, source_layer);
		this->request.access = DiagDirection::SE;
		// Direct elevators and Underground relay machinery both terminate at
		// Surface; the Deep rail source must use the intermediate Underground pair.
		const MapLayer receiving = direct_elevator || source_layer == MapLayer::Underground ? MapLayer::Surface : MapLayer::Underground;
		this->upper = MultiLayerMap::TileInLayer(this->request.portal, receiving);
		std::vector<TileIndex> floor;
		// Both actual rock-plane endpoints and the complete loading/machinery area
		// are fixture inputs. A direct elevator deliberately opens no intermediate floor.
		for (MapLayer layer : {source_layer, receiving}) {
			if (layer == MapLayer::Surface) continue;
			for (uint y = 18; y <= 24; ++y) {
				for (uint x = 18; x <= 24; ++x) floor.push_back(TileXYL(x, y, layer));
			}
		}
		this->Open(floor);
		if (direct_elevator) {
			MakeRoadLayerPortal(this->upper, _current_company, DiagDirection::SE, MapLayer::DeepUnderground, ROADTYPE_ROAD);
			MakeRoadLayerPortal(this->request.portal, _current_company, DiagDirection::NW, receiving, ROADTYPE_ROAD);
		} else {
			MakeRailLayerPortal(this->upper, _current_company, DiagDirection::SE, source_layer, RAILTYPE_RAIL);
			MakeRailLayerPortal(this->request.portal, _current_company, DiagDirection::NW, receiving, RAILTYPE_RAIL);
		}
		LayerPortalWorkRecord record;
		record.pair_key = this->upper; record.company = _current_company; record.state = LayerPortalWorkState::Connected;
		REQUIRE(ValidateLayerPortalWorkRecords(std::array{record}, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
		StageLoadedLayerPortalWork({record}); AcceptLoadedLayerPortalWork();
	}
	/** Release only this boundary fixture's accepted utility/portal owners before their map/company references disappear. */
	~GeothermalRiserRig() { ResetUtilityPowerState(); ResetLayerPortalWorkState(); }
};

/** Quoting a riser supplies physical admission and frozen terms without creating heat, electricity, a building or a bill. */
TEST_CASE("Riser admission quotes both actual connected housings without granting construction", "[geothermal-riser]")
{
	GeothermalRiserRig rig;
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto utility = ComputeUtilityPowerDigest();
	const auto portals = ComputeLayerPortalWorkDigest();
	const auto cash = Company::Get(_current_company)->money;
	const auto rng = _random;
	const auto quote = PrepareGeothermalRiser(rig.request);
	REQUIRE(quote.error == GeothermalError::None);
	REQUIRE(quote.construction != nullptr);
	CHECK(quote.construction->capital_cost == 120000);
	CHECK(quote.construction->work_ticks == 296);
	CHECK(quote.construction->transfer_capacity_w == 200000);
	CHECK(quote.footprint.size() == 4);
	CHECK(quote.destination == rig.upper);
	CHECK(quote.portal_revision == GetLayerPortalWorkRecord(rig.upper)->revision);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeUtilityPowerDigest() == utility);
	CHECK(ComputeLayerPortalWorkDigest() == portals);
	CHECK(Company::Get(_current_company)->money == cash);
	CHECK(std::ranges::equal(rng.state, _random.state));
}

/** A direct elevator reaches only its two real endpoints; an Underground cavity is not granted as a side effect. */
TEST_CASE("Direct Deep riser admission leaves intervening Underground solid", "[geothermal-riser]")
{
	GeothermalRiserRig rig(true);
	const auto intervening = MultiLayerMap::TileInLayer(rig.request.portal, MapLayer::Underground);
	REQUIRE(GetGeologyWorldState().terrain.Get(intervening) == ExcavationState::Solid);
	const auto quote = PrepareGeothermalRiser(rig.request);
	REQUIRE(quote.error == GeothermalError::None);
	CHECK(GetTileLayer(quote.destination) == MapLayer::Surface);
	CHECK(GetGeologyWorldState().terrain.Get(intervening) == ExcavationState::Solid);
}

/** The second physical rail riser is quotable on its own Underground floor, without implying forwarded electrical credit. */
TEST_CASE("Underground relay riser admission resolves its actual Surface endpoint", "[geothermal-riser]")
{
	GeothermalRiserRig rig(false, MapLayer::Underground);
	const auto quote = PrepareGeothermalRiser(rig.request);
	REQUIRE(quote.error == GeothermalError::None);
	CHECK(GetTileLayer(quote.destination) == MapLayer::Surface);
	CHECK(GetTileLayer(quote.footprint.front()) == MapLayer::Underground);
	CHECK_FALSE(HasUtilityPowerState());
}

/** Invalid two-endpoint ownership, technology and full-site input refuse atomically before any material/link owner participates. */
TEST_CASE("Riser admission refuses foreign sealed locked and incomplete sites", "[geothermal-riser]")
{
	GeothermalRiserRig rig;
	SECTION("foreign receiving housing") { SetTileOwner(rig.upper, CompanyID{1}); }
	SECTION("foreign sending housing") { SetTileOwner(rig.request.portal, CompanyID{1}); }
	SECTION("invalid portal") { rig.request.portal = INVALID_TILE; }
	SECTION("before1980") { TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, 11, 31); }
	SECTION("sealed") {
		auto records = CaptureLayerPortalWorkRecords(); records.front().state = LayerPortalWorkState::Sealed;
		StageLoadedLayerPortalWork(records); AcceptLoadedLayerPortalWork();
	}
	SECTION("unexcavated machinery") { rig.request.tile = rig.request.port = TileXYL(26, 26, MapLayer::DeepUnderground); }
	SECTION("different-layer floor") { rig.request.tile = MultiLayerMap::TileInLayer(rig.request.tile, MapLayer::Underground); }
	SECTION("missing access") { rig.request.access = DiagDirection::Invalid; }
	SECTION("nonadjacent port") { rig.request.port = TileAddWrap(rig.request.tile, 1, 1); }
	SECTION("occupied rail floor") { MakeRailNormal(rig.request.tile, _current_company, TrackBits{Track::X}, RAILTYPE_RAIL); }
	SECTION("map edge") { rig.request.tile = TileXYL(Map::MaxX() - 1, 20, MapLayer::DeepUnderground); }
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto utility = ComputeUtilityPowerDigest();
	const auto portals = ComputeLayerPortalWorkDigest();
	const auto cash = Company::Get(_current_company)->money;
	CHECK(PrepareGeothermalRiser(rig.request).error != GeothermalError::None);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(ComputeUtilityPowerDigest() == utility);
	CHECK(ComputeLayerPortalWorkDigest() == portals);
	CHECK(Company::Get(_current_company)->money == cash);
}
