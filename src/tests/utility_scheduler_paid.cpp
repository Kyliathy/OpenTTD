/* This file is part of OpenTTD. Licensed under the GNU GPL version 2. */
/** @file utility_scheduler_paid.cpp Same-world canonical purchase, loaded Tube and native shared consumers.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md
 * The existing bounded fixture supplies its disclosed floor/material/frame inputs.
 * New ground stock is paid and dispatched only by the canonical order/station
 * owners. A separately declared17-unit native payload exercises actual movement;
 * this component case makes no genuine bootstrap or cross-layer delivery claim.
 */
#include "../stdafx.h"
#include <filesystem>
#include "utility_scheduler_mixed_fixture.h"
#include "layer_window_fixture.h"
#include "save_candidate_lifecycle_fixture.h"
#include "../map_layer_gui.h"
#include "../viewport_func.h"
#include "../viewport_type.h"
#include "../surface_supply_cmd.h"
#include "../surface_supply_order_runtime.h"
#include "../economy_base.h"
#include "../safeguards.h"

/** Dispatch actual native station scheduling at the same declared component tick. */
extern void OnTick_Station();

static const NWidgetPart _paid_scheduler_widgets[] = {
    NWidget(NWID_VIEWPORT, Colours::Invalid, 0), SetMinimalSize(320, 200), SetResize(1, 1),
}; ///< Native camera only; no synchronized owner is selected by this widget.
static WindowDesc _paid_scheduler_desc(WindowPosition::Manual, {}, 320, 200,
    WindowClass::ExtraViewport, WindowClass::None, {}, _paid_scheduler_widgets);

/** Actual layer camera used to compare synchronized ownership with freight hidden. */
class PaidSchedulerViewport final : public Window {
public:
    /** @param tile Native moving front's initial tile; it supplies no simulation ownership. */
    explicit PaidSchedulerViewport(TileIndex tile) : Window(_paid_scheduler_desc)
    {
        this->InitNested(0);
        this->GetWidget<NWidgetViewport>(0)->InitializeViewport(this, tile, ZoomLevel::Viewport);
    }
};

/** One bounded combined world with the existing native generator/town/orbital/Tubefleet owners. */
class PaidSchedulerRig final : public UtilitySchedulerMixedRig {
    AutoRestoreBackup<uint64_t> inflation{_economy.inflation_prices, uint64_t{1} << 16}; ///< Declared uninflated fixture quote; actual command settlement stays native.
public:
    Station *origin = nullptr; ///< New actual owned Surface freight station.
    SurfaceSupplyOrderID intent = 0; ///< Sole canonical monotonic purchase identity.

    /** Construct only the missing Surface source and pay for one canonical complete lot. */
    PaidSchedulerRig() : UtilitySchedulerMixedRig(GeothermalGeneratorModel::VentTap)
    {
        ResetSurfaceSupplyOrders();
        const auto tile = TileXYL(20, 20, MapLayer::Surface);
        REQUIRE(Command<Commands::BuildRailStation>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile,
            std::make_tuple(tile, RAILTYPE_RAIL, Axis::X, uint8_t{1}, uint8_t{1}, STAT_CLASS_DFLT, uint16_t{0}, StationID::Invalid(), false)).Succeeded());
        this->origin = Station::GetByTile(tile); REQUIRE(this->origin != nullptr);
        this->intent = GetNextSurfaceSupplyOrderID();
        constexpr uint32_t steel_wire = 0x5354454C; // STEL's explicit synchronized command encoding.
        const auto cash = Company::Get(_current_company)->money;
        const auto purchase = Command<Commands::PlaceSurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, INVALID_TILE,
            std::make_tuple(this->origin->index, steel_wire, uint16_t{4}, this->intent));
        REQUIRE(purchase.Succeeded()); REQUIRE(purchase.GetCost() > Money{0});
        CHECK(Company::Get(_current_company)->money == cash - purchase.GetCost());
        const auto retry = Command<Commands::PlaceSurfaceSupplyOrder>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, INVALID_TILE,
            std::make_tuple(this->origin->index, steel_wire, uint16_t{4}, this->intent));
        REQUIRE(retry.Succeeded()); CHECK(retry.GetCost() == Money{0});
        CHECK(Company::Get(_current_company)->money == cash - purchase.GetCost());
        REQUIRE(GetSurfaceSupplyOrder(this->intent)->Native()->state == SurfaceSupplyOrderState::Pending);
    }

    /** Release actual station-held packets before inherited pool teardown, without a gameplay refund. */
    ~PaidSchedulerRig()
    {
        // Pool cleanup assumes the application also cleans the cargo pool.
        // This bounded fixture instead retires each real station container,
        // preserving independent packet identities for the next camera replay.
        for (auto *station : Station::Iterate()) {
            for (auto &goods : station->goods) {
                if (goods.HasData()) goods.GetData().cargo.Truncate();
            }
        }
        ResetSurfaceSupplyOrders();
    }
};

/** Complete synchronized owner observations at one matched native opportunity. */
struct PaidSchedulerFrame {
    SurfaceSupplyOrderSnapshot book; ///< Exact original tagged order history.
    decltype(CandidateLiveState{}.packets) packets; ///< Every real fragment and finite supply/custody field.
    decltype(CandidateLiveState{}.accounts) accounts; ///< Every native account and expense/cargo ledger.
    std::array<uint64_t, 12> fields; ///< Native clock/RNG, all combined domains and actual moving/stopped pose.
    /** @param other Same native tick in the other camera world. @return Exact synchronized ownership equality. */
    bool operator==(const PaidSchedulerFrame &other) const = default;
};

/**
 * Execute matched native opportunities with all named participants in the same world.
 * @param visible True observes the actual Deep freight; false observes Space.
 * @return Complete original book/packet/account/domain observations per matched tick.
 */
static std::vector<PaidSchedulerFrame> RunPaidScheduler(bool visible)
{
    PaidSchedulerRig rig;
    auto *moving = rig.Buy(), *manual = rig.Buy(true);
    REQUIRE(CargoPacket::CanAllocateItem());
    auto *packet = CargoPacket::Create(StationID::Invalid(), uint16_t{17}, Source{Source::Invalid, SourceType::Industry});
    packet->UpdateLoadingTile(moving->tile);
    moving->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep); moving->CargoChanged();
    LayerWindowTestUI ui(WindowClass::ExtraViewport, _current_company);
    Window *view = new PaidSchedulerViewport(moving->tile);
    std::vector<PaidSchedulerFrame> result;
    bool moved = false;
    const auto start = moving->tile;
    // Each matched opportunity uses the same one-pass allocator and actual
    // station/orbital/vehicle owners. The camera supplies no work, stock, cash
    // or random state; only its own presentation coordinates may differ.
    for (uint frame = 0; frame < Ticks::STATION_ACCEPTANCE_TICKS + 8; ++frame) {
        const auto layer = visible ? MapLayer::DeepUnderground : MapLayer::Space;
        SetViewportLayer(view, layer);
        ScrollWindowToTile(MultiLayerMap::TileInLayer(moving->tile, layer), view, true);
        {
            AutoRestoreBackup clipped(_screen.width, 0);
            UpdateViewportPosition(view, 0);
        }
        rig.Opportunity(std::array{moving, manual});
        OnTick_Station();
        const auto utility = ComputeUtilityPowerDigest();
        const auto money = Company::Get(_current_company)->money;
        PrepareTubePowerTick();
        CHECK(ComputeUtilityPowerDigest() == utility); CHECK(Company::Get(_current_company)->money == money);
        CHECK(manual->vehstatus.Test(VehState::Stopped)); CHECK(moving->cargo.StoredCount() == 17);
        moved |= moving->tile != start;
        const auto snapshot = CaptureCandidateLiveState();
        result.push_back({snapshot.book, snapshot.packets, snapshot.accounts,
            {TimerGameTick::counter, _random.state[0], _random.state[1], utility,
             ComputeGeothermalGeneratorDigest(), ComputeTubeGeothermalPowerDigest(), ComputeSpaceStructureDigest(),
             ComputeGeologyStateDigest(GetGeologyWorldState()), moving->tile.base(), static_cast<uint64_t>(moving->x_pos),
             static_cast<uint64_t>(moving->y_pos), manual->reliability}});
    }
    CHECK(moved); CHECK(GetUtilityPowerConsumer(rig.town_consumer)->town_effect_applied);
    CHECK(GetGeothermalGeneratorState(rig.generator)->active_service_ticks > 0);
    CHECK(rig.assembly->module->manufacture->work_ticks > 0);
    CHECK(rig.processor->module->processing->paid_lifetime_j > 0);
    REQUIRE(GetSurfaceSupplyOrder(rig.intent)->Native()->state == SurfaceSupplyOrderState::Fulfilled);
    CHECK(rig.origin->goods[GetCargoTypeByLabel(CargoLabel{"STEL"})].TotalCount() == 4);
    CHECK(CmdCancelSurfaceSupplyOrder({}, rig.intent).Failed());
    fmt::print("MIXED_PAID_SCHEDULER visible={} frames={} moved={} paid_quantity=4 fulfilled=1 manual_stopped=1\n", visible, result.size(), moved);
    return result;
}

/** Matched camera worlds retain the complete original paid/packet/account ledger per native tick. */
TEST_CASE("Combined paid ground orbital geothermal town and loaded Tube work ignore camera layer", "[utility-scheduler-paid-visible-hidden]")
{
    MockEnvironment::Instance(); LoadGeneratedWindowTestLanguage();
    const auto visible = RunPaidScheduler(true), hidden = RunPaidScheduler(false);
    REQUIRE(visible.size() == hidden.size());
    // Compare each actual original tick, so a duplicated purchase, skipped
    // consumer, changed grant priority or lost packet cannot hide in an end total.
    for (size_t index = 0; index < visible.size(); ++index) {
        INFO("combined paid native frame=" << index);
        REQUIRE(visible[index].book == hidden[index].book);
        REQUIRE(visible[index].packets == hidden[index].packets);
        REQUIRE(visible[index].accounts == hidden[index].accounts);
        // Identify the exact synchronized owner when a frame differs, so the
        // merged boundary can be corrected without expanding this workload.
        for (size_t field = 0; field < visible[index].fields.size(); ++field) {
            INFO("owner field=" << field);
            REQUIRE(visible[index].fields[field] == hidden[index].fields[field]);
        }
    }
}
