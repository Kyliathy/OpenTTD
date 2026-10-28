/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_cargo_origin.cpp Full-width natural provenance, native packet copies and gated extraction inputs.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group15.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 14; merged source values, the legacy upgrade hook and packet operations.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../source_type.h"
#include "../space_mining_inventory.h"
#include "../space_mining_tuning.h"
#include "../saveload/cargo_source_sl.h"
#include "probe_mining_fixture.h"
#include "../safeguards.h"

/** Typed origins preserve boundary IDs and reject ambiguous tags/scalars without changing the old native domains. */
TEST_CASE("Asteroid source carries full identity and rejects contradictory representations", "[asteroid-cargo-origin]")
{
	constexpr auto unknown = Source{};
	static_assert(unknown.IsCanonical() && !unknown.IsValid());
	constexpr auto full = Source::FromAsteroid(65'536, CargoLabel{"NORE"});
	static_assert(full.IsValid() && full.GetAsteroidOrigin()->body == 65'536);
	CHECK(to_underlying(SourceType::Industry) == 0);
	CHECK(to_underlying(SourceType::Town) == 1);
	CHECK(to_underlying(SourceType::Headquarters) == 2);
	CHECK(to_underlying(SourceType::Asteroid) == 3);
	CHECK(to_underlying(SourceType::CompanySupply) == 4);
	// Crossing the old sentinel/width boundary must never alias body zero or
	// collapse distinct original IDs into the same native merge key.
	for (uint32_t id : {65'534U, 65'535U, 65'536U, 131'072U, UINT32_MAX - 1}) {
		auto source = Source::FromAsteroid(id, CargoLabel{"NORE"});
		CHECK(source.IsCanonical()); CHECK(source.IsValid());
		CHECK(source.id == Source::Invalid);
		CHECK(source.GetAsteroidOrigin() == AsteroidCargoOrigin{id, CargoLabel{"NORE"}});
		CHECK(source != Source{Source::Invalid, SourceType::Asteroid});
		const auto before = source;
		source.SetIndex(SourceID{17}); source.SetIndex(IndustryID{19});
		CHECK(source == before);
		CHECK_FALSE(IsValidCargoPacketSource(source, 0, SaveLoadVersion::SpaceMiningVehicles)); // Old CAPA cannot flatten this value.
	}
	CHECK(Source::FromAsteroid(65'536, CargoLabel{"NORE"}) != Source::FromAsteroid(131'072, CargoLabel{"NORE"}));
	CHECK(Source::FromAsteroid(65'536, CargoLabel{"NORE"}) != Source::FromAsteroid(65'536, CargoLabel{"COAL"}));
	CHECK_FALSE(Source::FromAsteroid(UINT32_MAX, CargoLabel{"NORE"}).IsValid());
	CHECK_FALSE(Source::FromAsteroid(65'536, CargoLabel{}).IsValid());
	CHECK_FALSE(Source{OrbitalBodyID{65'536}, SourceType::Asteroid}.IsValid());
	CHECK_FALSE(Source{Source::Invalid, SourceType::Asteroid}.IsValid());
	auto dual = full; dual.id = 42;
	CHECK_FALSE(dual.IsCanonical()); CHECK_FALSE(dual.IsValid()); CHECK_FALSE(dual.GetAsteroidOrigin());
	CHECK_FALSE(IsValidCargoPacketSource(dual, 0, SaveLoadVersion::SpaceMiningVehicles));
	auto wrong_tag = full; wrong_tag.type = SourceType::Town;
	CHECK_FALSE(wrong_tag.IsCanonical()); CHECK_FALSE(wrong_tag.IsValid()); CHECK_FALSE(wrong_tag.GetAsteroidOrigin());
	CHECK_FALSE(IsValidCargoPacketSource(wrong_tag, 0, SaveLoadVersion::SpaceMiningVehicles));
	auto retired = full; retired.MakeInvalid();
	CHECK(retired.IsCanonical()); CHECK_FALSE(retired.IsValid()); CHECK_FALSE(retired.GetAsteroidOrigin());
	CHECK(retired.type == SourceType::Asteroid);
	Source town{TownID{4}, SourceType::Town}; town.SetIndex(TownID{5});
	CHECK(town.IsCanonical()); CHECK(town.ToTownID() == TownID{5}); CHECK_FALSE(town.GetAsteroidOrigin());
	Source industry{IndustryID{6}, SourceType::Industry}; industry.SetIndex(SourceID{7});
	CHECK(industry.ToIndustryID() == IndustryID{7});
	Source headquarters{CompanyID{1}, SourceType::Headquarters};
	CHECK(headquarters.ToCompanyID() == CompanyID{1});
	Source supply{CompanyID{2}, SourceType::CompanySupply}; supply.MakeInvalid();
	CHECK(supply.IsCompanySupply()); CHECK_FALSE(supply.IsValid()); CHECK_FALSE(supply.GetAsteroidOrigin());
	CHECK(IsValidCargoPacketSource(supply, 0, SaveLoadVersion::CompanySupplyPackets));
	CHECK_FALSE(IsValidCargoPacketSource(supply, 1, SaveLoadVersion::CompanySupplyPackets));
}

/** Detached actual native packets exercise the existing split constructor and both original merge predicates. */
TEST_CASE("Asteroid provenance survives native packet split and both cargo merge boundaries", "[asteroid-cargo-origin]")
{
	ProbeExtractionRig rig;
	const auto source = Source::FromAsteroid(65'536, CargoLabel{"NORE"});
	const auto tile = GetSpaceMiningProductionTile(GetAsteroidField().bodies.front());
	REQUIRE(tile);
	/** @param origin Explicit source of this detached component packet. @return One owned real native packet, deleted before the fixture world closes. No extraction or paid procurement is claimed. */
	auto packet = [&](Source origin) {
		REQUIRE(CargoPacket::CanAllocateItem());
		auto result = std::unique_ptr<CargoPacket>{CargoPacket::Create(StationID::Invalid(), uint16_t{10}, origin)};
		result->UpdateLoadingTile(*tile);
		return result;
	};
	auto original = packet(source);
	auto split = std::unique_ptr<CargoPacket>{original->Split(4)};
	REQUIRE(split);
	CHECK(original->Count() + split->Count() == 10);
	CHECK(original->GetSource() == source); CHECK(split->GetSource() == source);
	CHECK(split->GetSource().GetAsteroidOrigin() == source.GetAsteroidOrigin());
	CHECK(VehicleCargoList::AreMergable(original.get(), split.get()));
	CHECK(StationCargoList::AreMergable(original.get(), split.get()));
	auto other_id = packet(Source::FromAsteroid(131'072, CargoLabel{"NORE"}));
	auto other_mineral = packet(Source::FromAsteroid(65'536, CargoLabel{"COAL"}));
	auto legacy = packet(Source{0, SourceType::Asteroid});
	// Matching source tiles and physical ages are insufficient when body/mineral
	// meaning differs, including modern versus old unqualified provenance.
	for (const auto *other : {other_id.get(), other_mineral.get(), legacy.get()}) {
		CHECK_FALSE(VehicleCargoList::AreMergable(original.get(), other));
		CHECK_FALSE(StationCargoList::AreMergable(original.get(), other));
	}
	auto copied = split->GetSource(); copied.MakeInvalid();
	CHECK(split->GetSource() == source);
}

/** Extraction preserves full provenance while refusing unsafe geometry and every copied/unregistered resource owner before allocation or reserve debit. */
TEST_CASE("Asteroid extraction prepares wide origins and commits only its original native body", "[asteroid-cargo-origin]")
{
	ProbeExtractionRig rig;
	auto &craft = rig.Probe();
	auto &body = rig.Attach(craft);
	// Ready work is an explicit component input for refusal testing, as in the
	// existing extraction unit adapter; this is not an earned-work runtime claim.
	craft.mission.work_ticks = GetSpaceMiningTuning(GetSpaceVehicleModel(craft))->extraction_ticks;
	const auto before = body;
	const auto fleet = ComputeSpaceVehicleDigest();
	const auto packet_count = CargoPacket::GetNumItems();
	for (const auto position : {SpacePoint{-1, 0}, SpacePoint{0, -1}, SpacePoint{INT32_MAX, 0}, SpacePoint{0, INT32_MIN},
		SpacePoint{static_cast<int32_t>(Map::SizeX()) * SPACE_Q16, 0}}) {
		auto invalid = body; invalid.motion.position = position;
		CHECK_FALSE(GetSpaceMiningProductionTile(invalid));
		CHECK_FALSE(CommitSpaceMiningExtraction(craft, invalid));
		CHECK(invalid.remaining_reserve == before.remaining_reserve);
		CHECK(ComputeSpaceVehicleDigest() == fleet);
		CHECK(CargoPacket::GetNumItems() == packet_count);
	}
	// A valid-looking same-ID copy still owns no native reserve. Wide copied
	// IDs likewise remain component metadata until the real field issues them;
	// their full provenance no longer depends on the removed legacy wire gate.
	auto copied = body;
	CHECK_FALSE(CommitSpaceMiningExtraction(craft, copied));
	for (uint32_t id : {65'534U, 65'535U, 65'536U}) {
		auto wide = body; wide.id = OrbitalBodyID{id};
		const auto source = PrepareAsteroidMiningSource(wide);
		REQUIRE(source); REQUIRE(source->GetAsteroidOrigin());
		CHECK(source->GetAsteroidOrigin()->body == id);
		AutoRestoreBackup target(craft.mission.target, wide.id);
		CHECK_FALSE(CommitSpaceMiningExtraction(craft, wide));
		CHECK(wide.remaining_reserve == before.remaining_reserve);
		CHECK(CargoPacket::GetNumItems() == packet_count);
	}
	auto invalid = body; invalid.id = INVALID_ORBITAL_BODY;
	CHECK_FALSE(PrepareAsteroidMiningSource(invalid));
	invalid = body; invalid.resource_class = static_cast<AsteroidClass>(3);
	CHECK_FALSE(PrepareAsteroidMiningSource(invalid));
	CHECK(body == before); CHECK(ComputeSpaceVehicleDigest() == fleet);
	REQUIRE(CommitSpaceMiningExtraction(craft, body));
	CHECK(craft.cargo.TotalCount() == 2);
	CHECK(body.remaining_reserve == before.remaining_reserve - 2);
	const auto emitted = craft.cargo.Packets()->front()->GetSource();
	CHECK(emitted.type == SourceType::Asteroid);
	CHECK(emitted.id == Source::Invalid);
	CHECK(emitted.GetAsteroidOrigin() == AsteroidCargoOrigin{body.id.base(), CargoLabel{"NORE"}});
	CHECK(ValidateSpaceVehicles());
}

/** Source values keep full identity next to ordinary and non-paying sources, and no generic rewrite reaches an asteroid origin. */
TEST_CASE("Merged source values keep asteroid identity beside ordinary and company-supply sources", "[asteroid-cargo-origin]")
{
	ProbeExtractionRig rig;
	const auto high = Source::FromAsteroid(65'536, CargoLabel{"NORE"});
	const auto low = Source::FromAsteroid(0, CargoLabel{"NORE"});
	// Body 65536 must never wrap onto body 0, and body 0 is a real origin.
	CHECK(high != low);
	CHECK(high.GetAsteroidOrigin()->body == 65'536); CHECK(low.GetAsteroidOrigin()->body == 0);
	CHECK(low.IsValid()); CHECK(low.id == Source::Invalid);
	// The mineral is part of the identity and the invalid sentinel is no origin.
	CHECK(Source::FromAsteroid(0, CargoLabel{"COAL"}) != low);
	CHECK(Source::FromAsteroid(UINT32_MAX, CargoLabel{"NORE"}) == Source(Source::Invalid, SourceType::Asteroid));
	CHECK_FALSE(Source::FromAsteroid(UINT32_MAX, CargoLabel{"NORE"}).GetAsteroidOrigin());
	// Ordinary equality is unchanged: same domain and index compare equal, the
	// same index in another domain does not.
	CHECK(Source{} == Source(Source::Invalid, SourceType::Industry));
	CHECK(Source(IndustryID{3}, SourceType::Industry) == Source(IndustryID{3}, SourceType::Industry));
	CHECK(Source(IndustryID{3}, SourceType::Industry) != Source(TownID{3}, SourceType::Town));
	// A bought supply keeps its non-paying purpose and carries no asteroid payload.
	const Source supply{CompanyID{1}, SourceType::CompanySupply};
	CHECK(supply.IsCompanySupply()); CHECK_FALSE(supply.GetAsteroidOrigin());
	CHECK(supply != Source::FromAsteroid(1, CargoLabel{"NORE"}));
	CHECK(supply != Source(Source::Invalid, SourceType::Asteroid));
	// Generic rewrites: the 16-bit setter cannot touch a legacy or typed asteroid
	// source, and company invalidation never matches asteroid cargo.
	Source legacy{SourceID{5}, SourceType::Asteroid};
	legacy.SetIndex(SourceID{9}); CHECK(legacy.id == 5);
	auto typed = high; typed.SetIndex(SourceID{9}); CHECK(typed == high);
	REQUIRE(CargoPacket::CanAllocateItem(3));
	std::unique_ptr<CargoPacket> mined{CargoPacket::Create(StationID::Invalid(), uint16_t{5}, high)};
	std::unique_ptr<CargoPacket> bought{CargoPacket::Create(StationID::Invalid(), uint16_t{5}, supply)};
	std::unique_ptr<CargoPacket> old_mined{CargoPacket::Create(StationID::Invalid(), uint16_t{5}, legacy)};
	CHECK_FALSE(VehicleCargoList::AreMergable(mined.get(), bought.get()));
	CHECK_FALSE(StationCargoList::AreMergable(mined.get(), bought.get()));
	CargoPacket::InvalidateAllFrom(supply);
	CHECK(mined->GetSource() == high); CHECK(old_mined->GetSource() == legacy);
	CHECK(bought->GetSource().IsCompanySupply()); CHECK_FALSE(bought->GetSource().IsValid());
}

/** The legacy upgrade hook qualifies one exact old origin and refuses everything else unchanged. */
TEST_CASE("Legacy asteroid upgrade refuses invalid, ordinary and already-typed mismatches", "[asteroid-cargo-origin]")
{
	ProbeExtractionRig rig;
	REQUIRE(CargoPacket::CanAllocateItem(6));
	/** @param source Exact packet source. @return One owned real native packet for this case only. */
	auto packet = [](Source source) { return std::unique_ptr<CargoPacket>{CargoPacket::Create(StationID::Invalid(), uint16_t{7}, source)}; };
	const auto nore = CargoLabel{"NORE"};
	// Each refusal leaves the source exactly as it was.
	const Source refused[]{
		Source(Source::Invalid, SourceType::Asteroid), // No identity to qualify.
		Source(IndustryID{7}, SourceType::Industry), // Ordinary freight is never natural provenance.
		Source(CompanyID{0}, SourceType::CompanySupply), // Bought supplies keep their purpose.
		Source::FromAsteroid(7, CargoLabel{"COAL"}), // Already typed with another mineral.
	};
	for (const auto &source : refused) {
		auto candidate = packet(source);
		CHECK_FALSE(candidate->UpgradeLegacyAsteroidSource(nore));
		CHECK(candidate->GetSource() == source);
	}
	auto empty_mineral = packet(Source{SourceID{7}, SourceType::Asteroid});
	CHECK_FALSE(empty_mineral->UpgradeLegacyAsteroidSource(CargoLabel{}));
	CHECK(empty_mineral->GetSource() == Source(SourceID{7}, SourceType::Asteroid));
	// A real legacy origin is qualified in place once; repeating it agrees, and
	// quantity and credit are untouched.
	auto old = packet(Source{SourceID{7}, SourceType::Asteroid}); old->AddFeederShare(Money{21});
	REQUIRE(old->UpgradeLegacyAsteroidSource(nore));
	CHECK(old->GetSource() == Source::FromAsteroid(7, nore));
	CHECK(old->UpgradeLegacyAsteroidSource(nore));
	CHECK(old->Count() == 7); CHECK(old->GetFeederShare() == Money{21});
}

/** Split, Merge and Reduce keep the source, receipt link, raw departure, age and proportional credit. */
TEST_CASE("Native packet split, merge and reduce keep every merged packet fact", "[asteroid-cargo-origin]")
{
	ProbeExtractionRig rig;
	REQUIRE(CargoPacket::CanAllocateItem(6));
	// Age and credit come from the historical-restore constructor; it carries no
	// source, so this packet checks the ordinary scalar facts.
	std::unique_ptr<CargoPacket> aged{CargoPacket::Create(uint16_t{10}, uint16_t{7}, StationID::Invalid(), INVALID_TILE, Money{100})};
	std::unique_ptr<CargoPacket> part{aged->Split(4)};
	REQUIRE(part);
	CHECK(aged->Count() == 6); CHECK(part->Count() == 4);
	CHECK(aged->GetFeederShare() + part->GetFeederShare() == Money{100});
	CHECK(part->GetPeriodsInTransit() == 7); CHECK(part->GetSource() == aged->GetSource());
	aged->Merge(part.release());
	CHECK(aged->Count() == 10); CHECK(aged->GetFeederShare() == Money{100}); CHECK(aged->GetPeriodsInTransit() == 7);
	aged->Reduce(3);
	CHECK(aged->Count() == 7); CHECK(aged->GetFeederShare() == Money{70}); CHECK(aged->GetPeriodsInTransit() == 7);
	// A bought supply with a receipt above UINT32_MAX and its departure flag, and a
	// typed asteroid origin: every operation keeps the exact facts on every part.
	const uint64_t receipt = (uint64_t{1} << 32) + 5;
	std::unique_ptr<CargoPacket> linked{CargoPacket::Create(StationID::Invalid(), uint16_t{9}, Source{CompanyID{0}, SourceType::CompanySupply})};
	REQUIRE(linked->RestoreCompanySupplyOrder(receipt, true));
	std::unique_ptr<CargoPacket> mined{CargoPacket::Create(StationID::Invalid(), uint16_t{9}, Source::FromAsteroid(65'536, CargoLabel{"NORE"}))};
	for (auto *original : {linked.get(), mined.get()}) {
		const auto source = original->GetSource();
		const auto order = original->GetCompanySupplyOrder();
		const auto departure = original->GetCompanySupplyShippedWire();
		std::unique_ptr<CargoPacket> split{original->Split(2)};
		REQUIRE(split);
		// Both parts are the same packet facts with different quantities.
		for (const auto *fragment : {original, split.get()}) {
			CHECK(fragment->GetSource() == source);
			CHECK(fragment->GetCompanySupplyOrder() == order);
			CHECK(fragment->GetCompanySupplyShippedWire() == departure);
		}
		original->Merge(split.release());
		original->Reduce(4);
		CHECK(original->Count() == 5);
		CHECK(original->GetSource() == source);
		CHECK(original->GetCompanySupplyOrder() == order);
		CHECK(original->GetCompanySupplyShippedWire() == departure);
	}
	CHECK(linked->GetCompanySupplyOrder() == receipt); CHECK(linked->GetCompanySupplyShippedWire() == 1);
}

