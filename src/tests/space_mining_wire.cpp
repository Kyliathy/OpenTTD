/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_mining_wire.cpp Contained mission scalar and same-file queue/lease checkpoints.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups23/25.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 18; the continuation schema follows the source's capability.
 * These are declared transport components, not newly paid hulls, earned cargo or executed departure evidence.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../saveload/space_mining_sl.h"
#include "../space_mining_target.h"
#include "../fios.h"
#include "../safeguards.h"

/** @param data Empty isolated candidate to fill with one configured miner, original instructions and independently declared identity/meaning rows. */
static void SetupMiningWireCandidate(LoadCheckData &data)
{
	data.map_size_x = data.map_size_y = 64;
	data.asteroid_next_body_id = 8; data.asteroid_body_rows = {{OrbitalBodyID{0}, AsteroidClass::Metallic}, {OrbitalBodyID{5}, AsteroidClass::Metallic}, {OrbitalBodyID{7}, AsteroidClass::Carbonaceous}};
	data.next_space_vehicle_generation = data.next_space_structure_generation = data.next_launch_center_generation = 10;
	SpaceOrderQueue queue; queue.started = true;
	queue.entries = {{SpaceOrderAction::Mine, {SpaceOrderTargetKind::Body, 0, 0}, SpaceMiningPolicy::FullLoadNearest},
		{SpaceOrderAction::Visit, {SpaceOrderTargetKind::Structure, 4, 3}}, {SpaceOrderAction::Unload, {SpaceOrderTargetKind::Structure, 4, 3}}};
	data.space_order_preview_rows.push_back({VehicleID{2}, 1, queue});
	NativeSpaceVehiclePreview native; native.vehicle = VehicleID{2}; native.type = VehicleType::Space; native.owner = Owner{0};
	native.mining_phase = to_underlying(SpaceMissionPhase::Approach); native.mining_target = 0; native.freight_cargo = CargoType{0};
	native.mining_continuation.mineral = CargoLabel{"NORE"}; native.mining_continuation.previous_body = OrbitalBodyID{0};
	data.native_space_vehicle_rows.push_back(native);
	SpaceProductionRecord product; product.craft = {VehicleID{2}, 1}; product.state.content_version = 1; product.state.model = SPACE_PROBE_MODEL;
	data.space_production_rows.push_back(product);
	NativeCargoDimensionPreview ore; ore.cargo = CargoType{0}; ore.label = CargoLabel{"NORE"}; ore.weight = 16;
	NativeCargoDimensionPreview ice; ice.cargo = CargoType{1}; ice.label = CargoLabel{"WICE"}; ice.weight = 16;
	data.native_cargo_dimension_rows = {ore, ice};
}

/** @param data Copied original candidate. Round-trip its complete contained intent and require the same independent semantic admission. */
static void CheckMiningWireRoundtrip(LoadCheckData &data)
{
	const auto before = data.native_space_vehicle_rows.front().mining_continuation;
	const auto decoded = DecodeSpaceMiningContinuation(EncodeSpaceMiningContinuation(before)); REQUIRE(decoded);
	CHECK(*decoded == before); data.native_space_vehicle_rows.front().mining_continuation = *decoded;
	CHECK(ValidateSpaceMiningContinuationPreview(data));
}

/** Every retained phase carries the same original instruction and bounded decision fields across explicit scalar adaptation. */
TEST_CASE("Unqueued local mining intent is narrowly admitted only by its source era", "[space-mining-wire][space-legacy-departure]")
{
	LoadCheckData data; SetupMiningWireCandidate(data); auto &native = data.native_space_vehicle_rows.front();
	auto &intent = native.mining_continuation; auto &queue = data.space_order_preview_rows.front().queue;
	queue = {}; native.mining_phase = to_underlying(SpaceMissionPhase::Holding);
	intent.phase = MiningContinuationPhase::Detaching; intent.departure_point = SpacePoint{5 * SPACE_Q16, 6 * SPACE_Q16};
	CheckMiningWireRoundtrip(data); CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data, false));
	SECTION("old default is inert") { intent = {}; CHECK(ValidateSpaceMiningContinuationPreview(data, false)); }
	SECTION("retired source is historical") { data.asteroid_body_rows.erase(data.asteroid_body_rows.begin()); CheckMiningWireRoundtrip(data); }
	SECTION("bounded local obstruction wait") {
		intent.departure_point.reset(); intent.waiting = MiningContinuationWait::DepartureBlocked; intent.retry_ticks = GetSpaceMiningTargetRetryDelay();
		CheckMiningWireRoundtrip(data);
	}
	SECTION("automatic seek forbidden") { intent.phase = MiningContinuationPhase::SeekMineral; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("automatic receiver forbidden") { intent.effective_target = SpaceOrderTarget{SpaceOrderTargetKind::Structure, 4, 3}; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("hidden route forbidden") { intent.route_cursor = 0; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("queue block forbidden") { queue.blocked = SpaceOrderBlock::ProducerUnavailable; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("partial work forbidden") { native.mining_work_ticks = 1; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("unrelated historical target forbidden") { native.mining_target = 5; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("wrong surviving mineral forbidden") { intent.mineral = CargoLabel{"WICE"}; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("hidden lease forbidden") { data.asteroid_mining_leases = {{OrbitalBodyID{0}, native.vehicle}}; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
}

/** Every configured phase remains admitted under its original443 scalar contract. */
TEST_CASE("Mining continuation transport preserves every contained phase", "[space-mining-wire]")
{
	const auto field = GetAsteroidField();
	// Each phase is a separate declared checkpoint. No selector, motion, cargo
	// producer or live mission is invoked to make these transport inputs valid.
	for (const auto phase : {MiningContinuationPhase::None, MiningContinuationPhase::Detaching, MiningContinuationPhase::SeekMineral,
		MiningContinuationPhase::ReturnCargo, MiningContinuationPhase::NeedService, MiningContinuationPhase::Waiting}) {
		LoadCheckData data; SetupMiningWireCandidate(data); auto &native = data.native_space_vehicle_rows.front(); auto &intent = native.mining_continuation;
		intent.phase = phase;
		if (phase != MiningContinuationPhase::None) {
			data.space_order_preview_rows.front().queue.started = false; native.mining_phase = to_underlying(SpaceMissionPhase::Holding); intent.route_cursor = 1;
		}
		switch (phase) {
			case MiningContinuationPhase::Detaching: intent.departure_point = SpacePoint{6 * SPACE_Q16, 7 * SPACE_Q16}; break;
			case MiningContinuationPhase::SeekMineral:
				data.space_order_preview_rows.front().queue.started = true; native.mining_phase = to_underlying(SpaceMissionPhase::Mining); native.mining_target = 5;
				intent.effective_target = SpaceOrderTarget{SpaceOrderTargetKind::Body, 5, 0}; data.asteroid_mining_leases = {{OrbitalBodyID{5}, VehicleID{2}}}; break;
			case MiningContinuationPhase::ReturnCargo:
			case MiningContinuationPhase::NeedService:
				intent.effective_target = SpaceOrderTarget{SpaceOrderTargetKind::Structure, 4, 3}; intent.travel_ticks = 400; break;
			case MiningContinuationPhase::Waiting: intent.waiting = MiningContinuationWait::NoCompatibleAsteroid; intent.retry_ticks = GetSpaceMiningTargetRetryDelay(); break;
			default: break;
		}
		CheckMiningWireRoundtrip(data);
	}
	CHECK(GetAsteroidField() == field);
}

/** Canonical wire presence and bounded scalars cannot silently acquire hidden targets, progress or future enum behavior. */
TEST_CASE("Mining continuation scalar decoding refuses malformed option and progress fields", "[space-mining-wire]")
{
	const auto original = EncodeSpaceMiningContinuation(SpaceMiningContinuationState{});
	CHECK(DecodeSpaceMiningContinuation(original) == SpaceMiningContinuationState{});
	SECTION("phase") { auto bad = original; bad.phase = to_underlying(MiningContinuationPhase::End); CHECK_FALSE(DecodeSpaceMiningContinuation(bad)); }
	SECTION("wait") { auto bad = original; bad.waiting = to_underlying(MiningContinuationWait::End); CHECK_FALSE(DecodeSpaceMiningContinuation(bad)); }
	SECTION("presence") { auto bad = original; bad.effective_present = 2; CHECK_FALSE(DecodeSpaceMiningContinuation(bad)); }
	SECTION("hidden target") { auto bad = original; bad.effective_id = 0; CHECK_FALSE(DecodeSpaceMiningContinuation(bad)); }
	SECTION("hidden generation") { auto bad = original; bad.effective_generation = 1; CHECK_FALSE(DecodeSpaceMiningContinuation(bad)); }
	SECTION("body generation") { auto bad = original; bad.effective_present = 1; bad.effective_id = 0; bad.effective_generation = 1; CHECK_FALSE(DecodeSpaceMiningContinuation(bad)); }
	SECTION("receiver generation") { auto bad = original; bad.effective_present = 1; bad.effective_kind = to_underlying(SpaceOrderTargetKind::Structure); bad.effective_id = 0; CHECK_FALSE(DecodeSpaceMiningContinuation(bad)); }
	SECTION("wrong domain") { auto bad = original; bad.effective_present = 1; bad.effective_kind = to_underlying(SpaceOrderTargetKind::Craft); bad.effective_id = 0; bad.effective_generation = 1; CHECK_FALSE(DecodeSpaceMiningContinuation(bad)); }
	SECTION("departure presence") { auto bad = original; bad.departure_present = 2; CHECK_FALSE(DecodeSpaceMiningContinuation(bad)); }
	SECTION("hidden departure") { auto bad = original; bad.departure_x = 1; CHECK_FALSE(DecodeSpaceMiningContinuation(bad)); }
	SECTION("retry") { auto bad = original; bad.retry_ticks = GetSpaceMiningTargetRetryDelay() + 1; CHECK_FALSE(DecodeSpaceMiningContinuation(bad)); }
	SECTION("route") { auto bad = original; bad.route_cursor = MAX_SPACE_ORDERS + 1; CHECK_FALSE(DecodeSpaceMiningContinuation(bad)); }
	SECTION("travel") { auto bad = original; bad.travel_ticks = (1U << 24) + 1; CHECK_FALSE(DecodeSpaceMiningContinuation(bad)); }
}

/** Missing bodies remain history only while detached; active replacement extraction requires the actual matching same-file lease and immutable mineral. */
TEST_CASE("Mining continuation candidate joins distinguish history from active resources", "[space-mining-wire]")
{
	LoadCheckData data; SetupMiningWireCandidate(data); auto &native = data.native_space_vehicle_rows.front(); auto &intent = native.mining_continuation;
	auto &queue = data.space_order_preview_rows.front().queue;
	SECTION("wrong mineral even empty hold") { intent.mineral = CargoLabel{"WICE"}; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("unconfigured definition") { data.native_cargo_dimension_rows.clear(); CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("wrong held mineral") { native.cargo_packet_refs.push_back(1); native.freight_cargo = CargoType{1}; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("future history") { intent.previous_body = OrbitalBodyID{8}; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("future original order") { queue.entries.front().target.id = 8; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("non-miner") { data.space_production_rows.front().state.model = 19; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("orphan mining") { native.mining_phase = to_underlying(SpaceMissionPhase::Mining); CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("reverse orphan lease") { data.asteroid_mining_leases = {{OrbitalBodyID{5}, VehicleID{2}}}; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data)); }
	SECTION("retired original target with real replacement") {
		data.asteroid_body_rows.erase(data.asteroid_body_rows.begin()); intent.phase = MiningContinuationPhase::SeekMineral;
		intent.route_cursor = 1; intent.effective_target = SpaceOrderTarget{SpaceOrderTargetKind::Body, 5, 0}; native.mining_target = 5;
		native.mining_phase = to_underlying(SpaceMissionPhase::Mining); data.asteroid_mining_leases = {{OrbitalBodyID{5}, VehicleID{2}}}; CheckMiningWireRoundtrip(data);
	}
	SECTION("retired original target with safe wait") {
		data.asteroid_body_rows.erase(data.asteroid_body_rows.begin()); queue.started = false; native.mining_phase = to_underlying(SpaceMissionPhase::Holding);
		intent.phase = MiningContinuationPhase::Waiting; intent.route_cursor = 1; intent.waiting = MiningContinuationWait::NoCompatibleAsteroid; CheckMiningWireRoundtrip(data);
	}
	SECTION("receiver generation history is inert but future issuance refuses") {
		queue.started = false; native.mining_phase = to_underlying(SpaceMissionPhase::Holding); intent.phase = MiningContinuationPhase::ReturnCargo; intent.route_cursor = 1;
		intent.effective_target = SpaceOrderTarget{SpaceOrderTargetKind::Structure, 4, 2}; CheckMiningWireRoundtrip(data);
		intent.effective_target->generation = data.next_space_structure_generation; CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data));
	}
	SECTION("queue edit clears its sole intent") {
		queue = {}; native.mining_phase = to_underlying(SpaceMissionPhase::Holding); CHECK_FALSE(ValidateSpaceMiningContinuationPreview(data));
		intent = {}; CHECK(ValidateSpaceMiningContinuationPreview(data));
	}
}

/** Merged 446 tables require all fourteen fields once; merged 441 and older tables keep inert absence and reject skipped keys. */
TEST_CASE("Mining continuation schema distinguishes genuine legacy absence", "[space-mining-wire]")
{
	std::vector<std::string> names;
	for (const auto key : {"phase", "mineral", "previous_body", "effective_present", "effective_kind", "effective_id", "effective_generation",
		"departure_present", "departure_x", "departure_y", "retry_ticks", "waiting", "route_cursor", "travel_ticks"}) names.push_back(std::string{"mission.continuation."} + key);
	std::vector<std::string_view> keys;
	for (const auto &name : names) keys.push_back(name);
	// Supported means the source carries SaveCapability::SpaceMiningContinuation (merged 446);
	// merged 441 and every older source must have none of the keys (m2o28 Group 18).
	CHECK(ValidateSpaceMiningContinuationSchema(keys, true));
	CHECK_FALSE(ValidateSpaceMiningContinuationSchema(keys, false));
	CHECK(ValidateSpaceMiningContinuationSchema({}, false));
	CHECK_FALSE(ValidateSpaceMiningContinuationSchema({}, true));
	const std::string unrelated{"mission.continuation.unknown"};
	auto foreign = std::vector<std::string_view>{unrelated};
	CHECK_FALSE(ValidateSpaceMiningContinuationSchema(foreign, false)); // A skipped future key in an older source.
	keys.pop_back(); CHECK_FALSE(ValidateSpaceMiningContinuationSchema(keys, true));
	keys.push_back(names.back()); keys.push_back(names.front()); CHECK_FALSE(ValidateSpaceMiningContinuationSchema(keys, true));
	LoadCheckData data; SetupMiningWireCandidate(data); data.Clear(); CHECK(data.asteroid_mining_leases.empty()); CHECK(data.next_launch_center_generation == 1);
}
