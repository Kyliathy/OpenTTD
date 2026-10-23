/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_baseline_receiver.cpp Explicit original receiver identity, exact Iron/Steel conversion and refusal characterization. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 11 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "mine_fixture.h"
#include "../layer_industry_baseline.h"
#include "../train.h"
#include "../economy_base.h"

#include "../safeguards.h"

/** Execute the actual simulation dispatcher declared by openttd.cpp; no test-only production schedule substitutes for it. */
void OnTick_Industry();

/** Real original definition meanings resolve into the shuffled fixture registry, then the ordinary funding command creates the reused steelworks. */
class BaselineReceiverRig : public MineAdmissionRig {
public:
	Industry *steelworks = nullptr; ///< Existing baseline identity, not a duplicate installed facility.
	/** Resolve actual original Coal/Iron/Steel labels exactly as GRF finalization does and fund the original steelworks. */
	BaselineReceiverRig()
	{
		// The reduced fixture omits GRF loading. Translate preserved original label properties into its actual registry as FinaliseIndustriesArray does.
		for (IndustryType type : {IndustryType{1}, IndustryType{8}}) {
			auto &spec = *const_cast<IndustrySpec *>(GetIndustrySpec(type));
			for (size_t slot = 0; slot < spec.accepts_cargo_label.size(); ++slot) {
				if (const auto *label = std::get_if<CargoLabel>(&spec.accepts_cargo_label[slot])) spec.accepts_cargo[slot] = GetCargoTypeByLabel(*label);
			}
			for (size_t slot = 0; slot < spec.produced_cargo_label.size(); ++slot) {
				if (const auto *label = std::get_if<CargoLabel>(&spec.produced_cargo_label[slot])) spec.produced_cargo[slot] = GetCargoTypeByLabel(*label);
			}
		}
		REQUIRE(GetLayerBaselineReceiverType(CT_COAL) == 1);
		REQUIRE(GetLayerBaselineReceiverType(CT_IRON_ORE) == 8);
		this->PrepareFunding();
		REQUIRE(CmdBuildIndustry(DoCommandFlag::Execute, TileXYL(30, 30, MapLayer::Surface), 8, 0, true, 123).Succeeded());
		this->steelworks = *Industry::Iterate().begin();
		for (TileIndex tile : this->steelworks->location) {
			if (this->steelworks->TileBelongsToIndustry(tile)) SetIndustryCompleted(tile);
		}
		_game_mode = GameMode::Normal;
		_pause_mode = {};
	}
};

/** Matching cargo is necessary but callbacks, independent rate output and incompatible conversion meaning cannot grant the baseline adapter. */
TEST_CASE("Baseline receiver reuse validates actual cargo and production semantics", "[layer-baseline-receiver]")
{
	BaselineReceiverRig rig;
	CHECK(rig.steelworks->type == 8);
	CHECK(GetLayerBaselineReceiverType(CargoLabel{"URAN"}) == IT_INVALID);
	CHECK(GetLayerBaselineReceiverType(CargoLabel{"CORE"}) == IT_INVALID);
	CHECK(IsLayerBaselineSteelDefinition(8));
	auto &spec = *const_cast<IndustrySpec *>(GetIndustrySpec(8));
	spec.input_cargo_multiplier[0][0] = 128;
	CHECK_FALSE(IsLayerBaselineSteelDefinition(8));
	CHECK(GetLayerBaselineReceiverType(CT_IRON_ORE) == IT_INVALID);
	spec.input_cargo_multiplier[0][0] = 256;
	spec.callback_mask.Set(IndustryCallbackMask::ProductionCargoArrival);
	CHECK_FALSE(IsLayerBaselineSteelDefinition(8));
	spec.callback_mask = {};
	spec.produced_cargo[0] = GetCargoTypeByLabel(CargoLabel{"RFMT"});
	CHECK_FALSE(IsLayerBaselineSteelDefinition(8));
}

/** Bounded baseline output converts only the admitted Iron and later retry consumes the retained remainder once. */
TEST_CASE("Reused steelworks retains Iron at full output and supplies exact Steel packets", "[layer-baseline-receiver]")
{
	BaselineReceiverRig rig;
	Industry &industry = *rig.steelworks;
	const CargoType iron = GetCargoTypeByLabel(CT_IRON_ORE);
	const CargoType steel = GetCargoTypeByLabel(CT_STEEL);
	REQUIRE(ReceiveLayerBaselineSteelCargo(industry, iron, 4) == 4);
	auto input = industry.GetCargoAccepted(iron);
	auto output = std::ranges::find(industry.produced, steel, &Industry::ProducedCargo::cargo);
	REQUIRE(input != industry.accepted.end());
	REQUIRE(output != industry.produced.end());
	output->waiting = UINT16_MAX - 1;
	CHECK(ProcessLayerBaselineSteel(industry) == 1);
	CHECK(input->waiting == 3);
	CHECK(output->waiting == UINT16_MAX);
	CHECK(ProcessLayerBaselineSteel(industry) == 0);
	CHECK(input->waiting == 3);
	CHECK(LayerBaselineSteelHasStoredCargo(industry));
	Station *station = rig.StationAt(TileXYL(29, 30, MapLayer::Surface));
	station->goods[steel].rating = UINT8_MAX;
	station->goods[steel].last_speed = 100;
	CHECK(DispatchLayerBaselineSteel(industry) == 255);
	CHECK(station->goods[steel].TotalCount() == 255);
	CHECK(ProcessLayerBaselineSteel(industry) == 3);
	CHECK(input->waiting == 0);
	CHECK(output->waiting + station->goods[steel].TotalCount() == uint32_t{UINT16_MAX - 1} + 4);
	CHECK(output->history[THIS_MONTH].production == 4);
	CHECK((*input->history)[THIS_MONTH].accepted == 4);
}

/** Real final-delivery payment/packet owners deliver Iron to the selected existing receiver before the actual industry dispatcher produces Steel. */
TEST_CASE("Iron packets deliver through the reused baseline identity and become Steel once", "[layer-baseline-receiver]")
{
	BaselineReceiverRig rig;
	REQUIRE(Vehicle::GetNumItems() == 0);
	Station *destination = rig.StationAt(TileXYL(29, 30, MapLayer::Surface));
	Station *origin = rig.StationAt(TileXYL(20, 20, MapLayer::Surface));
	const CargoType iron = GetCargoTypeByLabel(CT_IRON_ORE);
	REQUIRE(Train::CanAllocateItem());
	Train *carrier = Train::Create();
	carrier->owner = _current_company;
	carrier->last_station_visited = destination->index;
	REQUIRE(CargoPayment::CanAllocateItem());
	CargoPayment *payment = CargoPayment::Create(carrier);
	carrier->cargo_payment = payment;
	REQUIRE(CargoPacket::CanAllocateItem());
	CargoPacket *packet = CargoPacket::Create(origin->index, 4, Source{IndustryID::Invalid(), SourceType::Industry});
	packet->UpdateLoadingTile(origin->xy);
	carrier->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
	carrier->cargo.Stage(true, destination->index, {}, OrderUnloadType::UnloadIfPossible, &destination->goods[iron], iron, payment, destination->xy);
	const auto delivered = Company::Get(_current_company)->cur_economy.delivered_cargo[iron];
	CHECK(carrier->cargo.Unload(4, &destination->goods[iron].GetOrCreateData().cargo, iron, payment, destination->xy) == 4);
	CHECK(carrier->cargo.TotalCount() == 0);
	CHECK(rig.steelworks->GetCargoAccepted(iron)->waiting == 4);
	OnTick_Industry(); // The actual bounded owner converts on the synchronized industry tick, with no deferred legacy pointer custody.
	CHECK(rig.steelworks->GetCargoAccepted(iron)->waiting == 0);
	auto steel = std::ranges::find(rig.steelworks->produced, GetCargoTypeByLabel(CT_STEEL), &Industry::ProducedCargo::cargo);
	REQUIRE(steel != rig.steelworks->produced.end());
	CHECK(steel->waiting == 4);
	CHECK(steel->history[THIS_MONTH].production == 4);
	CHECK(Company::Get(_current_company)->cur_economy.delivered_cargo[iron] == delivered + 4);
	CHECK(ProcessLayerBaselineSteel(*rig.steelworks) == 0);
	delete payment;
	_vehicle_pool.CleanPool(); // Isolated packet carrier has no constructed engine/statistics; release its real packet containers before world teardown.
}
