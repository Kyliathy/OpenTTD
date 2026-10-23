/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_tube_power_fixture.h Actual paid native plant, purchased depot and electrical corridor for original geothermal tube source ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group6BBB.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Floor/materials and a nonzero native depot price are explicit boundary inputs; this is not paid-TBM/bootstrap or capsule-work acceptance.
 */
#ifndef TESTS_GEOTHERMAL_TUBE_POWER_FIXTURE_H
#define TESTS_GEOTHERMAL_TUBE_POWER_FIXTURE_H

#include "geothermal_generator_operating_fixture.h"
#include "../transport_tube_geothermal.h"
#include "../utility_power_transfer.h"
#include "../rail_cmd.h"
#include "../rail_map.h"
#include "../landscape_cmd.h"
#include "../economy_func.h"
#include "../safeguards.h"

/** Real completed native producer and two purchased tube depots/cable branches; no synthetic electrical source or granted quantity. */
class GeothermalTubePowerRig : public GeothermalGeneratorOperatingRig {
	AutoRestoreBackup<Money> depot_price{_price[Price::BuildDepotTrain], Money{10000}}; ///< Nonzero actual native depot invoice input, restored after this isolated world.
public:
	std::array<TileIndex, 2> inlets; ///< Original actual command-purchased native depots, distinct from electrical/source IDs.
	UtilityProducerID output{UINT32_MAX}; ///< Original native exact-conversion source, acquired only through actual operation.
	/** @param model Actual dated native plant definition. @param serialized Match shipped native content slots for ordinary save transport. Build real paid recipients/path, then publish one native source opportunity. */
	explicit GeothermalTubePowerRig(GeothermalGeneratorModel model = GeothermalGeneratorModel::MagmaLoop, bool serialized = false) : GeothermalGeneratorOperatingRig(serialized, true, 1000000, model)
	{
		ResetTubePowerState();
		const auto rail = InitialiseTubeRailType(this->ActivatedContent()); REQUIRE(rail != INVALID_RAILTYPE);
		Company::Get(_current_company)->avail_railtypes.Set(rail);
		this->inlets = {TileXYL(32, 24, MapLayer::DeepUnderground), TileXYL(32, 26, MapLayer::DeepUnderground)};
		const std::array cells{this->plant_port, TileXYL(35, 24, MapLayer::DeepUnderground), TileXYL(34, 24, MapLayer::DeepUnderground),
			TileXYL(33, 24, MapLayer::DeepUnderground), this->inlets[0], TileXYL(33, 25, MapLayer::DeepUnderground),
			TileXYL(32, 25, MapLayer::DeepUnderground), this->inlets[1]};
		this->Open(cells);
		// Each native depot includes its explicit excavated door approach and
		// pays through the ordinary command kernel before cable coexists with it.
		for (const auto inlet : this->inlets) {
			const auto delta = TileIndexDiffCByDiagDir(DiagDirection::SE); this->Open(std::array{TileAddWrap(inlet, delta.x, delta.y)});
			this->BuyDepot(inlet);
		}
		// The plant's original paid cell remains in place; every new branch cell
		// purchases exactly its own native installed electrical identity.
		for (const auto tile : cells) {
			if (!GetGeothermalUtilityLink(tile, UtilityEnergyDomain::Electricity).has_value()) this->Build(tile, UtilityEnergyDomain::Electricity);
		}
		this->Operate(); this->output = GetGeothermalGeneratorState(this->generator)->electrical_producer;
		REQUIRE(GetUtilityPowerProducer(this->output) != nullptr);
	}
	/** Clear original native depots while their actual companies and shared owners still exist, then reset only this world's tube metadata. */
	~GeothermalTubePowerRig()
	{
		// A takeover changes actual tile ownership. Cleanup uses that retained
		// native owner rather than trying to demolish the successor's depot as its predecessor.
		for (const auto inlet : this->inlets) {
			if (!IsRailDepotTile(inlet)) continue;
			AutoRestoreBackup actor(_current_company, CompanyID{GetTileOwner(inlet)});
			CHECK(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, inlet, std::make_tuple(inlet)).Succeeded());
		}
		ResetTubePowerState();
	}
	/** @param inlet Actual owned empty excavated recipient and approach. Purchase one native tube depot through its real invoice owner. */
	void BuyDepot(TileIndex inlet)
	{
		const auto before = Company::Get(_current_company)->money;
		REQUIRE(Command<Commands::BuildRailDepot>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, inlet,
			std::make_tuple(inlet, GetTubeRailType(), DiagDirection::SE)).Succeeded());
		CHECK(Company::Get(_current_company)->money < before); REQUIRE(GetTubeTerminal(inlet, _current_company).has_value());
	}
	/** @param inlet Existing actual recipient index. @return Successful original physical source association, without publishing or allocating electricity. */
	TubeGeothermalPowerResult Connect(size_t inlet = 0)
	{
		const auto result = ConnectTubeGeothermalPower(this->generator, this->inlets[inlet]); REQUIRE(result.error == TubePowerError::None); return result;
	}
	/** @param requests Actual existing root/recipient requests. Publish real collector Heat, convert/pay service and run one whole common allocation without a fabricated electrical rate. */
	void Allocate(std::span<const UtilityPowerRequest> requests)
	{
		++TimerGameTick::counter; AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
		PublishGeothermalHeatSources(GetUtilityPowerInterval(), 1); RunGeothermalGenerators(GetUtilityPowerInterval(), 1);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), requests) == UtilityPowerError::None);
	}
};

#endif /* TESTS_GEOTHERMAL_TUBE_POWER_FIXTURE_H */
