/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply.cpp Actual-owner and detached whole paid-order admission boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13C.1.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15B.
 * All isolated inputs are declared component fixtures; no packet/facility procurement is claimed.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../surface_supply.h"
#include "../multilayer_bootstrap.h"
#include "../economy_base.h"
#include "../cargotype.h"
#include "../station_base.h"
#include "../station_cmd.h"
#include "../station_map.h"
#include "../town.h"
#include "../town_kdtree.h"
#include "../newgrf_station.h"
#include "../rail.h"
#include "../station_func.h"
#include "../cargopacket.h"
#include "../core/backup_type.hpp"
#include "../table/strings.h"
#include "excavation_fixture.h"
#include "mock_environment.h"
#include "layer_window_fixture.h"
#include "surface_supply_fixture.h"
#include "../safeguards.h"

/** Every whole native seed quote is pure, label-resolved and priced through the current economy owner. */
TEST_CASE("Surface seed quotes read actual owned freight stations without spending or reserving", "[surface-supply-admission]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig;
	const auto money = Company::Get(_current_company)->money; const auto packets = CargoPacket::GetNumItems(); const auto random = _random;
	const auto definition = GENERATE(0, 1, 2, 3);
	const std::array<CargoLabel, 4> labels{CargoLabel{"STEL"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}, CargoLabel{"BATY"}};
	const std::array<uint16_t, 4> caps{256, 128, 64, 64};
	const std::array<uint32_t, 4> prices{100, 180, 500, 350};
	const std::array<int, 4> first_years{1950, 1950, 1965, 1965};
	SurfaceSupplyOrderRequest request{rig.station, _current_company, labels[definition], caps[definition]};
	const auto view = CaptureSurfaceSupplyOrderView(request, 0);
	const auto quote = PrepareSurfaceSupplyOrder(request, view);
	CHECK(quote.error == SurfaceSupplyOrderError::None); CHECK(quote.quantity == caps[definition]);
	CHECK(quote.cargo == GetCargoTypeByLabel(labels[definition])); CHECK(quote.cost == Money{caps[definition]} * prices[definition]);
	auto date_boundary = view;
	date_boundary.year = TimerGameCalendar::Year{first_years[definition] - 1};
	CHECK(PrepareSurfaceSupplyOrder(request, date_boundary).error == SurfaceSupplyOrderError::Date);
	date_boundary.year = TimerGameCalendar::Year{first_years[definition]};
	CHECK(PrepareSurfaceSupplyOrder(request, date_boundary).error == SurfaceSupplyOrderError::None);
	CHECK(Company::Get(_current_company)->money == money); CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(Station::Get(rig.station)->goods[view.cargo].TotalCount() == 0);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	CHECK(CaptureSurfaceSupplyOrderView(request, 0).company_quantity_today == 0);
}

/** Detached boundaries isolate every refusal and exact overflow/cap edge without allocating gigantic fake stocks. */
TEST_CASE("Surface seed whole admission refuses missing owners limits dates funds and native room", "[surface-supply-admission]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig;
	SurfaceSupplyOrderRequest request{rig.station, _current_company, CargoLabel{"ELEC"}, 2};
	auto view = CaptureSurfaceSupplyOrderView(request, 62);
	REQUIRE(PrepareSurfaceSupplyOrder(request, view).error == SurfaceSupplyOrderError::None);
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
	SurfaceSupplyOrderError expected = SurfaceSupplyOrderError::Station;
	switch (fault) {
		case 0: view.profile = MultiLayerGameplayProfileID::Classic; expected = SurfaceSupplyOrderError::Profile; break;
		case 1: view.company_exists = false; expected = SurfaceSupplyOrderError::Company; break;
		case 2: view.station_exists = false; break;
		case 3: view.station_owner = OWNER_NONE; break;
		case 4: view.station_layer = MapLayer::Underground; break;
		case 5: view.loading_station = false; break;
		case 6: view.cargo = INVALID_CARGO; expected = SurfaceSupplyOrderError::Cargo; break;
		case 7: request.label = CargoLabel{"PROP"}; expected = SurfaceSupplyOrderError::Cargo; break;
		case 8: request.quantity = 0; expected = SurfaceSupplyOrderError::Quantity; break;
		case 9: view.year = TimerGameCalendar::Year{1964}; expected = SurfaceSupplyOrderError::Date; break;
		case 10: view.company_quantity_today = 63; expected = SurfaceSupplyOrderError::DailyLimit; break;
		case 11: view.company_quantity_today = UINT32_MAX; expected = SurfaceSupplyOrderError::DailyLimit; break;
		case 12: view.station_quantity = UINT32_MAX - 1; expected = SurfaceSupplyOrderError::StationSpace; break;
		case 13: view.packet_room = false; expected = SurfaceSupplyOrderError::PacketSpace; break;
		case 14: view.inflation_prices = UINT64_MAX; expected = SurfaceSupplyOrderError::Price; break;
		case 15: view.available_money = 999; expected = SurfaceSupplyOrderError::Funds; break;
		case 16: view.cargo = GetCargoTypeByLabel(CargoLabel{"STEL"}); expected = SurfaceSupplyOrderError::Cargo; break;
	}
	const auto quote = PrepareSurfaceSupplyOrder(request, view);
	CHECK(quote.error == expected); CHECK(quote.quantity == 0); CHECK(quote.cost == 0); CHECK(quote.cargo == INVALID_CARGO);
}

/** Exact-price and native-room edges admit the entire order, never a lossy prefix. */
TEST_CASE("Surface seed quotes retain native inflation exact cash and whole stock boundaries", "[surface-supply-admission]")
{
	MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage(); SurfaceSupplyAdmissionRig rig;
	SurfaceSupplyOrderRequest request{rig.station, _current_company, CargoLabel{"STEL"}, 2};
	auto view = CaptureSurfaceSupplyOrderView(request, 254);
	view.inflation_prices = 3 * (uint64_t{1} << 15); view.available_money = 300; view.station_quantity = UINT32_MAX - 2;
	const auto quote = PrepareSurfaceSupplyOrder(request, view);
	CHECK(quote.error == SurfaceSupplyOrderError::None); CHECK(quote.quantity == 2); CHECK(quote.cost == 300);
	view.available_money = 299; CHECK(PrepareSurfaceSupplyOrder(request, view).error == SurfaceSupplyOrderError::Funds);
	view.available_money = 300; view.inflation_prices = 0; CHECK(PrepareSurfaceSupplyOrder(request, view).error == SurfaceSupplyOrderError::Price);
	request.origin = StationID::Invalid(); CHECK(PrepareSurfaceSupplyOrder(request, CaptureSurfaceSupplyOrderView(request, 0)).error == SurfaceSupplyOrderError::Station);
	request.origin = rig.station; request.buyer = CompanyID::Invalid(); CHECK(PrepareSurfaceSupplyOrder(request, CaptureSurfaceSupplyOrderView(request, 0)).error == SurfaceSupplyOrderError::Company);
}
