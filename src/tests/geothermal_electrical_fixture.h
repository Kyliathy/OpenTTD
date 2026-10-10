/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_electrical_fixture.h Actual command-funded two-riser relay on explicitly supplied test floor/material inputs.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group5
 */

#ifndef TESTS_GEOTHERMAL_ELECTRICAL_FIXTURE_H
#define TESTS_GEOTHERMAL_ELECTRICAL_FIXTURE_H

#include "geothermal_riser_fixture.h"
#include "../geothermal_electrical.h"
#include "../geothermal_utility_cmd.h"

/** @param tile Native world cell. @return Every native packed field through public accessors; no raw storage or host padding participates in coexistence proofs. */
inline std::array<uint32_t, 10> ElectricalNativeFloor(TileIndex tile)
{
	Tile native{tile}; return {native.type(), native.height(), native.m1(), native.m2(), native.m3(), native.m4(), native.m5(), native.m6(), native.m7(), native.m8()};
}

/** @param tile Actual excavated world cell. @param domain Exact network. @param remove Remove an existing installation instead. @param estimate Quote without expense/identity changes. @return Ordinary native command-kernel settlement/refusal. */
inline CommandCost ElectricalCableCommand(TileIndex tile, UtilityEnergyDomain domain = UtilityEnergyDomain::Electricity, bool remove = false, bool estimate = false)
{
	if (remove) return Command<Commands::RemoveGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate, tile, std::make_tuple(tile, domain));
	return Command<Commands::BuildGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate, tile, std::make_tuple(tile, domain));
}

/** Two actual complete paid industries and command-connected rail pairs; accounts alone provide no relay cable, generation or grant. */
class NativeElectricalRelayRig : public NativeRiserRig {
public:
	GeothermalRiserRequest upper; ///< Actual Underground sending machinery and separate Surface/Underground portal pair.
	IndustryID upper_industry = IndustryID::Invalid(); ///< Actual funded native second machine.
	UtilityConsumerID source{UINT32_MAX}, relay{UINT32_MAX}, destination_account{UINT32_MAX}; ///< Registered Deep/Underground/Surface reservations.
	UtilityLinkID first{UINT32_MAX}, second{UINT32_MAX}; ///< Original shared paid physical directions.
	UtilityProducerID boundary_producer{UINT32_MAX}; ///< Optional actual shared fixture electricity input; it proves no operating geothermal conversion.
	TileIndex source_port = TileXYL(21, 19, MapLayer::DeepUnderground); ///< Explicit installed input needed before the first machinery inlet.
	/** @param serialized_native_slots Match shipped cargo identities for actual native saves. @param boundary_electricity Publish an explicit shared external test offer before registering the source reservation; no plant/heat conversion is fabricated. */
	explicit NativeElectricalRelayRig(bool serialized_native_slots = false, bool boundary_electricity = false) : NativeRiserRig(false, serialized_native_slots)
	{
		REQUIRE(this->Fund().Succeeded()); this->DeliverBill(); this->Tick(296);
		this->upper = {this->request.owner, TileXYL(30, 20, MapLayer::Underground), TileXYL(31, 20, MapLayer::Underground),
			TileXYL(31, 20, MapLayer::Underground), DiagDirection::SE};
		std::vector<TileIndex> floor;
		// Explicit fixture floor connects two separately built native pairs.
		// Paid excavation and driven delivery remain the later bootstrap proof.
		for (uint y = 18; y <= 24; ++y) {
			for (uint x = 25; x <= 34; ++x) floor.push_back(TileXYL(x, y, MapLayer::Underground));
		}
		this->Open(floor);
		const auto surface = MultiLayerMap::TileInLayer(this->upper.portal, MapLayer::Surface);
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, surface, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, INVALID_ROADTYPE).Succeeded());
		// Real native approach ownership/counts and Connected state authorize
		// both machines; no test-side commissioning flag is assigned directly.
		for (const auto endpoint : {this->upper.portal, surface}) {
			REQUIRE(CmdBuildSingleRail(DoCommandFlag::Execute, GetLayerPortalDoorTile(endpoint), RAILTYPE_RAIL,
				AxisToTrack(DiagDirToAxis(GetLayerPortalShaftDirection(endpoint))), false).Succeeded());
		}
		REQUIRE(ChangeLayerPortalConnection(this->upper.portal, this->request.owner, true, true) == LayerPortalWorkError::None);
		{
			AutoRestoreBackup site(this->request, this->upper);
			REQUIRE(this->Fund().Succeeded()); this->DeliverBill(); this->Tick(296); this->upper_industry = this->Riser().index;
		}
		std::vector<UtilityProducerID> source_producers;
		if (boundary_electricity) {
			const auto source = RegisterUtilityPowerProducer(this->request.owner, MapLayer::DeepUnderground, UtilityGenerationKind::External);
			REQUIRE(source.has_value()); this->boundary_producer = *source; source_producers.push_back(*source);
			REQUIRE(PublishUtilityGeneration(*source, GetUtilityPowerInterval(), 1000000, 74) == UtilityPowerError::None);
		}
		const auto source_id = RegisterUtilityPowerConsumer(this->request.owner, MapLayer::DeepUnderground, UtilityConsumerEffect::AssemblyWork, 1000000, source_producers);
		const auto relay_id = RegisterUtilityPowerConsumer(this->request.owner, MapLayer::Underground, UtilityConsumerEffect::AssemblyWork, 1000000, {});
		const auto destination_id = RegisterUtilityPowerConsumer(this->request.owner, MapLayer::Surface, UtilityConsumerEffect::AssemblyWork, 1000000, {});
		REQUIRE(source_id.has_value()); REQUIRE(relay_id.has_value()); REQUIRE(destination_id.has_value());
		this->source = *source_id; this->relay = *relay_id; this->destination_account = *destination_id;
		const auto first_id = RegisterGeothermalRiserLink(this->Riser().index, this->source, this->relay);
		const auto second_id = RegisterGeothermalRiserLink(this->upper_industry, this->relay, this->destination_account);
		REQUIRE(first_id.has_value()); REQUIRE(second_id.has_value()); this->first = *first_id; this->second = *second_id;
	}
	/** Install the source and exact machinery-port cells through ordinary capital settlement; this action creates no energy. */
	void InstallSourceCable()
	{
		REQUIRE(ElectricalCableCommand(this->source_port).Succeeded());
		REQUIRE(ElectricalCableCommand(this->request.port).Succeeded());
	}
	/** @param gap Optional missing Underground column for a real disconnected-path proof. Install remaining housing-to-inlet cells, retaining every native traffic field. */
	void InstallRelayCable(std::optional<uint> gap = {})
	{
		// Every actual cardinal cell, including both portal housings and the
		// second machine's sole port, must be installed and charged separately.
		for (uint x = 20; x <= 31; ++x) {
			if (gap == x) continue;
			const auto tile = TileXYL(x, 20, MapLayer::Underground);
			const auto native = ElectricalNativeFloor(tile);
			REQUIRE(ElectricalCableCommand(tile).Succeeded()); CHECK(ElectricalNativeFloor(tile) == native);
		}
	}
};

#endif /* TESTS_GEOTHERMAL_ELECTRICAL_FIXTURE_H */
