/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_work.cpp Face work and the actual-train cut participant: bounded work per tick against the one reserved
 * face, the saved-heading segment check, precise material waits, and one atomic paid cut that revalidates the swept
 * envelope, lays support, loads real Earth and Rock packets into the assigned wagons and debits the company.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Groups 6, 7, 8 and 10
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 9.3 (the logistics pause of new cutting)
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, Group 13A (loaded train-mass continuity)
 */

#include "stdafx.h"
#include "tbm.h"
#include "tbm_hangar.h"
#include "tbm_exchange.h"
#include "train.h"
#include "cargopacket.h"
#include "company_func.h"
#include "company_base.h"
#include "window_func.h"
#include "vehicle_gui.h"
#include "vehicle_func.h"
#include "direction_func.h"
#include "map_func.h"
#include "multilayer_content.h"
#include "timer/timer_game_calendar.h"

#include "safeguards.h"

/** Spoil quantity assigned to one cargo part. */
struct TBMSpoilShare {
	Train *part = nullptr; ///< Dedicated wagon part receiving it.
	uint32_t tonnes = 0; ///< Whole tonnes assigned.
};

/** Everything one cut needs from the train, prepared before any write. */
struct TBMCutPreparation {
	std::vector<TBMSpoilShare> earth; ///< Earth shares in stable chain order.
	std::vector<TBMSpoilShare> rock; ///< Rock shares in stable chain order.
	size_t packets = 0; ///< Cargo packets the shares need.
	TBMSupportPlan support; ///< Support pieces of this cut.
	TileIndex work_site = INVALID_TILE; ///< Canonical loading coordinate: the cut's cutter position.
	Train *front = nullptr; ///< Cutter whose caches change.
};

/**
 * Free spoil space of one cargo part.
 * @param part Wagon part.
 * @return Free tonnes; none while a loading reservation is pending, because kept cargo may only join a quiet list.
 */
static uint32_t GetTBMPartSpace(const Train &part)
{
	if (part.cargo.ActionCount(VehicleCargoList::MoveToAction::Load) != 0) return 0;
	return part.cargo_cap > part.cargo.TotalCount() ? part.cargo_cap - part.cargo.TotalCount() : 0;
}

/**
 * Assign one spoil type to the wagons of its role, filling each part in stable chain order.
 * @param layout Validated consist layout.
 * @param role Earth or Rock wagon role.
 * @param tonnes Spoil to place.
 * @return The shares, or nothing when the wagons cannot hold it all.
 */
static std::optional<std::vector<TBMSpoilShare>> PlanTBMSpoil(const TBMConsistLayout &layout, TBMConsistRole role, uint64_t tonnes)
{
	std::vector<TBMSpoilShare> shares;
	// Earlier wagons fill first, so the same consist and spoil always give the same distribution.
	for (const auto &unit : layout.exchangeable_units) {
		if (tonnes == 0) break;
		if (unit.role != role) continue;
		Train *head = Train::Get(unit.vehicle);
		const Train *next_unit = head->GetNextUnit();
		// Every cargo part of the unit takes part, articulated parts included.
		for (Train *part = head; part != nullptr && part != next_unit && tonnes != 0; part = part->Next()) {
			const uint32_t take = static_cast<uint32_t>(std::min<uint64_t>(GetTBMPartSpace(*part), tonnes));
			if (take == 0) continue;
			shares.push_back({part, take});
			tonnes -= take;
		}
	}
	if (tonnes != 0) return std::nullopt;
	return shares;
}

/**
 * Real material snapshot of an assigned consist.
 * @param project Assigned project.
 * @return Snapshot; all zero when the cutter or consist is not live.
 */
ExcavationMaterialSnapshot CaptureTBMMaterial(const TBMProject &project)
{
	ExcavationMaterialSnapshot snapshot;
	const Train *front = Train::GetIfValid(project.cutter);
	if (front == nullptr) return snapshot;
	const auto layout = InspectTBMConsist(*front, project);
	if (layout.error != TBMBlockedReason::None) return snapshot;
	uint64_t stamp = 14695981039346656037ULL;
	/**
	 * Fold one value into the snapshot stamp.
	 * @param value Value to fold.
	 */
	const auto fold = [&stamp](uint64_t value) {
		for (uint i = 0; i < 8; ++i) {
			stamp ^= (value >> (i * 8)) & 255;
			stamp *= 1099511628211ULL;
		}
	};
	fold(project.id.base());
	fold(project.revision);
	// Space is counted per role and per part; the stamp also changes when cargo moves between parts of equal totals.
	for (const auto &unit : layout.exchangeable_units) {
		const Train *head = Train::Get(unit.vehicle);
		const Train *next_unit = head->GetNextUnit();
		for (const Train *part = head; part != nullptr && part != next_unit; part = part->Next()) {
			const uint32_t space = GetTBMPartSpace(*part);
			if (unit.role == TBMConsistRole::EarthWagon) {
				snapshot.earth_space_t += space;
			} else {
				snapshot.rock_space_t += space;
			}
			fold(part->index.base());
			fold(part->cargo.TotalCount());
			fold(space);
		}
	}
	// The permanent support parts install the lining, paid for by the work charge; they exist in every valid layout.
	snapshot.support_units = UINT32_MAX;
	snapshot.available_funds = GetAvailableMoney(project.company);
	fold(static_cast<uint64_t>(snapshot.available_funds.base()));
	snapshot.revision = stamp;
	return snapshot;
}

/**
 * @param a First snapshot.
 * @param b Second snapshot.
 * @return Whether every field matches.
 */
static bool IsSameTBMMaterial(const ExcavationMaterialSnapshot &a, const ExcavationMaterialSnapshot &b)
{
	return a.revision == b.revision && a.earth_space_t == b.earth_space_t && a.rock_space_t == b.rock_space_t && a.support_units == b.support_units && a.available_funds == b.available_funds;
}

/**
 * Prepare the train's side of one cut without changing anything.
 * @param project Assigned project.
 * @param packet_budget Most packets the commit may allocate.
 * @param expected Snapshot the geology owner quoted against.
 * @param delta Complete cut receipt to take part in.
 * @param[out] refusal Precise reason when this refuses, None otherwise.
 * @return The preparation, or nothing on refusal.
 */
static std::optional<TBMCutPreparation> PrepareTBMCutParticipation(const TBMProject &project, size_t packet_budget, const ExcavationMaterialSnapshot &expected, const ExcavationCutResult &delta, TBMBlockedReason &refusal)
{
	/**
	 * Record a refusal.
	 * @param reason Precise wait.
	 * @return Nothing.
	 */
	const auto fail = [&refusal](TBMBlockedReason reason) -> std::optional<TBMCutPreparation> {
		refusal = reason;
		return std::nullopt;
	};
	refusal = TBMBlockedReason::None;
	TBMCutPreparation prepared;
	// The train and the cash must still be exactly what the geology owner quoted against.
	if (!IsSameTBMMaterial(CaptureTBMMaterial(project), expected) || expected.revision == 0) return fail(TBMBlockedReason::InvalidConsist);
	prepared.front = Train::GetIfValid(project.cutter);
	if (prepared.front == nullptr) return fail(TBMBlockedReason::MissingCutter);
	const auto layout = InspectTBMConsist(*prepared.front, project);
	if (layout.error != TBMBlockedReason::None) return fail(TBMBlockedReason::InvalidConsist);
	const auto *job = project.job == ExcavationJobID{UINT32_MAX} ? nullptr : GetExcavationJob(project.job);
	if (job == nullptr || job->cursor >= job->route.size()) return fail(TBMBlockedReason::MissingProject);
	prepared.work_site = job->route[job->cursor].cutter_position;
	// The whole swept envelope must be free of other vehicles; only this consist's own parts may stand in it.
	for (TileIndex tile : job->route[job->cursor].swept_tiles) {
		/**
		 * @param v Vehicle on the tile.
		 * @return Whether it is anything but a part of this consist.
		 */
		const auto foreign = [&prepared](const Vehicle *v) { return v->type != VehicleType::Effect && v->First() != prepared.front; };
		if (HasVehicleOnTile(tile, foreign)) return fail(TBMBlockedReason::Obstruction);
	}

	/* Each spoil type fits its own wagons; neither can borrow the other's space. */
	auto earth = PlanTBMSpoil(layout, TBMConsistRole::EarthWagon, delta.earth_t);
	if (!earth.has_value()) return fail(TBMBlockedReason::EarthFull);
	auto rock = PlanTBMSpoil(layout, TBMConsistRole::RockWagon, delta.rock_t);
	if (!rock.has_value()) return fail(TBMBlockedReason::RockFull);
	prepared.earth = std::move(*earth);
	prepared.rock = std::move(*rock);
	if (delta.support_units > expected.support_units) return fail(TBMBlockedReason::MissingSupport);
	// A negative charge would be a refund, which a cut never pays.
	if (delta.cost < 0 || delta.cost > expected.available_funds) return fail(TBMBlockedReason::InsufficientFunds);

	/* Storage: the packet count and the support pieces are known now, before any write. */
	for (const auto *shares : {&prepared.earth, &prepared.rock}) {
		for (const auto &share : *shares) prepared.packets += CeilDiv(share.tonnes, CargoPacket::MAX_COUNT);
	}
	if (prepared.packets > packet_budget || !CargoPacket::CanAllocateItem(prepared.packets)) return fail(TBMBlockedReason::AllocationFailure);
	prepared.support = PrepareTBMSupport(project, *job);
	if (prepared.support.error != TBMBlockedReason::None) return fail(prepared.support.error);
	return prepared;
}

/** @return The actual consist's material, support and funds. */
ExcavationMaterialSnapshot TBMCutParticipant::Capture() const
{
	return CaptureTBMMaterial(this->project);
}

/**
 * @param expected Snapshot used by the pure preflight.
 * @param delta Complete proposed change.
 * @return Whether the train still accepts exactly this cut.
 */
bool TBMCutParticipant::Validate(const ExcavationMaterialSnapshot &expected, const ExcavationCutResult &delta) const
{
	return PrepareTBMCutParticipation(this->project, this->packet_budget, expected, delta, this->refusal).has_value();
}

/**
 * Commit the train's side of one cut: allocate every packet, then lay support, load the spoil and debit the charge.
 * @param expected Validated snapshot.
 * @param delta Complete receipt.
 * @return Success, or false with nothing changed.
 */
bool TBMCutParticipant::Commit(const ExcavationMaterialSnapshot &expected, const ExcavationCutResult &delta) noexcept
{
	auto prepared = PrepareTBMCutParticipation(this->project, this->packet_budget, expected, delta, this->refusal);
	if (!prepared.has_value()) return false;

	/* Storage first: every packet exists before any visible write, so nothing below can fail. */
	std::vector<std::pair<Train *, CargoPacket *>> loads;
	loads.reserve(prepared->packets);
	for (const auto *shares : {&prepared->earth, &prepared->rock}) {
		for (const auto &share : *shares) {
			// A share larger than one packet's count is split over several packets for the same part.
			for (uint32_t left = share.tonnes; left != 0;) {
				const uint16_t count = static_cast<uint16_t>(std::min<uint32_t>(left, CargoPacket::MAX_COUNT));
				loads.emplace_back(share.part, CargoPacket::Create(StationID::Invalid(), count, Source{Source::Invalid, SourceType::Industry}));
				left -= count;
			}
		}
	}

	/* Writes: support track, then the spoil in stable wagon order, then the work charge. */
	PlaceTBMSupport(this->project, prepared->support);
	// Each packet starts its journey at the work site and joins its wagon as kept cargo.
	for (auto &[part, packet] : loads) {
		packet->UpdateLoadingTile(prepared->work_site);
		part->cargo.Append(packet, VehicleCargoList::MoveToAction::Keep);
	}
	if (!loads.empty()) {
		// Paid spoil becomes physical train mass in this same atomic cut. Without rebuilding both caches,
		// movement uses empty-train physics until a later rearrangement, while loading a save uses the correct loaded mass.
		prepared->front->CargoChanged();
		prepared->front->UpdateAcceleration();
		// Visuals reflect the authoritative packets after the physical caches agree with their wagon contents.
		SetWindowDirty(GetWindowClassForVehicleType(VehicleType::Train), prepared->front->owner);
		SetWindowDirty(WindowClass::VehicleDetails, prepared->front->index);
		prepared->front->MarkDirty();
	}
	if (delta.cost != 0) SubtractMoneyFromCompany(this->project.company, CommandCost(ExpensesType::Construction, delta.cost));
	return true;
}

/**
 * Check that only this project advances the claim, and that its retreat runs on real support.
 * @param job Accepted job.
 * @param cut The next cut (its retreat follows from the job's cursor).
 * @return Whether the face may accumulate work.
 */
bool TBMCutParticipant::IsSupported(const ExcavationJob &job, [[maybe_unused]] const ExcavationCutGeometry &cut) const
{
	this->refusal = TBMBlockedReason::None;
	const TBMProject *assigned = GetTBMProjectForCutter(this->project.cutter);
	// The claim is this project's alone: its job and company, its cutter (or still unbound), its own store entry.
	if (job.id != this->project.job || job.company != this->project.company || (job.cutter != VehicleID::Invalid() && job.cutter != this->project.cutter) || assigned == nullptr || assigned->id != this->project.id) {
		this->refusal = TBMBlockedReason::MissingProject;
		return false;
	}
	// The loaded return runs back over every completed position and the exchange path; each cell needs usable rail.
	for (TileIndex tile : GetExcavationRetreatPath(job, job.cursor)) {
		if (!IsTBMSupportUsable(this->project, tile)) {
			this->refusal = TBMBlockedReason::MissingSupport;
			return false;
		}
	}
	return true;
}

/**
 * @param project Assigned project.
 * @return Whether the completed-cut count is an exchange breakpoint and the wagons carry spoil.
 */
bool IsTBMExchangeDue(const TBMProject &project)
{
	const auto *job = project.job == ExcavationJobID{UINT32_MAX} ? nullptr : GetExcavationJob(project.job);
	if (job == nullptr || !job->pass_terms.has_value()) return false;
	// Cancellation and requested parking must settle the real partial load;
	// neither requires another cut merely to reach a planned pass breakpoint.
	const bool cancelled = job->state == ExcavationJobState::Cancelled;
	if (!cancelled && !IsTBMParking(project.phase) && (job->cursor == 0 || !std::ranges::binary_search(job->pass_terms->exchange_after, static_cast<uint16_t>(job->cursor)))) return false;
	const Train *front = Train::GetIfValid(project.cutter);
	if (front == nullptr) return false;
	const auto layout = InspectTBMConsist(*front, project);
	// Only spoil actually aboard makes the exchange due; empty wagons after a real exchange do not.
	for (const auto &unit : layout.exchangeable_units) {
		const Train *head = Train::Get(unit.vehicle);
		const Train *next_unit = head->GetNextUnit();
		for (const Train *part = head; part != nullptr && part != next_unit; part = part->Next()) {
			if (part->cargo.TotalCount() != 0) return true;
		}
	}
	return false;
}

/**
 * Translate a geology refusal into the controller's saved wait.
 * @param error Refusal from the job, work or cut owner.
 * @return Precise wait; None for no refusal or incomplete work.
 */
static TBMBlockedReason GetTBMWorkWait(ExcavationError error)
{
	switch (error) {
		case ExcavationError::None:
		case ExcavationError::NoProgress:
			return TBMBlockedReason::None;
		case ExcavationError::MissingSupport:
			return TBMBlockedReason::MissingSupport;
		case ExcavationError::InsufficientFunds:
			return TBMBlockedReason::InsufficientFunds;
		case ExcavationError::InsufficientCapacity:
			return TBMBlockedReason::InvalidConsist; // Real space was checked first; only the frozen terms remain.
		case ExcavationError::LockedDeep:
		case ExcavationError::InvalidPolicy:
			return TBMBlockedReason::LockedModel;
		case ExcavationError::InvalidJob:
		case ExcavationError::AlreadyCompleted:
			return TBMBlockedReason::MissingProject;
		case ExcavationError::Limit:
		case ExcavationError::StaleParticipant:
			return TBMBlockedReason::AllocationFailure;
		default:
			return TBMBlockedReason::Obstruction; // Geometry, claims, ownership or infrastructure on the path.
	}
}

/**
 * Advance the face once.
 * @param project Live project in Bore.
 * @return What happened.
 */
TBMFaceWork AdvanceTBMFaceWork(const TBMProject &project)
{
	TBMFaceWork result;
	result.work_remainder = project.work_remainder;
	const auto *model = GetTBMCutterModel(project.model_id);
	const auto *job = project.job == ExcavationJobID{UINT32_MAX} ? nullptr : GetExcavationJob(project.job);
	if (model == nullptr || job == nullptr) {
		result.wait = TBMBlockedReason::MissingProject;
		return result;
	}
	// A company in bankruptcy starts no new work; what it already cut, loaded and supported stays untouched.
	const Company *company = Company::GetIfValid(project.company);
	if (company == nullptr || company->months_of_bankruptcy != 0) {
		result.wait = TBMBlockedReason::InsufficientFunds;
		return result;
	}
	// Spoil that cannot leave its haulers stops new cutting; the return, exchange and haul of what is loaded go on (Plan 5 Group 9.3).
	if (IsTBMLogisticsStalled(project)) {
		result.wait = TBMBlockedReason::LogisticsStall;
		return result;
	}
	// The model must still be allowed to cut on this layer today: profile, introduction and the Deep calendar gate.
	if (!IsTBMCutterAvailable(project.model_id, GetTileLayer(job->commissioning_anchor), GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) {
		result.wait = TBMBlockedReason::LockedModel;
		return result;
	}
	const auto estimate = EstimateExcavationCut(project.job);
	if (estimate.error != ExcavationError::None) {
		result.wait = GetTBMWorkWait(estimate.error);
		return result;
	}
	// A cut larger than the frozen wagon terms can never fit this train, however much is unloaded.
	if (!job->pass_terms.has_value() || estimate.earth_t > job->pass_terms->earth_capacity_t || estimate.rock_t > job->pass_terms->rock_capacity_t) {
		result.wait = TBMBlockedReason::InvalidConsist;
		return result;
	}
	const auto &cut = job->route[job->cursor];
	const TileIndex approach = job->cursor == 0 ? job->commissioning_anchor : job->route[job->cursor - 1].cutter_position;
	const auto step = TileIndexDiffCByDir(cut.heading);
	// The cutter only advances: one tile from the face, straight or 45 degrees off its saved heading. A sharper or
	// stationary step can never turn it, however often it is requested.
	if (!IsValidDirection(cut.heading) || !IsValidDirection(project.heading) || TileAddWrap(approach, step.x, step.y) != cut.cutter_position) {
		result.wait = TBMBlockedReason::IllegalTurn;
		return result;
	}
	const DirDiff turn = DirDifference(cut.heading, project.heading);
	if (turn != DirDiff::Same && turn != DirDiff::Left45 && turn != DirDiff::Right45) {
		result.wait = TBMBlockedReason::IllegalTurn;
		return result;
	}
	TBMCutParticipant participant(project);

	/* Work: rate-limited and never past this cut's requirement, so a waiting face cannot bank work for the next cut. */
	const uint64_t remaining = estimate.work_ticks > job->partial_work_ticks ? estimate.work_ticks - job->partial_work_ticks : 0;
	if (remaining > 0) {
		const uint32_t total = uint32_t{model->work_rate_numerator} + project.work_remainder;
		const uint64_t work = total / model->work_rate_denominator;
		const uint64_t add = std::min(work, remaining);
		if (add != 0) {
			const auto error = AdvanceExcavationWork(project.job, add, participant);
			if (error != ExcavationError::None) {
				// Nothing was worked, so the fraction is kept as well.
				result.wait = participant.GetRefusal() != TBMBlockedReason::None ? participant.GetRefusal() : GetTBMWorkWait(error);
				return result;
			}
			result.work_added = add;
		}
		result.work_remainder = static_cast<uint16_t>(total % model->work_rate_denominator);
		if (add < remaining) return result;
	}

	/* Boundary: precise waits from the estimate in the geology owner's order, then one complete commit. */
	const auto material = participant.Capture();
	if (estimate.earth_t > material.earth_space_t) {
		result.wait = TBMBlockedReason::EarthFull;
	} else if (estimate.rock_t > material.rock_space_t) {
		result.wait = TBMBlockedReason::RockFull;
	} else if (estimate.support_units > material.support_units) {
		result.wait = TBMBlockedReason::MissingSupport;
	} else if (estimate.cost > material.available_funds) {
		result.wait = TBMBlockedReason::InsufficientFunds;
	}
	if (result.wait != TBMBlockedReason::None) return result;
	const auto *current = GetExcavationJob(project.job);
	const auto receipt = CommitExcavationCut({project.job, current->revision, current->cursor}, participant);
	if (receipt.error != ExcavationError::None) {
		result.wait = participant.GetRefusal() != TBMBlockedReason::None ? participant.GetRefusal() : GetTBMWorkWait(receipt.error);
		return result;
	}
	result.cut_committed = true;
	result.heading = cut.heading; // The cutter now faces along the cut it opened.
	result.receipt = receipt;
	return result;
}

/**
 * Hold the complete native train when work or unsafe project travel owns its motion.
 * @param front Actual front engine; a real depot order retains ordinary service travel.
 * @return Whether both native locomotive substeps must leave every attached part stationary.
 */
bool IsTBMHoldingTrain(const Train &front)
{
	const TBMProject *project = GetTBMProjectForCutter(front.First()->index);
	// A service/depot order safely interrupts work before the native train
	// step. Holding Bore unconditionally would strand that legitimate visit.
	if (project == nullptr) return false;
	if (GetTBMTravelHoldReason(front) != TBMBlockedReason::None) return true;
	if (front.current_order.IsType(OT_GOTO_DEPOT) && !IsTBMStagingHangarOrder(front)) return false;
	// The cutter stands still while it cuts, while it waits for a hauler at its face or in its bay, and while stored.
	return project->phase == TBMPhase::Bore || IsTBMAwaitingFaceTransfer(project->phase) || IsTBMWaitingForExchange(project->phase) || IsTBMStored(project->phase);
}
