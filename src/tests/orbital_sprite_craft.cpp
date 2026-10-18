/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file orbital_sprite_craft.cpp Native read-only orbital source selection and owner isolation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K
 * Declared hulls, packets, leases and countdowns are component inputs only.
 * These fixtures claim no paid procurement, earned service or ordinary-game acceptance.
 */
#include "../stdafx.h"
#include "probe_mining_fixture.h"
#include "../orbital_sprite.h"
#include "../space_sprite.h"
#include "../space_manifest.h"
#include "../space_structure.h"
#include "../company_base.h"
#include <array>
#include <set>
#include "../safeguards.h"

/** Isolated actual native hull/container owners with explicitly declared presentation inputs. */
class OrbitalCraftArtRig : public ProbeExtractionRig {
	AutoRestoreBackup<uint8_t> steel_weight{CargoSpec::Get(GetCargoTypeByLabel(CargoLabel{"STEL"}))->weight, uint8_t{16}}; ///< Explicit one-tonne component dimension, matching the base rig's ore; no cargo capacity is granted directly.
public:
	/** Require a clean contained-service domain so no unit hull can see another test's assignment. */
	OrbitalCraftArtRig() { REQUIRE(SpaceStructure::GetNumItems() == 0); }

	/** Create one native component hull of an existing canonical role.
	 * @param model Original logical hull role; no commissioning or paid history is claimed.
	 * @return Actual generation-qualified hull with its own original queue and packet hold.
	 */
	SpaceVehicle &Craft(uint16_t model)
	{
		REQUIRE(EnsureSpaceEngineDefinition(model));
		auto &craft = this->Probe(4, false);
		craft.engine_type = GetSpaceEngine(model);
		craft.construction.placement = SpacePlacement::Orbital;
		craft.construction.content_version = 0; // Explicit legacy component boundary, never a paid-heavy save claim.
		craft.cargo_type = GetCargoTypeByLabel(model == 16 || model == 17 ? CargoLabel{"STEL"} : CargoLabel{"NORE"});
		craft.cargo_cap = GetSpaceFreightCapacity(model, craft.cargo_type);
		return craft;
	}

	/** Give one original hull an explicit valid typed destination without moving it.
	 * @param craft Native hull whose original queue receives the component input.
	 * @param started Whether the original instruction is already started.
	 */
	void Visit(SpaceVehicle &craft, bool started = true)
	{
		const SpaceOrder order{SpaceOrderAction::Visit, {SpaceOrderTargetKind::Body, 0, 0}};
		REQUIRE(IsSpaceOrderAllowed(GetSpaceVehicleModel(craft), order));
		craft.space_orders.entries = {order};
		craft.space_orders.cursor = 0; craft.space_orders.started = started;
		craft.space_orders.blocked = SpaceOrderBlock::None;
		SpacePoint destination;
		REQUIRE(ResolveSpaceNavigationTarget(craft, order.target, destination) == SpaceOrderBlock::None);
		REQUIRE(destination != craft.position);
	}

	/** Declare original packet input in the hull's current cargo label; drawing must conserve it.
	 * @param craft Actual sole physical packet holder.
	 * @param quantity Positive whole units, or zero to retain the real empty hold.
	 */
	void Load(SpaceVehicle &craft, uint16_t quantity)
	{
		if (quantity == 0) return;
		REQUIRE(CargoPacket::CanAllocateItem());
		craft.cargo.Append(CargoPacket::Create(quantity, uint16_t{0}, StationID::Invalid(), craft.tile, Money{0}));
	}
};

/** Resolve a required actual component pose without permitting absent-source fallback.
 * @param craft Original native hull used by this focused source fixture.
 * @param direction Exact renderer-requested genuine source view.
 * @return Its declared native pose after asserting successful source selection.
 */
static OrbitalArtPose RequiredOrbitalPose(const SpaceVehicle &craft, Direction direction = Direction::N)
{
	const auto pose = GetOrbitalCraftArtPose(craft, direction);
	REQUIRE(pose);
	return *pose;
}

/** All976logical source slots preserve published order; undeclared indices cannot cross native families. */
TEST_CASE("Orbital source resolver retains every native part and refuses invalid indices", "[orbital-art-craft]")
{
	std::set<SpriteID> seen;
	SpriteID expected = SPR_MAP_LAYER_BASE + 5167;
	// Enumerate the independent public state/frame/view/tile/part order. Every
	// honest absent part and still alias keeps its own distinct native source ID.
	for (size_t family = 0; family < _orbital_sprite_ranges.size(); ++family) {
		const auto &range = _orbital_sprite_ranges[family];
		for (uint8_t state = 0; state < range.states; ++state) for (uint8_t frame = 0; frame < range.frames; ++frame)
			for (uint8_t view = 0; view < range.directions; ++view) for (uint8_t tile = 0; tile < range.tiles; ++tile)
				for (uint8_t part = 0; part < range.parts; ++part) {
					const OrbitalArtPose pose{static_cast<OrbitalSpriteAsset>(family), state, frame, static_cast<Direction>(view), tile, part};
					const auto sprite = ResolveOrbitalSprite(pose); REQUIRE(sprite);
					CHECK(*sprite == expected++); CHECK(seen.insert(*sprite).second);
				}
		OrbitalArtPose bad; bad.asset = static_cast<OrbitalSpriteAsset>(family);
		bad.state = range.states; CHECK_FALSE(ResolveOrbitalSprite(bad)); bad.state = 0;
		bad.frame = range.frames; CHECK_FALSE(ResolveOrbitalSprite(bad)); bad.frame = 0;
		bad.direction = static_cast<Direction>(range.directions); CHECK_FALSE(ResolveOrbitalSprite(bad)); bad.direction = Direction::N;
		bad.tile = range.tiles; CHECK_FALSE(ResolveOrbitalSprite(bad)); bad.tile = 0;
		bad.part = range.parts; CHECK_FALSE(ResolveOrbitalSprite(bad));
	}
	CHECK(seen.size() == 976); CHECK(expected == SPR_MAP_LAYER_BASE + 6143);
	OrbitalArtPose bad; bad.asset = OrbitalSpriteAsset::End; CHECK_FALSE(ResolveOrbitalSprite(bad));
}

/** Static, moving and blocked hauler poses always retain their original empty/partial/full physical freight. */
TEST_CASE("Hauler native source preserves actual loads across movement and refusal", "[orbital-art-craft]")
{
	OrbitalCraftArtRig rig;
	// Independent hulls retain their own actual packets. A blocked or animated
	// view cannot borrow another tier or clear the original hold to show empty art.
	for (uint8_t tier = 0; tier < 3; ++tier) {
		auto &craft = rig.Craft(17);
		REQUIRE(craft.cargo_cap > 1);
		const uint16_t quantity = tier == 0 ? 0 : tier == 1 ? 1 : craft.cargo_cap;
		rig.Load(craft, quantity); rig.Visit(craft);
		auto pose = RequiredOrbitalPose(craft); CHECK(pose.state == tier); CHECK(pose.frame == 0);
		craft.cur_speed = 4; craft.tick_counter = 8;
		pose = RequiredOrbitalPose(craft); CHECK(pose.state == 3 + tier); CHECK(pose.frame == 1);
		craft.space_orders.blocked = SpaceOrderBlock::Timeout;
		pose = RequiredOrbitalPose(craft); CHECK(pose.state == 7 + tier); CHECK(pose.frame == 0);
		CHECK(craft.cargo.TotalCount() == quantity);
		VehicleSpriteSeq sequence; craft.GetImage(Direction::N, EngineImageType::OnMap, &sequence);
		const auto drawn = RequiredOrbitalPose(craft, GetSpaceCraftArtDirection(craft));
		REQUIRE(sequence.count == 1); CHECK(sequence.seq[0].sprite == *ResolveOrbitalSprite(drawn));
	}
}

/** Stale movement fields cannot animate an unstarted, arrived, stopped, globally paused or legacy-only non-miner. */
TEST_CASE("Orbital craft movement requires the original outstanding typed destination", "[orbital-art-craft]")
{
	OrbitalCraftArtRig rig;
	// Both non-mining roles use the same original typed navigation domain.
	// Their unrelated retained mining fields cannot become a substitute route.
	for (uint16_t model : {16, 17}) {
		auto &craft = rig.Craft(model); craft.cur_speed = 4; craft.tick_counter = 8;
		craft.mission.phase = SpaceMissionPhase::Approach; craft.mission.target = GetAsteroidField().bodies.front().id;
		CHECK(RequiredOrbitalPose(craft).state == 0); CHECK_FALSE(GetSpaceCraftTravelArtFrame(craft));
		rig.Visit(craft, false); CHECK(RequiredOrbitalPose(craft).state == 0);
		craft.space_orders.started = true;
		const auto moving = model == 16 ? 1 : 3;
		CHECK(RequiredOrbitalPose(craft).state == moving);
		craft.vehstatus.Set(VehState::Stopped); CHECK(RequiredOrbitalPose(craft).state == 0);
		craft.vehstatus.Reset(VehState::Stopped);
		_pause_mode.Set(PauseMode::Normal); CHECK(RequiredOrbitalPose(craft).state == 0); _pause_mode.Reset(PauseMode::Normal);
		REQUIRE(ResolveSpaceNavigationTarget(craft, craft.space_orders.entries[0].target, craft.position) == SpaceOrderBlock::None);
		CHECK(RequiredOrbitalPose(craft).state == 0);
		craft.space_orders.cursor = 1; craft.space_orders.blocked = SpaceOrderBlock::Completed;
		CHECK(RequiredOrbitalPose(craft).state == 0);
	}
}

/** Native tuning excludes16/17service; heavy21 reads only its original positive countdown, retaining canonical pause. */
TEST_CASE("Orbital craft sources cannot invent unsupported probe service or builder work", "[orbital-art-craft]")
{
	OrbitalCraftArtRig rig;
	// Positive stale/corrupt compartment metadata is an explicit refusal input,
	// never permission to turn these two unsupported roles into repair producers.
	for (uint16_t model : {16, 17}) {
		auto &craft = rig.Craft(model); auto &manifest = GetOrCreateSpaceManifest(craft);
		manifest.service_ticks = 24; craft.space_orders.work_ticks = 1;
		CHECK_FALSE(GetSpaceServiceTuning(model)); CHECK_FALSE(GetSpaceCraftServiceArtFrame(craft));
		CHECK(RequiredOrbitalPose(craft).state == 0); CHECK(RequiredOrbitalPose(craft).frame == 0);
		CHECK(manifest.service_ticks == 24); CHECK(manifest.services_completed == 0);
	}
	auto &heavy = rig.Craft(21); auto &manifest = GetOrCreateSpaceManifest(heavy);
	manifest.service_ticks = 24;
	CHECK(RequiredOrbitalPose(heavy).state == 6); CHECK(RequiredOrbitalPose(heavy).frame == 1);
	manifest.service_paused = true; CHECK(RequiredOrbitalPose(heavy).frame == 0);
	manifest.service_paused = false; _pause_mode.Set(PauseMode::Normal);
	CHECK(RequiredOrbitalPose(heavy).state == 6); CHECK(RequiredOrbitalPose(heavy).frame == 0);
	_pause_mode.Reset(PauseMode::Normal); CHECK(manifest.service_ticks == 24);
}

/** Real positive packets remain visible even when the current native freight dimension grants zero capacity. */
TEST_CASE("Hauler zero-capacity refusal never erases original positive freight", "[orbital-art-craft]")
{
	OrbitalCraftArtRig rig; auto &craft = rig.Craft(17); rig.Load(craft, 1);
	AutoRestoreBackup<uint8_t> weight(CargoSpec::Get(craft.cargo_type)->weight, uint8_t{0});
	REQUIRE(GetSpaceFreightCapacity(17, craft.cargo_type) == 0);
	CHECK(RequiredOrbitalPose(craft).state == 1);
	craft.space_orders.blocked = SpaceOrderBlock::HoldIncompatible;
	CHECK(RequiredOrbitalPose(craft).state == 8); CHECK(craft.cargo.TotalCount() == 1);
}

/** Explicit component lease/work inputs test exact original-owner isolation; they are not earned-heavy-world evidence. */
TEST_CASE("Heavy source work requires its own reciprocal lease and unpaused native admission", "[orbital-art-craft]")
{
	OrbitalCraftArtRig rig; auto &heavy = rig.Craft(21); auto &other = rig.Craft(21);
	auto *body = GetMutableAsteroidBody(GetAsteroidField().bodies.front().id); REQUIRE(body);
	REQUIRE(body->remaining_reserve != 0); REQUIRE(heavy.cargo_cap != 0);
	// These explicit isolated owner fields supply the read-only component input.
	// Actual paid mining/placement/work acceptance remains with its original scenarios.
	body->mining_slot = heavy.index;
	heavy.mission.target = body->id; heavy.mission.phase = SpaceMissionPhase::Mining; heavy.mission.work_ticks = 1;
	other.mission.target = body->id; other.mission.phase = SpaceMissionPhase::Mining; other.mission.work_ticks = 1;
	REQUIRE(IsSpaceMiningWorkTick(heavy));
	CHECK(RequiredOrbitalPose(heavy).state == 3); CHECK(RequiredOrbitalPose(heavy).frame == 1);
	CHECK_FALSE(IsSpaceMiningWorkTick(other)); CHECK(RequiredOrbitalPose(other).state == 0);
	_pause_mode.Set(PauseMode::Normal); CHECK(RequiredOrbitalPose(heavy).state == 2); CHECK(RequiredOrbitalPose(heavy).frame == 0);
	_pause_mode.Reset(PauseMode::Normal); heavy.vehstatus.Set(VehState::Stopped);
	CHECK(RequiredOrbitalPose(heavy).state == 2); CHECK(heavy.mission.work_ticks == 1);
	CHECK(body->mining_slot == heavy.index);
}

/** Repeated actual native renderer queries cannot change original queues, resources, packets, service, money, poses or RNG. */
TEST_CASE("Orbital craft renderer queries preserve all original native owners", "[orbital-art-craft]")
{
	OrbitalCraftArtRig rig;
	std::array<SpaceVehicle *, 3> craft = {&rig.Craft(21), &rig.Craft(16), &rig.Craft(17)};
	rig.Load(*craft[2], 1); rig.Visit(*craft[2]); craft[2]->cur_speed = 4; craft[2]->tick_counter = 8;
	auto &service = GetOrCreateSpaceManifest(*craft[0]); service.service_ticks = 24;
	const auto field = ComputeAsteroidFieldDigest(GetAsteroidField()), queues = ComputeSpaceOrdersDigest(), manifests = ComputeSpaceManifestDigest();
	const auto random = _random, interactive = _interactive_random;
	const auto money = Company::Get(_current_company)->money;
	// Exercise repeated real GetImage and direct queries on each independent role;
	// compare physical ownership and counters rather than a promised const signature.
	for (const auto *hull : craft) {
		const auto position = hull->position; const auto direction = hull->direction;
		const auto count = hull->cargo.TotalCount();
		const auto work = hull->mission.work_ticks;
		const auto clock = hull->tick_counter;
		for (uint i = 0; i < 20; ++i) {
			const auto pose = RequiredOrbitalPose(*hull, Direction::SW); CHECK(ResolveOrbitalSprite(pose));
			VehicleSpriteSeq sequence; hull->GetImage(Direction::N, EngineImageType::OnMap, &sequence); REQUIRE(sequence.count == 1);
		}
		CHECK(hull->position == position); CHECK(hull->direction == direction); CHECK(hull->tick_counter == clock);
		CHECK(hull->cargo.TotalCount() == count); CHECK(hull->mission.work_ticks == work);
	}
	CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field); CHECK(ComputeSpaceOrdersDigest() == queues);
	CHECK(ComputeSpaceManifestDigest() == manifests); CHECK(Company::Get(_current_company)->money == money);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]); CHECK(_interactive_random.state[1] == interactive.state[1]);
	auto &probe = rig.Craft(15); CHECK_FALSE(GetOrbitalCraftArtPose(probe, Direction::N));
	CHECK_FALSE(GetOrbitalCraftArtPose(*craft[0], Direction::End));
}
