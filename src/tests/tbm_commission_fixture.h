/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file tbm_commission_fixture.h Real receivers and a sealed commissioning site shared by command/UI tests. @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md and r2mlvs7-underground-construction-and-fleet.md */

#ifndef TESTS_TBM_COMMISSION_FIXTURE_H
#define TESTS_TBM_COMMISSION_FIXTURE_H

#include "spoil_receiver_fixture.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"

/** The receiver rig with real receivers at its origin and a second, empty Sealed pair to commission at. */
class TBMCommissionRig {
public:
	SpoilReceiverRig rig; ///< Engines, origin, receivers, and the first pair with its own project.
	TileIndex portal = TileXYL(32, 20, MapLayer::Surface); ///< Upper end of the second pair.
	TileIndex rock = INVALID_TILE; ///< Its rock-side housing.
	TileIndex start = INVALID_TILE; ///< First corridor cell beyond the housing, straight out of the door.
	TileIndex end = INVALID_TILE; ///< Third corridor cell.

	/** Install the receivers and build the second pair through the real portal command. */
	TBMCommissionRig()
	{
		this->rig.BuildReceiver(SpoilReceiverKind::Earth, 15, 22);
		this->rig.BuildReceiver(SpoilReceiverKind::Rock, 14, 16);
		this->rig.RefreshOrigin();
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, this->portal, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
		REQUIRE(GetLayerPortalWorkState(this->portal) == LayerPortalWorkState::Sealed);
		this->rock = GetLayerPortalPairTile(this->portal);
		const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(this->rock));
		this->start = TileAddWrap(this->rock, step.x, step.y);
		this->end = TileAddWrap(this->rock, step.x * 3, step.y * 3);
		REQUIRE(IsValidTile(this->end));
	}

	/**
	 * Commission at the second pair through the command.
	 * @param flags Estimate or execute.
	 * @param origin Origin station; Invalid takes the rig's origin.
	 * @param earth Earth hoppers.
	 * @param rock Rock hoppers.
	 * @return The command result.
	 */
	CommandCost Commission(DoCommandFlags flags, StationID origin = StationID::Invalid(), uint8_t earth = 2, uint8_t rock = 2) const
	{
		if (origin == StationID::Invalid()) origin = this->rig.fixture.origin->index;
		return CmdCommissionTBM(flags, this->portal, 0, earth, rock, origin, this->start, this->end, false);
	}
};

#endif /* TESTS_TBM_COMMISSION_FIXTURE_H */
