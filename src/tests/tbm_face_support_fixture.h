/* This file is part of OpenTTD, licensed under GNU GPL version 2. */
/** @file tbm_face_support_fixture.h Reversible missing-piece fault on actual paid support.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native physical support ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-2-come-space.md Groups23CB/23CC.
 */
#ifndef TESTS_TBM_FACE_SUPPORT_FIXTURE_H
#define TESTS_TBM_FACE_SUPPORT_FIXTURE_H
#include "../rail_map.h"
#include "../pathfinder/yapf/yapf_cache.h"

/** Restore the real paid face's track layout after an explicit missing-piece fault. */
class TBMFacePieceFault {
	TileIndex tile; ///< Existing paid, initially unoccupied face.
	TrackBits original; ///< Its original native pieces, restored before fixture teardown.
public:
	/** @param tile Existing plain support tile whose actual layout is temporarily made incompatible. */
	explicit TBMFacePieceFault(TileIndex tile) : tile(tile), original(GetTrackBits(tile))
	{
		REQUIRE(IsPlainRailTile(tile));
		const Track across = original.Test(Track::X) ? Track::Y : Track::X;
		SetTrackBits(tile, TrackBits{across});
		YapfNotifyTrackLayoutChange(tile, Track::Invalid);
	}
	/** Restore only the injected track pieces and invalidate their derived pathfinder cache. */
	~TBMFacePieceFault()
	{
		SetTrackBits(this->tile, this->original);
		YapfNotifyTrackLayoutChange(this->tile, Track::Invalid);
	}
};

#endif /* TESTS_TBM_FACE_SUPPORT_FIXTURE_H */
