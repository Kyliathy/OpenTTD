/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/**
 * @file launch_fleet_sprite.cpp Focused native rocket presentation and source-policy observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62J
 * Isolated native components below grant no paid world, work or procurement.
 * The Python runner independently compares phase observations with the frozen
 * source producer; tests never rewrite source pixels or historical receipts.
 */
#include "../stdafx.h"
#include "probe_mining_fixture.h"
#include "../launch_sprite.h"
#include "../space_sprite.h"
#include "../orbital_sprite.h"
#include "../space_transition.h"
#include "../space_manifest.h"
#include "../space_mining_tuning.h"
#include "../station_base.h"
#include <set>
#include "../safeguards.h"

/** Existing native hull fixture with an explicitly selected model14 component. */
class LaunchRocketArtRig : public ProbeExtractionRig {
public:
	/** Create one original native component hull with the delivered rocket Engine.
	 * @return Same Vehicle owner; no paid recipe or earned flight is claimed.
	 */
	SpaceVehicle &Rocket()
	{
		REQUIRE(EnsureSpaceEngineDefinition(14));
		auto &craft = Probe(4, false);
		craft.engine_type = GetSpaceEngine(14);
		craft.construction.model = 14;
		craft.construction.content_version = MULTILAYER_SPACE_CONTENT_VERSION;
		craft.manifest = std::make_unique<SpaceManifest>();
		return craft;
	}
};

/** All declared physical source positions resolve uniquely; malformed indices cannot cross ranges. */
TEST_CASE("Launch ranges cover both qualified families without wrapping invalid poses", "[launch-rocket-art]")
{
	std::set<SpriteID> sprites;
	// Both the twelve-cell facility and whole rocket retain their full declared
	// matrices, including transparent parts and pixel-identical source aliases.
	for (size_t family = 0; family < _launch_fleet_sprite_ranges.size(); ++family) {
		const auto &range = _launch_fleet_sprite_ranges[family];
		for (uint8_t state = 0; state < range.states; ++state) for (uint8_t frame = 0; frame < range.frames; ++frame)
			for (uint8_t direction = 0; direction < range.directions; ++direction) for (uint8_t tile = 0; tile < range.tiles; ++tile)
				for (uint8_t part = 0; part < range.parts; ++part) {
					const auto sprite = ResolveLaunchSprite({static_cast<LaunchFleetSpriteAsset>(family), state, frame, direction, tile, part});
					REQUIRE(sprite); CHECK(sprites.insert(*sprite).second);
					CHECK(*sprite >= SPR_MAP_LAYER_BASE + range.offset);
					CHECK(*sprite < SPR_MAP_LAYER_BASE + range.offset + range.count);
				}
		LaunchArtPose bad; bad.asset = static_cast<LaunchFleetSpriteAsset>(family);
		bad.state = range.states; CHECK_FALSE(ResolveLaunchSprite(bad)); bad.state = 0;
		bad.frame = range.frames; CHECK_FALSE(ResolveLaunchSprite(bad)); bad.frame = 0;
		bad.direction = range.directions; CHECK_FALSE(ResolveLaunchSprite(bad)); bad.direction = 0;
		bad.tile = static_cast<uint8_t>(range.tiles); CHECK_FALSE(ResolveLaunchSprite(bad)); bad.tile = 0;
		bad.part = range.parts; CHECK_FALSE(ResolveLaunchSprite(bad));
	}
	CHECK(sprites.size() == 1648);
	CHECK(*sprites.begin() == SPR_MAP_LAYER_BASE + 6143);
	CHECK(*sprites.rbegin() == SPR_MAP_LAYER_BASE + 7790);
	LaunchArtPose bad; bad.asset = LaunchFleetSpriteAsset::End; CHECK_FALSE(ResolveLaunchSprite(bad));
}

/** Observe every phase/block/burn combination and full active countdown for independent source comparison. */
TEST_CASE("Rocket source states follow native funded legs and never advance their owners", "[launch-rocket-art]")
{
	LaunchRocketArtRig rig; auto &craft = rig.Rocket();
	craft.transition = std::make_unique<SpaceTransition>();
	auto &flight = *craft.transition;
	const auto position = craft.position, home = craft.home;
	const auto direction = craft.direction;
	const auto status = craft.vehstatus;
	const auto random = _random, interactive = _interactive_random;
	const auto tick = TimerGameTick::counter;
	const auto orders = ComputeSpaceOrdersDigest(), field = ComputeAsteroidFieldDigest(GetAsteroidField());
	REQUIRE(GetMultiLayerSpaceBudget().flight_ticks == 222);
	// Exhaustive adverse inputs distinguish real funded movement from a held
	// or unfunded shell. This is visual refusal, not native save admission.
	for (uint8_t phase = 0; phase < to_underlying(SpaceFlightPhase::End); ++phase)
		for (uint8_t block = 0; block < to_underlying(SpaceFlightBlock::End); ++block)
			for (uint8_t burns = 0; burns < 4; ++burns) for (uint16_t remaining : {0, 1, 206, 214, 222, 223}) {
				flight.phase = static_cast<SpaceFlightPhase>(phase); flight.blocked = static_cast<SpaceFlightBlock>(block);
				flight.outbound_burned = (burns & 1) != 0; flight.recovery_burned = (burns & 2) != 0;
				flight.remaining_ticks = remaining;
				const auto before = flight;
				const auto pose = GetLaunchRocketArtPose(craft, Direction::NE); REQUIRE(pose);
				REQUIRE(ResolveLaunchSprite(*pose)); CHECK(flight == before);
				fmt::print("LAUNCH_ROCKET_POLICY {} {} {} {} {} {}\n", phase, block, burns, remaining, pose->state, pose->frame);
			}
	// Each native opportunity is observed twice on both active legs. Repeated
	// drawing must retain the same original countdown and phase/burn receipts.
	for (auto phase : {SpaceFlightPhase::Ascent, SpaceFlightPhase::ReturnRecovery}) for (uint16_t remaining = 1; remaining <= 222; ++remaining) {
		flight.phase = phase; flight.blocked = SpaceFlightBlock::HomeLost;
		flight.outbound_burned = true; flight.recovery_burned = phase == SpaceFlightPhase::ReturnRecovery; flight.remaining_ticks = remaining;
		const auto before = flight;
		const auto first = GetLaunchRocketArtPose(craft, Direction::SW), second = GetLaunchRocketArtPose(craft, Direction::SW);
		REQUIRE(first); REQUIRE(second); CHECK(first->state == second->state); CHECK(first->frame == second->frame); CHECK(flight == before);
		fmt::print("LAUNCH_ROCKET_POLICY {} {} {} {} {} {}\n", to_underlying(phase), to_underlying(flight.blocked), phase == SpaceFlightPhase::Ascent ? 1 : 3, remaining, first->state, first->frame);
	}
	flight.phase = SpaceFlightPhase::End; CHECK_FALSE(GetLaunchRocketArtPose(craft, Direction::N));
	flight.phase = SpaceFlightPhase::GroundLoading; flight.blocked = SpaceFlightBlock::End; CHECK_FALSE(GetLaunchRocketArtPose(craft, Direction::N));
	CHECK_FALSE(GetLaunchRocketArtPose(craft, Direction::End));
	CHECK(craft.position == position); CHECK(craft.home == home); CHECK(craft.direction == direction); CHECK(craft.vehstatus == status);
	CHECK(craft.cargo.TotalCount() == 0); CHECK(craft.manifest->fuel_burned == 0); CHECK(craft.manifest->services_completed == 0);
	CHECK(craft.manifest->supplies[0].AvailableCount() == 0); CHECK(craft.manifest->service_ticks == 0);
	CHECK(ComputeSpaceOrdersDigest() == orders); CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field); CHECK(TimerGameTick::counter == tick);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]); CHECK(_interactive_random.state[1] == interactive.state[1]);
}

/** Genuine headings follow the exact owned facility or existing recovery interpolation, never a saved art direction. */
TEST_CASE("Rocket headings retain physical facility rotations and eight native recovery directions", "[launch-rocket-art]")
{
	LaunchRocketArtRig rig; auto &craft = rig.Rocket();
	const auto anchor = TileXYL(20, 20, MapLayer::Surface);
	REQUIRE(Station::CanAllocateItem()); std::unique_ptr<Station> center{Station::Create(anchor)};
	center->owner = craft.owner; center->launch_center = std::make_unique<LaunchCenterAnchor>(anchor, 17, 0);
	craft.construction.home = GetLaunchCenterReference(*center); craft.construction.placement = SpacePlacement::SurfaceStored;
	craft.direction = Direction::E;
	// Every genuine facility view has its matching independently authored body;
	// station generation and company ownership must remain the native authority.
	for (uint8_t rotation = 0; rotation < 4; ++rotation) {
		center->launch_center->rotation = rotation;
		CHECK(GetLaunchRocketArtDirection(craft) == static_cast<Direction>(1 + 2 * rotation));
	}
	center->launch_center->rotation = 4; CHECK(GetLaunchRocketArtDirection(craft) == Direction::E);
	center->launch_center->rotation = 0; ++craft.construction.home.generation; CHECK(GetLaunchRocketArtDirection(craft) == Direction::E);
	--craft.construction.home.generation; center->owner = OWNER_NONE; CHECK(GetLaunchRocketArtDirection(craft) == Direction::E);
	center->owner = craft.owner;
	craft.transition = std::make_unique<SpaceTransition>(); auto &flight = *craft.transition;
	craft.construction.placement = SpacePlacement::InTransit; flight.layer = MapLayer::Space;
	flight.phase = SpaceFlightPhase::ReturnRecovery; flight.blocked = SpaceFlightBlock::HomeLost;
	flight.departure = {8 * SPACE_Q16, 8 * SPACE_Q16};
	const std::array<SpacePoint, 8> vectors = {{{-1,-1},{-3,-1},{-1,1},{1,3},{1,1},{3,1},{1,-1},{-1,-3}}};
	// These fixed world vectors were independently qualified against the native
	// 2:1 projection; lost-home recovery still uses its retained actual geometry.
	for (uint8_t view = 0; view < 8; ++view) {
		craft.home = {flight.departure.x + vectors[view].x * SPACE_Q16, flight.departure.y + vectors[view].y * SPACE_Q16};
		CHECK(GetLaunchRocketArtDirection(craft) == static_cast<Direction>(view));
	}
	craft.home = flight.departure; CHECK(GetLaunchRocketArtDirection(craft) == Direction::E);
	craft.direction = Direction::End; CHECK(GetLaunchRocketArtDirection(craft) == Direction::NE);
	flight.phase = SpaceFlightPhase::OrbitalDelivery; flight.destination = {}; CHECK(GetLaunchRocketArtDirection(craft) == Direction::NE);
	const auto &body = GetAsteroidField().bodies.front();
	const SpacePoint target{body.motion.position.x + craft.mission.attachment_offset.x, body.motion.position.y + craft.mission.attachment_offset.y};
	flight.destination = {SpaceOrderTargetKind::Body, body.id.base(), 0}; flight.blocked = SpaceFlightBlock::Approach;
	// Actual delivery targets retain the public native attachment offset. Every
	// map image must use that physical vector, regardless of a caller's list view.
	for (uint8_t view = 0; view < 8; ++view) {
		craft.position = {target.x - vectors[view].x * SPACE_Q16, target.y - vectors[view].y * SPACE_Q16};
		const auto position = craft.position;
		CHECK(GetLaunchRocketArtDirection(craft) == static_cast<Direction>(view));
		VehicleSpriteSeq sequence; craft.GetImage(Direction::N, EngineImageType::OnMap, &sequence);
		REQUIRE(sequence.count == 1); CHECK(sequence.seq[0].sprite == *ResolveLaunchSprite({LaunchFleetSpriteAsset::VehicleModel14, 3, 0, view}));
		CHECK(craft.position == position); CHECK_FALSE(craft.space_orders.approach);
	}
	CHECK(flight.blocked == SpaceFlightBlock::Approach); CHECK(craft.construction.placement == SpacePlacement::InTransit);
	craft.construction.home = {}; // The component center may now retire without a live fixture dependency.
}

/** Native GetImage uses the new rocket owner while retaining existing probe/orbital families and GUI headings. */
TEST_CASE("Native image dispatch selects genuine rocket poses and preserves other role families", "[launch-rocket-art]")
{
	LaunchRocketArtRig rig; auto &craft = rig.Rocket();
	const auto dormant = GetLaunchRocketArtPose(craft, Direction::N); REQUIRE(dormant); CHECK(dormant->state == 0); CHECK(dormant->frame == 0);
	craft.direction = Direction::W; craft.transition = std::make_unique<SpaceTransition>();
	craft.transition->phase = SpaceFlightPhase::Ascent; craft.transition->outbound_burned = true; craft.transition->remaining_ticks = 214;
	const auto status = craft.vehstatus;
	// All eight GUI headings remain explicit caller requests; on-map rendering
	// reads physical native geometry independently of a list's requested facing.
	for (uint8_t view = 0; view < 8; ++view) {
		VehicleSpriteSeq sequence; craft.GetImage(static_cast<Direction>(view), EngineImageType::InList, &sequence);
		REQUIRE(sequence.count == 1); CHECK(sequence.seq[0].sprite == *ResolveLaunchSprite({LaunchFleetSpriteAsset::VehicleModel14, 2, 1, view}));
	}
	VehicleSpriteSeq sequence; craft.GetImage(Direction::N, EngineImageType::OnMap, &sequence);
	REQUIRE(sequence.count == 1); CHECK(sequence.seq[0].sprite == *ResolveLaunchSprite({LaunchFleetSpriteAsset::VehicleModel14, 2, 1, to_underlying(Direction::W)}));
	CHECK(craft.vehstatus == status); CHECK(craft.direction == Direction::W); CHECK(craft.transition->remaining_ticks == 214);
	craft.transition.reset();
	// Existing models keep their previous selector, native range and image
	// dispatch. Adding a rocket cannot substitute another role's capabilities.
	for (uint16_t model : std::array<uint16_t, 4>{15, 19, 20, SPACE_HEAVY_MINER_MODEL}) {
		REQUIRE(EnsureSpaceEngineDefinition(model)); craft.engine_type = GetSpaceEngine(model); craft.construction.model = model;
		CHECK_FALSE(GetLaunchRocketArtPose(craft, Direction::N));
		craft.GetImage(Direction::N, EngineImageType::InList, &sequence); REQUIRE(sequence.count == 1);
		const auto space = GetSpaceCraftArtPose(craft, Direction::N);
		if (space) { CHECK(sequence.seq[0].sprite == *ResolveSpaceSprite(*space)); }
		else { const auto orbital = GetOrbitalCraftArtPose(craft, Direction::N); REQUIRE(orbital); CHECK(sequence.seq[0].sprite == *ResolveOrbitalSprite(*orbital)); }
	}
}
