/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file manufacturing_replacement-1.cpp Detached same-file replacement issuance/retirement ownership and strict bounded receipt transport.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC
 * Copied rows deliberately contain no installed native hull/facility; these tests guard full/preview ownership without borrowing live-world identities.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../space_replacement.h"
#include "../space_structure.h"
#include "../multilayer_space_content.h"
#include "../saveload/space_replacement_sl.h"
#include "../safeguards.h"

/** Same-file historical epochs and minimal immutable paid receipts, used only for detached link validation. */
struct ReplacementSnapshot {
	std::vector<SpaceStructureRecord> structures{1}; ///< One actual transport container, with no native pool installation.
	std::vector<SpaceProductionRecord> products; ///< Existing original paid receipt plus optional issued new body.
	SpaceReplacementIntent intent{{VehicleID{1}, 2}, 15}; ///< Historical original incarnation, bounded by next12.
	/** Prepare a pending orbital producer and the surviving original immutable paid receipt. */
	ReplacementSnapshot()
	{
		structures[0].module.emplace().manufacture.emplace(); auto &job = *structures[0].module->manufacture;
		job.model = 20; job.phase = OrbitalManufacturePhase::Queued; job.replacement = intent;
		SpaceProductionRecord source; source.craft = intent.source; source.state.model = 15;
		source.state.content_version = MULTILAYER_SPACE_CONTENT_VERSION; source.state.capital_paid = GetSpaceVehicleConstruction(15)->capital;
		products.push_back(source);
	}
	/** @return Whether the complete detached producer/original/product relationships agree, without any current-world lookup. */
	bool Valid() const { return ValidateSpaceReplacementLinks(structures, products, 12); }
	/** @return The original actual orbital job's optional receipt for independent malformed-field cases. */
	SpaceReplacementIntent &Receipt() { return *structures[0].module->manufacture->replacement; }
	/** Replace orbital authority with one actual paid unfinished Earth body, preserving its newer issued incarnation. */
	void Earth()
	{
		structures.clear(); SpaceProductionRecord body; body.craft = {VehicleID{3}, 6}; body.state.model = 20;
		body.state.content_version = MULTILAYER_SPACE_CONTENT_VERSION; body.state.capital_paid = GetSpaceVehicleConstruction(20)->capital;
		body.state.placement = SpacePlacement::Assembling; body.state.remaining_ticks = 1; body.state.replacement = intent; products.push_back(body);
	}
};

/** Full and copied preview require bounded same-file historical identities and a single contained producer, while legitimate source loss remains a blocked paid request. */
TEST_CASE("Space replacement copied producer ownership is strict", "[manufacturing-replacement]")
{
	ReplacementSnapshot snapshot; bool accepted = false;
	SECTION("Original pending request") { accepted = true; }
	SECTION("Lost original remains historical pending work") { snapshot.products.clear(); accepted = true; }
	SECTION("Numerical successor never becomes the old incarnation") { snapshot.products[0].craft.generation = 9; accepted = true; }
	SECTION("Future original") { snapshot.Receipt().source.generation = 12; }
	SECTION("Zero original generation") { snapshot.Receipt().source.generation = 0; }
	SECTION("Invalid original index") { snapshot.Receipt().source.vehicle = VehicleID::Invalid(); }
	SECTION("Unknown original role") { snapshot.Receipt().source_model = 65535; }
	SECTION("Passive source is not a native vehicle") { snapshot.Receipt().source_model = 18; }
	SECTION("Rocket history cannot become another controller") { snapshot.Receipt().source_model = 14; }
	SECTION("Pending credit cannot prefinance production") { snapshot.Receipt().sale_credit = 1; }
	SECTION("Surviving source paid capital is immutable") { --snapshot.products[0].state.capital_paid; }
	SECTION("Surviving source role is immutable") { snapshot.products[0].state.model = 19; }
	SECTION("Surviving source content is immutable") { snapshot.products[0].state.content_version = 0; }
	SECTION("Clone and replacement cannot own one product") { snapshot.structures[0].module->manufacture->clone.emplace(); }
	SECTION("Duplicate orbital original") { const auto copy = snapshot.structures[0]; snapshot.structures.push_back(copy); }
	SECTION("Two physical producers cannot retire the same original") {
		SpaceProductionRecord body; body.craft = {VehicleID{3}, 6}; body.state.model = 20; body.state.content_version = MULTILAYER_SPACE_CONTENT_VERSION;
		body.state.placement = SpacePlacement::Assembling; body.state.remaining_ticks = 1; body.state.replacement = snapshot.intent; snapshot.products.push_back(body);
	}
	SECTION("Settled orbital receipt needs completed work") { snapshot.Receipt().settled = true; snapshot.products.clear(); }
	SECTION("Completed work cannot retain pending retirement") { snapshot.structures[0].module->manufacture->phase = OrbitalManufacturePhase::Complete; }
	SECTION("Retired original cannot survive settlement") {
		snapshot.Receipt().settled = true; snapshot.structures[0].module->manufacture->phase = OrbitalManufacturePhase::Complete;
		snapshot.structures[0].module->manufacture->product = {VehicleID{3}, 6};
	}
	SECTION("Bounded completed history survives original removal") {
		snapshot.Receipt().settled = true; snapshot.Receipt().sale_credit = 100; snapshot.products.clear();
		snapshot.structures[0].module->manufacture->phase = OrbitalManufacturePhase::Complete; snapshot.structures[0].module->manufacture->product = {VehicleID{3}, 6}; accepted = true;
	}
	CHECK(snapshot.Valid() == accepted);
}

/** A real unfinished Earth body retains a bounded pending request until commission, cancellation or legitimate unhoming; dormant products never acquire an orbital running request. */
TEST_CASE("Space replacement copied Earth receipt preserves its lifecycle", "[manufacturing-replacement]")
{
	ReplacementSnapshot snapshot; snapshot.Earth(); auto &body = snapshot.products.back(); auto &intent = *body.state.replacement; bool accepted = false;
	SECTION("Actual unfinished Earth body") { accepted = true; }
	SECTION("Unhomed unfinished body stays paid and pending") { body.state.placement = SpacePlacement::Unhomed; accepted = true; }
	SECTION("Earth cannot resume orbital orders") { intent.resume = true; }
	SECTION("Earth cannot also claim an orbital factory") { body.state.assembly = {SpaceStructureID{1}, 2}; }
	SECTION("Pending body must retain unfinished work") { body.state.remaining_ticks = 0; }
	SECTION("Pending body cannot be a deployed old-source proxy") { body.state.placement = SpacePlacement::Orbital; }
	SECTION("New incarnation must postdate original") { body.craft.generation = 2; }
	SECTION("Future product cannot become a paid receipt") { body.craft.generation = 12; }
	SECTION("Settled receipt cannot retain work") { intent.settled = true; snapshot.products.erase(snapshot.products.begin()); }
	SECTION("Settled credit is bounded by actual old recipe") { intent.settled = true; intent.sale_credit = GetSpaceVehicleConstruction(15)->capital + 1; body.state.remaining_ticks = 0; snapshot.products.erase(snapshot.products.begin()); }
	SECTION("Valid settled Earth history") { intent.settled = true; body.state.remaining_ticks = 0; body.state.placement = SpacePlacement::SurfaceStored; snapshot.products.erase(snapshot.products.begin()); accepted = true; }
	CHECK(snapshot.Valid() == accepted);
}

/** Optional receipt roundtrips preserve exact old/new accounting fields and reject byte values that could normalize malformed presence or running/retirement authority. */
TEST_CASE("Space replacement codec bounds optional authority", "[manufacturing-replacement]")
{
	std::optional<SpaceReplacementIntent> intent = SpaceReplacementIntent{{VehicleID{42}, 99}, 20, true, true, 321};
	std::vector<uint8_t> bytes;
	/** @param value Canonical unsigned saved field. @param width Exact big-endian byte count. */
	const auto write = [&](uint64_t value, uint width) { for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) bytes.push_back(static_cast<uint8_t>(value >> shift)); };
	WriteSpaceReplacementTransport(intent, write); REQUIRE(bytes.size() == SpaceReplacementTransportLength(intent)); REQUIRE(bytes.size() == 21);
	size_t cursor = 0;
	/** @param width Exact requested field extent. @return Bounded decoded integer; unit framing throws before an out-of-range access. */
	const auto read = [&](uint width) {
		if (width > bytes.size() - cursor) throw std::out_of_range("replacement unit family");
		uint64_t value = 0; while (width-- != 0) value = (value << 8) | bytes[cursor++]; return value;
	};
	CHECK(ReadSpaceReplacementTransport(read) == intent); CHECK(cursor == bytes.size());
	// Each truncation boundary exercises the actual shared codec's complete field order without granting any native pool or finance authority.
	for (size_t extent = 0; extent < 21; ++extent) {
		auto truncated = bytes; truncated.resize(extent); std::swap(bytes, truncated); cursor = 0;
		CHECK_THROWS(ReadSpaceReplacementTransport(read)); std::swap(bytes, truncated);
	}
	// Presence and each boolean are strict independently; silently treating2 as true would authorize a malformed retirement or resume in both load paths.
	for (size_t offset : {size_t{0}, size_t{15}, size_t{16}}) {
		bytes[offset] = 2; cursor = 0; CHECK_THROWS(ReadSpaceReplacementTransport(read)); bytes[offset] = 1;
	}
	intent.reset(); bytes.clear(); WriteSpaceReplacementTransport(intent, write); CHECK(bytes.size() == 1); cursor = 0; CHECK_FALSE(ReadSpaceReplacementTransport(read).has_value());
}
