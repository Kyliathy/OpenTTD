/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_cargo_native.cpp Actual CAPA source migration, preview isolation and native resave checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups21-28B.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 15; source validity is judged by an accepted format, not a pending version name.
 * Existing saved assets and freight are observed in place. Optional policy/departure modes deliberately transition one existing hull; no packet, hull, material, service or income is synthesized.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "asteroid_cargo_native.h"
#include "../asteroid.h"
#include "../asteroid_lifecycle.h"
#include "../cargopacket.h"
#include "../fios.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_mining_continuation.h"
#include "../space_departure.h"
#include "../vehicle_cmd.h"
#include "../company_base.h"
#include "../command_func.h"
#include "../space_structure.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../space_transition.h"
#include "../launch_operations.h"
#include "../orbital_recipe.h"
#include "../saveload/saveload_func.h"
#include "../saveload/cargo_source_sl.h"
#include "../core/random_func.hpp"
#include "../core/backup_type.hpp"
#include "../core/format.hpp"
#include "../timer/timer_game_tick.h"
#include <cstdlib>
#include <filesystem>
#include <tuple>
#include "../safeguards.h"

/** Actual native packet values; copying this record grants no inventory ownership. */
struct NativeCargoOriginObservation {
	CargoPacketID id; Source source; uint16_t quantity, age; TileIndex tile; StationID first; Money feeder;
};

/** @return Original packet identity, quantity and source/accounting values in native pool order; creates no packet or cargo. */
static std::vector<NativeCargoOriginObservation> ObserveCargoOrigins()
{
	std::vector<NativeCargoOriginObservation> result;
	// Every allocated actual packet participates, including ordinary station and
	// carrier freight; a Space-only subset could hide changed payment purpose.
	for (const auto *packet : CargoPacket::Iterate()) result.push_back({packet->index, packet->GetSource(), packet->Count(), packet->GetPeriodsInTransit(),
		packet->GetSourceTile(), packet->GetFirstStation(), packet->GetFeederShare()});
	return result;
}

/** @param before Original accepted packet observations. Require the exact native owners and all source/accounting values to remain unchanged. */
static void CheckOriginalCargoOrigins(std::span<const NativeCargoOriginObservation> before)
{
	REQUIRE(CargoPacket::GetNumItems() == before.size());
	// Saving and independent previews cannot issue replacements, merge original
	// packet identities or erase ordinary/natural financial and production history.
	for (const auto &old : before) {
		const auto *packet = CargoPacket::GetIfValid(old.id); REQUIRE(packet);
		CHECK(packet->GetSource() == old.source); CHECK(packet->Count() == old.quantity); CHECK(packet->GetPeriodsInTransit() == old.age);
		CHECK(packet->GetSourceTile() == old.tile); CHECK(packet->GetFirstStation() == old.first); CHECK(packet->GetFeederShare() == old.feeder);
	}
}

/** @param before Original accepted packet observations. Require independent preview rows to decode the same original quantities, provenance, production and virtual credit. */
static void CheckCopiedCargoOrigins(std::span<const NativeCargoOriginObservation> before)
{
	REQUIRE(_load_check_data.native_cargo_packet_rows.size() == before.size());
	// The native preview has independently admitted its candidate before these
	// comparisons; its actual validator cannot borrow the open world's packets.
	for (const auto &old : before) {
		const auto found = std::ranges::lower_bound(_load_check_data.native_cargo_packet_rows, old.id, {}, &NativeCargoPacketPreview::packet);
		REQUIRE(found != _load_check_data.native_cargo_packet_rows.end()); CHECK(found->packet == old.id);
		CHECK(found->source == old.source); CHECK(found->quantity == old.quantity); CHECK(found->feeder_share == old.feeder);
		CHECK(found->source_tile == old.tile); CHECK(found->first_station == old.first);
	}
}

/** @param field Original accepted field. Require candidate-only identity, issuance and disclosure metadata to match its original owner. */
static void CheckCopiedAsteroidField(const AsteroidField &field)
{
	CHECK(_load_check_data.asteroid_active == (field.generation_version != 0));
	CHECK(_load_check_data.asteroid_body_rows == GetAsteroidIdentityRecords(field));
	CHECK(_load_check_data.asteroid_next_body_id == field.next_body_id);
	CHECK(_load_check_data.asteroid_knowledge_rows == field.knowledge);
	if (IsSavegameVersionBefore(SaveLoadVersion::SpaceMiningContinuation)) return;
	// Current saved mission/queue decisions must independently decode exactly;
	// comparing only fleet counts could hide a lost pin or replacement target.
	for (const auto *craft : SpaceVehicle::Iterate()) {
		const auto native = std::ranges::find(_load_check_data.native_space_vehicle_rows, craft->index, &NativeSpaceVehiclePreview::vehicle); REQUIRE(native != _load_check_data.native_space_vehicle_rows.end());
		CHECK(native->mining_continuation == craft->mission.continuation);
		const auto queue = std::ranges::find(_load_check_data.space_order_preview_rows, craft->index, &SpaceOrderRecord::vehicle); REQUIRE(queue != _load_check_data.space_order_preview_rows.end());
		CHECK(queue->generation == craft->generation); CHECK(queue->queue == craft->space_orders);
	}
}

/** Exercise the actual registered player command against one restored current Mine, retaining original work, pose and stopped choice. */
static void CheckNativeMiningPolicyEdit()
{
	bool edited = false;
	// Select an existing actual original Mine; this observer cannot fabricate
	// a replacement hull or queue to satisfy command admission.
	for (auto *craft : SpaceVehicle::Iterate()) {
		const auto &q = craft->space_orders;
		if (q.cursor >= q.entries.size() || q.entries[q.cursor].action != SpaceOrderAction::Mine) continue;
		AutoRestoreBackup actor(_current_company, craft->owner);
		auto expected = q; expected.entries[q.cursor].mining_policy = SpaceMiningPolicy::ReturnNearest;
		const auto intent = craft->mission.continuation; const auto position = craft->position;
		const auto work = craft->mission.work_ticks; const auto stopped = craft->vehstatus.Test(VehState::Stopped);
		CHECK_FALSE(Command<Commands::SetSpaceMiningPolicy>::Post(craft->index, craft->generation + 1, q.cursor, SpaceMiningPolicy::ReturnNearest));
		REQUIRE(Command<Commands::SetSpaceMiningPolicy>::Post(craft->index, craft->generation, q.cursor, SpaceMiningPolicy::ReturnNearest));
		CHECK(craft->space_orders == expected); CHECK(craft->mission.continuation == intent); CHECK(craft->position == position);
		CHECK(craft->mission.work_ticks == work); CHECK(craft->vehstatus.Test(VehState::Stopped) == stopped);
		edited = true; break;
	}
	REQUIRE(edited);
	fmt::print("MINING_POLICY_EDIT stale_refused=true original_work_preserved=true\n");
}

/** Publish only the explicit legacy-departure component on a genuine working unordered miner.
 * @param stopped Whether the actual native start/stop command should leave this hull stopped before publication.
 * This test-only transition is not evidence of normal-loop unsafe-step activation or a terminal body event.
 */
static void CheckNativeLegacyDeparture(bool stopped)
{
	bool published = false;
	// Select the actual same-file leased legacy miner; never manufacture a Mine,
	// hull, attachment pose or completed work to satisfy this transport gate.
	for (auto *craft : SpaceVehicle::Iterate()) {
		if (!craft->space_orders.entries.empty() || craft->mission.phase != SpaceMissionPhase::Mining) continue;
		AutoRestoreBackup actor(_current_company, craft->owner);
		const auto plan = PlanSpaceDeparture(*craft); REQUIRE(plan.status == SpaceDepartureStatus::Ready);
		if (craft->vehstatus.Test(VehState::Stopped) != stopped) REQUIRE(CmdStartStopVehicle(DoCommandFlag::Execute, craft->index, true).Succeeded());
		const auto position = craft->position; const auto home = craft->home; const auto money = Company::Get(craft->owner)->money;
		const auto cost = craft->mission.cost_remainder; const auto extracted = craft->extracted_units; const auto target = craft->mission.target;
		REQUIRE(BeginSpaceMiningContinuation(*craft, plan.destination));
		CHECK(craft->position == position); CHECK(craft->home == home); CHECK(craft->vehstatus.Test(VehState::Stopped) == stopped);
		CHECK(Company::Get(craft->owner)->money == money); CHECK(craft->mission.cost_remainder == cost); CHECK(craft->extracted_units == extracted);
		CHECK(craft->space_orders == SpaceOrderQueue{}); CHECK(craft->mission.work_ticks == 0);
		CHECK(GetMutableAsteroidBody(target)->mining_slot == VehicleID::Invalid()); CHECK(ValidateNativeSpaceOrders());
		published = true; break;
	}
	REQUIRE(published);
	fmt::print("LEGACY_DEPARTURE stopped={} original_pose_home_cargo_costs_preserved=true native_motion_enabled=false\n", stopped);
}

/** Release an actual saved ownerless attachment as a declared pre-erasure transport component.
 * The original body remains live; this verifies no-target Return transport without claiming a normal-loop exit or earned scan.
 */
static void CheckNativeSurveyDeparture()
{
	auto field = GetAsteroidField();
	/** @param survey Actual saved ownerless controller. @return Whether it owns an actual incomplete attached observation. */
	const auto preview = std::ranges::find_if(field.surveys, [](const auto &survey) { return survey.phase == SpaceSurveyPhase::Attached; });
	REQUIRE(preview != field.surveys.end()); const auto original = *preview; const auto bodies = field.bodies.size();
	const std::array removed{original.target}; ForgetSpaceSurveyBodyTargets(field, removed);
	CHECK(preview->position == original.position); CHECK(preview->home == original.home); CHECK(preview->completed_surveys == original.completed_surveys);
	CHECK(preview->target == INVALID_ORBITAL_BODY); CHECK(preview->phase == SpaceSurveyPhase::Return); CHECK(preview->remaining_ticks == 0);
	CHECK(field.bodies.size() == bodies); CHECK(field.renewal == GetAsteroidField().renewal);
	REQUIRE(InstallAsteroidField(std::move(field)));
	fmt::print("SURVEY_DEPARTURE no_target_return=true original_pose_home_count_preserved=true body_erased=false\n");
}

/** Exercise actual native atomic retirement from a declared final-contained body geometry.
 * This observer requires original natural packet history for body0 and no active lease. It changes no native pose/work/cargo, claims no elapsed geometry travel and leaves normal-loop activation to Group29.
 */
static void CheckNativeAsteroidRetirement()
{
	const auto origins = ObserveCargoOrigins();
	/** @param packet Actual original packet. @return Whether this checkpoint really carries typed natural history for the exact retiring body. */
	REQUIRE(std::ranges::any_of(origins, [](const auto &packet) { const auto origin = packet.source.GetAsteroidOrigin(); return origin && origin->body == 0; }));
	auto field = GetAsteroidField(); auto *body = FindAsteroidBody(field, OrbitalBodyID{0}); REQUIRE(body);
	REQUIRE(body->mining_slot == VehicleID::Invalid()); const auto reserve = body->remaining_reserve;
	const auto exit = ForecastAsteroidExit(*body, *GetSpacePlaneBounds(field.width, field.height)); REQUIRE(exit); REQUIRE(exit->ticks_until_exit);
	body->motion = exit->last_safe_motion; field.elapsed_ticks = std::max<uint64_t>(field.elapsed_ticks, 1);
	REQUIRE(InstallAsteroidField(std::move(field))); const std::array removed{OrbitalBodyID{0}};
	const auto issued = GetAsteroidField().next_body_id; const auto quantity = GetMutableAsteroidBody(removed.front())->remaining_reserve;
	CHECK(quantity == reserve); REQUIRE(RetireAsteroidBatch(removed).status == AsteroidLifecycleStatus::Retired);
	CHECK(GetAsteroidField().next_body_id == issued); CHECK_FALSE(GetMutableAsteroidBody(removed.front())); CHECK(ValidateNativeSpaceOrders());
	fmt::print("ASTEROID_RETIREMENT native_batch=true geometry_component=true original_history_retained=true normal_loop_enabled=false\n");
}

/** Exercise actual one-body publication from this original saved field's already earned positive credit.
 * This test-only due checkpoint claims no elapsed interval; original native packets/poses/work and global clocks remain unchanged.
 */
static void CheckNativeAsteroidCreation()
{
	auto field = GetAsteroidField(); REQUIRE(field.renewal.credits != 0); field.renewal.ticks_remaining = 0;
	REQUIRE(InstallAsteroidField(std::move(field))); const auto before = GetAsteroidField();
	const auto quote = PlanNativeAsteroidRenewal(); REQUIRE(quote.body); REQUIRE(quote.attempt);
	const auto expected = FinishAsteroidRenewalAttempt(before.renewal, *quote.attempt, true, before.next_body_id); REQUIRE(expected);
	const auto result = CommitAsteroidRenewal(quote); REQUIRE(result.status == AsteroidLifecycleStatus::Spawned); REQUIRE(result.spawned);
	CHECK(result.attempt_closed); CHECK(result.spawned->base() == before.next_body_id); CHECK(GetAsteroidField().renewal == *expected);
	CHECK(GetAsteroidField().knowledge == before.knowledge);
	// Original finite records retain their exact identities, quantities and
	// leases; only the actual newly admitted body receives a fresh record.
	for (const auto &body : before.bodies) CHECK(*GetMutableAsteroidBody(body.id) == body);
	REQUIRE(ValidateNativeSpaceOrders());
	fmt::print("ASTEROID_CREATION native_one_body=true declared_due_checkpoint=true original_owners_retained=true normal_loop_enabled=false\n");
}

/** @copydoc VerifyAsteroidCargoNative */
void VerifyAsteroidCargoNative(const char *source, const char *output)
{
	const auto before = ObserveCargoOrigins(); auto field = GetAsteroidField();
	auto fleet = ComputeSpaceVehicleDigest(), orders = ComputeSpaceOrdersDigest();
	const auto structures = ComputeSpaceStructureDigest(); const auto clock = TimerGameTick::counter;
	const auto random = std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	uint natural = 0;
	// Every loaded source already passed its genuine era and same-file migration.
	// Natural packets now have complete origins; ordinary/supply fields retain
	// their own original purpose, invalid sentinels and credit rules. They must be
	// valid for the combined writer that carries typed origins (m2o28 Group 15).
	const auto combined = CargoSourceFormat::FromLineage({SaveLineageStatus::Accepted, SaveDialect::Merged, CURRENT_MERGED_SAVE_VERSION});
	for (const auto &row : before) {
		CHECK(IsValidCargoPacketSource(row.source, row.feeder, combined));
		if (row.source.type == SourceType::Asteroid) { REQUIRE(row.source.GetAsteroidOrigin()); ++natural; }
	}
	REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	CheckCopiedCargoOrigins(before);
	CheckCopiedAsteroidField(field);
	if (const char *bad = std::getenv("OTTD_SPACE_FIXES_CARGO_BAD")) {
		if (std::getenv("OTTD_SPACE_FIXES_CARGO_FULL") != nullptr) {
			// This deliberately rejected destructive load belongs only to this
			// disposable native process. No original pointer is dereferenced after
			// teardown; the ordinary harness exits immediately on return.
			CHECK(SaveOrLoad(bad, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::ReInit);
			fmt::print("CARGO_ORIGIN full_rejected=true natural={}\n", natural); return;
		}
		CHECK(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
		REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		CheckCopiedCargoOrigins(before);
		CheckCopiedAsteroidField(field);
	} else {
		if (std::getenv("OTTD_SPACE_FIXES_ASTEROID_RETIREMENT") != nullptr) { CheckNativeAsteroidRetirement(); field = GetAsteroidField(); }
		if (std::getenv("OTTD_SPACE_FIXES_ASTEROID_CREATION") != nullptr) { CheckNativeAsteroidCreation(); field = GetAsteroidField(); }
		if (std::getenv("OTTD_SPACE_FIXES_SURVEY_DEPARTURE") != nullptr) { CheckNativeSurveyDeparture(); field = GetAsteroidField(); }
		if (const char *departure = std::getenv("OTTD_SPACE_FIXES_LEGACY_DEPARTURE")) {
			CheckNativeLegacyDeparture(std::string_view{departure} == "stopped");
			field = GetAsteroidField(); fleet = ComputeSpaceVehicleDigest(); orders = ComputeSpaceOrdersDigest();
		}
		if (std::getenv("OTTD_SPACE_FIXES_MINING_POLICY") != nullptr) {
			CheckNativeMiningPolicyEdit(); fleet = ComputeSpaceVehicleDigest(); orders = ComputeSpaceOrdersDigest();
		}
		const auto saved = (std::filesystem::path{output} / "cargo-origins-current.sav").string();
		REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Save, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		REQUIRE(SaveOrLoad(saved, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		CheckCopiedCargoOrigins(before);
		CheckCopiedAsteroidField(field);
	}
	CheckOriginalCargoOrigins(before); CHECK(GetAsteroidField() == field); CHECK(TimerGameTick::counter == clock);
	CHECK(ComputeSpaceVehicleDigest() == fleet); CHECK(ComputeSpaceStructureDigest() == structures);
	CHECK(ComputeSpaceOrdersDigest() == orders);
	CHECK(std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == random);
	fmt::print("CARGO_ORIGIN packets={} natural={} preview=true unchanged=true resave={}\n", before.size(), natural, std::getenv("OTTD_SPACE_FIXES_CARGO_BAD") == nullptr);
}

/** Detached current candidate decisions used only to compare independent previews; owns no native resource, hull, packet or company. */
struct SpaceTransportCandidateSnapshot {
	std::vector<uint64_t> scalars;
	std::vector<uint> companies;
	std::vector<AsteroidIdentityRecord> bodies;
	std::vector<std::pair<OrbitalBodyID, VehicleID>> leases;
	std::vector<AsteroidKnowledgeRecord> knowledge;
	std::vector<std::pair<SpaceCraftRef, SpaceOrderQueue>> queues;
	std::vector<std::tuple<VehicleID, VehicleType, Owner, SpacePoint, std::vector<uint32_t>, uint32_t, uint32_t, uint16_t, uint8_t,
		bool, CargoType, uint16_t, SpaceMiningContinuationState>> hulls;
	std::vector<std::tuple<CargoPacketID, uint16_t, Source, Money, TileIndex, StationID, bool>> packets;
	std::vector<std::tuple<CargoType, CargoLabel, uint8_t>> meanings;
	std::vector<std::tuple<SpaceCraftRef, uint16_t, uint16_t, uint8_t, LaunchCenterRef>> roles;
	/** @param other Independently admitted snapshot of the same candidate. @return Whether all captured decisions and identity/meaning evidence agree exactly. */
	bool operator==(const SpaceTransportCandidateSnapshot &other) const = default;
};

/** @return Complete relevant same-file field/source/mission decisions from the most recent accepted preview; reads no native owner. */
static SpaceTransportCandidateSnapshot ObserveSpaceTransportCandidate()
{
	const auto &data = _load_check_data; SpaceTransportCandidateSnapshot result;
	result.scalars = {data.map_size_x, data.map_size_y, data.map_layers, data.gameplay_profile_id, data.multilayer_feature_flags, data.native_tick_counter,
		data.asteroid_seed, data.asteroid_next_body_id, data.next_launch_center_generation, data.next_space_structure_generation, data.next_space_vehicle_generation,
		data.asteroid_present, data.asteroid_active, data.asteroid_knowledge_present, data.space_orders_present, data.native_cargo_packets_present,
		data.native_cargo_dimensions_present, data.space_manifests_present, data.space_production_present, data.space_structures_present, data.launch_centers_present};
	result.bodies = data.asteroid_body_rows; result.leases = data.asteroid_mining_leases; result.knowledge = data.asteroid_knowledge_rows;
	// Copy only candidate evidence, including absent/empty families. A second
	// preview must rebuild these values instead of inheriting the open world or
	// the prior candidate's companies, cargo, role, effective target or lease.
	for (const auto &[id, company] : data.companies) { (void)company; result.companies.push_back(id); }
	for (const auto &row : data.space_order_preview_rows) result.queues.emplace_back(SpaceCraftRef{row.vehicle, row.generation}, row.queue);
	for (const auto &row : data.native_space_vehicle_rows) result.hulls.emplace_back(row.vehicle, row.type, row.owner, row.position, row.cargo_packet_refs,
		row.mining_target, row.extracted_units, row.mining_work_ticks, row.mining_phase, row.cargo_complete, row.freight_cargo, row.freight_capacity, row.mining_continuation);
	for (const auto &row : data.native_cargo_packet_rows) result.packets.emplace_back(row.packet, row.quantity, row.source, row.feeder_share, row.source_tile, row.first_station, row.legacy_asteroid_origin);
	for (const auto &row : data.native_cargo_dimension_rows) result.meanings.emplace_back(row.cargo, row.label, row.weight);
	for (const auto &row : data.space_production_rows) result.roles.emplace_back(row.craft, row.state.content_version, row.state.model, to_underlying(row.state.placement), row.state.home);
	return result;
}

/** @return Native original family/accounting digests; no copied candidate can replace these owners during preview. */
static std::array<uint64_t, 8> ObserveNativeSpaceFamilies()
{
	return {ComputeSpaceVehicleDigest(), ComputeSpaceOrdersDigest(), ComputeSpaceStructureDigest(), ComputeSpaceManifestDigest(),
		ComputeSpacePodDigest(), ComputeSpaceTransitionDigest(), ComputeLaunchProductionDigest(), ComputeOrbitalRecipeDigest()};
}

/** @copydoc VerifySpaceTransportIsolation */
void VerifySpaceTransportIsolation(const char *source)
{
	const char *candidate = std::getenv("OTTD_SPACE_FIXES_PREVIEW_CANDIDATE"); REQUIRE(candidate);
	const auto packets = ObserveCargoOrigins(); const auto field = GetAsteroidField(); const auto families = ObserveNativeSpaceFamilies();
	const auto clock = TimerGameTick::counter; const auto random = std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]};
	const auto pools = std::array{Vehicle::GetNumItems(), CargoPacket::GetNumItems(), Company::GetNumItems(), SpaceStructure::GetNumItems(), SpacePod::GetNumItems()};
	REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	const auto original = ObserveSpaceTransportCandidate();
	REQUIRE(SaveOrLoad(candidate, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	const auto expected = ObserveSpaceTransportCandidate(); REQUIRE(expected != original);
	if (const char *bad = std::getenv("OTTD_SPACE_FIXES_CARGO_BAD")) {
		REQUIRE(SaveOrLoad(bad, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
	}
	REQUIRE(SaveOrLoad(candidate, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	CHECK(ObserveSpaceTransportCandidate() == expected);
	REQUIRE(SaveOrLoad(source, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
	CHECK(ObserveSpaceTransportCandidate() == original);
	CheckOriginalCargoOrigins(packets); CHECK(GetAsteroidField() == field); CHECK(ObserveNativeSpaceFamilies() == families);
	CHECK(TimerGameTick::counter == clock); CHECK(std::array{_random.state[0], _random.state[1], _interactive_random.state[0], _interactive_random.state[1]} == random);
	CHECK(std::array{Vehicle::GetNumItems(), CargoPacket::GetNumItems(), Company::GetNumItems(), SpaceStructure::GetNumItems(), SpacePod::GetNumItems()} == pools);
	fmt::print("TRANSPORT_PREVIEW distinct_world=true candidate_bodies={} candidate_packets={} unchanged=true\n", expected.bodies.size(), expected.packets.size());
}
