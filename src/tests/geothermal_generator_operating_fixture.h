/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_generator_operating_fixture.h Native collector/plant/pipe/cable component operating boundary; prepared floor and explicit materials leave the complete paid-TBM/carrier bootstrap to Group8F.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8DC-8E.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_GEOTHERMAL_GENERATOR_OPERATING_FIXTURE_H
#define TESTS_GEOTHERMAL_GENERATOR_OPERATING_FIXTURE_H

#include "geothermal_generator_fixture.h"
#include "../geothermal_collector.h"
#include "../geothermal_utility.h"
#include "../geothermal_utility_cmd.h"
#include "../transport_tube.h"

/** Actual funded completed native machinery and installed cardinal route, without fake source/consumer accounts or private energy quantities. */
class GeothermalGeneratorOperatingRig : public GeothermalGeneratorRig {
	AutoRestoreBackup<PauseModes> pause{_pause_mode, PauseModes{}}; ///< Restore actual pause policy after synchronized operating opportunities.
public:
	IndustryID generator = IndustryID::Invalid(); ///< Original paid native generator, distinct from the actual collector.
	IndustryID collector = IndustryID::Invalid(); ///< Original paid native thermal source.
	TileIndex source_port = INVALID_TILE; ///< Actual collector's consumed blueprint boundary.
	TileIndex plant_port = INVALID_TILE; ///< Actual generator's consumed blueprint boundary.
	std::vector<TileIndex> pipe; ///< Explicit purchased physical route, including both ports; no energy balance.
	/** @param serialized Match shipped native cargo slots for ordinary operating files. @param install_electricity Install the actual paid output cell immediately. @param thermal_rate Declared original natural-field fixture rate, before any machinery/source is bound. @param model Actual canonical dated machinery, sharing the same physical source/path/service owner. */
	explicit GeothermalGeneratorOperatingRig(bool serialized = false, bool install_electricity = true, uint64_t thermal_rate = 1000000,
		GeothermalGeneratorModel model = GeothermalGeneratorModel::VentTap) : GeothermalGeneratorRig(serialized, model)
	{
		ResetUtilityPowerState(); ResetTubePowerVehicles(); ResetGeothermalCollectors();
		auto geology = CaptureGeologyState(); geology.vents.front().thermal_capacity_w = thermal_rate;
		GeologyWorldState original; REQUIRE(PrepareGeologyRestoration(geology, original) == GeologyError::None);
		StageLoadedGeologyState(std::move(original)); AcceptLoadedGeologyState();
		REQUIRE(this->Fund().Succeeded()); this->DeliverBill();
		this->Tick(static_cast<uint16_t>(GetMultiLayerGeothermalConstruction(this->request.kind)->work_ticks));
		this->generator = this->Generator().index; this->plant_port = this->request.endpoint;
		REQUIRE(InitialiseGeothermalCollectorDefinitions(this->ActivatedContent()));
		const auto vent = GetGeologyWorldState().vents.front(); // Opening the explicit fixture floor replaces geology storage; retain the original value, never a dangling field reference.
		this->source_port = TileXYL(vent.x, vent.y, vent.layer);
		GeothermalSiteRequest source{vent.id, _current_company, MultiLayerGeothermalConstructionKind::Collector, this->source_port, this->source_port, DiagDirection::SE};
		std::vector<TileIndex> floor;
		// Supply complete source floor as an explicit native boundary input.
		// Construction still charges capital, consumes native stock and earns work.
		for (int y = -1; y <= 3; ++y) {
			for (int x = -1; x <= 3; ++x) floor.push_back(TileAddWrap(source.tile, x, y));
		}
		this->Open(floor); const auto claim = ClaimGeothermalSite(source); REQUIRE(claim.error == GeothermalError::None);
		REQUIRE(Command<Commands::BuildGeothermalCollector>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, source.tile,
			std::make_tuple(source.tile, claim.id, uint32_t{123})).Succeeded());
		this->collector = FindGeothermalCollectorForVent(vent.id); REQUIRE(Industry::IsValidID(this->collector));
		// Explicit receiver-boundary material inputs still pass the real whole construction owner; no first-plant transport claim is made.
		for (const auto &input : GetMultiLayerGeothermalConstruction(MultiLayerGeothermalConstructionKind::Collector)->inputs) {
			REQUIRE(ReceiveGeothermalCollectorCargo(*Industry::Get(this->collector), GetCargoTypeByLabel(input.label), input.quantity) == input.quantity);
		}
		this->Tick(296); REQUIRE(IsGeothermalCollectorOperational(this->collector));
		this->pipe = {this->source_port, TileAddWrap(this->source_port, -1, 0)};
		auto tile = this->pipe.back();
		// Exit the sole collector port to its outside column, follow an outside
		// row, then approach the generator's sole port from its outside column.
		const int row = static_cast<int>(TilePlaneY(this->plant_port)) - 1;
		while (static_cast<int>(TilePlaneY(tile)) != row) {
			tile = TileAddWrap(tile, 0, static_cast<int>(TilePlaneY(tile)) < row ? 1 : -1); this->pipe.push_back(tile);
		}
		const int column = static_cast<int>(TileX(this->plant_port)) - 1;
		while (static_cast<int>(TileX(tile)) != column) {
			tile = TileAddWrap(tile, static_cast<int>(TileX(tile)) < column ? 1 : -1, 0); this->pipe.push_back(tile);
		}
		this->pipe.push_back(TileAddWrap(this->plant_port, -1, 0)); this->pipe.push_back(this->plant_port);
		this->Open(this->pipe);
		for (const auto cell : this->pipe) this->Build(cell, UtilityEnergyDomain::Heat); // Every distinct installed path cell pays its real synchronized construction invoice.
		if (install_electricity) this->Build(this->plant_port, UtilityEnergyDomain::Electricity);
	}
	/** Retire native machinery while its actual utility owner still exists, then clear only the isolated world authorities. */
	~GeothermalGeneratorOperatingRig()
	{
		// Retire actual machinery/coupled roles before clearing its original shared quantity owner.
		for (Industry *industry : Industry::Iterate()) delete industry;
		ResetGeothermalCollectors(); ResetUtilityPowerState(); ResetTubePowerVehicles();
	}
	/** @param tile Exact open native physical cell. @param domain Separate purchased network domain. Install through the real paid command kernel. */
	void Build(TileIndex tile, UtilityEnergyDomain domain)
	{
		REQUIRE(Command<Commands::BuildGeothermalUtilityLink>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, tile, std::make_tuple(tile, domain)).Succeeded());
	}
	/** Exercise native components in their required order without wiring the later production scheduler.
	 * @param count Explicit synchronized component opportunities; real floor/material arrival remain declared fixture inputs.
	 */
	void Operate(uint16_t count = 1)
	{
		// A declared component opportunity uses the same pause/mode gate as
		// normal simulation. Each native owner still owns its own admission,
		// publication, once-only service and quantity mutation.
		for (uint16_t index = 0; index < count; ++index) {
			++TimerGameTick::counter;
			if (_game_mode != GameMode::Normal || _pause_mode.Any()) continue;
			{
				AutoRestoreBackup system{_current_company, OWNER_DEITY};
				if (IsUtilityPowerAllocated()) REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
			}
			const auto interval = GetUtilityPowerInterval();
			PublishGeothermalHeatSources(interval, 1); RunGeothermalGenerators(interval, 1);
			AutoRestoreBackup system{_current_company, OWNER_DEITY};
			const auto closed = AllocateUtilityPower(interval, {});
			// A temporary calendar lock may refuse the numerical close too;
			// ordinary admitted opportunities must complete the one shared pass.
			if (CheckDeepLayerAvailability(GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) CHECK(closed == UtilityPowerError::None);
		}
	}

};

#endif /* TESTS_GEOTHERMAL_GENERATOR_OPERATING_FIXTURE_H */
