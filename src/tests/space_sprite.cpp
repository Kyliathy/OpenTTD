/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_sprite.cpp Final Space art reads native owners without changing work, packets or geometry.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG
 * Explicit isolated poses, service receipts and construction materials below test presentation only; paid gameplay acceptance remains with the original native scenarios.
 */
#include "../stdafx.h"
#include "probe_mining_fixture.h"
#include "../space_sprite.h"
#include "../space_pod.h"
#include "../space_manifest.h"
#include "../space_structure.h"
#include "../orbital_service.h"
#include "../company_base.h"
#include <set>
#include "../safeguards.h"

/** Native component owners with explicit input receipts; no procurement or elapsed gameplay is claimed. */
class SpaceArtRig : public ProbeExtractionRig {
public:
	/** Require empty physical pools so a presentation test never changes an unrelated live owner. */
	SpaceArtRig() { REQUIRE(SpaceStructure::GetNumItems() == 0); REQUIRE(SpacePod::GetNumItems() == 0); }
	/** Release the declared component owners before the base fixture restores the original world. */
	~SpaceArtRig() { _space_pod_pool.CleanPool(); _space_structure_pool.CleanPool(); }
	/** Select a delivered role on one isolated native hull, without claiming its paid assembly.
	 * @param model Existing canonical model15,19 or20.
	 * @return Actual component hull with original native queue and containers.
	 */
	SpaceVehicle &Craft(uint16_t model)
	{
		REQUIRE(EnsureSpaceEngineDefinition(model));
		auto &craft = Probe(4, false); craft.engine_type = GetSpaceEngine(model);
		craft.construction.placement = SpacePlacement::Orbital;
		craft.construction.content_version = MULTILAYER_SPACE_CONTENT_VERSION;
		return craft;
	}
	/** Create one explicit isolated native site; materials and completed work remain separate test inputs.
	 * @return Original component Core blueprint with a generation-qualified identity.
	 */
	SpaceStructure &Site()
	{
		auto *site = SpaceStructure::Create(); site->generation = 1;
		site->state.owner = _current_company; site->state.x = 4; site->state.y = 4;
		return *site;
	}
};

/** Every authored Cartesian pose has its own protected native ID, and invalid indices cannot cross family boundaries. */
TEST_CASE("Final Space ranges resolve every authored pose and refuse missing indices", "[space-art]")
{
	std::set<SpriteID> seen;
	// Traverse the complete published source matrix, including explicit static aliases: native IDs must remain distinct even when their reviewed pixels agree.
	for (size_t family = 0; family < _space_sprite_ranges.size(); ++family) {
		const auto &r = _space_sprite_ranges[family];
		for (uint8_t state = 0; state < r.states; ++state) for (uint8_t frame = 0; frame < r.frames; ++frame)
			for (uint8_t view = 0; view < r.directions; ++view) for (uint8_t size = 0; size < r.sizes; ++size) {
				SpaceArtPose pose{static_cast<SpaceSpriteAsset>(family), state, frame, static_cast<Direction>(view), size};
				const auto sprite = ResolveSpaceSprite(pose); REQUIRE(sprite); CHECK(seen.insert(*sprite).second);
				CHECK(*sprite >= SPR_MAP_LAYER_BASE + r.offset); CHECK(*sprite < SPR_MAP_LAYER_BASE + r.offset + r.count);
			}
		SpaceArtPose bad; bad.asset = static_cast<SpaceSpriteAsset>(family);
		bad.state = r.states; CHECK_FALSE(ResolveSpaceSprite(bad)); bad.state = 0;
		bad.frame = r.frames; CHECK_FALSE(ResolveSpaceSprite(bad)); bad.frame = 0;
		bad.direction = Direction::End; CHECK_FALSE(ResolveSpaceSprite(bad)); bad.direction = Direction::N;
		bad.size = r.sizes; CHECK_FALSE(ResolveSpaceSprite(bad));
	}
	CHECK(seen.size() == 1568); CHECK(*seen.begin() == SPR_MAP_LAYER_BASE + SPACE_SPRITE_START);
	CHECK(*seen.rbegin() == SPR_MAP_LAYER_BASE + SPACE_SPRITE_END - 1);
	SpaceArtPose bad; bad.asset = SpaceSpriteAsset::End; CHECK_FALSE(ResolveSpaceSprite(bad));
}

/** Eight genuine views follow fixed projection; extreme Q16 endpoints and coincident positions cannot overflow or manufacture saved orientation. */
TEST_CASE("Space art facing uses all eight projected directions safely", "[space-art]")
{
	const std::array<SpacePoint, 8> targets = {{{-1,-1},{-3,-1},{-1,1},{1,3},{1,1},{3,1},{1,-1},{-1,-3}}};
	// Each native direction receives an independent world vector, rather than rotating or reflecting an authored raster.
	for (uint8_t d = 0; d < 8; ++d) CHECK(GetSpaceArtDirection({}, targets[d], Direction::NE) == static_cast<Direction>(d));
	CHECK(GetSpaceArtDirection({}, {}, Direction::SW) == Direction::SW);
	CHECK(GetSpaceArtDirection({}, {}, Direction::End) == Direction::NE);
	CHECK(GetSpaceArtDirection({INT32_MIN, INT32_MAX}, {INT32_MAX, INT32_MIN}, Direction::N) == Direction::W);
}

/** Actual native pursuit earns its own lease; repeated art queries neither extract reserve nor change motion, RNG, queues or cargo. */
TEST_CASE("Miner and body art follow the exact native lease without progressing it", "[space-art]")
{
	SpaceArtRig rig; auto &craft = rig.Probe(); auto &body = rig.Attach(craft);
	craft.mission.work_ticks = 1; // Explicit visual parity input; no paid mission evidence is claimed here.
	const auto field = ComputeAsteroidFieldDigest(GetAsteroidField()), orders = ComputeSpaceOrdersDigest();
	const auto random = _random, interactive = _interactive_random; const auto direction = craft.direction;
	const auto position = craft.position; const auto cargo = craft.cargo.TotalCount();
	REQUIRE(IsSpaceMiningWorkTick(craft));
	const auto miner = GetSpaceCraftArtPose(craft, Direction::SW); REQUIRE(miner); CHECK(miner->state == 3); CHECK(miner->frame == 1);
	const auto asteroid = GetSpaceAsteroidArtPose(body.id, _current_company); REQUIRE(asteroid); CHECK(asteroid->state == 2); CHECK(asteroid->frame == 1);
	CHECK(GetSpaceCraftArtDirection(craft) == GetSpaceArtDirection(craft.position, body.motion.position, craft.direction));
	CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field); CHECK(ComputeSpaceOrdersDigest() == orders);
	CHECK(craft.direction == direction); CHECK(craft.position == position); CHECK(craft.cargo.TotalCount() == cargo);
	CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
	CHECK(_interactive_random.state[0] == interactive.state[0]); CHECK(_interactive_random.state[1] == interactive.state[1]);
	craft.vehstatus.Set(VehState::Stopped);
	CHECK(GetSpaceCraftArtPose(craft, Direction::N)->state == 2); CHECK(GetSpaceAsteroidArtPose(body.id, _current_company)->state == 1);
	body.mining_slot = VehicleID::Invalid(); CHECK(GetSpaceAsteroidArtPose(body.id, _current_company)->state == 0);
}

/** Company-authorized zero is visible only through the original survey owner; velocity reflection cannot rotate a body or its attachment geometry. */
TEST_CASE("Asteroid art preserves saved sizes and company survey isolation", "[space-art]")
{
	SpaceArtRig rig; auto &body = *GetMutableAsteroidBody(OrbitalBodyID{0}); body.remaining_reserve = 0;
	std::unique_ptr<Company> foreign{Company::Create()}; REQUIRE(foreign->index != _current_company);
	REQUIRE(InstallLoadedAsteroidKnowledge(std::array{AsteroidKnowledgeRecord{body.id, uint64_t{1} << _current_company.base()}}));
	const auto field = ComputeAsteroidFieldDigest(GetAsteroidField());
	const auto known = GetSpaceAsteroidArtPose(body.id, _current_company); REQUIRE(known); CHECK(known->state == 3);
	CHECK(GetSpaceAsteroidArtPose(body.id, foreign->index)->state == 0); CHECK(GetSpaceAsteroidArtPose(body.id, CompanyID::Invalid())->state == 0);
	CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == field);
	body.motion.velocity_x = -body.motion.velocity_x; body.motion.velocity_y = -body.motion.velocity_y;
	CHECK(GetSpaceAsteroidArtPose(body.id, _current_company)->direction == known->direction);
	// Saved size and class select independent final ranges; visual inspection cannot reroll either native geological definition.
	for (uint8_t size = 0; size < 3; ++size) for (uint8_t cls = 0; cls < 3; ++cls) {
		body.size = static_cast<AsteroidSize>(size); body.resource_class = static_cast<AsteroidClass>(cls);
		const auto pose = GetSpaceAsteroidArtPose(body.id, _current_company); REQUIRE(pose); CHECK(pose->size == size);
		CHECK(pose->asset == static_cast<SpaceSpriteAsset>(to_underlying(SpaceSpriteAsset::AsteroidNickel) + cls));
	}
	CHECK_FALSE(GetSpaceAsteroidArtPose(INVALID_ORBITAL_BODY, _current_company));
}

/** Original countdowns and exact hull generations supply service art, including a frozen phase when actual service is paused. */
TEST_CASE("Service art reads original countdown and generation-qualified contained job", "[space-art]")
{
	SpaceArtRig rig; auto &craft = rig.Craft(19); auto &manifest = GetOrCreateSpaceManifest(craft);
	manifest.service_ticks = 24;
	CHECK(GetSpaceCraftArtPose(craft, Direction::E)->state == 4); CHECK(GetSpaceCraftArtPose(craft, Direction::E)->frame == 1);
	manifest.service_paused = true; CHECK(GetSpaceCraftArtPose(craft, Direction::E)->frame == 0);
	manifest.service_ticks = 0;
	auto &site = rig.Site(); site.module = std::make_unique<OrbitalModule>();
	auto &job = site.module->service; job.craft = GetSpaceCraftReference(craft); job.remaining_ticks = 40;
	CHECK(GetOrbitalServiceJobState(craft) == &job); CHECK(GetSpaceCraftArtPose(craft, Direction::E)->state == 4);
	CHECK(job.remaining_ticks == 40); job.paused = true; CHECK(GetSpaceCraftArtPose(craft, Direction::E)->frame == 0);
	++job.craft.generation; CHECK(GetOrbitalServiceJobState(craft) == nullptr); CHECK(GetSpaceCraftArtPose(craft, Direction::E)->state == 0);
}

/** Actual queues and freight supply independent role states; stale speed at stopped, unstarted or arrived instructions earns no animated exhaust. */
TEST_CASE("Probe art uses original orders cargo and movement admission", "[space-art]")
{
	SpaceArtRig rig; auto &survey = rig.Craft(19); survey.cur_speed = 4; survey.tick_counter = 8;
	survey.space_orders.entries.push_back({SpaceOrderAction::Visit, {SpaceOrderTargetKind::Body, 0, 0}});
	CHECK(GetSpaceCraftArtPose(survey, Direction::N)->frame == 0);
	survey.space_orders.started = true; CHECK(GetSpaceCraftArtPose(survey, Direction::N)->frame == 1);
	survey.vehstatus.Set(VehState::Stopped); CHECK(GetSpaceCraftArtPose(survey, Direction::N)->frame == 0);
	survey.vehstatus.Reset(VehState::Stopped);
	REQUIRE(ResolveSpaceNavigationTarget(survey, survey.space_orders.entries[0].target, survey.position) == SpaceOrderBlock::None);
	CHECK(GetSpaceCraftArtPose(survey, Direction::N)->frame == 0);
	survey.space_orders.blocked = SpaceOrderBlock::Timeout; CHECK(GetSpaceCraftArtPose(survey, Direction::N)->state == 5);
	survey.space_orders.cursor = 1; survey.space_orders.blocked = SpaceOrderBlock::Completed;
	CHECK(GetSpaceCraftArtPose(survey, Direction::N)->state == 0);
	auto &builder = rig.Craft(20); builder.cargo_type = GetCargoTypeByLabel(CargoLabel{"STEL"});
	CHECK(GetSpaceCraftArtPose(builder, Direction::N)->state == 0);
	builder.cargo.Append(CargoPacket::Create(uint16_t{2}, uint16_t{0}, StationID::Invalid(), builder.tile, Money{0}));
	const auto count = builder.cargo.TotalCount(); CHECK(GetSpaceCraftArtPose(builder, Direction::N)->state == 2);
	builder.cargo_type = GetCargoTypeByLabel(CargoLabel{"GOOD"}); CHECK(GetSpaceCraftArtPose(builder, Direction::N)->state == 3);
	CHECK(builder.cargo.TotalCount() == count);
	REQUIRE(EnsureSpaceEngineDefinition(14)); builder.engine_type = GetSpaceEngine(14); CHECK_FALSE(GetSpaceCraftArtPose(builder, Direction::N));
}

/** A passive pod has one original custody and countdown; success-ready art never creates a second shell, packet owner or persistent recovery marker. */
TEST_CASE("Passive pod art hides carried custody and reads only its live descent", "[space-art]")
{
	SpaceArtRig rig; auto *pod = SpacePod::Create(); pod->state.custody = SpacePodCustody::Craft;
	CHECK_FALSE(GetSpacePodArtPose(*pod)); pod->state.custody = SpacePodCustody::CenterStored; CHECK_FALSE(GetSpacePodArtPose(*pod));
	pod->state.custody = SpacePodCustody::Drifting; CHECK(GetSpacePodArtPose(*pod)->state == 3);
	pod->state.custody = SpacePodCustody::Descent; pod->descent = std::make_unique<SpaceDescentJob>();
	pod->descent->remaining_ticks = 24; const auto state = pod->state;
	CHECK(GetSpacePodArtPose(*pod)->state == 1); CHECK(GetSpacePodArtPose(*pod)->frame == 1);
	CHECK(pod->descent->remaining_ticks == 24); CHECK(pod->state == state); CHECK(SpacePod::GetNumItems() == 1);
	pod->descent->remaining_ticks = 1; CHECK(GetSpacePodArtPose(*pod)->state == 2); CHECK(GetSpacePodArtPose(*pod)->frame == 1);
	pod->descent->blocked = SpaceDescentBlock::StorageFull; CHECK(GetSpacePodArtPose(*pod)->state == 3);
	CHECK(pod->descent->packets.TotalCount() == 0); CHECK(SpacePod::GetNumItems() == 1);
}

/** A same-opportunity work receipt belongs to the original physically admitted builder; an untouched zero tick or another hull cannot borrow it. */
TEST_CASE("Construction art retains committed work only for its actual native worker", "[space-art]")
{
	SpaceArtRig rig; auto &builder = rig.Craft(20); GetOrCreateSpaceManifest(builder);
	auto &other = rig.Craft(20); GetOrCreateSpaceManifest(other); auto &site = rig.Site();
	const SpaceOrderTarget target{SpaceOrderTargetKind::Blueprint, site.index.base(), site.generation};
	builder.space_orders.entries.push_back({SpaceOrderAction::Construct, target});
	other.space_orders.entries.push_back({SpaceOrderAction::Construct, target});
	REQUIRE(RequestSpaceApproach(builder, target, true)); PrepareSpaceApproachTick();
	REQUIRE(builder.space_orders.approach); REQUIRE(builder.space_orders.approach->slot == 0);
	builder.position = *GetSpaceStructureApproachSlot(site, 0); builder.UpdateSpacePosition();
	REQUIRE(GetSpaceStructureBerth(builder, site));
	const auto *definition = GetSpaceConstructionDefinition(site.state.kind); REQUIRE(definition);
	// Explicit original native material containers exercise presentation admission only; these supplied packets are never counted as paid construction evidence.
	for (uint i = 0; i < site.job.inputs.size(); ++i) {
		site.job.inputs[i].Append(CargoPacket::Create(static_cast<uint16_t>(definition->inputs[i].quantity), uint16_t{0}, StationID::Invalid(), builder.tile, Money{0}));
	}
	REQUIRE(HasOrbitalConstructionInputs(site));
	TimerGameTick::counter = 0; site.job.last_work_tick = 0; CHECK_FALSE(GetSpaceConstructionVisualWork(builder));
	TimerGameTick::counter = 77; site.job.last_work_tick = 77; site.job.work_ticks = 3;
	CHECK(GetSpaceConstructionVisualWork(builder) == 3); CHECK(GetSpaceCraftArtPose(builder, Direction::N)->state == 4);
	CHECK(GetSpaceCraftArtPose(builder, Direction::N)->frame == 1); CHECK(site.job.work_ticks == 3); CHECK(site.job.last_work_tick == 77);
	other.position = builder.position; CHECK_FALSE(GetSpaceConstructionVisualWork(other));
	builder.vehstatus.Set(VehState::Stopped); CHECK_FALSE(GetSpaceConstructionVisualWork(builder));
	builder.vehstatus.Reset(VehState::Stopped); ++builder.space_orders.entries[0].target.generation;
	CHECK_FALSE(GetSpaceConstructionVisualWork(builder));
}
