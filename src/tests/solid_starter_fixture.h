/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file solid_starter_fixture.h Real sealed rail pair, Surface origin and reference train for solid-start boundary tests.
 * The pair is built by the real sealed command, so its destination housing and pocket keep Solid rock. The reference
 * train is a pool-owned placeholder for the Commissioning binding only; it is not a TBM, and no actual wagons exist.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Group 8B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs2-geology-and-excavation.md, Group 14B
 * @see docs/zz-rhr/upgrades/2026-10/portal-tbm/r2mlvs3mp-f-solid-starter-contract.md
 */

#ifndef TESTS_SOLID_STARTER_FIXTURE_H
#define TESTS_SOLID_STARTER_FIXTURE_H

#include "excavation_fixture.h"
#include "../layer_portal_cmd.h"
#include "../layer_portal_map.h"
#include "../station_base.h"
#include "../train.h"
#include "../vehicle_func.h"
#include "../rail.h"
#include "../road.h"

/** Interface participant with explicit independent spoil spaces, support units and the real company's cash. */
class StarterParticipant final : public ExcavationMaterialParticipant, public ExcavationWorkParticipant {
public:
	uint64_t earth_space = 0, rock_space = 0; ///< Free Earth and Rock tonnes; neither can absorb the other.
	uint64_t earth = 0, rock = 0, revision = 0; ///< Committed fixture stock and stamp.
	uint32_t supports = 64, commits = 0; ///< Available support units and committed receipt count.
	CompanyID company = _current_company; ///< Real cash owner.

	/** @return Current spaces, supports and real funds, without mutation. */
	ExcavationMaterialSnapshot Capture() const override
	{
		return {this->revision, this->earth_space - this->earth, this->rock_space - this->rock, this->supports, Company::Get(this->company)->money};
	}

	/**
	 * Accept only an unchanged owner state that still fits the whole receipt.
	 * @param expected Snapshot captured by the preflight.
	 * @param delta Complete proposed receipt.
	 * @return Whether every captured quantity is unchanged and the whole delta fits.
	 */
	bool Validate(const ExcavationMaterialSnapshot &expected, const ExcavationCutResult &delta) const override
	{
		const auto now = this->Capture();
		// All four independent quantities must match the captured snapshot and cover the complete delta; any drift refuses.
		return expected.revision == now.revision && expected.earth_space_t == now.earth_space_t && expected.rock_space_t == now.rock_space_t && expected.support_units == now.support_units && expected.available_funds == now.available_funds && delta.earth_t <= now.earth_space_t && delta.rock_t <= now.rock_space_t && delta.support_units <= now.support_units && delta.cost <= now.available_funds;
	}

	/**
	 * Apply the whole receipt once.
	 * @param expected Snapshot captured by the preflight.
	 * @param delta Complete receipt.
	 * @return True after the full receipt, false with nothing changed.
	 */
	bool Commit(const ExcavationMaterialSnapshot &expected, const ExcavationCutResult &delta) noexcept override
	{
		if (!this->Validate(expected, delta)) return false;
		this->earth += delta.earth_t;
		this->rock += delta.rock_t;
		this->supports -= delta.support_units;
		Company::Get(this->company)->money -= delta.cost;
		++this->revision;
		++this->commits;
		return true;
	}

	/**
	 * A straddling consist is backed by the housing and shaft, which this interface fixture does not model.
	 * @param job Actual job.
	 * @param cut Next segment; its housing/shaft support is not modelled by this interface fixture.
	 * @return Whether the job still has a real company; actual support belongs to the later Plan 4 controller.
	 */
	bool IsSupported(const ExcavationJob &job, [[maybe_unused]] const ExcavationCutGeometry &cut) const override
	{
		return Company::IsValidID(job.company);
	}
};

/** Sealed Surface/Underground rail pair at (20,20) with its door facing SE, plus a Surface railway origin. */
class SolidStarterFixture {
public:
	ExcavationFixture world; ///< Real profile, company and geology; destroyed last.
	TileIndex source = TileXYL(20, 20, MapLayer::Surface); ///< Accessible upper endpoint and pair key.
	TileIndex destination = INVALID_TILE; ///< Solid rock-side housing.
	Station *origin = nullptr; ///< Real pool-owned Surface railway station.
	Train *reference = nullptr; ///< Pool-owned placeholder bound by a Commissioning record; never a TBM.
	LayerPortalCommissionRequest request; ///< Actual source request with an unallocated proposal.

	/** Build the pair through the real sealed command and an owned Surface origin. */
	SolidStarterFixture()
	{
		REQUIRE(Vehicle::GetNumItems() == 0);
		REQUIRE(Station::GetNumItems() == 0);
		ResetLayerPortalWorkState();
		ResetRailTypes();
		ResetRoadTypes(); // Blocking and reuse cases build ordinary roads on the pair's floor.
		REQUIRE(CmdBuildLayerPortal(DoCommandFlag::Execute, this->source, TransportType::Rail, MapLayer::Underground, DiagDirection::SE, RAILTYPE_RAIL, ROADTYPE_ROAD).Succeeded());
		this->destination = GetLayerPortalPairTile(this->source);
		REQUIRE(Station::CanAllocateItem());
		this->origin = Station::Create(TileXYL(18, 20, MapLayer::Surface));
		this->origin->owner = _current_company;
		this->origin->facilities.Set(StationFacility::Train);
		this->request.portal = this->source;
		this->request.proposal.company = _current_company;
		this->request.proposal.origin = this->origin->index;
		this->request.proposal.model_id = 0;
		this->request.proposal.commissioning_anchor = this->destination;
		this->request.proposal.pass_terms.emplace();
	}

	/** Release fixture pools and portal state before the world fixture tears down its company and map. */
	~SolidStarterFixture()
	{
		_vehicle_pool.CleanPool();
		ResetVehicleHash();
		_station_pool.CleanPool();
		ResetLayerPortalWorkState();
	}

	/**
	 * Planning pose of a cutter in the housing, facing the door, with the whole consist still in the shaft.
	 * @param proof Issued proof.
	 * @param points Planning history points, one per occupied consist tile (3..16).
	 * @return Start pose for the canonical planner.
	 */
	static ExcavationPlanningPose StartPose(const LayerPortalBootstrapProof &proof, size_t points = 3)
	{
		return {proof.destination, proof.exit_heading, std::vector<TileIndex>(points, proof.destination)};
	}

	/**
	 * Fill a proposal whose only segment is one straight advance from the housing.
	 * @param proof Issued proof.
	 * @param heading Advance heading; the door heading leaves legally, tests pass another one to exercise refusals.
	 * @return Proposal with requested end cell, authorized sweep and route, or an empty route on geometry refusal.
	 */
	ExcavationJob FirstSegmentProposal(const LayerPortalBootstrapProof &proof, Direction heading) const
	{
		auto proposal = this->request.proposal;
		auto pose = StartPose(proof);
		pose.heading = heading; // A straight step in the requested heading, so only the starter rules can refuse it.
		const auto step = AdvanceExcavationPlanningPose(pose, heading);
		if (step.error != ExcavationError::None) return proposal;
		proposal.requested_tiles = {step.next.position};
		proposal.authorized_tiles = step.cut.swept_tiles;
		proposal.route = {step.cut};
		return proposal;
	}

	/**
	 * Publish the pair as Commissioning one reserved job, through the real loader validation boundary.
	 * @param job Reserved geological job.
	 * @return The published record.
	 */
	LayerPortalWorkRecord Publish(ExcavationJobID job)
	{
		if (this->reference == nullptr) {
			REQUIRE(Train::CanAllocateItem());
			this->reference = Train::Create();
			this->reference->owner = _current_company;
			this->reference->SetArticulatedPart(); // A reference-only object joins no engine or group stock accounting.
			this->reference->vehstatus.Set(VehState::Hidden);
		}
		auto record = *GetLayerPortalWorkRecord(this->source);
		record.state = LayerPortalWorkState::Commissioning;
		record.project = job;
		record.cutter = this->reference->index;
		record.origin = this->origin->index;
		++record.revision;
		const std::array rows{record};
		REQUIRE(ValidateLayerPortalWorkRecords(rows, GetCurrentMultiLayerProfileSelection(), GetGeologyWorldState()) == LayerPortalWorkError::None);
		StageLoadedLayerPortalWork({record});
		AcceptLoadedLayerPortalWork();
		return record;
	}
};

#endif /* TESTS_SOLID_STARTER_FIXTURE_H */
