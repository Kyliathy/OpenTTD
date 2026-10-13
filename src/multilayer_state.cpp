/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file multilayer_state.cpp Metadata validation/staging without an alternate profile or feature pool. @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md; r2mlvs6-mines-and-resource-chains.md; r2mlvs12-solar-farms-mirrors-and-eti.md */

#include "stdafx.h"
#include "asteroid.h"
#include "launch_center.h"
#include "multilayer_space_content.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_pod.h"
#include "space_transition.h"
#include "multilayer_state.h"
#include "debug.h"
#include "excavation.h"
#include "layer_portal_cmd.h"
#include "tbm.h"
#include "spoil_receiver.h"
#include "tbm_exchange.h"
#include "multilayer_industry.h"
#include "layer_industry_processor.h"
#include "utility_power.h"
#include "transport_tube.h"
#include "geothermal_collector.h"
#include "geothermal_riser.h"
#include "geothermal_heat.h"

#include "safeguards.h"

static std::optional<MultiLayerStateSchema> _pending_multilayer_schema; ///< Unaccepted load transport, never gameplay authority.
static uint64_t _multilayer_geology_seed = 0; ///< Metadata-owned root published by completed geology generation.
static uint64_t _multilayer_orbital_seed = 0; ///< Metadata-owned orbital seed root, unused until Space implementation.

/**
 * Check the candidate's schema/profile/package and feature-owned reference boundaries.
 * @param schema Candidate metadata transport snapshot.
 * @param checks Actual feature-supplied read-only reference checks.
 * @return Structural/reference refusal or None; no live policy is published.
 */
MultiLayerStateError ValidateMultiLayerStateReferences(const MultiLayerStateSchema &schema, std::span<const MultiLayerStateReferenceCheck> checks)
{
	if (schema.schema_version != MULTILAYER_STATE_SCHEMA_VERSION) return MultiLayerStateError::UnsupportedSchema;
	const auto *profile = GetMultiLayerContentProfile(schema.profile.id);
	if (profile == nullptr) return MultiLayerStateError::UnsupportedProfile;
	if (schema.profile.version != profile->version) return MultiLayerStateError::UnsupportedProfile;
	// Only delivered feature descriptors are recognized; Classic cannot hide geological authority behind metadata.
	if ((schema.feature_flags & ~(MULTILAYER_FEATURE_GEOLOGY | MULTILAYER_FEATURE_VENTS | MULTILAYER_FEATURE_EXCAVATION_JOBS | MULTILAYER_FEATURE_EXCAVATION_PASSES | MULTILAYER_FEATURE_PORTAL_WORK | MULTILAYER_FEATURE_TBM_PROJECTS | MULTILAYER_FEATURE_SPOIL_RECEIVERS | MULTILAYER_FEATURE_TBM_EXCHANGE | MULTILAYER_FEATURE_MINES | MULTILAYER_FEATURE_PROCESSORS | MULTILAYER_FEATURE_UTILITY_POWER | MULTILAYER_FEATURE_TUBE_POWER | MULTILAYER_FEATURE_TUBE_VEHICLES | MULTILAYER_FEATURE_GEOTHERMAL_SITES | MULTILAYER_FEATURE_GEOTHERMAL_COLLECTORS | MULTILAYER_FEATURE_UTILITY_HEAT | MULTILAYER_FEATURE_LOCAL_UTILITY_LINKS | MULTILAYER_FEATURE_GEOTHERMAL_HEAT_ROUTES | MULTILAYER_FEATURE_GEOTHERMAL_RISERS | MULTILAYER_FEATURE_ASTEROIDS | MULTILAYER_FEATURE_LAUNCH_CENTERS | MULTILAYER_FEATURE_LAUNCH_HUBS | MULTILAYER_FEATURE_SURFACE_HANDLING)) != 0 || (schema.feature_flags != 0 && schema.profile.id == MultiLayerGameplayProfileID::Classic)) return MultiLayerStateError::UnsupportedFeatures;
	// Retired riser machinery still owns paid native construction on excavated
	// floor. Its original portal may legitimately be gone, and no energy ledger
	// is required until the separate operating exporter actually exists.
	if ((schema.feature_flags & MULTILAYER_FEATURE_GEOTHERMAL_RISERS) != 0 && (schema.feature_flags & MULTILAYER_FEATURE_GEOLOGY) == 0) return MultiLayerStateError::UnsupportedFeatures;
	// Physical Heat rights cannot omit their sole quantity owner or paid paths.
	const uint32_t heat_route_owners = MULTILAYER_FEATURE_UTILITY_HEAT | MULTILAYER_FEATURE_LOCAL_UTILITY_LINKS;
	const uint32_t handling_owners = MULTILAYER_FEATURE_LAUNCH_CENTERS | MULTILAYER_FEATURE_LAUNCH_HUBS;
	if ((schema.feature_flags & MULTILAYER_FEATURE_SURFACE_HANDLING) != 0 && (schema.feature_flags & handling_owners) != handling_owners) return MultiLayerStateError::UnsupportedFeatures;
	if ((schema.feature_flags & MULTILAYER_FEATURE_GEOTHERMAL_HEAT_ROUTES) != 0 && (schema.feature_flags & heat_route_owners) != heat_route_owners) return MultiLayerStateError::UnsupportedFeatures;
	// Heat and electricity share native identities/accounting. A heat feature
	// cannot claim balances or roles while omitting that sole utility authority.
	if ((schema.feature_flags & MULTILAYER_FEATURE_UTILITY_HEAT) != 0 && (schema.feature_flags & MULTILAYER_FEATURE_UTILITY_POWER) == 0) return MultiLayerStateError::UnsupportedFeatures;
	// Installed local cells need the shared identity owner and actual geology;
	// metadata cannot promote a geometry-only declaration into a paid path.
	if ((schema.feature_flags & MULTILAYER_FEATURE_LOCAL_UTILITY_LINKS) != 0 &&
		(schema.feature_flags & (MULTILAYER_FEATURE_UTILITY_POWER | MULTILAYER_FEATURE_GEOLOGY)) != (MULTILAYER_FEATURE_UTILITY_POWER | MULTILAYER_FEATURE_GEOLOGY)) return MultiLayerStateError::UnsupportedFeatures;
	// Cold planning cannot manufacture its natural source or hide geology-free
	// reservations; every retained site refers to the same save's vent owner.
	const uint32_t site_owners = MULTILAYER_FEATURE_GEOLOGY | MULTILAYER_FEATURE_VENTS;
	if ((schema.feature_flags & MULTILAYER_FEATURE_GEOTHERMAL_SITES) != 0 && (schema.feature_flags & site_owners) != site_owners) return MultiLayerStateError::UnsupportedFeatures;
	// Actual collector construction cannot forget its consumed original blueprint
	// or natural source; metadata grants no implicit paid machinery or stock.
	const uint32_t collector_owners = site_owners | MULTILAYER_FEATURE_GEOTHERMAL_SITES;
	if ((schema.feature_flags & MULTILAYER_FEATURE_GEOTHERMAL_COLLECTORS) != 0 && (schema.feature_flags & collector_owners) != collector_owners) return MultiLayerStateError::UnsupportedFeatures;
	// A physical paid inlet cannot exist without the same save's electrical,
	// commissioned portal and excavated floor authorities.
	const uint32_t tube_owners = MULTILAYER_FEATURE_UTILITY_POWER | MULTILAYER_FEATURE_PORTAL_WORK | MULTILAYER_FEATURE_GEOLOGY;
	if ((schema.feature_flags & MULTILAYER_FEATURE_TUBE_POWER) != 0 && (schema.feature_flags & tube_owners) != tube_owners) return MultiLayerStateError::UnsupportedFeatures;
	// A capsule reservation resolves its own installed service; a standalone
	// binding bit cannot manufacture either physical or electrical authority.
	if ((schema.feature_flags & MULTILAYER_FEATURE_TUBE_VEHICLES) != 0 && (schema.feature_flags & MULTILAYER_FEATURE_TUBE_POWER) == 0) return MultiLayerStateError::UnsupportedFeatures;
	// Every finite binding references existing geology; a mine flag cannot manufacture reserve authority on load.
	if ((schema.feature_flags & MULTILAYER_FEATURE_MINES) != 0 && (schema.feature_flags & MULTILAYER_FEATURE_GEOLOGY) == 0) return MultiLayerStateError::UnsupportedFeatures;
	// Natural vent geometry depends on the delivered rock-plane owner; a standalone bit cannot fabricate a usable thermal field.
	if ((schema.feature_flags & MULTILAYER_FEATURE_VENTS) != 0 && (schema.feature_flags & MULTILAYER_FEATURE_GEOLOGY) == 0) return MultiLayerStateError::UnsupportedFeatures;
	// Job claims and retired IDs belong to active geological authority, never an independent hidden work/cargo owner.
	if ((schema.feature_flags & MULTILAYER_FEATURE_EXCAVATION_JOBS) != 0 && (schema.feature_flags & MULTILAYER_FEATURE_GEOLOGY) == 0) return MultiLayerStateError::UnsupportedFeatures;
	if ((schema.feature_flags & MULTILAYER_FEATURE_EXCAVATION_PASSES) != 0 && (schema.feature_flags & MULTILAYER_FEATURE_EXCAVATION_JOBS) == 0) return MultiLayerStateError::UnsupportedFeatures;
	if ((schema.feature_flags & MULTILAYER_FEATURE_TBM_PROJECTS) != 0 && (schema.feature_flags & MULTILAYER_FEATURE_EXCAVATION_JOBS) == 0) return MultiLayerStateError::UnsupportedFeatures; // Every project was published with a real job, so its allocation history implies job history. // A return/capacity proof belongs to an actual job; metadata cannot invent standalone work.
	// Every assignment and set was made for a real project, so exchange history implies project history.
	if ((schema.feature_flags & MULTILAYER_FEATURE_TBM_EXCHANGE) != 0 && (schema.feature_flags & MULTILAYER_FEATURE_TBM_PROJECTS) == 0) return MultiLayerStateError::UnsupportedFeatures;
	if (schema.profile.id == MultiLayerGameplayProfileID::Classic) {
		if (!schema.profile.legacy) return MultiLayerStateError::InvalidPolicy;
		if (!schema.required_content.grfid.Empty()) return MultiLayerStateError::ChangedContent;
		if (schema.required_content.md5sum != MD5Hash{}) return MultiLayerStateError::ChangedContent;
		if (schema.content_version != 0) return MultiLayerStateError::ChangedContent;
	} else {
		if (schema.profile.legacy) return MultiLayerStateError::InvalidPolicy;
		const auto &required = *profile->required_content;
		if (schema.required_content.grfid != required.package.grfid) return MultiLayerStateError::ChangedContent;
		if (schema.required_content.md5sum != required.package.md5sum) return MultiLayerStateError::ChangedContent;
		if (schema.content_version != required.version) return MultiLayerStateError::ChangedContent;
	}
	if (checks.size() > 64) return MultiLayerStateError::InvalidReference;
	std::set<std::string_view> owners;
	// Every actual feature validates its own typed references once; no absent record is repaired by a second owner.
	for (const auto &check : checks) {
		if (check.owner.empty()) return MultiLayerStateError::InvalidReference;
		if (!owners.insert(check.owner).second) return MultiLayerStateError::InvalidReference;
		if (check.validate == nullptr) return MultiLayerStateError::InvalidReference;
		if (!check.validate()) return MultiLayerStateError::InvalidReference;
	}
	return MultiLayerStateError::None;
}

/**
 * Capture metadata from its real owners for one serialization/preview transport object.
 * @return Current accepted policy, required manifest and seed roots.
 */
MultiLayerStateSchema CaptureMultiLayerStateSchema()
{
	MultiLayerStateSchema schema;
	if (GetAsteroidField().generation_version != 0) schema.feature_flags |= MULTILAYER_FEATURE_ASTEROIDS;
	if (GetNextLaunchCenterGeneration() != 1) schema.feature_flags |= MULTILAYER_FEATURE_LAUNCH_CENTERS;
	if (GetNextLaunchHubMemberGeneration() != 1) schema.feature_flags |= MULTILAYER_FEATURE_LAUNCH_HUBS;
	if (GetNextSurfaceHandlingID() != 1) schema.feature_flags |= MULTILAYER_FEATURE_SURFACE_HANDLING;
	schema.profile = GetCurrentMultiLayerProfileSelection();
	schema.geology_seed = _multilayer_geology_seed;
	schema.orbital_seed = _multilayer_orbital_seed;
	if (GetGeologyWorldState().generator_version != 0) schema.feature_flags |= MULTILAYER_FEATURE_GEOLOGY;
	if (!GetGeologyWorldState().vents.empty()) schema.feature_flags |= MULTILAYER_FEATURE_VENTS;
	if (GetGeologyWorldState().next_job_id != 0) schema.feature_flags |= MULTILAYER_FEATURE_EXCAVATION_JOBS;
	if (GetNextTBMProjectID().base() != 0) schema.feature_flags |= MULTILAYER_FEATURE_TBM_PROJECTS; // Allocation history, like job IDs, survives retirement.
	if (!CaptureSpoilReceivers().empty()) schema.feature_flags |= MULTILAYER_FEATURE_SPOIL_RECEIVERS; // Receivers live and die with their industries; no allocation history.
	if (!CaptureLayerMines().empty()) schema.feature_flags |= MULTILAYER_FEATURE_MINES; // Physical bindings live and die with their industries; reserves keep their geological owner.
	if (!CaptureLayerProcessors().empty()) schema.feature_flags |= MULTILAYER_FEATURE_PROCESSORS; // Choice/work/history belong to actual processors, independently of geology.
	if (HasUtilityPowerState()) schema.feature_flags |= MULTILAYER_FEATURE_UTILITY_POWER; // Retained source/epoch identity survives zero supply and source retirement.
	if (HasUtilityHeatState()) schema.feature_flags |= MULTILAYER_FEATURE_UTILITY_HEAT; // Actual typed heat identity/phase history resides in the same native utility owner.
	if (HasUtilityLocalLinks()) schema.feature_flags |= MULTILAYER_FEATURE_LOCAL_UTILITY_LINKS; // Retained paid physical identities share the utility issuer, with no generated energy implied.
	if (!CaptureGeothermalHeatConsumers().empty()) schema.feature_flags |= MULTILAYER_FEATURE_GEOTHERMAL_HEAT_ROUTES; // Retained physical ports cannot silently lose their exact source-slice paths on reload.
	if (!CaptureTubePowerConnections().empty()) schema.feature_flags |= MULTILAYER_FEATURE_TUBE_POWER; // Invoice history survives closing a service; joules stay in UtilityRoot.
	if (!CaptureTubePowerVehicles().empty()) schema.feature_flags |= MULTILAYER_FEATURE_TUBE_VEHICLES; // Retired bindings retain ledger lineage and cannot adopt reused pool IDs.
	if (!CaptureGeothermalCollectors().empty()) schema.feature_flags |= MULTILAYER_FEATURE_GEOTHERMAL_COLLECTORS; // Actual construction survives shortages/abandonment without copying native input stock or source capacity.
	if (!CaptureGeothermalRisers().empty()) schema.feature_flags |= MULTILAYER_FEATURE_GEOTHERMAL_RISERS; // Retained original-housing loss/work survives demolition and empty energy state without inventing a live portal.
	if (!CaptureGeothermalSiteClaims().empty()) schema.feature_flags |= MULTILAYER_FEATURE_GEOTHERMAL_SITES; // Cancelled cold identities persist without claiming finished construction or energy.
	if (GetNextTBMExchangeAssignmentID() != 0 || GetNextTBMWagonSetID() != 0) schema.feature_flags |= MULTILAYER_FEATURE_TBM_EXCHANGE; // Identity cursors, like project IDs, survive release.
	if (GetLayerPortalWorkPairCount() != 0) schema.feature_flags |= MULTILAYER_FEATURE_PORTAL_WORK; // Metadata/diagnostic queries read accepted owner cardinality without allocating a copied save transport.
	// Only actual retained validated input enables the new family; old unvalidated paused jobs remain recoverable.
	for (const auto &job : GetGeologyWorldState().jobs) {
		if (job.pass_terms) schema.feature_flags |= MULTILAYER_FEATURE_EXCAVATION_PASSES;
	}
	const auto *profile = GetMultiLayerContentProfile(schema.profile.id);
	// Classic preserves arbitrary legacy content; only the explicit profile pins the required first-party manifest.
	if (profile != nullptr && profile->required_content != nullptr) {
		schema.required_content = profile->required_content->package;
		schema.content_version = profile->required_content->version;
	}
	return schema;
}

/** Clear load transport and feature seed roots during ordinary world initialization. */
void ResetMultiLayerStateSchema()
{
	ResetSpaceContentLoadStaging();
	ResetSpaceProductionLoadStaging();
	ResetSpaceManifestLoadStaging();
	ResetSpacePods();
	ResetSpaceDescents();
	ResetSpaceTransitionLoadStaging();
	ResetSpaceOrders();
	ResetAsteroidField();
	ResetLaunchCenters();
	_pending_multilayer_schema.reset();
	_multilayer_geology_seed = 0;
	_multilayer_orbital_seed = 0;
	// World initialization must not leave a previous world's electrical accounts
	// or allocation epoch available to a new native or Classic world.
	ResetUtilityPowerState();
	ResetTubePowerState();
	ResetGeothermalCollectors(); // Constructed bindings cannot survive into another native industry pool or natural source owner.
	ResetGeothermalRisers(); // Paid machinery and portal incarnations cannot leak across native worlds or profiles.
	ResetGeothermalSiteClaims(); // Cold geometry reservations cannot survive into another world, profile or geological source family.
}

/**
 * Publish the completed geological candidate's root without introducing another mutable seed authority.
 * @param seed Independently derived synchronized geology seed.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md
 */
void SetMultiLayerGeologySeed(uint64_t seed)
{
	_multilayer_geology_seed = seed;
}

void SetMultiLayerOrbitalSeed(uint64_t seed) { _multilayer_orbital_seed = seed; }

/**
 * Stage one validated metadata row without accepting its policy or changing current seeds.
 * @param schema Read-only parsed row; save handler validates before calling.
 */
void StageLoadedMultiLayerStateSchema(const MultiLayerStateSchema &schema)
{
	_pending_multilayer_schema = schema;
}

/**
 * Inspect the transient load row before activated definitions are available.
 * @return Candidate pointer, or nullptr for legacy absence.
 */
const MultiLayerStateSchema *GetPendingMultiLayerStateSchema()
{
	return _pending_multilayer_schema ? &*_pending_multilayer_schema : nullptr;
}

/**
 * Restore the actual owner fields only after content activation validates the candidate.
 * @param landscape Loaded climate, independent of pending new-game UI settings.
 * @param activated Actual GRF/cargo registry at the post-loader boundary.
 * @return Refusal cause, or None; failure retains initialized Classic policy and seed roots.
 */
MultiLayerProfileError AcceptLoadedMultiLayerStateSchema(LandscapeType landscape, const GRFConfigList &activated)
{
	if (!_pending_multilayer_schema) {
		Debug(Facility::Misc, Severity::Info, "MLVS_METADATA absent profile=0 version=0 legacy=true");
		return MultiLayerProfileError::None;
	}
	const auto &schema = *_pending_multilayer_schema;
	if (ValidateMultiLayerStateReferences(schema) != MultiLayerStateError::None) return MultiLayerProfileError::UnknownProfile;
	const auto error = RestoreMultiLayerProfileSelection(schema.profile, landscape, activated);
	if (error != MultiLayerProfileError::None) return error;
	_multilayer_geology_seed = schema.geology_seed;
	_multilayer_orbital_seed = schema.orbital_seed;
	Debug(Facility::Misc, Severity::Info, "MLVS_METADATA schema={} profile={} version={} legacy={} editor={} features={} geology_seed={} orbital_seed={}", schema.schema_version, to_underlying(schema.profile.id), schema.profile.version, schema.profile.legacy, schema.profile.editor_override, schema.feature_flags, schema.geology_seed, schema.orbital_seed);
	_pending_multilayer_schema.reset();
	return MultiLayerProfileError::None;
}

/**
 * Append explicit big-endian integer bytes to a local diagnostic hash.
 * @param digest Local FNV accumulator; never a gameplay random state.
 * @param value Validated unsigned field value.
 * @param width Declared byte width, already validated as 1/2/4/8.
 */
static void HashCanonicalUnsigned(uint64_t &digest, uint64_t value, uint8_t width)
{
	// Explicit bytes avoid ABI padding/endian differences between save/network clients.
	for (int shift = (width - 1) * 8; shift >= 0; shift -= 8) {
		digest ^= static_cast<uint8_t>(value >> shift);
		digest *= 1099511628211ULL;
	}
}

/** Stable feature/ID ordering independent of pool insertion or unordered container iteration. */
struct CanonicalRecordLess {
	/**
	 * Compare two actual owner views in canonical feature/ID order.
	 * @param a Left record.
	 * @param b Right record.
	 * @return Whether a precedes b; fields are sorted separately by tag.
	 */
	bool operator()(const MultiLayerCanonicalRecord &a, const MultiLayerCanonicalRecord &b) const
	{
		if (a.kind != b.kind) return to_underlying(a.kind) < to_underlying(b.kind);
		return a.id < b.id;
	}
};

/**
 * Digest explicit metadata and feature-owned integer fields without presentation inputs.
 * @param schema Valid metadata transport snapshot.
 * @param records Actual owner views; duplicate identities/tags or invalid widths refuse.
 * @return Canonical diagnostic hash or invalid-input refusal.
 */
std::optional<uint64_t> ComputeMultiLayerStateDigest(const MultiLayerStateSchema &schema, std::span<const MultiLayerCanonicalRecord> records)
{
	if (ValidateMultiLayerStateReferences(schema) != MultiLayerStateError::None) return std::nullopt;
	if (records.size() > 65536) return std::nullopt;
	uint64_t digest = 14695981039346656037ULL;
	HashCanonicalUnsigned(digest, schema.schema_version, 2);
	HashCanonicalUnsigned(digest, to_underlying(schema.profile.id), 1);
	HashCanonicalUnsigned(digest, schema.profile.version, 2);
	HashCanonicalUnsigned(digest, schema.profile.legacy, 1);
	HashCanonicalUnsigned(digest, schema.profile.editor_override, 1);
	// Labels already expose their four canonical bytes; do not reinterpret them through host integer storage.
	for (const auto byte : schema.required_content.grfid) HashCanonicalUnsigned(digest, byte, 1);
	for (const auto byte : schema.required_content.md5sum) HashCanonicalUnsigned(digest, byte, 1);
	HashCanonicalUnsigned(digest, schema.content_version, 4);
	HashCanonicalUnsigned(digest, schema.feature_flags, 4);
	HashCanonicalUnsigned(digest, schema.geology_seed, 8);
	HashCanonicalUnsigned(digest, schema.orbital_seed, 8);
	HashCanonicalUnsigned(digest, records.size(), 4);
	std::vector<MultiLayerCanonicalRecord> ordered(records.begin(), records.end());
	std::ranges::sort(ordered, CanonicalRecordLess{});
	std::set<std::pair<MultiLayerCanonicalKind, uint32_t>> identities;
	// Encode every actual authoritative record once; owner/pool order never changes the digest.
	for (const auto &record : ordered) {
		if (to_underlying(record.kind) < 1) return std::nullopt;
		if (record.kind >= MultiLayerCanonicalKind::End) return std::nullopt;
		if (record.id == UINT32_MAX) return std::nullopt;
		if (!identities.emplace(record.kind, record.id).second) return std::nullopt;
		if (record.fields.size() > 128) return std::nullopt;
		HashCanonicalUnsigned(digest, to_underlying(record.kind), 1);
		HashCanonicalUnsigned(digest, record.id, 4);
		HashCanonicalUnsigned(digest, record.fields.size(), 2);
		std::vector<MultiLayerCanonicalField> fields(record.fields.begin(), record.fields.end());
		std::ranges::sort(fields, {}, &MultiLayerCanonicalField::tag);
		std::set<uint16_t> tags;
		// Explicit widths and unique tags prevent truncation or ambiguous absent/empty field encodings.
		for (const auto &field : fields) {
			if (!tags.insert(field.tag).second) return std::nullopt;
			// Only fixed integer widths have defined portable wire semantics.
			if (field.width != 1 && field.width != 2 && field.width != 4 && field.width != 8) return std::nullopt;
			// Width eight has no shift bound; narrower fields must fit before any diagnostic encoding.
			if (field.width < 8 && field.value >= (uint64_t{1} << (field.width * 8))) return std::nullopt;
			HashCanonicalUnsigned(digest, field.tag, 2);
			HashCanonicalUnsigned(digest, field.width, 1);
			HashCanonicalUnsigned(digest, field.value, field.width);
		}
	}
	return digest;
}

/**
 * Add profile/schema/seed/policy fields to the existing desync log without simulation mutation.
 * @param schema Actual validated metadata snapshot.
 */
void LogMultiLayerStateDiagnostics(const MultiLayerStateSchema &schema)
{
	const std::array geology_fields = {MultiLayerCanonicalField{1, 8, ComputeGeologyStateDigest(GetGeologyWorldState())}};
	const std::array portal_fields = {MultiLayerCanonicalField{1, 8, ComputeLayerPortalWorkDigest()}};
	std::vector<MultiLayerCanonicalRecord> records;
	if (GetGeologyWorldState().generator_version != 0) {
		// Only published geological authority participates; the compact owner digest includes every saved field without per-tile view allocations.
		records.push_back({MultiLayerCanonicalKind::GeologyRoot, 0, geology_fields});
	}
	if ((schema.feature_flags & MULTILAYER_FEATURE_PORTAL_WORK) != 0) records.push_back({MultiLayerCanonicalKind::PortalWorkRoot, 0, portal_fields});
	const std::array project_fields = {MultiLayerCanonicalField{1, 8, ComputeTBMProjectDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_TBM_PROJECTS) != 0) records.push_back({MultiLayerCanonicalKind::TBMProject, 0, project_fields});
	const std::array receiver_fields = {MultiLayerCanonicalField{1, 8, ComputeSpoilReceiverDigest()}};
	// Receiver side state uses the reserved industry-inventory kind; the stock itself is digested with it.
	if ((schema.feature_flags & MULTILAYER_FEATURE_SPOIL_RECEIVERS) != 0) records.push_back({MultiLayerCanonicalKind::IndustryInventory, 0, receiver_fields});
	const std::array exchange_fields = {MultiLayerCanonicalField{1, 8, ComputeTBMExchangeDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_TBM_EXCHANGE) != 0) records.push_back({MultiLayerCanonicalKind::Exchange, 0, exchange_fields});
	const std::array mine_fields = {MultiLayerCanonicalField{1, 8, ComputeLayerMineDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_MINES) != 0) records.push_back({MultiLayerCanonicalKind::MineRoot, 0, mine_fields});
	const std::array processor_fields = {MultiLayerCanonicalField{1, 8, ComputeLayerProcessorDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_PROCESSORS) != 0) records.push_back({MultiLayerCanonicalKind::ProcessorRoot, 0, processor_fields});
	const std::array utility_fields = {MultiLayerCanonicalField{1, 8, ComputeUtilityPowerDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_UTILITY_POWER) != 0) records.push_back({MultiLayerCanonicalKind::UtilityRoot, 0, utility_fields});
	const std::array tube_power_fields = {MultiLayerCanonicalField{1, 8, ComputeTubePowerDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_TUBE_POWER) != 0) records.push_back({MultiLayerCanonicalKind::TubePowerRoot, 0, tube_power_fields});
	const std::array tube_vehicle_fields = {MultiLayerCanonicalField{1, 8, ComputeTubePowerVehicleDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_TUBE_VEHICLES) != 0) records.push_back({MultiLayerCanonicalKind::TubeVehicleRoot, 0, tube_vehicle_fields});
	const std::array geothermal_site_fields = {MultiLayerCanonicalField{1, 8, ComputeGeothermalSiteDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_GEOTHERMAL_SITES) != 0) records.push_back({MultiLayerCanonicalKind::GeothermalSiteRoot, 0, geothermal_site_fields});
	const std::array geothermal_collector_fields = {MultiLayerCanonicalField{1, 8, ComputeGeothermalCollectorDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_GEOTHERMAL_COLLECTORS) != 0) records.push_back({MultiLayerCanonicalKind::GeothermalCollectorRoot, 0, geothermal_collector_fields});
	const std::array geothermal_route_fields = {MultiLayerCanonicalField{1, 8, ComputeGeothermalHeatRouteDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_GEOTHERMAL_HEAT_ROUTES) != 0) records.push_back({MultiLayerCanonicalKind::GeothermalHeatRouteRoot, 0, geothermal_route_fields});
	const std::array geothermal_riser_fields = {MultiLayerCanonicalField{1, 8, ComputeGeothermalRiserDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_GEOTHERMAL_RISERS) != 0) records.push_back({MultiLayerCanonicalKind::GeothermalRiserRoot, 0, geothermal_riser_fields});
	const std::array asteroid_fields = {MultiLayerCanonicalField{1, 8, ComputeAsteroidFieldDigest(GetAsteroidField())}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_ASTEROIDS) != 0) records.push_back({MultiLayerCanonicalKind::AsteroidFieldRoot, 0, asteroid_fields});
	const std::array space_vehicle_fields = {MultiLayerCanonicalField{1, 8, ComputeSpaceVehicleDigest()}};
	if (GetSpaceVehicleCount() != 0 || GetNextSpaceVehicleGeneration() != 1) records.push_back({MultiLayerCanonicalKind::SpaceVehicleRoot, 0, space_vehicle_fields});
	const std::array space_pod_fields = {MultiLayerCanonicalField{1, 8, ComputeSpacePodDigest()}};
	if (SpacePod::GetNumItems() != 0 || GetNextSpacePodGeneration() != 1) records.push_back({MultiLayerCanonicalKind::SpacePodRoot, 0, space_pod_fields});
	const std::array launch_fields = {MultiLayerCanonicalField{1, 8, ComputeLaunchCenterDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_LAUNCH_CENTERS) != 0) records.push_back({MultiLayerCanonicalKind::LaunchCenterRoot, 0, launch_fields});
	const std::array hub_fields = {MultiLayerCanonicalField{1, 8, ComputeLaunchHubDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_LAUNCH_HUBS) != 0) records.push_back({MultiLayerCanonicalKind::LaunchHubRoot, 0, hub_fields});
	const std::array handling_fields = {MultiLayerCanonicalField{1, 8, ComputeSurfaceHandlingDigest()}};
	if ((schema.feature_flags & MULTILAYER_FEATURE_SURFACE_HANDLING) != 0) records.push_back({MultiLayerCanonicalKind::SurfaceHandlingRoot, 0, handling_fields});
	const auto digest = ComputeMultiLayerStateDigest(schema, records);
	if (!digest) return;
	Debug(Facility::Desync, Severity::Info, "MLVS_STATE schema={} profile={} version={} legacy={} editor={} geology_seed={} orbital_seed={} digest={:016X}", schema.schema_version, to_underlying(schema.profile.id), schema.profile.version, schema.profile.legacy, schema.profile.editor_override, schema.geology_seed, schema.orbital_seed, *digest);
}
