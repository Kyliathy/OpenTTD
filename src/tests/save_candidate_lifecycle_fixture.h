/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_candidate_lifecycle_fixture.h Read-only native evidence for preview isolation and consecutive load recovery.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 * This is test observation, never a replacement canonical state owner or persistence format.
 */
#ifndef TESTS_SAVE_CANDIDATE_LIFECYCLE_FIXTURE_H
#define TESTS_SAVE_CANDIDATE_LIFECYCLE_FIXTURE_H
#include "../3rdparty/catch2/catch.hpp"
#include "../company_base.h"
#include "../cargopacket.h"
#include "../core/random_func.hpp"
#include "../surface_supply_order_runtime.h"
#include "../geothermal_generator.h"
#include "../transport_tube_geothermal.h"
#include "../transport_tube.h"
#include "../space_vehicle.h"
#include "../space_structure.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "../fios.h"
#include "../saveload/saveload_func.h"
#include "../saveload/saveload.h"
#include <tuple>

/** Exact native stock, financial and view scalars plus existing domain diagnostics; no new hash scheme. */
struct CandidateLiveState {
	SurfaceSupplyOrderSnapshot book; ///< Actual tagged receipts, including issuer and era-specific evidence.
	std::vector<std::tuple<CargoPacketID, uint16_t, Source, Money, uint64_t, uint8_t>> packets; ///< Every physical fragment, original purpose and departure byte.
	std::vector<std::tuple<CompanyID, Money, uint8_t, Money, std::array<Expenses, 3>, Money, Money, CargoArray>> accounts; ///< Real money, fractions, debt, invoices and current-quarter cargo receipts.
	std::array<uint32_t, 4> random; ///< Simulation and interactive random state, observed without drawing either.
	std::array<uint64_t, 6> domains; ///< Existing utility, construction, source, capsule, Space and structure diagnostics.
	uint64_t tick; ///< Native simulation counter; preview cannot advance work.
	std::vector<int64_t> camera; ///< Actual main viewport layer, zoom, position and target; empty only without a native viewport.
	/** @param other Independently recaptured native observations. @return Exact equality, without restoring or repairing any owner. */
	bool operator==(const CandidateLiveState &other) const = default;
};

/** Read actual installed owners before/after a copied attempt; never borrows candidate fields.
 * @return Exact scalar observations and existing native domain digests.
 */
inline CandidateLiveState CaptureCandidateLiveState()
{
	CandidateLiveState result; result.book = CaptureSurfaceSupplyOrders();
	// Every real fragment is compared separately so matching totals cannot hide
	// a lost packet, changed transfer credit or altered permanent supply purpose.
	for (const auto *packet : CargoPacket::Iterate()) result.packets.emplace_back(packet->index, packet->Count(), packet->GetSource(),
		packet->GetFeederShare(), packet->GetCompanySupplyOrder(), packet->GetCompanySupplyShippedWire());
	// Financial owners remain independent of receipt counters and utility meters;
	// a preview must not issue an invoice, credit, loan change or delivery receipt.
	for (const auto *company : Company::Iterate()) result.accounts.emplace_back(company->index, company->money, company->money_fraction,
		company->current_loan, company->yearly_expenses, company->cur_economy.income, company->cur_economy.expenses, company->cur_economy.delivered_cargo);
	result.random = {_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	result.domains = {ComputeUtilityPowerDigest(), ComputeGeothermalGeneratorDigest(), ComputeTubeGeothermalPowerDigest(),
		ComputeTubePowerVehicleDigest(), ComputeSpaceVehicleDigest(), ComputeSpaceStructureDigest()};
	result.tick = TimerGameTick::counter;
	const auto *main = FindWindowById(WindowClass::MainWindow, 0);
	// A native viewport must stay on its exact chosen layer and target while
	// copied files inspect independent geometry; headless fixture worlds have none.
	if (main != nullptr && main->viewport != nullptr) {
		const auto &view = *main->viewport;
		result.camera = {view.left, view.top, view.width, view.height, view.virtual_left, view.virtual_top, view.virtual_width, view.virtual_height,
			to_underlying(view.zoom), to_underlying(view.layer), view.follow_vehicle.base(), view.scrollpos_x, view.scrollpos_y, view.dest_scrollpos_x, view.dest_scrollpos_y};
	}
	return result;
}

/** Assert a refused copied candidate exposes no reusable paid work, grants or provider bindings.
 * Its localized error remains available so clearing cannot hide the refusal reason.
 */
inline void CheckDiscardedLoadCandidate()
{
	CHECK(_load_check_data.error != INVALID_STRING_ID); CHECK(_load_check_data.checkable);
	CHECK_FALSE(_load_check_data.surface_supply_orders_present); CHECK(_load_check_data.surface_supply_orders.snapshot.records.empty());
	CHECK(_load_check_data.geothermal_generator_rows.empty()); CHECK(_load_check_data.utility_conversion_rows.empty());
	CHECK(_load_check_data.utility_power_state.producers.empty()); CHECK(_load_check_data.tube_geothermal_state.sources.empty());
	CHECK(_load_check_data.tube_vehicle_rows.empty()); CHECK(_load_check_data.space_production_rows.empty());
	CHECK(_load_check_data.native_cargo_packet_rows.empty()); CHECK_FALSE(GetLoadCapabilities().has_value());
}
#endif /* TESTS_SAVE_CANDIDATE_LIFECYCLE_FIXTURE_H */
