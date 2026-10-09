/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file layer_portal_gui.cpp The pair window: native pair status, validated ordinary connection controls, the
 * commissioning purchase with its corridor length, and the player's controls of the boring project working through
 * the pair (R2MLVS3MP master 7.9): its machine, hauler, retirement, the withdrawn machine's sale and the conversion of
 * its temporary track. What the window offers comes from GetTBMPairControls, so it never offers a refused step.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Groups 8/13
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 2 and tasks 16.1/16.3
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs5-spoil-wagons-and-haulage.md, task 13.4
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs7-underground-construction-and-fleet.md, shared area-control defaults
 */

#include "stdafx.h"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "map_layer_gui.h"
#include "widgets/layer_portal_widget.h"
#include "company_func.h"
#include "company_base.h"
#include "command_func.h"
#include "window_func.h"
#include "window_gui.h"
#include "tbm.h"
#include "tbm_cmd.h"
#include "tbm_controls.h"
#include "tbm_gui.h"
#include "station_base.h"
#include "cargotype.h"
#include "map_func.h"
#include "multilayer_content.h"
#include "multilayer_sprite.h"
#include "train.h"
#include "vehicle_cmd.h"
#include "vehicle_gui.h"
#include "gfx_func.h"
#include "zoom_func.h"
#include "strings_func.h"
#include "textbuf_gui.h"
#include "tilehighlight_func.h"
#include "error.h"
#include "timer/timer.h"
#include "timer/timer_window.h"
#include "timer/timer_game_calendar.h"
#include "table/strings.h"
#include "table/sprites.h"
#include "safeguards.h"

/** Shortest corridor the commissioning control asks for, in cells beyond the housing. */
static constexpr int TBM_COMMISSION_MIN_CORRIDOR = 2;
/** Longest corridor the commissioning control asks for; the command's area stays within its 64-tile bound. */
static constexpr int TBM_COMMISSION_MAX_CORRIDOR = 60;
/** Hoppers of each kind the window's commissioning control buys. */
/** Corridor length the next commissioning asks for; kept for the session, shared by every pair window. */
static int _tbm_commission_corridor = 6;

/**
 * Choose the cutter the commissioning control buys for a pair.
 * @param tile Either actual pair endpoint; its upper end assembles and its lower end works.
 * @return The first catalogue cutter commissioned at a rail portal that serves both ends and is available today, or
 *         UINT16_MAX when there is none.
 */
uint16_t GetLayerPortalDefaultTBMModel(TileIndex tile)
{
	const auto pair = GetLayerPortalPairKey(tile);
	if (!pair) return UINT16_MAX;
	const TileIndex key = *pair;
	if (GetLayerPortalTransportType(key) != TransportType::Rail) return UINT16_MAX;
	const MapLayer work = GetTileLayer(GetLayerPortalPairTile(key));
	// Catalogue ordering selects the first compatible available cutter without inventing another model table.
	for (const auto &model : GetMultiLayerVehicleCapabilities()) {
		// Only rail-portal cutters that serve both layers qualify; the command validates the rest again.
		if (model.commissioning != MultiLayerCommissioningFacility::RailPortalBay || !model.supported_layers.Test(GetTileLayer(key)) || !model.supported_layers.Test(work)) continue;
		if (IsTBMCutterAvailable(model.model_id, work, GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return model.model_id;
	}
	return UINT16_MAX;
}

/**
 * @param st Station.
 * @return Whether its catchment has receivers for both Earth and Rock, as a project origin needs.
 */
static bool DoesStationAcceptSpoil(const Station &st)
{
	for (const CargoLabel label : {CargoLabel{"ERTH"}, CargoLabel{"ROCK"}}) {
		const CargoType cargo = GetCargoTypeByLabel(label);
		if (!IsValidCargoType(cargo) || !st.goods[cargo].status.Test(GoodsEntry::State::Acceptance)) return false;
	}
	return true;
}

/**
 * Choose the origin the commissioning control names for a pair.
 * @param tile Actual pair endpoint.
 * @param company Actual requesting company.
 * @return The company's Surface railway station nearest the pair among those whose catchment takes both Earth
 *         and Rock; failing any such station, the nearest one, so the command names what is missing. Invalid if none.
 */
StationID GetLayerPortalDefaultTBMOrigin(TileIndex tile, CompanyID company)
{
	const auto pair = GetLayerPortalPairKey(tile);
	// Missing geometry or ownership cannot choose an unrelated company's spoil origin.
	if (!pair || !Company::IsValidID(company)) return StationID::Invalid();
	const TileIndex key = *pair;
	StationID best = StationID::Invalid();
	bool best_accepts = false;
	uint best_distance = UINT_MAX;
	// Find a real owned Surface disposal origin, preferring actual acceptance before geographical proximity.
	for (const Station *st : Station::Iterate()) {
		// Loaded spoil always goes to the company's own Surface railway station first.
		if (st->owner != company || !st->facilities.Test(StationFacility::Train) || GetTileLayer(st->xy) != MapLayer::Surface) continue;
		const bool accepts = DoesStationAcceptSpoil(*st);
		const uint distance = DistanceManhattan(st->xy, key);
		// A station with both receivers beats any without; among equals the nearer one wins.
		if ((accepts && !best_accepts) || (accepts == best_accepts && distance < best_distance)) {
			best = st->index;
			best_accepts = accepts;
			best_distance = distance;
		}
	}
	return best;
}

/**
 * @param phase Project phase.
 * @return Its short description for the progress line.
 */
StringID GetTBMPhaseString(TBMPhase phase)
{
	switch (phase) {
		case TBMPhase::Commissioning: return STR_PORTAL_PHASE_COMMISSIONING;
		case TBMPhase::TravelToFace: return STR_PORTAL_PHASE_TRAVEL;
		case TBMPhase::Bore: return STR_PORTAL_PHASE_BORE;
		case TBMPhase::AdvanceSupport: return STR_PORTAL_PHASE_ADVANCE;
		case TBMPhase::ReturnForExchange: return STR_PORTAL_PHASE_RETURN;
		case TBMPhase::WaitForEmpties: return STR_PORTAL_PHASE_WAIT;
		case TBMPhase::Resume: return STR_PORTAL_PHASE_RESUME;
		case TBMPhase::Completed: return STR_PORTAL_PHASE_COMPLETED;
		case TBMPhase::ReturnToPark: return STR_TBM_PHASE_RETURN_PARK;
		case TBMPhase::WaitToPark: return STR_TBM_PHASE_WAIT_PARK;
		case TBMPhase::Parked: return STR_TBM_PHASE_PARKED;
		case TBMPhase::ReturnToRelease: return STR_TBM_PHASE_RETURN_RELEASE;
		case TBMPhase::WaitToRelease: return STR_TBM_PHASE_WAIT_RELEASE;
		case TBMPhase::ReleasePending: return STR_TBM_PHASE_RELEASE_PENDING;
		case TBMPhase::ReturnForHangar: return STR_TBM_PHASE_RETURN_HANGAR;
		case TBMPhase::WaitForHangar: return STR_TBM_PHASE_UNLOAD_HANGAR;
		case TBMPhase::TravelToHangar: return STR_TBM_PHASE_TRAVEL_HANGAR;
		case TBMPhase::ParkedHangar: return STR_TBM_PHASE_PARKED_HANGAR;
		case TBMPhase::FollowOrders: return STR_TBM_PHASE_FOLLOW_ORDERS;
		default: return STR_PORTAL_PHASE_CANCELLED;
	}
}

/**
 * @param reason Saved hold of a project, not None.
 * @return Its description for the hold line.
 */
StringID GetTBMHoldString(TBMBlockedReason reason)
{
	switch (reason) {
		case TBMBlockedReason::Stopped: return STR_PORTAL_HOLD_STOPPED;
		case TBMBlockedReason::Breakdown: return STR_PORTAL_HOLD_BREAKDOWN;
		case TBMBlockedReason::Servicing: return STR_PORTAL_HOLD_SERVICING;
		case TBMBlockedReason::MissingCutter: return STR_PORTAL_HOLD_MISSING_CUTTER;
		case TBMBlockedReason::MissingOrigin: return STR_PORTAL_HOLD_MISSING_ORIGIN;
		case TBMBlockedReason::MissingPortal: return STR_PORTAL_HOLD_MISSING_PORTAL;
		case TBMBlockedReason::MissingExchange: return STR_PORTAL_HOLD_MISSING_EXCHANGE;
		case TBMBlockedReason::InvalidConsist: return STR_PORTAL_HOLD_INVALID_CONSIST;
		case TBMBlockedReason::LockedModel: return STR_PORTAL_HOLD_LOCKED_MODEL;
		case TBMBlockedReason::MissingSupport: return STR_PORTAL_HOLD_MISSING_SUPPORT;
		case TBMBlockedReason::EarthFull: return STR_PORTAL_HOLD_EARTH_FULL;
		case TBMBlockedReason::RockFull: return STR_PORTAL_HOLD_ROCK_FULL;
		case TBMBlockedReason::InsufficientFunds: return STR_PORTAL_HOLD_FUNDS;
		case TBMBlockedReason::Obstruction: return STR_PORTAL_HOLD_OBSTRUCTION;
		case TBMBlockedReason::AllocationFailure: return STR_PORTAL_HOLD_ALLOCATION;
		case TBMBlockedReason::Paused: return STR_PORTAL_HOLD_PAUSED;
		case TBMBlockedReason::IllegalTurn: return STR_PORTAL_HOLD_ILLEGAL_TURN;
		case TBMBlockedReason::LogisticsStall: return STR_PORTAL_HOLD_LOGISTICS;
		case TBMBlockedReason::MissingHangar: return STR_ERROR_TBM_HANGAR_MISSING;
		case TBMBlockedReason::OccupiedHangar: return STR_ERROR_TBM_HANGAR_OCCUPIED;
		case TBMBlockedReason::MissingOrders: return STR_TBM_HOLD_MISSING_ORDERS;
		default: return STR_PORTAL_HOLD_MISSING_PROJECT;
	}
}

/**
 * @param phase Hauler cycle phase.
 * @return Its description for the hauler line.
 */
StringID GetTBMHaulerPhaseString(TBMHaulerPhase phase)
{
	switch (phase) {
		case TBMHaulerPhase::ReturnEmpties: return STR_PORTAL_HAULER_RETURN_EMPTIES;
		case TBMHaulerPhase::AwaitExchange: return STR_PORTAL_HAULER_AWAIT_EXCHANGE;
		case TBMHaulerPhase::CollectLoaded: return STR_PORTAL_HAULER_COLLECT_LOADED;
		case TBMHaulerPhase::VisitOrigin: return STR_PORTAL_HAULER_VISIT_ORIGIN;
		default: return STR_PORTAL_HAULER_DISPOSE;
	}
}

/**
 * @param wait Hauler hold, not None.
 * @return Its description for the hauler line.
 */
StringID GetTBMHaulerWaitString(TBMHaulerWait wait)
{
	switch (wait) {
		case TBMHaulerWait::Stopped: return STR_PORTAL_WAIT_STOPPED;
		case TBMHaulerWait::Crashed: return STR_PORTAL_WAIT_CRASHED;
		case TBMHaulerWait::LostProject: return STR_PORTAL_WAIT_LOST_PROJECT;
		case TBMHaulerWait::LostOrigin: return STR_PORTAL_WAIT_LOST_ORIGIN;
		case TBMHaulerWait::WrongWagons: return STR_PORTAL_WAIT_WRONG_WAGONS;
		case TBMHaulerWait::NoEmpties: return STR_PORTAL_WAIT_NO_EMPTIES;
		case TBMHaulerWait::OccupiedBay: return STR_PORTAL_WAIT_OCCUPIED_BAY;
		case TBMHaulerWait::SaturatedReceiver: return STR_PORTAL_WAIT_SATURATED;
		default: return STR_PORTAL_WAIT_BLOCKED_LINE;
	}
}

/**
 * @param step Next step of the pair.
 * @return Its description for the next-step line.
 */
static StringID GetTBMPlayerStepString(TBMPlayerStep step)
{
	switch (step) {
		case TBMPlayerStep::Commission: return STR_PORTAL_STEP_COMMISSION;
		case TBMPlayerStep::StartMachine: return STR_PORTAL_STEP_START_MACHINE;
		case TBMPlayerStep::Working: return STR_PORTAL_STEP_WORKING;
		case TBMPlayerStep::Resume: return STR_PORTAL_STEP_RESUME;
		case TBMPlayerStep::AssignHauler: return STR_PORTAL_STEP_ASSIGN_HAULER;
		case TBMPlayerStep::FixHold: return STR_PORTAL_STEP_FIX_HOLD;
		case TBMPlayerStep::DeliverLast: return STR_PORTAL_STEP_DELIVER_LAST;
		case TBMPlayerStep::WaitForHauler: return STR_PORTAL_STEP_WAIT_FOR_HAULER;
		case TBMPlayerStep::ReleaseHauler: return STR_PORTAL_STEP_RELEASE_HAULER;
		case TBMPlayerStep::StopMachine: return STR_PORTAL_STEP_STOP_MACHINE;
		case TBMPlayerStep::Retire: return STR_PORTAL_STEP_RETIRE;
		case TBMPlayerStep::SellMachine: return STR_PORTAL_STEP_SELL_MACHINE;
		case TBMPlayerStep::ConvertTrack: return STR_PORTAL_STEP_CONVERT_TRACK;
		case TBMPlayerStep::Connect: return STR_PORTAL_STEP_CONNECT;
		default: return STR_PORTAL_STEP_CONNECTED;
	}
}

/** Lines of the project panel. */
static constexpr int PORTAL_INFO_LINES = 11;

/** One status/control window per canonical pair, independent of the currently selected viewport layer. */
class LayerPortalWindow : public Window {
	TileIndex pair_key; ///< Canonical pair key: the upper end; never a camera-relative address or duplicate work state.
	TBMPairControls controls; ///< What the pair offers, refreshed at every paint and click.

	/**
	 * Choose the purchase picture without changing the actual commissioning model/date query.
	 * @return Current project/available cutter, or the first compatible future catalogue design for a locked preview.
	 */
	uint16_t GetPurchasePreviewModel() const
	{
		if (this->controls.project) return this->controls.project->model_id;
		const uint16_t available = GetLayerPortalDefaultTBMModel(this->pair_key);
		if (available != UINT16_MAX) return available;
		const auto pair = GetLayerPortalPairKey(this->pair_key);
		if (!pair) return UINT16_MAX;
		const MapLayer upper = GetTileLayer(*pair);
		const MapLayer lower = GetTileLayer(GetLayerPortalPairTile(*pair));
		// The catalogue owns compatibility. A future compatible design supplies
		// only a locked UI picture; the existing command still refuses its purchase.
		for (const auto &model : GetMultiLayerVehicleCapabilities()) {
			// A portal preview must serve both real endpoints and be rail-origin
			// machinery; freight/capsule pictures would suggest a wrong purchase path.
			if (model.commissioning == MultiLayerCommissioningFacility::RailPortalBay &&
				model.supported_layers.Test(upper) && model.supported_layers.Test(lower)) return model.model_id;
		}
		return UINT16_MAX;
	}

	/** Refresh the window every second, so the project's progress shows while the game runs. */
	const IntervalTimer<TimerWindow> refresh_interval = {std::chrono::seconds(1), [this](auto) { this->SetDirty(); }};

	/** @return The vehicle the Machine button opens: the project's cutter, or the withdrawn train in the portal. */
	const Train *GetMachine() const
	{
		if (this->controls.project.has_value()) return Train::GetIfValid(this->controls.project->cutter);
		return Train::GetIfValid(this->controls.sellable);
	}

	/**
	 * Confirm a cancellation, then post it for the project the window shows at that moment.
	 * @param w The pair window that asked.
	 * @param confirmed Whether the player confirmed.
	 */
	static void CancelConfirmationCallback(Window *w, bool confirmed)
	{
		if (!confirmed) return;
		LayerPortalWindow *window = static_cast<LayerPortalWindow *>(w);
		window->controls = GetTBMPairControls(window->pair_key, _local_company);
		if (!window->controls.project.has_value()) return;
		Command<Commands::ControlTBMProject>::Post(STR_ERROR_CAN_T_CONTROL_TBM_PROJECT, window->controls.project->id, TBMProjectAction::Cancel);
	}

public:
	/** @param desc Actual registered widget/window descriptor. @param key Actual canonical pair identity. */
	LayerPortalWindow(WindowDesc &desc, TileIndex key) : Window(desc), pair_key(key)
	{
		this->controls = GetTBMPairControls(key, _local_company);
		this->InitNested(key.base());
	}

	/**
	 * @param widget Widget to size.
	 * @param size Size to fill in.
	 * @param padding Padding the widget gets.
	 * @param fill Unused fill step.
	 * @param resize Unused resize step.
	 */
	void UpdateWidgetSize(WidgetID widget, Dimension &size, const Dimension &padding, [[maybe_unused]] Dimension &fill, [[maybe_unused]] Dimension &resize) override
	{
		if (widget == PW_PREVIEW) {
			size.height = std::max<uint>(size.height, ScaleGUITrad(64) + GetCharacterHeight(FontSize::Normal) + padding.height);
		}
		if (widget == PW_COMMISSION) {
			// Measure both availability states so a future date or temporary owner
			// refusal changes only the cue, without clipping or moving the controls.
			for (const bool available : {false, true}) {
				if (const auto icon = GetMultiLayerPurchaseSprite(this->GetPurchasePreviewModel(), available)) {
					const Dimension picture = GetSquareScaledSpriteSize(*icon);
					const Dimension caption = GetStringBoundingBox(STR_PORTAL_COMMISSION);
					size.width = std::max(size.width, caption.width + 2 * (picture.width + WidgetDimensions::scaled.hsep_wide) + padding.width);
					size.height = std::max(size.height, std::max(picture.height, caption.height) + padding.height);
				}
			}
		}
		if (widget != PW_INFO) return;
		size.height = std::max<uint>(size.height, PORTAL_INFO_LINES * GetCharacterHeight(FontSize::Normal) + padding.height);
	}

	/**
	 * @param widget Widget whose text is wanted.
	 * @param stringid Its default string.
	 * @return The text with its live parameters.
	 */
	std::string GetWidgetString(WidgetID widget, StringID stringid) const override
	{
		switch (widget) {
			case PW_CORRIDOR: return GetString(STR_PORTAL_CORRIDOR, _tbm_commission_corridor);
			case PW_START_STOP: return GetString(this->controls.running ? STR_PORTAL_STOP_MACHINE : STR_PORTAL_START_MACHINE);
			case PW_PAUSE: return GetString(this->controls.resume ? STR_PORTAL_RESUME : STR_PORTAL_PAUSE);
			default: return this->Window::GetWidgetString(widget, stringid);
		}
	}

	/**
	 * Draw the project panel: progress, hold, spoil dug, hauler and the next step, one line each.
	 * @param r Panel rectangle.
	 * @param widget Widget to draw.
	 */
	void DrawWidget(const Rect &r, WidgetID widget) const override
	{
		if (widget == PW_PREVIEW) {
			const std::array endpoints{this->pair_key, GetLayerPortalPairTile(this->pair_key)};
			// Both actual world endpoints keep their independent layer, door and
			// saved phase. The current camera or a preview cannot open the pair.
			for (size_t endpoint = 0; endpoint < endpoints.size(); ++endpoint) {
				const TileIndex tile = endpoints[endpoint];
				const Rect half = r.WithWidth(r.Width() / 2, endpoint != 0);
				DrawLayerPortalPreview(half.left + half.Width() / 2, half.top + ScaleSpriteTrad(33),
					GetLayerPortalKind(tile), GetLayerPortalDoorDirection(tile), MapLayer::Invalid, tile);
				DrawString(half.left, half.right, half.bottom - GetCharacterHeight(FontSize::Normal) + 1, GetMapLayerName(GetTileLayer(tile)), TextColour::Black, AlignmentH::Centre);
			}
			return;
		}
		if (widget != PW_INFO) return;
		Rect tr = r.Shrink(WidgetDimensions::scaled.framerect);
		const int line = GetCharacterHeight(FontSize::Normal);
		const TBMPairControls &c = this->controls;
		if (c.project.has_value()) {
			const TBMProject &project = *c.project;
			DrawString(tr, GetString(STR_PORTAL_PROJECT_PROGRESS, GetTBMPhaseString(project.phase), c.cuts_done, c.cuts_total));
			tr.top += line;
			DrawString(tr, c.work.blocked == TBMBlockedReason::None ? GetString(STR_PORTAL_PROJECT_NO_HOLD) : GetString(STR_PORTAL_PROJECT_HOLD, GetTBMHoldString(c.work.blocked)));
			tr.top += line;
			if (c.hardness.has_value()) {
				DrawString(tr, GetString(STR_PORTAL_PROJECT_FACE, GetMapLayerName(GetTileLayer(c.work.face)), TileX(c.work.face), TilePlaneY(c.work.face), GetGeologyHardnessString(*c.hardness)));
			} else {
				DrawString(tr, GetString(STR_PORTAL_PROJECT_NO_FACE));
			}
			tr.top += line;
			DrawString(tr, c.pass_total == 0 ? GetString(STR_PORTAL_PROJECT_NO_PASS) : GetString(STR_PORTAL_PROJECT_PASS, c.pass_number, c.pass_total, c.pass_cuts_done, c.pass_cuts_total));
			tr.top += line;
			DrawString(tr, GetString(STR_PORTAL_PROJECT_FACE_WORK, c.work.partial_work_ticks, c.face_work_ticks));
			tr.top += line;
			DrawString(tr, GetString(STR_PORTAL_PROJECT_EARTH_CAPACITY, c.earth_loaded_t, c.earth_capacity_t));
			tr.top += line;
			DrawString(tr, GetString(STR_PORTAL_PROJECT_ROCK_CAPACITY, c.rock_loaded_t, c.rock_capacity_t));
			tr.top += line;
			DrawString(tr, GetString(STR_PORTAL_PROJECT_RUNNING_COST, c.active_cost_per_year, c.idle_cost_per_year));
			tr.top += line;
			DrawString(tr, GetString(STR_PORTAL_PROJECT_DUG, c.earth_t, c.rock_t));
			tr.top += line;
			// A hauler shows its phase and any wait; without one, the line says what a hauler needs.
			if (!c.hauler.has_value()) {
				DrawString(tr, GetString(STR_PORTAL_HAULER_NONE, c.earth_hoppers, c.rock_hoppers, project.origin));
			} else if (c.hauler->wait == TBMHaulerWait::None) {
				DrawString(tr, GetString(STR_PORTAL_HAULER, c.hauler->hauler, GetTBMHaulerPhaseString(c.hauler->phase)));
			} else {
				DrawString(tr, GetString(STR_PORTAL_HAULER_WAITING, c.hauler->hauler, GetTBMHaulerPhaseString(c.hauler->phase), GetTBMHaulerWaitString(c.hauler->wait)));
			}
		} else {
			DrawString(tr, GetString(STR_PORTAL_PROJECT_NONE));
			tr.top += line * (PORTAL_INFO_LINES - 2);
		}
		tr.top += line;
		DrawString(tr, GetString(STR_PORTAL_NEXT, GetTBMPlayerStepString(c.step)));
	}

	/** Paint saved status and refresh pure owner/date/floor/occupancy permission without publishing any gameplay state. */
	void OnPaint() override
	{
		if (!GetLayerPortalPairKey(this->pair_key)) { this->Close(); return; }
		StringID state = STR_PORTAL_STATE_SEALED;
		switch (GetLayerPortalWorkState(this->pair_key)) {
			case LayerPortalWorkState::Commissioning: state = STR_PORTAL_STATE_COMMISSIONING; break;
			case LayerPortalWorkState::Connected: state = STR_PORTAL_STATE_CONNECTED; break;
			default: break;
		}
		if (CheckLayerPortalAvailability(this->pair_key) == LayerPortalWorkError::LockedDeep) state = STR_ERROR_EXCAVATION_DEEP_LOCKED;
		this->GetWidget<NWidgetCore>(PW_STATUS)->SetStringTip(state, STR_PORTAL_STATUS_TOOLTIP);
		this->SetWidgetDisabledState(PW_CONNECT, ChangeLayerPortalConnection(this->pair_key, _local_company, true, false) != LayerPortalWorkError::None);
		this->SetWidgetDisabledState(PW_SEAL, ChangeLayerPortalConnection(this->pair_key, _local_company, false, false) != LayerPortalWorkError::None);
		const auto *work = GetLayerPortalWorkRecord(this->pair_key);
		// Only the local company's empty Sealed rail pair can commission; the command checks everything else and prices it.
		const bool can_commission = work != nullptr && work->state == LayerPortalWorkState::Sealed && work->company == _local_company && GetLayerPortalTransportType(this->pair_key) == TransportType::Rail && CheckLayerPortalAvailability(this->pair_key) == LayerPortalWorkError::None && GetLayerPortalDefaultTBMModel(this->pair_key) != UINT16_MAX && GetLayerPortalDefaultTBMOrigin(this->pair_key, _local_company) != StationID::Invalid();
		this->SetWidgetDisabledState(PW_COMMISSION, !can_commission);
		this->GetWidget<NWidgetCore>(PW_COMMISSION)->SetSprite(GetMultiLayerPurchaseSprite(this->GetPurchasePreviewModel(), can_commission).value_or(SPR_CURSOR_QUERY));
		this->SetWidgetDisabledState(PW_CORRIDOR_DOWN, _tbm_commission_corridor <= TBM_COMMISSION_MIN_CORRIDOR);
		this->SetWidgetDisabledState(PW_CORRIDOR_UP, _tbm_commission_corridor >= TBM_COMMISSION_MAX_CORRIDOR);

		// Every project control follows the commands' own estimates, refreshed for this paint.
		this->controls = GetTBMPairControls(this->pair_key, _local_company);
		const TBMPairControls &c = this->controls;
		this->SetWidgetDisabledState(PW_SHOW_MACHINE, this->GetMachine() == nullptr);
		this->SetWidgetDisabledState(PW_START_STOP, !c.project.has_value() || Train::GetIfValid(c.project->cutter) == nullptr);
		this->SetWidgetDisabledState(PW_PAUSE, !c.pause && !c.resume);
		this->SetWidgetDisabledState(PW_CANCEL, !c.cancel);
		this->SetWidgetDisabledState(PW_RETIRE, !c.retire);
		this->SetWidgetDisabledState(PW_ASSIGN, !c.assign);
		this->SetWidgetDisabledState(PW_RELEASE, !c.release);
		this->SetWidgetDisabledState(PW_SELL, c.sellable == VehicleID::Invalid());
		this->SetWidgetDisabledState(PW_CONVERT, c.convertible.empty());
		// A pick in progress ends if the project stops taking haulers.
		if (this->IsWidgetLowered(PW_ASSIGN) && !c.assign) ResetObjectToPlace();
		this->DrawWidgets();
	}

	/** @param pt Unused click coordinates. @param widget Actual selected control. @param click_count Unused repeated-click count. */
	void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		this->controls = GetTBMPairControls(this->pair_key, _local_company);
		const TBMPairControls &c = this->controls;
		switch (widget) {
			case PW_CONNECT:
			case PW_SEAL:
				// Both controls submit the same synchronized backend; UI estimates never grant a private routing or excavation permission.
				Command<Commands::ChangeLayerPortalConnection>::Post(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL, this->pair_key, widget == PW_CONNECT);
				break;

			case PW_COMMISSION: {
				// A straight corridor out of the far door, planned, validated and priced by the command for the actual consist.
				const TileIndex rock = GetLayerPortalPairTile(this->pair_key);
				const auto step = TileIndexDiffCByDiagDir(GetLayerPortalDoorDirection(rock));
				const TileIndex start = TileAddWrap(rock, step.x, step.y);
				const TileIndex end = TileAddWrap(rock, step.x * _tbm_commission_corridor, step.y * _tbm_commission_corridor);
				if (!IsValidTile(start) || !IsValidTile(end)) {
					ShowErrorMessage(GetEncodedString(STR_ERROR_CAN_T_COMMISSION_TBM), GetEncodedString(STR_ERROR_TBM_COMMISSION_AREA), WarningLevel::Error);
					break;
				}
				const auto consist = GetDefaultTBMConsist(GetTileLayer(rock));
				Command<Commands::CommissionTBM>::Post(STR_ERROR_CAN_T_COMMISSION_TBM, this->pair_key, GetLayerPortalDefaultTBMModel(this->pair_key), consist.earth_wagons, consist.rock_wagons, GetLayerPortalDefaultTBMOrigin(this->pair_key, _local_company), start, end, false);
				break;
			}

			case PW_CORRIDOR_DOWN:
			case PW_CORRIDOR_UP: {
				// Ctrl steps by five; the length stays within the command's bound.
				const int change = (_ctrl_pressed ? 5 : 1) * (widget == PW_CORRIDOR_UP ? 1 : -1);
				_tbm_commission_corridor = std::clamp(_tbm_commission_corridor + change, TBM_COMMISSION_MIN_CORRIDOR, TBM_COMMISSION_MAX_CORRIDOR);
				break;
			}

			case PW_SHOW_MACHINE:
				if (const Train *machine = this->GetMachine(); machine != nullptr) ShowVehicleViewWindow(machine);
				break;

			case PW_START_STOP:
				if (c.project.has_value()) {
					if (const Train *cutter = Train::GetIfValid(c.project->cutter); cutter != nullptr) StartStopVehicle(cutter, true);
				}
				break;

			case PW_PAUSE:
				if (c.project.has_value()) Command<Commands::ControlTBMProject>::Post(STR_ERROR_CAN_T_CONTROL_TBM_PROJECT, c.project->id, c.resume ? TBMProjectAction::Resume : TBMProjectAction::Pause);
				break;

			case PW_CANCEL:
				ShowQuery(GetEncodedString(STR_PORTAL_CANCEL_QUERY_CAPTION), GetEncodedString(STR_PORTAL_CANCEL_QUERY_TEXT), this, CancelConfirmationCallback);
				break;

			case PW_RETIRE:
				if (c.project.has_value()) Command<Commands::ControlTBMProject>::Post(STR_ERROR_CAN_T_CONTROL_TBM_PROJECT, c.project->id, TBMProjectAction::Retire);
				break;

			case PW_ASSIGN:
				// The next train the player clicks, in a view, a depot or a vehicle list, becomes the hauler.
				this->ToggleWidgetLoweredState(PW_ASSIGN);
				if (this->IsWidgetLowered(PW_ASSIGN)) {
					SetObjectToPlaceWnd(SPR_CURSOR_MOUSE, PAL_NONE, HT_VEHICLE, this);
				} else {
					ResetObjectToPlace();
				}
				break;

			case PW_RELEASE:
				if (c.hauler.has_value()) Command<Commands::ReleaseTBMHauler>::Post(STR_ERROR_CAN_T_RELEASE_TBM_HAULER, c.hauler->hauler);
				break;

			case PW_SELL:
				if (const Train *train = Train::GetIfValid(c.sellable); train != nullptr) {
					Command<Commands::SellVehicle>::Post(STR_ERROR_CAN_T_SELL_TRAIN, train->tile, train->index, true, false, ClientID::Invalid);
				}
				break;

			case PW_CONVERT:
				// Each released tile is its own command; the first refusal ends the batch with its message.
				for (const TileIndex tile : c.convertible) {
					if (!Command<Commands::ConvertTBMSupport>::Post(STR_ERROR_CAN_T_CONVERT_TBM_SUPPORT, tile)) break;
				}
				break;

			default:
				return;
		}
		this->SetDirty();
	}

	/**
	 * Assign the clicked train as the project's hauler.
	 * @param v Clicked primary vehicle.
	 * @return Always true: the click belongs to the pick.
	 */
	bool OnVehicleSelect(const Vehicle *v) override
	{
		this->controls = GetTBMPairControls(this->pair_key, _local_company);
		if (!this->controls.project.has_value()) {
			ResetObjectToPlace();
			return true;
		}
		// Only a train can haul spoil wagons; the command checks everything else and names its refusal.
		if (v->type != VehicleType::Train) {
			ShowErrorMessage(GetEncodedString(STR_ERROR_CAN_T_ASSIGN_TBM_HAULER), GetEncodedString(STR_ERROR_TBM_HAULER_TRAIN), WarningLevel::Error);
			return true;
		}
		if (Command<Commands::AssignTBMHauler>::Post(STR_ERROR_CAN_T_ASSIGN_TBM_HAULER, this->controls.project->id, v->index)) ResetObjectToPlace();
		this->SetDirty();
		return true;
	}

	/** The pick ended without a train; raise its button again. */
	void OnPlaceObjectAbort() override
	{
		this->RaiseWidget(PW_ASSIGN);
		this->SetWidgetDirty(PW_ASSIGN);
	}
};

static constexpr std::initializer_list<NWidgetPart> _portal_widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, Colours::DarkGreen),
		NWidget(WWT_CAPTION, Colours::DarkGreen), SetStringTip(STR_PORTAL_STATUS_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, Colours::DarkGreen),
		NWidget(NWID_VERTICAL), SetPadding(WidgetDimensions::unscaled.framerect),
			// Text takes its background from the enclosing panel; assigning a colour makes window construction throw.
			NWidget(WWT_TEXT, Colours::Invalid, PW_STATUS), SetStringTip(STR_PORTAL_STATE_SEALED, STR_PORTAL_STATUS_TOOLTIP), SetMinimalSize(460, 20), SetFill(1, 0),
			NWidget(WWT_EMPTY, Colours::Invalid, PW_PREVIEW), SetMinimalSize(460, 64), SetFill(1, 0),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::Grey, PW_CONNECT), SetStringTip(STR_PORTAL_CONNECT, STR_PORTAL_CONNECT_TOOLTIP), SetFill(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::Grey, PW_SEAL), SetStringTip(STR_PORTAL_SEAL, STR_PORTAL_SEAL_TOOLTIP), SetFill(1, 0),
				NWidget(WWT_PUSHIMGTEXTBTN, Colours::Grey, PW_COMMISSION), SetSpriteStringTip(SPR_CURSOR_QUERY, STR_PORTAL_COMMISSION, STR_PORTAL_COMMISSION_TOOLTIP), SetFill(1, 0),
			EndContainer(),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHARROWBTN, Colours::Grey, PW_CORRIDOR_DOWN), SetArrowWidgetTypeTip(ArrowWidgetType::Decrease, STR_PORTAL_CORRIDOR_DECREASE_TOOLTIP),
				NWidget(WWT_PUSHARROWBTN, Colours::Grey, PW_CORRIDOR_UP), SetArrowWidgetTypeTip(ArrowWidgetType::Increase, STR_PORTAL_CORRIDOR_INCREASE_TOOLTIP),
				NWidget(WWT_TEXT, Colours::Invalid, PW_CORRIDOR), SetStringTip(STR_PORTAL_CORRIDOR, STR_PORTAL_CORRIDOR_TOOLTIP), SetPadding(0, 0, 0, WidgetDimensions::unscaled.hsep_normal), SetFill(1, 0),
			EndContainer(),
		EndContainer(),
	EndContainer(),
	NWidget(WWT_PANEL, Colours::DarkGreen, PW_INFO), SetFill(1, 0), SetMinimalSize(460, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL, NWidContainerFlag::EqualSize),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, PW_SHOW_MACHINE), SetStringTip(STR_PORTAL_SHOW_MACHINE, STR_PORTAL_SHOW_MACHINE_TOOLTIP), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, PW_START_STOP), SetStringTip(STR_PORTAL_START_MACHINE, STR_PORTAL_START_STOP_TOOLTIP), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, PW_PAUSE), SetStringTip(STR_PORTAL_PAUSE, STR_PORTAL_PAUSE_TOOLTIP), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, PW_CANCEL), SetStringTip(STR_PORTAL_CANCEL, STR_PORTAL_CANCEL_TOOLTIP), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, PW_RETIRE), SetStringTip(STR_PORTAL_RETIRE, STR_PORTAL_RETIRE_TOOLTIP), SetFill(1, 0),
	EndContainer(),
	NWidget(NWID_HORIZONTAL, NWidContainerFlag::EqualSize),
		NWidget(WWT_TEXTBTN, Colours::Grey, PW_ASSIGN), SetStringTip(STR_PORTAL_ASSIGN_HAULER, STR_PORTAL_ASSIGN_HAULER_TOOLTIP), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, PW_RELEASE), SetStringTip(STR_PORTAL_RELEASE_HAULER, STR_PORTAL_RELEASE_HAULER_TOOLTIP), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, PW_SELL), SetStringTip(STR_PORTAL_SELL_MACHINE, STR_PORTAL_SELL_MACHINE_TOOLTIP), SetFill(1, 0),
		NWidget(WWT_PUSHTXTBTN, Colours::Grey, PW_CONVERT), SetStringTip(STR_PORTAL_CONVERT_TRACK, STR_PORTAL_CONVERT_TRACK_TOOLTIP), SetFill(1, 0),
	EndContainer(),
};

static WindowDesc _portal_desc(WindowPosition::Automatic, {}, 0, 0, WindowClass::LayerPortal, WindowClass::None, {}, _portal_widgets);

/**
 * Open actual native pair controls from either endpoint.
 * @param tile Actual clicked world tile.
 */
void ShowLayerPortalWindow(TileIndex tile)
{
	const auto key = GetLayerPortalPairKey(tile);
	if (!key) return;
	if (BringWindowToFrontById(WindowClass::LayerPortal, key->base()) != nullptr) return;
	new LayerPortalWindow(_portal_desc, *key);
}
