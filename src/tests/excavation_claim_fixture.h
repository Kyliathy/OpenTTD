/* This file is part of OpenTTD, licensed under GNU GPL version 2. */
/** @file excavation_claim_fixture.h Real independent job reservation for planning-conflict tests.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md original geological ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Groups23C-23CB.
 */
#ifndef TESTS_EXCAVATION_CLAIM_FIXTURE_H
#define TESTS_EXCAVATION_CLAIM_FIXTURE_H
#include "excavation_fixture.h"

/**
 * Reserve another project's work: a job of two straight SW (+x) cuts from an opened starter, whose unopened swept cells
 * (x+2..x+3, y-1..y+1 around the anchor) are claimed. No route of a different job may sweep them.
 * @param world Existing native fixture whose terrain/claim owners are used.
 * @param anchor Starter cell; its 3x3 is opened first.
 * @return The reserved job.
 */
inline ExcavationJobID ClaimSeparateExcavationWork(ExcavationFixture &world, TileIndex anchor)
{
	std::vector<TileIndex> starter;
	// Declare only the foreign job's initial supported pocket; its unfinished cells must be claimed by the native owner.
	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) starter.push_back(TileAddWrap(anchor, x, y));
	}
	world.Open(starter);
	ExcavationJob job;
	job.company = _current_company;
	job.model_id = GetTileLayer(anchor) == MapLayer::DeepUnderground ? 1 : 0;
	job.commissioning_anchor = anchor;
	job.requested_tiles = {TileAddWrap(anchor, 2, 0)};
	for (int y = -1; y <= 1; ++y) {
		for (int x = 0; x <= 3; ++x) job.authorized_tiles.push_back(TileAddWrap(anchor, x, y));
	}
	// Both cuts advance SW from the opened starter, each sweeping its whole 3x3 body.
	for (int step = 1; step <= 2; ++step) {
		ExcavationCutGeometry cut;
		cut.cutter_position = TileAddWrap(anchor, step, 0);
		cut.heading = Direction::SW;
		cut.support_tiles = {anchor};
		for (int y = -1; y <= 1; ++y) {
			for (int x = -1; x <= 1; ++x) cut.swept_tiles.push_back(TileAddWrap(cut.cutter_position, x, y));
		}
		job.route.push_back(std::move(cut));
	}
	const auto reserved = ReserveExcavationJob(job, true);
	REQUIRE(reserved.error == ExcavationError::None);
	return reserved.id;
}

#endif /* TESTS_EXCAVATION_CLAIM_FIXTURE_H */
