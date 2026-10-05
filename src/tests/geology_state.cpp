/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file geology_state.cpp Finite reserve, cavity, deleted-ID and corrupt-transport restoration boundaries. @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 4 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../excavation_state.h"
#include "../settings_type.h"
#include "../multilayer_state.h"

#include "../safeguards.h"

/**
 * Prepare actual generation authority as a save snapshot without publishing a new profile.
 * @return Complete transport for a caller-allocated 64x64/four-plane map.
 */
static GeologyStateSnapshot GeologySnapshotFixture()
{
	const auto generated = PrepareGeologyGeneration(DeriveGeologySeed(1234));
	REQUIRE(generated.error == GeologyError::None);
	GeologyStateSnapshot snapshot;
	snapshot.generator_version = generated.state.generator_version;
	snapshot.width = Map::SizeX();
	snapshot.height = Map::SizeY();
	snapshot.freeform_edges = _settings_game.construction.freeform_edges;
	snapshot.next_deposit_id = generated.state.deposits.NextID().base();
	snapshot.packed.assign(generated.state.terrain.Data().begin(), generated.state.terrain.Data().end());
	snapshot.deposits.assign(generated.state.deposits.Records().begin(), generated.state.deposits.Records().end());
	snapshot.vents = generated.state.vents;
	snapshot.next_vent_id = generated.state.next_vent_id;
	return snapshot;
}

/** Verify depleted balances, saved information and excavated terrain survive rebuilt typed lookup. */
TEST_CASE("Geology snapshot retains depleted balances cavities and deleted identity cursor", "[geology-state]")
{
	Map::Allocate(64, 64, 4);
	// Initialize both fence policies through their actual owner, keeping restoration off the camera's selected layer.
	for (const auto layer : {MapLayer::Underground, MapLayer::DeepUnderground}) MultiLayerMap::InitialiseLayer(layer);
	auto snapshot = GeologySnapshotFixture();
	snapshot.deposits[0].remaining_reserve = 17;
	snapshot.deposits[1].remaining_reserve = 0;
	snapshot.deposits[0].surveyed_companies = 1;
	snapshot.deposits.erase(snapshot.deposits.begin() + 2);
	PackedExcavationState terrain;
	REQUIRE(terrain.Restore(Map::PlaneSize(), snapshot.packed));
	const auto cavity = TileXYL(10, 20, MapLayer::Underground);
	REQUIRE(terrain.Set(cavity, ExcavationState::Excavated));
	snapshot.packed.assign(terrain.Data().begin(), terrain.Data().end());
	GeologyWorldState restored;
	CHECK(PrepareGeologyRestoration(snapshot, restored, false) == GeologyError::None);
	CHECK(restored.deposits.Get(GeologyDepositID{0})->remaining_reserve == 17);
	CHECK(restored.deposits.Get(GeologyDepositID{1})->remaining_reserve == 0);
	CHECK(restored.deposits.Get(GeologyDepositID{0})->surveyed_companies == 1);
	CHECK(restored.deposits.Get(GeologyDepositID{2}) == nullptr);
	CHECK(restored.deposits.NextID().base() == 10);
	CHECK(restored.terrain.Get(cavity) == ExcavationState::Excavated);
	const auto &span = snapshot.deposits.front().spans.front();
	CHECK(restored.deposits.Find(TileXYL(span.first_x, span.y, MapLayer::Underground)) == GeologyDepositID{0});
	CHECK(restored.deposits.Claim(GeologyDepositID{1}, CompanyID{0}) == GeologyError::Depleted);
}

/** Verify invalid/truncated/reserved/reference state cannot partially replace a valid output candidate. */
TEST_CASE("Geology restoration refuses malformed authority without partial publication", "[geology-state-refusal]")
{
	Map::Allocate(64, 64, 4);
	for (const auto layer : {MapLayer::Underground, MapLayer::DeepUnderground}) MultiLayerMap::InitialiseLayer(layer);
	const auto valid = GeologySnapshotFixture();
	GeologyWorldState restored;
	REQUIRE(PrepareGeologyRestoration(valid, restored, false) == GeologyError::None);
	const auto before_count = restored.deposits.Records().size();
	SECTION("Truncation and reserved state encoding") {
		auto bad = valid;
		bad.packed.pop_back();
		CHECK(PrepareGeologyRestoration(bad, restored, false) == GeologyError::InvalidShape);
		bad = valid;
		bad.packed[0] = 3;
		CHECK(PrepareGeologyRestoration(bad, restored, false) == GeologyError::InvalidShape);
		bad.packed[0] = 1;
		CHECK(PrepareGeologyRestoration(bad, restored, false) == GeologyError::ClaimConflict);
	}
	SECTION("Geometry and monotonic identity") {
		auto bad = valid;
		bad.width *= 2;
		CHECK(PrepareGeologyRestoration(bad, restored, false) == GeologyError::UnsupportedMap);
		bad = valid;
		bad.next_deposit_id = 1;
		CHECK(PrepareGeologyRestoration(bad, restored, false) == GeologyError::InvalidID);
		bad = valid;
		bad.deposits[0].spans.front().last_x = 64;
		CHECK(PrepareGeologyRestoration(bad, restored, false) == GeologyError::InvalidShape);
	}
	SECTION("Actual company and finite reserve references") {
		auto bad = valid;
		bad.deposits[0].claimant = CompanyID{0};
		CHECK(PrepareGeologyRestoration(bad, restored, true) == GeologyError::ClaimConflict);
		bad = valid;
		bad.deposits[0].remaining_reserve = bad.deposits[0].initial_reserve + 1;
		CHECK(PrepareGeologyRestoration(bad, restored, false) == GeologyError::InvalidReserve);
	}
	CHECK(restored.deposits.Records().size() == before_count);
	CHECK(restored.deposits.Get(GeologyDepositID{0})->remaining_reserve == 32000);
}

/** Verify inactive legacy transport never manufactures rock, even when current map layers exist. */
TEST_CASE("Inactive geology restores free legacy terrain without new reserve", "[geology-state-legacy]")
{
	Map::Allocate(64, 64, 4);
	auto inactive = CaptureGeologyState();
	REQUIRE(inactive.generator_version == 0);
	GeologyWorldState restored;
	CHECK(PrepareGeologyRestoration(inactive, restored, false) == GeologyError::None);
	CHECK(restored.terrain.StorageBytes() == 0);
	CHECK(restored.deposits.Records().empty());
	inactive.next_deposit_id = 5;
	CHECK(PrepareGeologyRestoration(inactive, restored, false) == GeologyError::InvalidShape);
	const auto *profile = GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry);
	REQUIRE(profile != nullptr);
	MultiLayerStateSchema schema;
	schema.profile = {profile->id, profile->version, false, false};
	schema.required_content = profile->required_content->package;
	schema.content_version = profile->required_content->version;
	schema.feature_flags = MULTILAYER_FEATURE_GEOLOGY;
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::None);
	schema.feature_flags = 0x80000000;
	CHECK(ValidateMultiLayerStateReferences(schema) == MultiLayerStateError::UnsupportedFeatures);
}
