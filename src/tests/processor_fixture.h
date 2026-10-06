/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file processor_fixture.h Actual native processor catalogue and command-funded Surface fixture. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 10 */

#ifndef TESTS_PROCESSOR_FIXTURE_H
#define TESTS_PROCESSOR_FIXTURE_H

#include "mine_fixture.h"
#include "../layer_industry_processor.h"
#include "../core/backup_type.hpp"

/** Real content/industry owners with a command-funded, fully constructed Surface processing facility. */
class ProcessorRig : public MineAdmissionRig {
	decltype(_industile_mngr.mappings) tile_mapping = _industile_mngr.mappings; ///< Restore caller tile identities.
	AutoRestoreBackup<int16_t> fluct{_economy.fluct, int16_t{1}}; ///< Explicit non-recession work fixture.
	AutoRestoreBackup<uint16_t> scale{_settings_game.economy.industry_cargo_scale, uint16_t{100}}; ///< Exact normal work rate.
public:
	Industry *industry = nullptr; ///< Actual pool-owned command-created facility.

	/** @param primary Frozen primary identity of the actual facility family to fund. @param x Surface anchor column. @param y Surface anchor row, evaluated after actual map setup. @param serialized_native_slots Match the pinned distributed custom-cargo layout for full game save/reload. */
	explicit ProcessorRig(MultiLayerRecipeID primary = MultiLayerRecipeID{"CFMT"}, uint x = 30, uint y = 30, bool serialized_native_slots = false) : MineAdmissionRig(0, serialized_native_slots)
	{
		REQUIRE(InitialiseLayerProcessorDefinitions(this->ActivatedContent()));
		this->PrepareFunding();
		const IndustryType type = GetLayerProcessorIndustryType(primary);
		REQUIRE(type != IT_INVALID);
		REQUIRE(CmdBuildIndustry(DoCommandFlag::Execute, TileXYL(x, y, MapLayer::Surface), type, 0, true, 123).Succeeded());
		REQUIRE(Industry::GetNumItems() == 1);
		this->industry = *Industry::Iterate().begin();
		// Complete the actual command footprint; production must never depend on a fabricated inventory-only industry.
		for (TileIndex tile : this->industry->location) {
			if (this->industry->TileBelongsToIndustry(tile)) SetIndustryCompleted(tile);
		}
		_game_mode = GameMode::Normal;
		_pause_mode = {};
		REQUIRE(GetLayerProcessorState(this->industry->index) != nullptr);
	}
	/** Restore tile identity mappings; the parent removes actual industries and side state before geological teardown. */
	~ProcessorRig() { _industile_mngr.mappings = std::move(this->tile_mapping); }
	/** @param label Stable delivered meaning. @return Actual sole input slot, refusing a missing native mapping. */
	Industry::AcceptedCargo &Input(CargoLabel label) const
	{
		auto slot = this->industry->GetCargoAccepted(GetCargoTypeByLabel(label));
		REQUIRE(slot != this->industry->accepted.end());
		return *slot;
	}
	/** @param label Stable product meaning. @return Actual sole output slot. */
	Industry::ProducedCargo &Output(CargoLabel label) const
	{
		auto slot = std::ranges::find(this->industry->produced, GetCargoTypeByLabel(label), &Industry::ProducedCargo::cargo);
		REQUIRE(slot != this->industry->produced.end());
		return *slot;
	}
	/** @param id Frozen choice. @return Its unchanged four-byte synchronized command encoding. */
	static uint32_t Encode(MultiLayerRecipeID id)
	{
		uint32_t value = 0;
		for (uint8_t byte : id) value = (value << 8) | byte;
		return value;
	}
};

#endif /* TESTS_PROCESSOR_FIXTURE_H */
