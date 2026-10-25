/* This file is part of OpenTTD. Licensed under the GNU GPL version 2. */
/** @file ground_network_merge.cpp Actual merged application peers and canonical paid command retries.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 * Original paid inputs are loaded normally; observers never supply stock, cash or utility grants.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "ground_network_active_fixture.h"
#include "ground_network_thermal_fixture.h"
#include "ground_network_power_fixture.h"
#include "../surface_supply_cmd.h"
#include "../surface_supply_order_runtime.h"
#include "../station_base.h"
#include "../vehicle_cmd.h"
#include "../company_cmd.h"
#include "../transport_tube_geothermal.h"
#include "../safeguards.h"

/** A genuinely joined active face uses the original native controller and complete writer. */
TEST_CASE("Merged real peers preserve paid active excavation", "[.merged-ground-peer-active]")
{
    RunGroundNetworkActivePeer(true);
}

/** Paid thermal conversion survives real agreed portal closure, repair and service commands. */
TEST_CASE("Merged real peers preserve thermal conversion recovery", "[.merged-ground-peer-thermal]")
{
    RunGroundNetworkThermalRiserPeer();
}

/** Real loaded capsule source closure preserves physical custody and once-only energy history. */
TEST_CASE("Merged real peers preserve loaded external source waits", "[.merged-ground-peer-power]")
{
    RunGroundNetworkPowerLossPeer();
}

/** A retained prepared binding component joins through real peers without claiming paid bootstrap. */
TEST_CASE("Merged real peers preserve geothermal capsule provider roles", "[.merged-ground-peer-geothermal]")
{
    std::optional<uint64_t> first;
    uint64_t initial_energy = 0;
    /** @param frame Actual agreed native frame. @param server Actual host/client role, never a replacement company. */
    RunGroundNetworkPeer([&](uint32_t frame, bool server) {
        const auto *source = GetTubeGeothermalSource(TubeGeothermalSourceID{0}); REQUIRE(source != nullptr); REQUIRE(source->active);
        const auto *output = GetUtilityPowerProducer(source->producer); REQUIRE(output != nullptr);
        if (!first.has_value()) { first = TimerGameTick::counter; initial_energy = output->consumed_lifetime_j; }
        const auto *manual = Train::Get(VehicleID{0}), *running = Train::Get(VehicleID{1});
        REQUIRE(manual != nullptr); REQUIRE(running != nullptr); CHECK(manual->vehstatus.Test(VehState::Stopped));
        const auto *binding = GetTubePowerVehicle(running->index); REQUIRE(binding != nullptr);
        CHECK(binding->geothermal_source == source->id); CHECK(binding->local_feed.base() != UINT32_MAX);
        CHECK(output->consumed_lifetime_j >= initial_energy);
        if (TimerGameTick::counter == *first + 512) {
            CHECK(output->consumed_lifetime_j > initial_energy);
            fmt::print("MERGED_PEER_GEOTHERMAL role={} frame={} provider={} feed={} consumed={} manual_stopped=1\n",
                server ? "server" : "client", frame, source->id.base(), binding->local_feed.base(), output->consumed_lifetime_j);
            std::fflush(stdout);
        }
    }, true, true);
}

/** A company client posts a canonical purchase twice and retries after real station fulfillment. */
TEST_CASE("Merged real company peers settle canonical supply once", "[.merged-ground-peer-supply]")
{
    /** Invite the client through the actual company access command before its native identify phase. */
    std::function<void()> prepare = [] {
        AutoRestoreBackup actor(_current_company, CompanyID{0});
        REQUIRE(Command<Commands::CompanyAllowListControl>::Post(CompanyAllowListCtrlAction::AllowAny, std::string{}));
    };
    AutoRestoreBackup hook(_ground_network_startup_observer, &prepare);
    IConsole::CmdRegister("ground_network_work_ready", GroundNetworkWorkReady);
    std::optional<uint64_t> first;
    SurfaceSupplyOrderID intent = 0;
    StationID origin = StationID::Invalid();
    Money quote{0};
    // Both peers observe the same actual opportunity. Only the real company
    // client posts input; server admission and later native station ticks own
    // settlement, dispatch and the permanent end of refund rights.
    /** @param frame Actual agreed native frame. @param server Actual host role; only the joined company client posts purchases. */
    RunGroundNetworkPeer([&](uint32_t frame, bool server) {
        if (!first.has_value()) {
            first = TimerGameTick::counter;
            intent = GetNextSurfaceSupplyOrderID();
            // Select existing company-owned Surface logistics, never create a
            // replacement station or teleport purchased material into a tube.
            for (const auto *station : Station::Iterate()) {
                if (station->owner != CompanyID{0}) continue;
                if (!MultiLayerMap::IsTileInLayer(station->xy, MapLayer::Surface)) continue;
                origin = station->index; break;
            }
            REQUIRE(origin != StationID::Invalid());
        }
        const auto offset = TimerGameTick::counter - *first;
        AutoRestoreBackup actor(_current_company, CompanyID{0});
        if (offset == 8) {
            const auto cash = Company::Get(_current_company)->money;
            const auto estimate = CmdPlaceSurfaceSupplyOrder({}, origin, 0x5354454C, uint16_t{4}, intent);
            REQUIRE(estimate.Succeeded()); quote = estimate.GetCost(); REQUIRE(quote > Money{0});
            CHECK(Company::Get(_current_company)->money == cash); CHECK(GetSurfaceSupplyOrder(intent) == nullptr);
            if (!server) {
                REQUIRE(_local_company == CompanyID{0});
                REQUIRE(Command<Commands::PlaceSurfaceSupplyOrder>::Post(origin, uint32_t{0x5354454C}, uint16_t{4}, intent));
                REQUIRE(Command<Commands::PlaceSurfaceSupplyOrder>::Post(origin, uint32_t{0x5354454C}, uint16_t{4}, intent));
            }
        }
        if (offset == 384) {
            const auto *order = GetSurfaceSupplyOrder(intent); REQUIRE(order != nullptr); REQUIRE(order->Native() != nullptr);
            CHECK(order->Native()->state == SurfaceSupplyOrderState::Fulfilled); CHECK(order->Native()->cost == quote);
            const auto cash = Company::Get(_current_company)->money;
            const auto retry = CmdPlaceSurfaceSupplyOrder({}, origin, 0x5354454C, uint16_t{4}, intent);
            REQUIRE(retry.Succeeded()); CHECK(retry.GetCost() == Money{0});
            CHECK(CmdCancelSurfaceSupplyOrder({}, intent).Failed()); CHECK(Company::Get(_current_company)->money == cash);
            if (!server) REQUIRE(Command<Commands::PlaceSurfaceSupplyOrder>::Post(origin, uint32_t{0x5354454C}, uint16_t{4}, intent));
            fmt::print("MERGED_PEER_SUPPLY role={} frame={} intent={} cost={} fulfilled=1 duplicate_refund=0\n", server ? "server" : "client", frame, intent, quote.base());
            std::fflush(stdout);
        }
    }, true, true);
}
