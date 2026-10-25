/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file normal_ground_video_fixture.h Native null-driver loop terminated by the actual application observer.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs16-integrated-acceptance-and-balance.md Group6DCDD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * Production null's iteration limit counts paused/loading opportunities as well
 * as simulation. This test driver retains its native rendering/startup and all
 * three ordinary public dispatchers,ending on the real application's exit.
 * The external controller owns the110-second wall-clock bound and child reaping.
 */
#ifndef TESTS_NORMAL_GROUND_VIDEO_FIXTURE_H
#define TESTS_NORMAL_GROUND_VIDEO_FIXTURE_H

#include "../video/null_v.h"
#include "../openttd.h"
#include "../window_func.h"

inline std::function<void()> *_normal_ground_complete_frame = nullptr; ///< Optional invocation-owned observation after all ordinary frame dispatchers.

/** No private clock,vehicle,industry or utility dispatch may replace the native application loop. */
class NormalGroundVideoDriver final : public VideoDriver_Null {
public:
	/** Run complete native application frames; the external runner owns timeout/reaping. */
	void MainLoop() override
	{
		// Keep simulation, player input and presentation in the real application order.
		while (!_exit_game) {
			::GameLoop();
			::InputLoop();
			::UpdateWindows();
			// Observe only a completed live frame; shutdown must not dispatch another owner.
			if (!_exit_game && _normal_ground_complete_frame != nullptr) (*_normal_ground_complete_frame)();
		}
	}
	/** @return Registered test-only driver identity, preserving the production null driver. */
	std::string_view GetName() const override { return "ground-acceptance-null"; }
};

/** Register a test-only driver through the actual native public factory. */
class FNormalGroundVideoDriver final : public DriverFactoryBase {
public:
	/** Register this bounded callback driver through the existing native factory. */
	FNormalGroundVideoDriver() : DriverFactoryBase(Driver::Type::Video, 0, "ground-acceptance-null", "Native normal-ground acceptance null loop") {}
	/** @return A fresh native null-driver instance with no private world state. */
	std::unique_ptr<Driver> CreateInstance() const override { return std::make_unique<NormalGroundVideoDriver>(); }
};

inline FNormalGroundVideoDriver _normal_ground_video_driver; ///< Registration only;no game state or private participant owner.

#endif /* TESTS_NORMAL_GROUND_VIDEO_FIXTURE_H */
