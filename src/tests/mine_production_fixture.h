/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file mine_production_fixture.h Shared real finite mine/normal-economy fixture. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Groups 7-8 */

#ifndef TESTS_MINE_PRODUCTION_FIXTURE_H
#define TESTS_MINE_PRODUCTION_FIXTURE_H

#include "mine_fixture.h"
#include "../core/backup_type.hpp"

/** Run the real economy-tick industry dispatcher, as declared by the main simulation loop in openttd.cpp. */
void OnTick_Industry();

/** Fund a real native mine and finish its machinery; reserve mutation still goes through geology's public owner. */
class MineProductionRig : public MineAdmissionRig {
	AutoRestoreBackup<decltype(_economy.fluct)> fluct; ///< Restore caller recession state.
	AutoRestoreBackup<decltype(_settings_game.economy.industry_cargo_scale)> scale; ///< Restore caller declared production scale.
public:
	Industry *industry = nullptr; ///< Actual fixture-funded pool object, destroyed by the shared fixture.

	/** @param index Published mine recipe index, including Deep minerals. @param serialized_native_slots Match distributed cargo slots for actual full-game save fixtures. */
	explicit MineProductionRig(size_t index = 0, bool serialized_native_slots = false) : MineAdmissionRig(index, serialized_native_slots), fluct(_economy.fluct, 1), scale(_settings_game.economy.industry_cargo_scale, uint16_t{100})
	{
		this->PrepareFunding();
		REQUIRE(this->Fund(DoCommandFlag::Execute).Succeeded());
		this->industry = *Industry::Iterate().begin();
		// Complete only this industry's real machinery; the loading floor remains ordinary excavated floor.
		for (TileIndex tile : this->site) {
			if (this->industry->TileBelongsToIndustry(tile)) SetIndustryCompleted(tile);
		}
		_game_mode = GameMode::Normal;
		_pause_mode = {};
	}

	/** @return The actual remaining reserve in the sole geological owner. */
	uint64_t Reserve() const { return GetGeologyWorldState().deposits.Get(this->request.deposit)->remaining_reserve; }

	/** @param remaining Smaller fixture balance installed through the validated geological restoration boundary. */
	void SetReserve(uint64_t remaining)
	{
		auto snapshot = CaptureGeologyState();
		auto record = std::ranges::find(snapshot.deposits, this->request.deposit, &GeologyDeposit::id);
		REQUIRE(record != snapshot.deposits.end());
		record->remaining_reserve = remaining;
		this->RestoreGeology(snapshot);
	}
};


#endif /* TESTS_MINE_PRODUCTION_FIXTURE_H */
