/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_lineage_preflight.cpp Native load-entry refusals, copied replay and per-attempt branch lifetime.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Groups 13 and 17B; real development-build 445 originals refuse without touching the world.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 19; every merged copied field resets between candidates.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 20; preview sequences through the one copied owner join.
 */
#include "../stdafx.h"
#include <filesystem>
#include <fstream>
#include "../3rdparty/nlohmann/json.hpp"
#include "save_envelope_fixture.h"
#include "save_supply_wire_fixture.h"
#include "save_candidate_lifecycle_fixture.h"
#include "transport_tube_fixture.h"
#include "../saveload/saveload_func.h"
#include "../saveload/saveload.h"
#include "../core/random_func.hpp"
#include "../fileio_func.h"
#include "../table/strings.h"
#include "../surface_supply_order_runtime.h"
#include "../multilayer_content.h"
#include "../map_func.h"
#include "../gfx_func.h"
#include "layer_save_fixture.h"

/** Exact immutable catalogue original, including an independently recorded SHA256-bound explicit origin. */
struct PreflightOriginal {
	std::filesystem::path path; ///< Location only; never classifier evidence.
	std::vector<uint8_t> bytes; ///< Original compressed bytes, never regenerated or header-downgraded.
	SaveOriginHint hint; ///< Independent retained digest and explicit source branch.
};

/** Resolve one retained original without using its path as classification authority.
 * @param origin Independently recorded catalogue provenance, A or B.
 * @param version Exact source version to select; no conversion is performed.
 * @return Original bytes and independent digest-bound hint.
 */
static PreflightOriginal OriginalForPreflight(std::string_view origin, uint16_t version)
{
	const auto root = std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path().parent_path().parent_path();
	std::ifstream input(root / "src/tests/fixtures/saveload/lineage-originals.json"); REQUIRE(input.good());
	const auto catalogue = nlohmann::json::parse(input);
	// Select only a catalogue identity. Runtime preflight receives the bytes and
	// optional exact digest; the filename/provenance label supplies no fact to it.
	for (const auto &row : catalogue.at("observations")) {
		if (row.at("origin").get<std::string>() != origin || row.at("version").get<uint16_t>() != version) continue;
		PreflightOriginal result;
		result.path = root / row.at("path").get<std::string>();
		std::ifstream file(result.path, std::ios::binary); REQUIRE(file.good());
		result.bytes.assign(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});
		REQUIRE(result.bytes.size() == row.at("bytes").get<size_t>());
		result.hint.dialect = origin == "A" ? SaveDialect::DestinationA : SaveDialect::DonorB;
		const auto digest = EnvelopeHex(row.at("sha256").get<std::string>()); REQUIRE(digest.size() == result.hint.source_digest.size());
		std::ranges::copy(digest, result.hint.source_digest.begin());
		return result;
	}
	FAIL("Required original is absent from the immutable input catalogue");
	return {};
}

/** One committed development-build original with its catalogued identity; the path is provenance, never classifier evidence. */
struct DevelopmentBuildOriginal {
	std::filesystem::path path; ///< Location only.
	std::vector<uint8_t> bytes; ///< Original compressed bytes, exactly as the incoming writer wrote them.
	std::string sha256; ///< Independently recorded digest from the catalogue.
};

/** Load every original listed in src/tests/fixtures/saveload/space-fixes-445/catalogue.json.
 * @return Each original with bytes checked against the catalogued size.
 */
static std::vector<DevelopmentBuildOriginal> DevelopmentBuildOriginals()
{
	const auto root = std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path().parent_path().parent_path();
	std::ifstream input(root / "src/tests/fixtures/saveload/space-fixes-445/catalogue.json"); REQUIRE(input.good());
	const auto catalogue = nlohmann::json::parse(input);
	std::vector<DevelopmentBuildOriginal> originals;
	// Read each catalogued original byte for byte; nothing is regenerated.
	for (const auto &row : catalogue.at("originals")) {
		DevelopmentBuildOriginal original{root / row.at("path").get<std::string>(), {}, row.at("sha256").get<std::string>()};
		std::ifstream file(original.path, std::ios::binary); REQUIRE(file.good());
		original.bytes.assign(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{});
		REQUIRE(original.bytes.size() == row.at("bytes").get<size_t>());
		originals.push_back(std::move(original));
	}
	REQUIRE_FALSE(originals.empty());
	return originals;
}

/** Observe actual native reads after admission; inspection and refused attempts expose no chosen capabilities. */
class ObservePreflightReader final : public LoadFilter {
public:
	std::vector<SaveLineageResult> observed; ///< Actual accepted native-context values seen during this raw source's reads.
	/** @param raw Actual original buffered network source. */
	explicit ObservePreflightReader(std::shared_ptr<LoadFilter> raw) : LoadFilter(std::move(raw)) {}
	/** @param buf Destination. @param len Capacity. @return Actual original bytes, recording any active native attempt. */
	size_t Read(uint8_t *buf, size_t len) override
	{
		if (const auto active = GetLoadCapabilities()) this->observed.push_back(*active);
		return this->chain->Read(buf, len);
	}
	/** Rewind original bytes only; every native replay must reconstruct its codec and ReadBuffer. */
	void Reset() override { this->chain->Reset(); }
};

/** Supply the isolated world's declared exact package to the ordinary copied-preview catalogue lookup; this loads no graphics or world. */
class PreflightContentCatalogue final {
	GRFConfigList original = std::move(_all_grfs); ///< Caller catalogue ownership restored after this test's native copied checks.
public:
	/** @param content The actual fixture's already activated, exact required package descriptor; never a replacement identity from the candidate file. */
	explicit PreflightContentCatalogue(const GRFConfigList &content) { CopyGRFConfigList(_all_grfs, content, false); }
	/** Restore the original scanned catalogue without retaining preview's package state. */
	~PreflightContentCatalogue() { _all_grfs = std::move(this->original); }
};

/** Make native absolute-file opening available in the isolated process without scanning user directories or changing content. */
class PreflightFileSearch final {
	std::vector<Searchpath> original = std::move(_valid_searchpaths); ///< Caller search inventory, restored after absolute-path checks.
public:
	/** Subdirectory::None uses the explicit absolute filename; one native search entry enables the ordinary opener. */
	PreflightFileSearch() { _valid_searchpaths = {Searchpath::WorkingDir}; }
	/** Restore the caller's exact search inventory without changing any directory or file. */
	~PreflightFileSearch() { _valid_searchpaths = std::move(this->original); }
};

/** Change only the replayed envelope after the inspector's complete original-byte passes have succeeded. */
class ChangedReplayEnvelope final : public LoadFilter {
	uint8_t resets = 0; ///< Native header, identity, inspection and replay each have their own raw restart.
public:
	/** @param raw Actual complete network save bytes; the underlying buffer never changes. */
	explicit ChangedReplayEnvelope(std::shared_ptr<LoadFilter> raw) : LoadFilter(std::move(raw)) {}
	/** @param buf Destination. @param len Capacity. @return Actual bytes, with a changed compression tag only at final replay. */
	size_t Read(uint8_t *buf, size_t len) override
	{
		const auto count = this->chain->Read(buf, len);
		// Only the fourth restart has completed both identity and bounded parsing;
		// a changed envelope there must still refuse before native initialization.
		if (this->resets == 4 && count != 0) buf[0] ^= 1;
		return count;
	}
	/** Reset the actual raw source while selecting the explicit replay-change point. */
	void Reset() override { ++this->resets; this->chain->Reset(); }
};

/** Actual ambiguous/corrupt file and network load requests refuse before initialization; the running world needs no rollback. */
TEST_CASE("Native lineage refusal preserves the actual open world", "[save-lineage-preflight]")
{
	TubePrototypeRig rig; auto *capsule = rig.BuyCapsule();
	PreflightFileSearch search;
	const auto original = OriginalForPreflight("A", 425);
	const auto cash = Company::Get(_current_company)->money;
	const auto id = capsule->index;
	const auto tile = capsule->tile;
	const auto map = Map::Size();
	const auto tick = TimerGameTick::counter;
	const std::array<uint32_t, 2> rng{_random.state[0], _random.state[1]};
	CHECK_FALSE(GetLoadCapabilities().has_value());
	auto network = std::make_shared<ObservePreflightReader>(EnvelopeNetworkReader(original.bytes));
	CHECK(LoadWithFilter(network) == SaveLoadResult::Error);
	CHECK(network->observed.empty()); CHECK_FALSE(GetLoadCapabilities().has_value());
	CHECK(SaveOrLoad(original.path.string(), SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
	CHECK(SaveOrLoad(original.path.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
	CHECK(_load_check_data.error == STR_GAME_SAVELOAD_ERROR_AMBIGUOUS_LINEAGE);
	auto mismatched = original.hint; mismatched.source_digest[31] ^= 1;
	CHECK(LoadWithFilter(EnvelopeNetworkReader(original.bytes), mismatched) == SaveLoadResult::Error);
	CHECK(LoadWithFilter(EnvelopeNetworkReader(EnvelopeStream({1, 2, 3}, "OTTZ"))) == SaveLoadResult::Error);
	CHECK(LoadWithFilter(std::make_shared<ChangedReplayEnvelope>(EnvelopeNetworkReader(original.bytes)), original.hint) == SaveLoadResult::Error);
	const auto missing = original.path.parent_path() / "no-lineage-input-created.sav";
	REQUIRE_FALSE(std::filesystem::exists(missing));
	CHECK(SaveOrLoad(missing.string(), SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
	CHECK(Vehicle::GetIfValid(id) == capsule); CHECK(capsule->tile == tile); CHECK(capsule->IsInDepot());
	CHECK(Company::Get(_current_company)->money == cash); CHECK(Map::Size() == map);
	CHECK(TimerGameTick::counter == tick); CHECK(_random.state[0] == rng[0]); CHECK(_random.state[1] == rng[1]);
	CHECK_FALSE(GetLoadCapabilities().has_value());
}

/** Two genuine same-number branch originals replay through copied native loading; no accepted choice survives return or the next refusal. */
TEST_CASE("Native copied lineage is bound to each original attempt", "[save-lineage-preflight]")
{
	TubePrototypeRig rig; auto *capsule = rig.BuyCapsule();
	PreflightFileSearch search;
	PreflightContentCatalogue catalogue(rig.ActivatedContent());
	const auto id = capsule->index;
	const auto cash = Company::Get(_current_company)->money;
	// Both425 originals need an explicit exact-bound hint; identical version
	// numbers must still expose different original capabilities during native reads.
	for (const auto branch : {"A", "B"}) {
		const auto original = OriginalForPreflight(branch, 425);
		auto network = std::make_shared<ObservePreflightReader>(EnvelopeNetworkReader(original.bytes));
		const auto result = LoadWithFilter(network, original.hint, true);
		INFO(branch); INFO(_load_check_data.error_msg);
		CHECK(result == SaveLoadResult::Ok);
		CHECK(SaveOrLoad(original.path.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false, original.hint) == SaveLoadResult::Ok);
		REQUIRE_FALSE(network->observed.empty());
		// Every native read belongs to this exact accepted original, including
		// later decoder refill; no earlier successful branch may leak into it.
		for (const auto &seen : network->observed) {
			CHECK(seen.status == SaveLineageStatus::Accepted); CHECK(seen.dialect == original.hint.dialect); CHECK(seen.original_version == 425);
		}
		CHECK_FALSE(GetLoadCapabilities().has_value());
		CHECK(Vehicle::GetIfValid(id) == capsule); CHECK(Company::Get(_current_company)->money == cash);
	}
	const auto current = OriginalForPreflight("A", 440);
	auto current_network = std::make_shared<ObservePreflightReader>(EnvelopeNetworkReader(current.bytes));
	CHECK(LoadWithFilter(current_network, std::nullopt, true) == SaveLoadResult::Ok);
	REQUIRE_FALSE(current_network->observed.empty());
	CHECK(current_network->observed.back().dialect == SaveDialect::DestinationA);
	CHECK(current_network->observed.back().original_version == 440);
	CHECK_FALSE(GetLoadCapabilities().has_value());
	const auto original = OriginalForPreflight("A", 425);
	CHECK(LoadWithFilter(EnvelopeNetworkReader(original.bytes), std::nullopt, true) == SaveLoadResult::Error);
	CHECK_FALSE(GetLoadCapabilities().has_value()); CHECK(Vehicle::GetIfValid(id) == capsule);
}

#ifdef WITH_LZO
/** Native entry alone retains checksum-framed pre-envelope LZO with its original host-order block words and version0. */
TEST_CASE("Native lineage inspection retains explicit ancient LZO compatibility", "[save-lineage-preflight]")
{
	// Literal original block framing: little-endian checksum/length followed by
	// an LZO literal run and EOF. No modern tag or invented version word exists.
	const auto input = EnvelopeHex("e701421c1300000020544553540000000310203000000000110000");
	auto raw = EnvelopeNetworkReader(input);
	CHECK_THROWS_AS(InspectSaveWireFacts(raw), SaveFramingError);
	const auto facts = InspectSaveWireFacts(raw, {}, true);
	CHECK(facts.original_version == 0); CHECK(facts.framing_complete); CHECK(facts.common_transport_valid);
	CHECK(EnvelopeDigest(facts.source_digest) == "900c0f4f41f13a65ad9835177e529bc31f759af2b94cbc46af597760553149da");
	CHECK(ClassifySaveDialect(facts).status == SaveLineageStatus::Accepted);
	CHECK(ClassifySaveDialect(facts).dialect == SaveDialect::Shared);
	CHECK(raw->read_bytes == 0); CHECK_FALSE(GetLoadCapabilities().has_value());
}
#endif

/** An intentional copied-read stop after admission, before any gameplay chunk can receive bytes. */
struct ObservedNativeCapabilityBoundary final : std::exception {};

/** Inspect real native scalar/descriptor decisions at the first admitted decoder read. */
class CapabilityBoundaryReader final : public LoadFilter {
	const std::function<void(const SaveLineageResult &)> &observe; ///< Explicit assertion callback, never a native handler or world initializer.
public:
	bool observed = false; ///< True only after the real native attempt has bound accepted capabilities.
	/** @param raw Genuine buffered original input. @param observe Checks against independent original source/version expectations. */
	CapabilityBoundaryReader(std::shared_ptr<LoadFilter> raw, const std::function<void(const SaveLineageResult &)> &observe) : LoadFilter(std::move(raw)), observe(observe) {}
	/** @param buf Destination. @param len Capacity. @return Original bytes during preflight; admitted reads stop before chunk dispatch. */
	size_t Read(uint8_t *buf, size_t len) override
	{
		if (const auto active = GetLoadCapabilities()) {
			this->observed = true; this->observe(*active);
			throw ObservedNativeCapabilityBoundary{};
		}
		return this->chain->Read(buf, len);
	}
	/** Rewind only original bytes; no accepted context exists during inspector passes. */
	void Reset() override { this->chain->Reset(); }
};

/** Explicit original enum/capability meanings; this oracle does not call the policy to derive expected thresholds. */
struct NativeCapabilityBoundary {
	SaveLoadVersion a_boundary; ///< Destination enum name; the other branch's same number never acquires that meaning.
	SaveCapability a_feature; ///< Exact destination feature.
	uint16_t introduced; ///< Independently fixed original A threshold.
};

/** All real catalogue originals exercise native scalar AND descriptor gates before later owner adapters, without inventing downgraded saves. */
TEST_CASE("Native scalar and descriptor gates preserve both original histories", "[save-lineage-capability-gates]")
{
	static constexpr NativeCapabilityBoundary a_features[]{
		{SaveLoadVersion::OrbitalAssemblyFacilities, SaveCapability::OrbitalAssemblyFacilities, 425},
		{SaveLoadVersion::OrbitalManufacturingReservations, SaveCapability::OrbitalManufacturingReservations, 426},
		{SaveLoadVersion::OrbitalManufacturingMaterialization, SaveCapability::OrbitalManufacturingMaterialization, 427},
		{SaveLoadVersion::OrbitalManufacturingClones, SaveCapability::OrbitalManufacturingClones, 428},
		{SaveLoadVersion::SpaceReplacementSettlement, SaveCapability::SpaceReplacementSettlement, 429},
		{SaveLoadVersion::SpaceOrderedActions, SaveCapability::SpaceOrderedActions, 430},
		{SaveLoadVersion::SpaceHeavyMinerCommissioning, SaveCapability::SpaceHeavyMinerCommissioning, 431},
		{SaveLoadVersion::SpaceHeavyMinerMissions, SaveCapability::SpaceHeavyMinerMissions, 432},
		{SaveLoadVersion::SpaceHeavyMinerSupplies, SaveCapability::SpaceHeavyMinerSupplies, 433},
		{SaveLoadVersion::SpaceHeavyMinerDescents, SaveCapability::SpaceHeavyMinerDescents, 434},
		{SaveLoadVersion::SpaceHeavyMinerOnboardServices, SaveCapability::SpaceHeavyMinerOnboardServices, 435},
		{SaveLoadVersion::SpaceHeavyMinerContainedServices, SaveCapability::SpaceHeavyMinerContainedServices, 436},
		{SaveLoadVersion::SpaceCargoDimensions, SaveCapability::SpaceCargoDimensions, 437},
		{SaveLoadVersion::OrbitalRecipeProcessing, SaveCapability::OrbitalRecipeProcessing, 438},
		{SaveLoadVersion::CompanySupplyPackets, SaveCapability::CompanySupplyPackets, 439},
		{SaveLoadVersion::SurfaceSupplyOrders, SaveCapability::SurfaceSupplyOrdersA, 440},
	};
	static constexpr std::pair<SaveCapability, uint16_t> b_features[]{
		{SaveCapability::TBMFaceTransfer, 425}, {SaveCapability::SurfaceSupplyOrdersB, 426},
		{SaveCapability::GeothermalGenerators, 427}, {SaveCapability::UtilityHeatConversion, 428},
		{SaveCapability::GeothermalGeneratorOperation, 429}, {SaveCapability::GeothermalMagmaLoop, 430},
		{SaveCapability::UtilityTownService, 431}, {SaveCapability::UtilityLocalElectricalFeed, 432},
		{SaveCapability::TubeGeothermalSources, 433}, {SaveCapability::TubeGeothermalBindings, 434},
	};
	static uint32_t untouched = 0xA5A51234; // Descriptor address only; no SlObject, native command or data write is permitted in this observer.
	const auto field = SaveLoad::Variable<VarFileType::U32>("original", SLE_GLOBAL_ADDRESS(untouched));
	// Only genuine original epochs are selected. A439 has no retained input;
	// its threshold is tested as a boundary against real438/440, never fabricated bytes.
	for (const auto branch : {"A", "B"}) {
		const uint16_t last = branch == std::string_view{"A"} ? 440 : 434;
		for (uint16_t version = 424; version <= last; ++version) {
			if (branch == std::string_view{"A"} && version == 439) continue; // Preserve the independently recorded original-input gap.
			const auto original = OriginalForPreflight(branch, version);
			INFO(branch); INFO(version);
			/** @param active Actual original classification bound by the native DoLoad scope, not a test-created policy result. */
			const std::function<void(const SaveLineageResult &)> observe = [&](const SaveLineageResult &active) {
				CHECK(active.original_version == version);
				CHECK(active.dialect == (version == 424 ? SaveDialect::Shared : original.hint.dialect));
				// A's newer fields and retirement intervals remain its own meaning.
				// Every B original retains the shared pre425 Space layout regardless
				// of the same numeric threshold's independent underground feature.
				for (const auto &boundary : a_features) {
					const bool available = branch == std::string_view{"A"} && version >= boundary.introduced;
					CHECK(SlHasSaveCapability(boundary.a_feature) == available);
					CHECK(IsSavegameVersionBefore(boundary.a_boundary) == !available);
					CHECK(IsSavegameVersionBeforeOrAt(boundary.a_boundary) == (branch == std::string_view{"B"} || version <= boundary.introduced));
					auto introduced = field; introduced.version_from = boundary.a_boundary;
					CHECK(SlIsObjectValidInSavegame(introduced) == available);
					auto retired = field; retired.version_to = boundary.a_boundary;
					CHECK(SlIsObjectValidInSavegame(retired) == !available);
				}
				// Explicit donor requirements apply equally to empty TABLE headers
				// and populated fields; an A field interval cannot substitute for them.
				for (const auto &[feature, threshold] : b_features) {
					const bool available = branch == std::string_view{"B"} && version >= threshold;
					CHECK(SlHasSaveCapability(feature) == available);
					CHECK(SlIsObjectValidInSavegame(field.WithCapability(feature)) == available);
				}
				CHECK_FALSE(SlHasSaveCapability(static_cast<SaveCapability>(250)));
				CHECK_FALSE(SlIsObjectValidInSavegame(field.WithCapability(static_cast<SaveCapability>(250))));
				CHECK_FALSE(IsSavegameVersionBefore(SaveLoadVersion::AsteroidStarterInputs));
				CHECK(IsSavegameVersionBeforeOrAt(SaveLoadVersion::AsteroidStarterInputs) == (version == 424));
			};
			auto reader = std::make_shared<CapabilityBoundaryReader>(EnvelopeNetworkReader(original.bytes), observe);
			CHECK(LoadWithFilter(reader, original.hint, true) == SaveLoadResult::Error);
			CHECK(reader->observed); CHECK_FALSE(GetLoadCapabilities().has_value()); CHECK(untouched == 0xA5A51234);
		}
	}
}

/** Real original wire families and packet purpose join complete copied custody without publishing staged rights. */
TEST_CASE("Native legacy supply wire conversion never publishes staged rights", "[save-supply-wire-integration]")
{
	TubePrototypeRig rig; auto *capsule = rig.BuyCapsule();
	PreflightContentCatalogue catalogue(rig.ActivatedContent());
	const auto cash = Company::Get(_current_company)->money;
	const auto id = capsule->index;
	const auto installed = CaptureSurfaceSupplyOrders();
	const auto a = OriginalForPreflight("A", 440), b = OriginalForPreflight("B", 426);
	CHECK(InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(b.bytes)).source_digest == b.hint.source_digest);
	CHECK(LoadWithFilter(EnvelopeNetworkReader(a.bytes), std::nullopt, true) == SaveLoadResult::Ok);
	CHECK(_load_check_data.legacy_a_supply_wire);
	CHECK(_load_check_data.surface_supply_orders_present);
	CHECK(_load_check_data.surface_supply_orders.saved_day == _load_check_data.native_economy_date);
	SaveLoadResult partial;
	{
		// The open world's slot deliberately disagrees with the pinned original
		// package. Copied admission must resolve B's material from its own content
		// identity, without reading or repairing the activated cargo catalogue.
		AutoRestoreBackup live_meaning(CargoSpec::Get(CargoType{22})->label, CargoLabel{"NOPE"});
		partial = LoadWithFilter(EnvelopeNetworkReader(b.bytes), std::nullopt, true);
	}
	INFO(_load_check_data.error_msg);
	CHECK(partial == SaveLoadResult::Ok);
	CHECK(_load_check_data.legacy_b_supply_packet_fields_present);
	CHECK(_load_check_data.legacy_b_supply_present[0]); CHECK(_load_check_data.legacy_b_supply_present[1]);
	CHECK_FALSE(_load_check_data.legacy_a_supply_wire);
	CHECK(_load_check_data.surface_supply_orders_present); // Detached metadata and complete copied custody grant no live refund or inventory rights.
	const auto legacy = _load_check_data.legacy_b_supply;
	const auto converted = _load_check_data.surface_supply_orders.snapshot;
	size_t linked = 0;
	// Real original B packets keep invalid-Industry scalars in detached raw
	// evidence. Only their exact admitted link projects to non-paying purpose;
	// the historical buyer remains unavailable even when today's owner exists.
	for (const auto &packet : _load_check_data.native_cargo_packet_rows) {
		const SurfaceSupplyPacketFacts raw{packet.packet, packet.quantity, packet.source, packet.feeder_share,
			packet.company_supply_order, packet.company_supply_shipped != 0};
		const auto projected = ProjectLegacyBSupplyPacket(raw, converted); REQUIRE(projected.has_value());
		if (packet.company_supply_order == 0) { CHECK(*projected == raw); continue; }
		++linked; CHECK(packet.source.type == SourceType::Industry); CHECK(packet.source.id == Source::Invalid);
		CHECK(projected->source.IsCompanySupply()); CHECK(projected->source.id == Source::Invalid);
		CHECK(projected->order == raw.order); CHECK(projected->shipped == raw.shipped);
	}
	CHECK(linked != 0); CHECK_FALSE(_load_check_data.native_cargo_owner_rows.empty());
	const auto meanings = GetLegacyMultiLayerCargoLabels(MultiLayerGameplayProfileID::MultilayerIndustry,
		_load_check_data.gameplay_profile_version, _load_check_data.landscape, _load_check_data.grfconfig);
	REQUIRE(meanings.has_value()); CHECK((*meanings)[CargoType{9}] == CargoLabel{"STEL"}); CHECK((*meanings)[CargoType{22}] == CargoLabel{"RFMT"});
	CHECK_FALSE(GetLegacyMultiLayerCargoLabels(MultiLayerGameplayProfileID::MultilayerIndustry, 2,
		_load_check_data.landscape, _load_check_data.grfconfig));
	CHECK_FALSE(GetLegacyMultiLayerCargoLabels(MultiLayerGameplayProfileID::MultilayerIndustry,
		_load_check_data.gameplay_profile_version, LandscapeType::Arctic, _load_check_data.grfconfig));
	REQUIRE(legacy.count != 0);
	CHECK(converted.records.size() == legacy.count); CHECK(converted.next_id == legacy.next_order);
	// Every original B scalar survives conversion to its legacy receipt tag;
	// no native payer, factor or Pending issuance is manufactured along the route.
	for (size_t index = 0; index < converted.records.size(); ++index) {
		const auto &record = converted.records[index]; const auto &original = legacy.orders[index];
		REQUIRE(record.Legacy() != nullptr); CHECK(record.Native() == nullptr);
		CHECK(record.id == original.id); CHECK(record.quantity == original.quantity);
		CHECK(record.Legacy()->unit_price == Money{original.unit_price});
		CHECK(record.Legacy()->refunded == original.refunded); CHECK(record.Legacy()->dispatched == original.dispatched);
	}
	ResetSurfaceSupplyOrderLoadStaging();
	CHECK(StageLoadedLegacyBSupplyRows(legacy.orders));
	CHECK(StageLoadedLegacyBSupplyHeader(legacy.schema, legacy.count, legacy.next_order));
	CHECK_FALSE(StageLoadedLegacyBSupplyHeader(legacy.schema, legacy.count, legacy.next_order));
	CHECK_FALSE(StageLoadedLegacyBSupplyRows(legacy.orders));
	CHECK_FALSE(StageLoadedSurfaceSupplyOrders({}, TimerGameEconomy::date));
	CHECK_FALSE(AcceptLoadedSurfaceSupplyOrders(true)); // Detached staged bytes have no original native DATE/MAPS/packet observations and cannot install refund or cap rights.
	ResetSurfaceSupplyOrderLoadStaging();
	CHECK(CaptureSurfaceSupplyOrders().records == installed.records);
	CHECK(CaptureSurfaceSupplyOrders().next_id == installed.next_id);
	CHECK(Vehicle::GetIfValid(id) == capsule); CHECK(Company::Get(_current_company)->money == cash);
	CHECK_FALSE(GetLoadCapabilities().has_value());
	// Returning to A must clear every preceding B component/proof flag; a stale
	// header or row cannot validate a later absent family by accidental reuse.
	CHECK(LoadWithFilter(EnvelopeNetworkReader(a.bytes), std::nullopt, true) == SaveLoadResult::Ok);
	CHECK_FALSE(_load_check_data.legacy_b_supply_present[0]); CHECK_FALSE(_load_check_data.legacy_b_supply_present[1]);
	CHECK_FALSE(_load_check_data.legacy_b_supply_packet_fields_present);
}

/** Malformed copies of genuine B426bytes refuse through the actual copied loader and cannot borrow the active world's owners or publish rights. */
TEST_CASE("Original B supply wire contradictions refuse without changing native owners", "[save-supply-wire-refusal]")
{
	const auto fault = GENERATE(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
	TubePrototypeRig rig; auto *capsule = rig.BuyCapsule(); PreflightContentCatalogue catalogue(rig.ActivatedContent());
	const auto original = OriginalForPreflight("B", 426);
	auto bytes = EnvelopeReplay(std::make_shared<EnvelopeTestReader>(original.bytes));
	const auto chunks = SupplyWireChunks(bytes);
	const auto &header = SupplyWireFind(chunks, "SSOH"), &orders = SupplyWireFind(chunks, "SSOR"), &packets = SupplyWireFind(chunks, "CAPA");
	REQUIRE(orders.rows.size() > 1); REQUIRE(packets.rows.size() > 1);
	const auto link = SupplyWireColumn(bytes, packets, "company_supply_order");
	const auto shipped = SupplyWireColumn(bytes, packets, "company_supply_shipped");
	const auto source_type = SupplyWireColumn(bytes, packets, "source_type");
	auto paid = packets.rows.begin() + 1;
	// Select actual original linked stock, skipping native ordinal holes and
	// ordinary freight; no same-label packet is manufactured for these cases.
	for (; paid != packets.rows.end(); ++paid) {
		if (paid->payload == paid->end) continue;
		if (SupplyWireGet(bytes, paid->payload + link, 4) != 0) break;
	}
	REQUIRE(paid != packets.rows.end());
	const auto installed = CaptureSurfaceSupplyOrders(); const auto cash = Company::Get(_current_company)->money;
	const auto day = TimerGameEconomy::date; const auto random = _random; const auto packet_count = CargoPacket::GetNumItems();
	INFO(fault);
	switch (fault) {
		case 0: bytes.erase(bytes.begin() + header.begin, bytes.begin() + header.end); break;
		case 1: bytes.erase(bytes.begin() + orders.begin, bytes.begin() + orders.end); break;
		case 2: SupplyWireRename(bytes, orders, "label", "owner"); break;
		case 3: {
			const auto count = SupplyWireGet(bytes, header.payload + 4, 4);
			SupplyWireSet(bytes, header.payload + 4, count + 1, 4); SupplyWireSet(bytes, header.payload + 8, count + 2, 4);
			const auto &row = orders.rows[1]; const std::vector<uint8_t> duplicate(bytes.begin() + row.prefix, bytes.begin() + row.end);
			bytes.insert(bytes.begin() + orders.end - 1, duplicate.begin(), duplicate.end()); break;
		}
		case 4: SupplyWireSet(bytes, header.payload + 4, SupplyWireGet(bytes, header.payload + 4, 4) + 1, 4); break;
		case 5: {
			const auto row = orders.rows[1].payload;
			const auto quantity = SupplyWireGet(bytes, row + SupplyWireColumn(bytes, orders, "quantity"), 2); REQUIRE(quantity > 1);
			SupplyWireSet(bytes, row + SupplyWireColumn(bytes, orders, "unit_price"), uint64_t{INT64_MAX} / quantity + 1, 8); break;
		}
		case 6: SupplyWireSet(bytes, paid->payload + link, UINT32_MAX, 4); break;
		case 7: SupplyWireSet(bytes, paid->payload + shipped, 2, 1); break;
		case 8: SupplyWireSet(bytes, paid->payload + source_type, to_underlying(SourceType::CompanySupply), 1); break;
		case 9: SupplyWireRename(bytes, packets, "company_supply_order", "deleted_supply_order"); break;
		case 10: {
			const std::vector<uint8_t> duplicate(bytes.begin() + orders.begin, bytes.begin() + orders.end);
			bytes.insert(bytes.begin() + orders.end, duplicate.begin(), duplicate.end()); break;
		}
		case 11: bytes[orders.begin + 4] = 0; break;
		case 12: { const auto &chunk = SupplyWireFind(chunks, "DATE"); bytes.erase(bytes.begin() + chunk.begin, bytes.begin() + chunk.end); break; }
		case 13: { const auto &chunk = SupplyWireFind(chunks, "MAPS"); bytes.erase(bytes.begin() + chunk.begin, bytes.begin() + chunk.end); break; }
		case 14: { const auto &chunk = SupplyWireFind(chunks, "PLYR"); bytes.erase(bytes.begin() + chunk.begin, bytes.begin() + chunk.end); break; }
		case 15: { const auto &chunk = SupplyWireFind(chunks, "STNN"); bytes.erase(bytes.begin() + chunk.begin, bytes.begin() + chunk.end); break; }
	}
	const auto malformed = EnvelopeStream(std::move(bytes), "OTTN", 426); // Recompression changes no original version or historical feature threshold.
	if (fault == 6) {
		// Retain this reproducible negative under the disposable test cache so
		// the separate real-application observer can test late full-load refusal.
		const auto root = std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path().parent_path().parent_path();
		const auto output_path = root / "test-cache/merge-supply/group41c-b426-invalid-link.sav";
		std::filesystem::create_directories(output_path.parent_path());
		std::ofstream output(output_path, std::ios::binary); REQUIRE(output.good());
		output.write(reinterpret_cast<const char *>(malformed.data()), malformed.size()); REQUIRE(output.good());
	}
	CHECK(LoadWithFilter(EnvelopeNetworkReader(malformed), std::nullopt, true) == SaveLoadResult::Error);
	CHECK(CaptureSurfaceSupplyOrders() == installed); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(TimerGameEconomy::date == day); CHECK(CargoPacket::GetNumItems() == packet_count);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	CHECK(Vehicle::GetIfValid(capsule->index) == capsule); CHECK_FALSE(GetLoadCapabilities().has_value());
}

/** Actual later-A readers retain original manufacturing, heavy and retired-recipe inputs while copied checking remains independent of the active world. */
TEST_CASE("Original A Space feature epochs pass actual copied family readers", "[save-space-lineage-originals]")
{
	TubePrototypeRig rig; auto *capsule = rig.BuyCapsule(); PreflightContentCatalogue catalogue(rig.ActivatedContent());
	const auto cash = Company::Get(_current_company)->money; const auto book = CaptureSurfaceSupplyOrders(); const auto random = _random;
	// Each named retained epoch carries its genuine original feature/layout.
	// No downgraded header or fresh writer output is used as compatibility input.
	for (const uint16_t version : {426, 427, 428, 429, 431, 434, 436, 437, 438}) {
		const auto original = OriginalForPreflight("A", version); INFO(version);
		auto reader = std::make_shared<ObservePreflightReader>(EnvelopeNetworkReader(original.bytes));
		const auto result = LoadWithFilter(reader, original.hint, true); INFO(_load_check_data.error_msg);
		REQUIRE(result == SaveLoadResult::Ok); REQUIRE_FALSE(reader->observed.empty());
		CHECK(_load_check_data.space_content_version == (version >= 431 ? 3 : 2));
		CHECK(_load_check_data.native_cargo_dimensions_present == (version >= 437));
		CHECK(_load_check_data.orbital_recipes_present == (version >= 438));
		CHECK_FALSE((_load_check_data.space_production_rows.empty() && _load_check_data.space_structure_rows.empty()));
		if (version == 438) CHECK_FALSE(_load_check_data.orbital_retired_recipe_batches.empty());
		CHECK(HasSaveCapability(reader->observed.back(), SaveCapability::OrbitalManufacturingReservations));
		CHECK(HasSaveCapability(reader->observed.back(), SaveCapability::SpaceHeavyMinerCommissioning) == (version >= 431));
		CHECK(Vehicle::GetIfValid(capsule->index) == capsule); CHECK(Company::Get(_current_company)->money == cash);
		CHECK(CaptureSurfaceSupplyOrders() == book); CHECK_FALSE(GetLoadCapabilities().has_value());
		CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	}
}

/** Actual donor files retain content2 and the shared Space prefix without acquiring identically numbered A layouts. */
TEST_CASE("Original donor Space content cannot enable later A families", "[save-space-lineage-originals]")
{
	TubePrototypeRig rig; PreflightContentCatalogue catalogue(rig.ActivatedContent());
	// Later native machinery remains Group43. These fully supported425/426
	// originals exercise actual family dispatch beyond the earlier gate-only probe.
	for (const uint16_t version : {425, 426}) {
		const auto original = OriginalForPreflight("B", version); INFO(version);
		auto reader = std::make_shared<ObservePreflightReader>(EnvelopeNetworkReader(original.bytes));
		const auto result = LoadWithFilter(reader, original.hint, true); INFO(_load_check_data.error_msg);
		REQUIRE(result == SaveLoadResult::Ok); REQUIRE_FALSE(reader->observed.empty());
		CHECK(_load_check_data.space_content_version == 2);
		CHECK_FALSE(_load_check_data.native_cargo_dimensions_present); CHECK_FALSE(_load_check_data.orbital_recipes_present);
		CHECK_FALSE(HasSaveCapability(reader->observed.back(), SaveCapability::OrbitalManufacturingReservations));
		CHECK_FALSE(HasSaveCapability(reader->observed.back(), SaveCapability::SpaceHeavyMinerCommissioning));
		CHECK_FALSE(HasSaveCapability(reader->observed.back(), SaveCapability::OrbitalRecipeProcessing));
		// Every actual shared receipt stays at its original content/role prefix;
		// current A models, replacement producers and manufacture work are absent.
		for (const auto &row : _load_check_data.space_production_rows) {
			CHECK(row.state.content_version <= 2); CHECK(row.state.model < 21); CHECK_FALSE(row.state.replacement.has_value());
		}
		for (const auto &row : _load_check_data.space_structure_rows) {
			CHECK(row.state.content_version <= 2);
			if (row.module) { CHECK(row.module->manufacture_serial == 0); CHECK_FALSE(row.module->manufacture.has_value()); }
		}
		CHECK_FALSE(GetLoadCapabilities().has_value());
	}
}

/** Required later-A metadata cannot default from the active world, and donor content2cannot acquire an appended A processing family. */
TEST_CASE("Original Space family omissions and foreign capabilities refuse copied loading", "[save-space-lineage-refusal]")
{
	const auto fault = GENERATE(0, 1, 2);
	TubePrototypeRig rig; auto *capsule = rig.BuyCapsule(); PreflightContentCatalogue catalogue(rig.ActivatedContent());
	const auto original = fault == 2 ? OriginalForPreflight("B", 426) : OriginalForPreflight("A", fault == 0 ? 438 : 437);
	auto bytes = EnvelopeReplay(std::make_shared<EnvelopeTestReader>(original.bytes));
	const auto chunks = SupplyWireChunks(bytes);
	if (fault < 2) {
		const auto &missing = SupplyWireFind(chunks, fault == 0 ? "SPRC" : "CDIM");
		bytes.erase(bytes.begin() + missing.begin, bytes.begin() + missing.end);
	} else {
		const auto a = OriginalForPreflight("A", 438);
		const auto foreign = EnvelopeReplay(std::make_shared<EnvelopeTestReader>(a.bytes));
		const auto a_chunks = SupplyWireChunks(foreign); const auto &recipe = SupplyWireFind(a_chunks, "SPRC");
		bytes.insert(bytes.end() - 4, foreign.begin() + recipe.begin, foreign.begin() + recipe.end);
	}
	const auto cash = Company::Get(_current_company)->money; const auto book = CaptureSurfaceSupplyOrders(); const auto random = _random;
	const auto malformed = EnvelopeStream(std::move(bytes), "OTTN", original.hint.dialect == SaveDialect::DonorB ? 426 : fault == 0 ? 438 : 437);
	CHECK(LoadWithFilter(EnvelopeNetworkReader(malformed), std::nullopt, true) == SaveLoadResult::Error);
	CHECK(CaptureSurfaceSupplyOrders() == book); CHECK(Company::Get(_current_company)->money == cash);
	CHECK(Vehicle::GetIfValid(capsule->index) == capsule); CHECK_FALSE(GetLoadCapabilities().has_value());
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
}

/** Original donor machinery boundaries retain their actual family identities and legacy external Tube defaults. */
TEST_CASE("Original donor machinery families decode through native copied readers", "[save-machinery-originals]")
{
	TubePrototypeRig rig; PreflightContentCatalogue catalogue(rig.ActivatedContent());
	const auto cash = Company::Get(_current_company)->money; const auto book = CaptureSurfaceSupplyOrders();
	// Each untouched original crosses a real schema/owner boundary. A table
	// required by a newer file must never be borrowed from an earlier selection.
	for (const uint16_t version : {427, 428, 429, 430, 431, 432, 433, 434}) {
		INFO(version); const auto original = OriginalForPreflight("B", version);
		const auto result = LoadWithFilter(EnvelopeNetworkReader(original.bytes), original.hint, true); INFO(_load_check_data.error_msg);
		REQUIRE(result == SaveLoadResult::Ok);
		CHECK(_load_check_data.geothermal_generator_present == std::array<bool, 2>{true, true});
		CHECK(_load_check_data.utility_conversion_present[0] == (version >= 428));
		CHECK(_load_check_data.utility_conversion_present[1] == (version >= 428));
		CHECK(_load_check_data.tube_geothermal_present[0] == (version >= 433));
		CHECK(_load_check_data.tube_geothermal_present[1] == (version >= 433));
		// Genuine pre434 capsules were external service consumers. Absence
		// retains both original sentinels and grants no geothermal provider/feed.
		if (version < 434) {
			for (const auto &row : _load_check_data.tube_vehicle_rows) {
				CHECK(row.geothermal_source.base() == UINT32_MAX); CHECK(row.local_feed.base() == UINT32_MAX);
			}
		}
		CHECK(CaptureSurfaceSupplyOrders() == book); CHECK(Company::Get(_current_company)->money == cash);
		CHECK_FALSE(GetLoadCapabilities().has_value());
	}
}

/** Resolve the immutable original B434 active two-capsule binding through its existing retained-input manifest.
 * @return Original bytes and independently recorded exact B digest; the alias only routes fixture input.
 */
static PreflightOriginal OriginalMachineryBinding()
{
	const auto root = std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path().parent_path().parent_path();
	std::ifstream manifest(root / "src/tests/fixtures/multilayer-inputs.json"); REQUIRE(manifest.good());
	const auto catalogue = nlohmann::json::parse(manifest);
	const auto &row = catalogue.at("inputs").at("donor/openttd3-75ea75718f/mlm/reviews/tubes/geothermal-bindings/20261017-183312-1b5d7d4a28/saves/geothermal-binding-0.sav");
	PreflightOriginal result; result.path = root / row.at("path").get<std::string>();
	std::ifstream input(result.path, std::ios::binary); REQUIRE(input.good());
	result.bytes.assign(std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{});
	const auto digest = EnvelopeHex(row.at("sha256").get<std::string>()); REQUIRE(digest.size() == result.hint.source_digest.size());
	result.hint.dialect = SaveDialect::DonorB; std::ranges::copy(digest, result.hint.source_digest.begin());
	return result;
}

/** Whole original families and their physical participants cannot default into free machinery, output or providers. */
TEST_CASE("Native copied machinery rejects omissions duplicates and mismatched original references", "[save-machinery-refusal]")
{
	const auto fault = GENERATE(range(0, 23)); INFO(fault);
	TubePrototypeRig rig; PreflightContentCatalogue catalogue(rig.ActivatedContent());
	const auto original = OriginalMachineryBinding();
	const auto initial = LoadWithFilter(EnvelopeNetworkReader(original.bytes), original.hint, true); INFO(_load_check_data.error_msg); REQUIRE(initial == SaveLoadResult::Ok);
	REQUIRE(_load_check_data.geothermal_generator_rows.size() == 1); REQUIRE(_load_check_data.tube_vehicle_rows.size() == 2);
	auto bytes = EnvelopeReplay(std::make_shared<EnvelopeTestReader>(original.bytes)); const auto chunks = SupplyWireChunks(bytes);
	const std::array<std::string_view, 6> families{"GPGH", "GPGD", "UCVH", "UCVD", "TGSH", "TGSD"};
	// Complete presence is required even when a family is empty. Duplicating
	// a header/table cannot create an alternate participant set or issuer history.
	if (fault < 12) {
		const auto &chunk = SupplyWireFind(chunks, families[fault % 6]);
		if (fault < 6) bytes.erase(bytes.begin() + chunk.begin, bytes.begin() + chunk.end);
		else {
			const std::vector<uint8_t> duplicate(bytes.begin() + chunk.begin, bytes.begin() + chunk.end);
			bytes.insert(bytes.end() - 4, duplicate.begin(), duplicate.end());
		}
	} else {
		const std::array<std::string_view, 10> family{"GPGD", "GPGD", "GPGD", "UCVD", "UCVD", "TGSD", "TGSD", "TGSD", "TUVD", "TUVD"};
		const std::array<std::string_view, 10> field{"site", "electrical_producer", "industry", "numerator", "input", "generator", "supply", "producer", "geothermal_source", "local_feed"};
		if (fault < 22) {
			const auto &chunk = SupplyWireFind(chunks, family[fault - 12]); REQUIRE(chunk.rows.size() > 1);
			const auto value = fault == 15 ? 100U : fault == 14 || fault == 17 ? 63999U : UINT32_MAX;
			const size_t width = fault == 15 ? 1 : fault == 14 || fault == 17 ? 2 : 4;
			SupplyWireSet(bytes, chunk.rows[1].payload + SupplyWireColumn(bytes, chunk, field[fault - 12]), value, width);
		} else {
			const auto &chunk = SupplyWireFind(chunks, "UPLD");
			const auto type = SupplyWireColumn(bytes, chunk, "type"), kind = SupplyWireColumn(bytes, chunk, "kind"); bool changed = false;
			// Replace an actual local feed's native plant only; unrelated utility
			// quantities and topology remain the untouched original evidence.
			for (size_t index = 1; index < chunk.rows.size(); ++index) {
				const auto payload = chunk.rows[index].payload;
				if (bytes[payload + type] != 4 || bytes[payload + kind] != 3) continue; // Only the original electrical-feed role carries a plant reference.
				SupplyWireSet(bytes, payload + SupplyWireColumn(bytes, chunk, "generator"), 63999, 2); changed = true; break;
			}
			REQUIRE(changed);
		}
	}
	const auto malformed = EnvelopeStream(std::move(bytes), "OTTN", 434);
	const auto cash = Company::Get(_current_company)->money; const auto book = CaptureSurfaceSupplyOrders(); const auto random = _random;
	CHECK(LoadWithFilter(EnvelopeNetworkReader(malformed), std::nullopt, true) == SaveLoadResult::Error);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(CaptureSurfaceSupplyOrders() == book);
	CHECK_FALSE(GetLoadCapabilities().has_value()); CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	// The permanent producer can rebuild every malformed full-reader input
	// here; generated copies never replace or become retained fixture originals.
	const auto root = std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path().parent_path().parent_path();
	const auto cache = root / "test-cache/merge-supply"; std::filesystem::create_directories(cache);
	std::ofstream output(cache / fmt::format("group43-b434-bad-{}.sav", fault), std::ios::binary); REQUIRE(output.good());
	output.write(reinterpret_cast<const char *>(malformed.data()), malformed.size()); REQUIRE(output.good());
}


/** Repeated copied selections and late semantic errors cannot lend a prior file's paid history or mutate its actual open world. */
TEST_CASE("Candidate previews discard failed transport across original lineages", "[save-candidate-lifecycle]")
{
	TubePrototypeRig rig; rig.BuyCapsule(); PreflightContentCatalogue catalogue(rig.ActivatedContent());
	const auto installed = CaptureCandidateLiveState();
	const std::array selections{std::pair{"B", uint16_t{434}}, std::pair{"A", uint16_t{440}}, std::pair{"B", uint16_t{427}},
		std::pair{"A", uint16_t{438}}, std::pair{"B", uint16_t{425}}, std::pair{"A", uint16_t{425}}, std::pair{"B", uint16_t{434}}};
	// Alternating genuine originals must prove their own complete presence,
	// defaults and dialect on every selection, even after richer feature previews.
	for (const auto &[branch, version] : selections) {
		INFO(branch); INFO(version); const auto original = OriginalForPreflight(branch, version);
		REQUIRE(LoadWithFilter(EnvelopeNetworkReader(original.bytes), original.hint, true) == SaveLoadResult::Ok);
		CHECK(CaptureCandidateLiveState() == installed); CHECK_FALSE(GetLoadCapabilities().has_value());
		CHECK(_load_check_data.geothermal_generator_present[0] == (std::string_view{branch} == "B" && version >= 427));
		CHECK(_load_check_data.utility_conversion_present[0] == (std::string_view{branch} == "B" && version >= 428));
		CHECK(_load_check_data.tube_geothermal_present[0] == (std::string_view{branch} == "B" && version >= 433));
		CHECK(_load_check_data.orbital_recipes_present == (std::string_view{branch} == "A" && version >= 438));
	}
	const auto original = OriginalForPreflight("B", 432); auto bytes = EnvelopeReplay(EnvelopeNetworkReader(original.bytes));
	const auto chunks = SupplyWireChunks(bytes); const auto &plants = SupplyWireFind(chunks, "GPGD"); REQUIRE(plants.rows.size() > 1);
	// This keeps framing, columns and classification valid, then refuses the
	// raw future model only after native preview transport has actually accumulated.
	bytes[plants.rows[1].payload + SupplyWireColumn(bytes, plants, "model")] = 255;
	const auto malformed = EnvelopeStream(std::move(bytes), "OTTN", 432);
	CHECK(LoadWithFilter(EnvelopeNetworkReader(malformed), std::nullopt, true) == SaveLoadResult::Error);
	CheckDiscardedLoadCandidate(); CHECK_FALSE(_load_check_data.error_msg.empty()); CHECK(CaptureCandidateLiveState() == installed);
	REQUIRE(LoadWithFilter(EnvelopeNetworkReader(original.bytes), original.hint, true) == SaveLoadResult::Ok);
	CHECK(CaptureCandidateLiveState() == installed);
	const auto absent = std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path().parent_path().parent_path() /
		"test-cache/merge-supply/intentionally-absent-candidate.sav";
	REQUIRE_FALSE(std::filesystem::exists(absent));
	CHECK(SaveOrLoad(absent.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
	CheckDiscardedLoadCandidate(); CHECK(CaptureCandidateLiveState() == installed);
	REQUIRE(LoadWithFilter(EnvelopeNetworkReader(original.bytes), original.hint, true) == SaveLoadResult::Ok);
	CHECK(CaptureCandidateLiveState() == installed);
	// A development build's file between two valid previews refuses before any
	// native read, keeps its own error, and lends nothing to the next candidate.
	const auto development = DevelopmentBuildOriginals().front();
	CHECK(LoadWithFilter(EnvelopeNetworkReader(development.bytes), std::nullopt, true) == SaveLoadResult::Error);
	CheckDiscardedLoadCandidate(); CHECK(_load_check_data.error == STR_GAME_SAVELOAD_ERROR_DEVELOPMENT_SAVEGAME);
	CHECK(CaptureCandidateLiveState() == installed);
	const auto space = OriginalForPreflight("A", 440);
	REQUIRE(LoadWithFilter(EnvelopeNetworkReader(space.bytes), space.hint, true) == SaveLoadResult::Ok);
	CHECK(CaptureCandidateLiveState() == installed);
}

/** Running-world facts a preview must never change: native state plus profile and stacked geometry. */
struct PreviewWorldFacts {
	CandidateLiveState live; ///< Accounts, packets, RNG, domains, tick and camera.
	MultiLayerGameplayProfileID profile; ///< Running world's selected profile.
	uint16_t profile_version; ///< Its exact contract version.
	std::array<uint, 3> geometry; ///< Plane width, plane height and layer count.
	/** @param other Independently recaptured facts. @return Exact equality. */
	bool operator==(const PreviewWorldFacts &other) const = default;
};

/** @return The running world's preview-invariant facts, read from the native owners only. */
static PreviewWorldFacts CapturePreviewWorldFacts()
{
	const auto &profile = GetCurrentMultiLayerProfileSelection();
	return {CaptureCandidateLiveState(), profile.id, profile.version, {Map::SizeX(), Map::SizeY(), Map::LayerCount()}};
}

/** @return Whether the kept refusal reason comes from the copied owner join, with or without the paid book. */
static bool IsOwnerJoinRefusal()
{
	const std::string_view reason = _load_check_data.error_msg;
	return reason.find("ownership") != std::string_view::npos || reason.find("custody") != std::string_view::npos;
}

/** A, development-build and merged candidates alternate with malformed custody against one running world (m2o28 Group 20).
 * A433 holds natural freight, paid supply and ordinary trains, so every refusal below runs through the one owner join.
 */
TEST_CASE("Owner-join previews alternate lineages and refuse malformed custody without touching the world", "[save-candidate-lifecycle]")
{
	TubePrototypeRig rig; rig.BuyCapsule(); PreflightContentCatalogue catalogue(rig.ActivatedContent());
	const auto root = std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path().parent_path().parent_path();
	const auto cache = root / "test-cache/merge-oct28/group20"; std::filesystem::create_directories(cache);
	// The running world itself is the merged candidate: the current writer saves it.
	const auto merged_path = cache / "running-world-merged.sav";
	{
		// The headless world has no cursor; the writer's busy toggle needs one entry that is neither mouse nor ZZZ.
		const auto cursor = _cursor.sprites; _cursor.sprites.assign(1, CursorSprite{0, PAL_NONE, 0, 0});
		LayerSaveContent content(rig.ActivatedContent(), true); // The file names the world's actual package, as a real save does.
		const auto saved = SaveWithFilter(std::make_shared<LayerSaveFileSink>(merged_path), false);
		_cursor.sprites = cursor; REQUIRE(saved == SaveLoadResult::Ok);
	}
	std::ifstream merged_file(merged_path, std::ios::binary); REQUIRE(merged_file.good());
	const std::vector<uint8_t> merged{std::istreambuf_iterator<char>{merged_file}, std::istreambuf_iterator<char>{}};
	const auto installed = CapturePreviewWorldFacts();
	const auto space = OriginalForPreflight("A", 433);
	/** @return Whether the A433 original previews, leaving the world exactly as installed. */
	const auto accept_space = [&] {
		const auto result = LoadWithFilter(EnvelopeNetworkReader(space.bytes), space.hint, true); INFO(_load_check_data.error_msg);
		CHECK(CapturePreviewWorldFacts() == installed); CHECK_FALSE(GetLoadCapabilities().has_value());
		return result == SaveLoadResult::Ok;
	};
	/** @param bytes Malformed copy. @return Whether it refused through the owner join, kept its reason, cleared its data and left the world alone. */
	const auto refuse = [&](std::span<const uint8_t> bytes) {
		const auto result = LoadWithFilter(EnvelopeNetworkReader(bytes), std::nullopt, true); INFO(_load_check_data.error_msg);
		CheckDiscardedLoadCandidate(); CHECK(CapturePreviewWorldFacts() == installed);
		return result == SaveLoadResult::Error && IsOwnerJoinRefusal();
	};
	REQUIRE(accept_space());
	const auto natural = std::ranges::count(_load_check_data.native_cargo_packet_rows, SourceType::Asteroid, [](const auto &row) { return row.source.type; });
	REQUIRE(natural > 0); REQUIRE_FALSE(_load_check_data.native_cargo_owner_rows.empty());
	const auto owners = _load_check_data.native_cargo_owner_rows; const auto packets = _load_check_data.native_cargo_packet_rows;
	// A development build's typed-origin file refuses before any native read.
	const auto development = DevelopmentBuildOriginals().front();
	CHECK(LoadWithFilter(EnvelopeNetworkReader(development.bytes), std::nullopt, true) == SaveLoadResult::Error);
	CheckDiscardedLoadCandidate(); CHECK(_load_check_data.error == STR_GAME_SAVELOAD_ERROR_DEVELOPMENT_SAVEGAME);
	CHECK(CapturePreviewWorldFacts() == installed);
	/** @return Whether the running world's merged save previews from its own marker, leaving the world exactly as installed. */
	const auto accept_merged = [&] {
		const auto result = LoadWithFilter(EnvelopeNetworkReader(merged), std::nullopt, true); INFO(_load_check_data.error_msg);
		CHECK(_load_check_data.error_msg.empty()); CHECK(CapturePreviewWorldFacts() == installed);
		return result == SaveLoadResult::Ok;
	};
	// The merged file previews from its own marker and lends nothing to the next candidate.
	REQUIRE(accept_merged());
	CHECK(_load_check_data.native_cargo_packet_rows.size() == CargoPacket::GetNumItems()); CHECK(CapturePreviewWorldFacts() == installed);
	const auto original = EnvelopeReplay(std::make_shared<EnvelopeTestReader>(space.bytes)); const auto chunks = SupplyWireChunks(original);
	SECTION("Natural source produced outside the Space plane") {
		auto bytes = original; const auto &capa = SupplyWireFind(chunks, "CAPA");
		const auto type = SupplyWireColumn(bytes, capa, "source_type"), tile = SupplyWireColumn(bytes, capa, "source_xy");
		bool changed = false;
		// Move one actual natural packet's production tile to the Surface; its body and container stay.
		for (size_t index = 1; index < capa.rows.size() && !changed; ++index) {
			const auto &row = capa.rows[index];
			if (row.end == row.payload || bytes[row.payload + type] != to_underlying(SourceType::Asteroid)) continue;
			SupplyWireSet(bytes, row.payload + tile, 272, 4); changed = true;
		}
		REQUIRE(changed);
		CHECK(refuse(EnvelopeStream(std::move(bytes), "OTTN", 433)));
	}
	SECTION("A copied station or carrier duplicates another owner's packets") {
		size_t refused = 0;
		/** @param chunk Original owner family. @param row Its row to copy. @param index Fresh one-byte sparse index, or no value for an ordered table. */
		const auto duplicate = [&](const SupplyWireChunk &chunk, const SupplyWireRow &row, std::optional<uint8_t> index) {
			auto bytes = original;
			std::vector<uint8_t> copy(bytes.begin() + row.prefix, bytes.begin() + row.end);
			if (index) copy[row.payload - row.prefix] = *index; // Same one-byte gamma width, so the row length is unchanged.
			bytes.insert(bytes.begin() + chunk.end - 1, copy.begin(), copy.end()); // Before the family terminator.
			const auto malformed = EnvelopeStream(std::move(bytes), "OTTN", 433);
			// A copy of an empty owner aliases nothing; one that holds packets gives them a second owner.
			if (LoadWithFilter(EnvelopeNetworkReader(malformed), std::nullopt, true) == SaveLoadResult::Ok) { CHECK(CapturePreviewWorldFacts() == installed); return; }
			INFO(_load_check_data.error_msg); CHECK(IsOwnerJoinRefusal()); CheckDiscardedLoadCandidate(); CHECK(CapturePreviewWorldFacts() == installed);
			++refused;
		};
		// Every station row is copied as the next ordered station.
		const auto &stations = SupplyWireFind(chunks, "STNN");
		for (size_t index = 1; index < stations.rows.size(); ++index) duplicate(stations, stations.rows[index], std::nullopt);
		// Every ordinary train row is copied under a fresh sparse index.
		const auto &vehicles = SupplyWireFind(chunks, "VEHS");
		for (size_t index = 1; index < vehicles.rows.size(); ++index) {
			const auto &row = vehicles.rows[index];
			if (row.end - row.payload < 2 || original[row.payload] >= 0x7F || original[row.payload + 1] != to_underlying(VehicleType::Train)) continue;
			duplicate(vehicles, row, uint8_t{0x7F});
		}
		CHECK(refused > 0);
	}
	// The A original previews again with exactly its own facts, after every refusal.
	REQUIRE(accept_space());
	CHECK(_load_check_data.native_cargo_packet_rows.size() == packets.size()); CHECK(_load_check_data.native_cargo_owner_rows.size() == owners.size());
	REQUIRE(accept_merged());
}

/** Clear() resets every field the merge added, so no previous candidate can lend copied evidence to the next. */
TEST_CASE("Candidate preview reset clears every merged copied field", "[save-candidate-lifecycle]")
{
	LoadCheckData data;
	// Give every merged field a non-default value, as a rich previous candidate would.
	data.native_cargo_packet_rows.push_back({CargoPacketID{3}, 2, Source::FromAsteroid(7, CargoLabel{"NORE"}), Money{0}, TileIndex{5}, StationID::Invalid(), true, 9, 1});
	data.native_cargo_owner_rows.push_back({4, CargoType{1}, StationID{2}});
	data.asteroid_present = data.asteroid_active = data.asteroid_knowledge_present = true;
	data.asteroid_next_body_id = 8; data.asteroid_body_count = 1;
	data.asteroid_body_rows.push_back({OrbitalBodyID{7}, AsteroidClass::Metallic});
	data.asteroid_mining_leases.emplace_back(OrbitalBodyID{7}, VehicleID{1});
	data.asteroid_knowledge_rows.push_back({OrbitalBodyID{7}, uint64_t{1}});
	data.next_launch_center_generation = 5;
	data.native_space_vehicle_rows.emplace_back(); data.native_space_vehicle_rows.back().mining_continuation.retry_ticks = 3;
	data.space_order_preview_rows.emplace_back();
	data.native_cargo_dimensions_present = true; data.native_cargo_dimension_rows.push_back({CargoType{1}, CargoLabel{"NORE"}, 16});
	data.legacy_a_supply_wire = data.legacy_b_supply_packet_fields_present = true;
	data.Clear();
	CHECK(data.native_cargo_packet_rows.empty()); CHECK(data.native_cargo_owner_rows.empty());
	CHECK_FALSE(data.asteroid_present); CHECK_FALSE(data.asteroid_active); CHECK_FALSE(data.asteroid_knowledge_present);
	CHECK(data.asteroid_next_body_id == 0); CHECK(data.asteroid_body_count == 0);
	CHECK(data.asteroid_body_rows.empty()); CHECK(data.asteroid_mining_leases.empty()); CHECK(data.asteroid_knowledge_rows.empty());
	CHECK(data.next_launch_center_generation == 1);
	CHECK(data.native_space_vehicle_rows.empty()); CHECK(data.space_order_preview_rows.empty());
	CHECK_FALSE(data.native_cargo_dimensions_present); CHECK(data.native_cargo_dimension_rows.empty());
	CHECK_FALSE(data.legacy_a_supply_wire); CHECK_FALSE(data.legacy_b_supply_packet_fields_present);
}

/** Real files a development build wrote at 445 are Unsupported from their own bytes and refuse before any native read. */
TEST_CASE("Native preflight refuses development-build originals without touching the world", "[save-lineage-preflight]")
{
	TubePrototypeRig rig; auto *capsule = rig.BuyCapsule();
	PreflightFileSearch search;
	const auto cash = Company::Get(_current_company)->money;
	const auto id = capsule->index;
	const auto tile = capsule->tile;
	const auto map = Map::Size();
	const auto tick = TimerGameTick::counter;
	const std::array<uint32_t, 2> rng{_random.state[0], _random.state[1]};
	// Each original: identity, detached classification and capabilities first,
	// then every native entry refuses without binding a lineage or reading a chunk.
	for (const auto &original : DevelopmentBuildOriginals()) {
		INFO(original.path.string());
		const auto facts = InspectSaveWireFacts(std::make_shared<EnvelopeTestReader>(original.bytes));
		CHECK(EnvelopeDigest(facts.source_digest) == original.sha256);
		CHECK(facts.asteroid_origin_columns == SaveEvidenceState::Valid);
		CHECK(facts.supply_book_shape == SupplyBookShape::Fixed);
		const auto result = ClassifySaveDialect(facts);
		CHECK(result.status == SaveLineageStatus::Unsupported); CHECK(result.original_version == 445);
		CHECK_FALSE(HasSaveCapability(result, SaveCapability::AsteroidCargoOrigins));
		CHECK_FALSE(HasSaveCapability(result, SaveCapability::SurfaceSupplyOrdersA));
		// A legacy hint naming a historical writer changes nothing for this number.
		CHECK(ClassifySaveDialect(facts, SaveOriginHint{SaveDialect::DonorB, facts.source_digest}).status == SaveLineageStatus::Unsupported);
		auto network = std::make_shared<ObservePreflightReader>(EnvelopeNetworkReader(original.bytes));
		CHECK(LoadWithFilter(network) == SaveLoadResult::Error);
		CHECK(network->observed.empty()); CHECK_FALSE(GetLoadCapabilities().has_value());
		CHECK(SaveOrLoad(original.path.string(), SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
		CHECK(_load_check_data.error == STR_GAME_SAVELOAD_ERROR_DEVELOPMENT_SAVEGAME);
		CHECK(SaveOrLoad(original.path.string(), SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
		CHECK_FALSE(GetLoadCapabilities().has_value());
	}
	// The open world is exactly as it was: vehicle, money, map, tick and RNG.
	CHECK(Vehicle::GetIfValid(id) == capsule); CHECK(capsule->tile == tile); CHECK(capsule->IsInDepot());
	CHECK(Company::Get(_current_company)->money == cash); CHECK(Map::Size() == map);
	CHECK(TimerGameTick::counter == tick); CHECK(_random.state[0] == rng[0]); CHECK(_random.state[1] == rng[1]);
}

