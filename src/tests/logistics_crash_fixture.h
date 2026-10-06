/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file logistics_crash_fixture.h Read-only receipts for native destruction of a genuinely crashed spoil train.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13AC
 */

#ifndef TESTS_LOGISTICS_CRASH_FIXTURE_H
#define TESTS_LOGISTICS_CRASH_FIXTURE_H

#include "../tbm_exchange.h"
#include "../spoil_receiver.h"
#include "../train.h"

/** Record original physical custody and count destruction only when the native owner deletes that exact wagon. */
class LogisticsCrashLedger {
	/** One original crashed unit; its cargo cannot be unloaded, reassigned or duplicated during delayed cleanup. */
	struct Unit {
		VehicleID id; ///< Original physical wagon identity.
		uint tonnes; ///< Real packet quantity retained when the crashed wait was observed.
		SpoilReceiverKind kind; ///< Original Earth/Rock cargo identity.
		bool deleted = false; ///< A native deletion has already been receipted once.
	};
	std::vector<Unit> units; ///< Immutable custody baseline with one receipt flag per original unit.

public:
	VehicleID hauler; ///< Actual crashed head; no replacement head is adopted.
	TBMWagonSetID set; ///< Actual exclusive set that disappears after its last wagon.
	std::array<uint64_t, 2> destroyed{}; ///< Earth and Rock tonnes lost through observed native deletion.

	/**
	 * Capture actual physical cargo before any delayed crash deletion.
	 * @param assignment Live assignment in the native Crashed wait after an ordinary collision.
	 */
	explicit LogisticsCrashLedger(const TBMExchangeAssignment &assignment) : hauler(assignment.hauler), set(assignment.set)
	{
		REQUIRE(assignment.wait == TBMHaulerWait::Crashed);
		const auto *carried = GetTBMWagonSet(this->set);
		REQUIRE(carried != nullptr);
		REQUIRE(carried->state == TBMWagonSetState::Loaded);
		// Keep each real unit rather than a pooled replacement quantity; delayed native deletion acts one wagon at a time.
		for (VehicleID id : carried->wagons) {
			const Train *wagon = Train::Get(id);
			REQUIRE(wagon->First()->index == this->hauler);
			REQUIRE(wagon->vehstatus.Test(VehState::Crashed));
			const auto kind = GetSpoilReceiverKindForCargo(wagon->cargo_type);
			REQUIRE(kind != SpoilReceiverKind::End);
			this->units.push_back({id, wagon->cargo.TotalCount(), kind});
		}
		REQUIRE_FALSE(this->units.empty());
	}

	/**
	 * Check retained custody and issue a receipt only for an actual missing original unit.
	 * @return True after the native owner has deleted the original head and every set unit.
	 */
	bool Observe()
	{
		// Surviving crash packets retain their quantity and physical carrier. Each disappearance contributes exactly once
		// to the destruction ledger, so save/load cannot disguise lost cargo as disposal income or a second exchange.
		for (auto &unit : this->units) {
			const Train *wagon = Train::GetIfValid(unit.id);
			if (unit.deleted) {
				REQUIRE(wagon == nullptr);
				continue;
			}
			if (wagon == nullptr) {
				unit.deleted = true;
				this->destroyed[unit.kind == SpoilReceiverKind::Earth ? 0 : 1] += unit.tonnes;
			} else {
				REQUIRE(wagon->vehstatus.Test(VehState::Crashed));
				REQUIRE(wagon->First()->index == this->hauler);
				CHECK(wagon->cargo.TotalCount() == unit.tonnes);
				CHECK(GetSpoilReceiverKindForCargo(wagon->cargo_type) == unit.kind);
			}
		}
		if (Train::GetIfValid(this->hauler) != nullptr) return false;
		CHECK(std::ranges::all_of(this->units, &Unit::deleted));
		CHECK(GetTBMWagonSet(this->set) == nullptr);
		CHECK(GetTBMExchangeAssignmentForHauler(this->hauler) == nullptr);
		return true;
	}
};

#endif /* TESTS_LOGISTICS_CRASH_FIXTURE_H */
