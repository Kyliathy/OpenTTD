/* This file is part of OpenTTD, licensed under GNU GPL version 2. */
/** @file tbm_face_status.cpp Presentation of a face meeting reached through actual native movement.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md view-independent work and cargo ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2df2-face-exchange-and-demo-acceptance.md Group6.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "tbm_face_lifecycle_fixture.h"
#include "layer_window_fixture.h"
#include "../tbm_gui.h"
#include "../safeguards.h"

/** A real held face meeting uses its face wording; opening and repainting the browser cannot advance it. */
TEST_CASE("TBM windows describe an actual native face wait without stepping it", "[tbm-face-status]")
{
	MockEnvironment::Instance();
	// Language loading rebuilds the ordinary cargo catalogue; do it before the native loop installs its component cargo fixture.
	LoadGeneratedWindowTestLanguage();
	SourceLoop loop(LoopStart::Long);
	TBMFaceLoopWatch watch;
	ReachTBMFaceWait(loop, watch, true);
	LayerWindowTestUI ui(WindowClass::TBMProjects, _current_company);
	REQUIRE(IsTBMFaceTurnHolder(loop.Cycle()));
	CHECK(GetTBMPhaseString(loop.Project().phase) == STR_TBM_PHASE_AWAIT_FACE);
	CHECK(GetTBMHaulerPhaseString(loop.Cycle()) == STR_PORTAL_HAULER_FACE_TRANSFER);
	const auto hauler_status = GetTBMVehicleStatus(*loop.hauler);
	REQUIRE(hauler_status.has_value());
	CHECK(hauler_status->find(GetString(STR_PORTAL_HAULER_FACE_TRANSFER)) != std::string::npos);
	const auto project = ComputeTBMProjectDigest();
	const auto exchange = ComputeTBMExchangeDigest();
	const auto geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto money = Company::Get(_current_company)->money;
	const std::array random{_random.state[0], _random.state[1]};
	Window *browser = ShowTBMProjectsWindow(MapLayer::Underground);
	REQUIRE(browser != nullptr);
	browser->OnPaint();
	CHECK(ComputeTBMProjectDigest() == project);
	CHECK(ComputeTBMExchangeDigest() == exchange);
	CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(_current_company)->money == money);
	CHECK((std::array{_random.state[0], _random.state[1]} == random));
}
