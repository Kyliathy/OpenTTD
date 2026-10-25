/* This file is part of OpenTTD, licensed under GNU GPL version 2. */
/** @file tbm_remainder_fixture.h Paid-cut and real receiver fixture for remainder and native command tests.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical cargo and geological ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Groups23C/23CC.
 * Exchanges explicitly empty packets and restore the component phase; this fixture does not certify driven haulage.
 */
#ifndef TESTS_TBM_REMAINDER_FIXTURE_H
#define TESTS_TBM_REMAINDER_FIXTURE_H
#include "spoil_receiver_fixture.h"

/**
 * A working project on six straight cuts out of the door, whose origin has both spoil receivers as a real quote needs,
 * with helpers to bore and to draw a new remainder.
 */
class TBMRemainderRig : public SpoilReceiverRig {
public:
	uint8_t points = 0; ///< Measured planning history of the real consist.

	/** Publish the project on six straight cuts, give its origin both receivers and measure its consist. */
	TBMRemainderRig() : SpoilReceiverRig(6)
	{
		this->BuildReceiver(SpoilReceiverKind::Earth, 15, 22);
		this->BuildReceiver(SpoilReceiverKind::Rock, 14, 16);
		this->RefreshOrigin();
		const auto dimensions = MeasureTBMConsist(*this->front, this->Project());
		REQUIRE(dimensions.error == TBMAcquisitionError::None);
		this->points = dimensions.planning_points;
	}

	/**
	 * Complete cuts. After each one the rig empties the wagons and returns to Bore, standing in for the exchange and the
	 * drive onto the new support that this rig cannot perform.
	 * @param cuts Number of cuts to complete.
	 */
	void Bore(uint cuts) const
	{
		// Earn each completed prefix through the paid cut owner, then use the declared component exchange.
		for (uint i = 0; i < cuts; ++i) {
			this->BoreOneCut();
			this->ExchangeAndReturn();
		}
	}

	/**
	 * A new floor beside the planned one: from the last completed cut, turn once and drive straight on.
	 * @param heading Heading of the first new advance, 45 degrees from the current one.
	 * @param cuts Number of advances.
	 * @return Outline whose floor is the last advance's cell and whose working space is every advance's sweep.
	 */
	ExcavationOutline Turn(Direction heading, uint cuts) const
	{
		auto pose = ReplayExcavationRoute(this->Job(), this->points, this->Job().cursor).next;
		std::vector<TileIndex> authorized;
		// Each advance keeps the turned heading; its sweep is accepted working space.
		for (uint i = 0; i < cuts; ++i) {
			const auto step = AdvanceExcavationPlanningPose(pose, heading);
			REQUIRE(step.error == ExcavationError::None);
			authorized.insert(authorized.end(), step.cut.swept_tiles.begin(), step.cut.swept_tiles.end());
			pose = step.next;
		}
		auto outline = NormalizeExcavationOutline(GetTileLayer(pose.position), std::array{pose.position});
		const auto work = NormalizeExcavationOutline(outline.layer, authorized);
		REQUIRE(outline.error == ExcavationError::None);
		REQUIRE(work.error == ExcavationError::None);
		outline.authorized_tiles = work.requested_tiles;
		return outline;
	}

	/** @return The job's own floor and working space, which its planned remainder already covers. */
	ExcavationOutline Same() const
	{
		ExcavationOutline outline;
		outline.layer = GetTileLayer(this->Job().commissioning_anchor);
		outline.requested_tiles = this->Job().requested_tiles;
		outline.authorized_tiles = this->Job().authorized_tiles;
		return outline;
	}

	/** @return Spoil aboard the wagons now, as the TBM owner measures it. */
	ExcavationLoadStart Load() const
	{
		return {this->Spoil(TBMConsistRole::EarthWagon), this->Spoil(TBMConsistRole::RockWagon)};
	}
};

#endif /* TESTS_TBM_REMAINDER_FIXTURE_H */
