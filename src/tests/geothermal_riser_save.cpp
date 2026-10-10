/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_riser_save.cpp Exact staged construction ownership, malformed refusal and copied-coordinate boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group5B
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_riser_fixture.h"
#include "../geothermal_riser_wire.h"
#include "../multilayer_state.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** Typed staging preserves actual native shortage/work/tick and cannot debit materials or advance labour during load acceptance. */
TEST_CASE("Native riser staged reload retains stock partial work and once-only ticks", "[geothermal-riser-save]")
{
	NativeRiserRig rig; REQUIRE(rig.Fund().Succeeded()); auto &industry = rig.Riser();
	REQUIRE(rig.Deliver(CargoLabel{"STEL"}, 4) == 4);
	const auto shortage = CaptureGeothermalRisers(); const auto digest = ComputeGeothermalRiserDigest();
	REQUIRE(ValidateGeothermalRisers(shortage)); ResetGeothermalRisers(); StageLoadedGeothermalRisers(shortage); REQUIRE(AcceptLoadedGeothermalRisers());
	CHECK(ComputeGeothermalRiserDigest() == digest); rig.Tick(10); CHECK(GetGeothermalRiserState(industry.index)->work_ticks == 0);
	REQUIRE(rig.Deliver(CargoLabel{"RFMT"}, 4) == 4); REQUIRE(rig.Deliver(CargoLabel{"ELEC"}, 2) == 2); REQUIRE(rig.Deliver(CargoLabel{"BATY"}, 2) == 2);
	rig.Tick(148);
	const auto active = CaptureGeothermalRisers(); const auto partial = ComputeGeothermalRiserDigest();
	ResetGeothermalRisers(); StageLoadedGeothermalRisers(active); REQUIRE(AcceptLoadedGeothermalRisers()); CHECK(ComputeGeothermalRiserDigest() == partial);
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); AdvanceGeothermalRiser(industry);
		CHECK(GetGeothermalRiserState(industry.index)->work_ticks == 148);
	}
	rig.Tick(148); CHECK(IsGeothermalRiserOperational(industry.index));
	CHECK(industry.GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting == 0);
}

/** Missing identities, forged receipts/stages and future references reject the entire candidate without changing any published native stock/work. */
TEST_CASE("Native riser staged reload refuses malformed participants without repair", "[geothermal-riser-save]")
{
	NativeRiserRig rig; REQUIRE(rig.Fund().Succeeded()); auto &industry = rig.Riser();
	auto rows = CaptureGeothermalRisers();
	SECTION("omitted native machinery") { rows.clear(); }
	SECTION("unknown industry") { rows[0].industry = IndustryID{63999}; }
	SECTION("duplicate participant") { rows.push_back(rows.front()); }
	SECTION("free receipt") { rows[0].materials_committed = true; }
	SECTION("excess labour") { rows[0].work_ticks = 297; rows[0].materials_committed = rows[0].has_work_tick = true; }
	SECTION("future portal stamp") { ++rows[0].portal_revision; }
	SECTION("false abandonment") { rows[0].site.owner = CompanyID::Invalid(); }
	SECTION("wrong native completion") { SetIndustryCompleted(rig.request.tile); }
	SECTION("free native stage") { SetIndustryConstructionStage(rig.request.tile, 2); }
	SECTION("extra native stock") { industry.GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting = 5; }
	const auto digest = ComputeGeothermalRiserDigest();
	CHECK_FALSE(ValidateGeothermalRisers(rows)); StageLoadedGeothermalRisers(rows); CHECK_FALSE(AcceptLoadedGeothermalRisers());
	CHECK(ComputeGeothermalRiserDigest() == digest); CHECK_FALSE(IsGeothermalRiserOperational(industry.index));
}

/** Retired original housing and actual paid machinery survive exact staging even without any current portal feature or energy owner. */
TEST_CASE("Native riser retired construction retains its own native save dependency", "[geothermal-riser-save]")
{
	NativeRiserRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(296);
	const auto id = rig.Riser().index;
	REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.destination).Succeeded());
	const auto rows = CaptureGeothermalRisers(); const auto digest = ComputeGeothermalRiserDigest();
	REQUIRE(rows[0].portal_retired); REQUIRE(ValidateGeothermalRisers(rows));
	ResetGeothermalRisers(); StageLoadedGeothermalRisers(rows); REQUIRE(AcceptLoadedGeothermalRisers());
	CHECK(ComputeGeothermalRiserDigest() == digest); CHECK_FALSE(IsGeothermalRiserOperational(id));
	const auto schema = CaptureMultiLayerStateSchema();
	CHECK((schema.feature_flags & MULTILAYER_FEATURE_GEOTHERMAL_RISERS) != 0);
	CHECK((schema.feature_flags & MULTILAYER_FEATURE_PORTAL_WORK) == 0);
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::None);
	auto missing = schema; missing.feature_flags &= ~MULTILAYER_FEATURE_GEOLOGY;
	CHECK(ValidateMultiLayerStateReferences(missing) == MultiLayerStateError::UnsupportedFeatures);
}

/** Copied local dimensions and exact scalars constrain independent preview without relying on the live map or runtime boolean normalization. */
TEST_CASE("Native riser wire validates original coordinates and exact loss receipt fields", "[geothermal-riser-save]")
{
	NativeRiserRig rig; REQUIRE(rig.Fund().Succeeded());
	const auto state = *GetGeothermalRiserState(rig.Riser().index); const auto wire = EncodeGeothermalRiser(state);
	REQUIRE(ValidateGeothermalRiserWire(wire, 64, 64, 4)); CHECK(DecodeGeothermalRiser(wire) == state);
	auto bad = wire;
	SECTION("out of copied floor") { bad.x = 100; }
	SECTION("Space machinery") { bad.layer = to_underlying(MapLayer::Space); }
	SECTION("remote destination") { ++bad.destination_x; }
	SECTION("invalid retirement boolean") { bad.portal_retired = 2; }
	SECTION("invalid receipt boolean") { bad.materials_committed = 2; }
	SECTION("missing portal incarnation") { bad.portal_revision = 0; }
	SECTION("free completion") { bad.work_ticks = 296; }
	CHECK_FALSE(ValidateGeothermalRiserWire(bad, 64, 64, 4));
}
