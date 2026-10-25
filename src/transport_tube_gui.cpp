/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file transport_tube_gui.cpp Explicit tube requests and native fleet views inside the existing underground palette.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups12C/6BDC.
 * @see docs/zz-reach2/architecture/gui/archi-gui.md.
 */

#include "stdafx.h"
#include "transport_tube_gui.h"
#include "transport_tube_model.h"
#include "transport_tube_geothermal.h"
#include "geothermal_generator.h"
#include "industry_map.h"
#include "multilayer_bootstrap.h"
#include "window_gui.h"
#include "strings_func.h"
#include "dropdown_func.h"
#include "dropdown_type.h"
#include "company_base.h"
#include "company_func.h"
#include "train.h"
#include "rail_gui.h"
#include "vehicle_gui.h"
#include "layer_portal_map.h"
#include "layer_portal_cmd.h"
#include "tilehighlight_func.h"
#include "command_func.h"
#include "cargotype.h"
#include "widgets/underground_widget.h"
#include "widgets/rail_widget.h"
#include "table/strings.h"
#include "table/sprites.h"
#include "safeguards.h"

/** @copydoc BuildUndergroundTubeFleetFilter */
std::vector<VehicleID> BuildUndergroundTubeFleetFilter(MapLayer layer, CompanyID company)
{
	std::vector<VehicleID> result;
	// This entry represents actual rock-plane capsules only; Surface/Space and
	// absent company contexts cannot acquire a tube fleet through a broad filter.
	if (!Company::IsValidID(company) || !IsUndergroundConstructionLayer(layer) || !MultiLayerMap::IsValidLayer(layer)) return result;
	// Native front engines are the only fleet identities. Conventional parts,
	// another company's vehicles and same-XY other-layer capsules never enter this view.
	for (const Train *train : Train::Iterate()) {
		if (!train->IsFrontEngine() || train->owner != company || !IsValidTile(train->tile) ||
			GetTileLayer(train->tile) != layer || !GetTubeVehicleCapability(train->engine_type)) continue;
		result.push_back(train->index);
	}
	return result;
}

/** @copydoc UndergroundTubeControls::UndergroundTubeControls */
UndergroundTubeControls::UndergroundTubeControls(MapLayer layer) : layer(layer), status(GetString(STR_TUBE_HELP)) {}

/** @copydoc UndergroundTubeControls::SelectedVehicle */
const Train *UndergroundTubeControls::SelectedVehicle() const
{
	const Train *train = Train::GetIfValid(this->vehicle);
	// Dropdown callbacks can arrive after sale, takeover or movement; recheck
	// current native identity/domain before opening a view or posting a binding.
	if (train == nullptr || !train->IsFrontEngine() || train->owner != _local_company || !IsValidTile(train->tile) ||
		GetTileLayer(train->tile) != this->layer || !GetTubeVehicleCapability(train->engine_type)) return nullptr;
	return train;
}

/** @copydoc UndergroundTubeControls::QuoteConnection */
CommandCost UndergroundTubeControls::QuoteConnection() const
{
	AutoRestoreBackup actor{_current_company, _local_company};
	if (this->geothermal_route) return CmdConnectTubeGeothermalPower({}, this->generator, this->geothermal_inlet, this->generator_output);
	// A draft needs both an explicit portal and endpoint before a service quote;
	// an incomplete cursor request cannot become a source at an inferred tile.
	if (this->portal == INVALID_TILE || this->path.count == 0) return CommandCost(STR_ERROR_TUBE_POWER_TOPOLOGY);
	return CmdConnectTubePower({}, this->portal, this->path.cells[this->path.count - 1], this->path);
}

/** @copydoc UndergroundTubeControls::QuoteBinding */
CommandCost UndergroundTubeControls::QuoteBinding() const
{
	const auto *train = this->SelectedVehicle();
	if (train == nullptr) return CommandCost(STR_ERROR_TUBE_POWER_REFUSED);
	AutoRestoreBackup actor{_current_company, _local_company};
	const bool reconnect = GetTubePowerVehicle(train->index) != nullptr;
	return this->geothermal_source.base() != UINT32_MAX ? CmdSetTubeVehicleGeothermalPower({}, train->index, this->geothermal_source, reconnect) :
		CmdSetTubeVehiclePower({}, train->index, this->source, reconnect);
}

/** @copydoc UndergroundTubeControls::SourceText */
std::optional<std::string> UndergroundTubeControls::SourceText(UtilityProducerID external, TubeGeothermalSourceID geothermal) const
{
	if (geothermal.base() != UINT32_MAX) {
		const auto *source = GetTubeGeothermalSource(geothermal);
		// An inactive or foreign original inlet cannot appear selected after
		// demolition/takeover; another inlet on the same plant remains distinct.
		if (source == nullptr || !source->active || source->owner != _local_company || !IsValidTile(source->inlet) || GetTileLayer(source->inlet) != this->layer) return std::nullopt;
		return GetString(STR_TUBE_GEOTHERMAL_ENTRY, source->id.base(), source->generator.base() + 1, TileX(source->inlet), TilePlaneY(source->inlet));
	}
	const auto *source = GetTubePowerConnection(external);
	// External labels use their real producer namespace and original portal
	// service identity; a same-valued native inlet never substitutes for one.
	if (source == nullptr || !source->active || source->owner != _local_company || !IsValidTile(source->interchange) || GetTileLayer(source->interchange) != this->layer) return std::nullopt;
	return GetString(STR_TUBE_SOURCE_ENTRY, external.base(), TileX(source->interchange), TilePlaneY(source->interchange));
}

/** @copydoc UndergroundTubeControls::Refresh */
void UndergroundTubeControls::Refresh(Window &window, bool locked) const
{
	const auto designs = BuildUndergroundVehicleFilter(this->layer, GetCurrentMultiLayerProfileSelection(),
		TimerGameCalendar::date, UndergroundFleetKind::TubeCapsules);
	// Physical construction needs the installed native definitions and current
	// canonical technology policy, independently of whether the fold-out is visible.
	const bool unavailable = locked || designs.empty() || GetTubeRailType() == INVALID_RAILTYPE || GetTubeEngine(9) == EngineID::Invalid();
	window.SetWidgetsDisabledState(unavailable, WID_UC_TUBE_GUIDEWAY, WID_UC_TUBE_TERMINAL, WID_UC_TUBE_DEPOT, WID_UC_TUBE_ROUTE);
	// Native plants exist only on dated Deep ground; early external bootstrap
	// remains available independently of this optional source family.
	window.SetWidgetDisabledState(WID_UC_TUBE_GEOTHERMAL, unavailable || this->layer != MapLayer::DeepUnderground);
	window.SetWidgetDisabledState(WID_UC_TUBE_CONNECT, unavailable || this->QuoteConnection().Failed());
	// A stale/inactive or foreign selection conveys no authority. The command
	// owner still revalidates every source, vehicle and invoice at execution.
	const bool missing = !this->SourceText(this->source, this->geothermal_source).has_value();
	AutoRestoreBackup actor{_current_company, _local_company};
	const auto *train = this->SelectedVehicle();
	window.SetWidgetDisabledState(WID_UC_TUBE_BIND, unavailable || missing || this->QuoteBinding().Failed());
	const auto close = this->geothermal_source.base() != UINT32_MAX ? CmdCloseTubeGeothermalPower({}, this->geothermal_source) : CmdCloseTubePower({}, this->source);
	window.SetWidgetDisabledState(WID_UC_TUBE_CLOSE, unavailable || missing || close.Failed());
	window.SetWidgetDisabledState(WID_UC_TUBE_VIEW, train == nullptr);
	window.SetWidgetLoweredState(WID_UC_TUBES, this->expanded);
}

/** @copydoc UndergroundTubeControls::OnClick */
bool UndergroundTubeControls::OnClick(Window &window, WidgetID widget)
{
	// Only this contiguous fold-out belongs to the component; later geothermal
	// construction controls retain their independent placement/request owner.
	if (widget < WID_UC_TUBES || widget > WID_UC_TUBE_PREVIEW) return false;
	if (window.IsWidgetDisabled(widget)) return true;
	switch (widget) {
		case WID_UC_TUBES:
			this->expanded = !this->expanded;
			// Hidden controls cannot retain an invisible power placement cursor;
			// cancelling the draft leaves installed sources and cargo untouched.
			if (!this->expanded && this->placing) ResetObjectToPlace();
			window.GetWidget<NWidgetStacked>(WID_UC_TUBE_STACK)->SetDisplayedPlane(this->expanded ? 0 : SZSP_NONE);
			window.ReInit();
			break;
		case WID_UC_TUBE_GUIDEWAY:
		case WID_UC_TUBE_TERMINAL:
		case WID_UC_TUBE_DEPOT: {
			ResetObjectToPlace(); // Choosing a native construction tool cancels any prior uncommitted power route.
			Window *toolbar = ShowBuildRailToolbar(GetTubeRailType());
			if (toolbar == nullptr) break;
			if (widget == WID_UC_TUBE_TERMINAL) toolbar->OnClick({}, WID_RAT_BUILD_STATION, 1);
			if (widget == WID_UC_TUBE_DEPOT) toolbar->OnClick({}, WID_RAT_BUILD_DEPOT, 1);
			break;
		}
		case WID_UC_TUBE_ROUTE:
			ResetObjectToPlace();
			this->portal = INVALID_TILE; this->path = {}; this->placing = true;
			SetObjectToPlaceWnd(SPR_CURSOR_QUERY, PAL_NONE, HT_RECT, &window);
			this->status = GetString(STR_TUBE_PICK_PORTAL);
			break;
		case WID_UC_TUBE_GEOTHERMAL:
			ResetObjectToPlace();
			this->geothermal_route = true; this->placing = true;
			SetObjectToPlaceWnd(SPR_CURSOR_QUERY, PAL_NONE, HT_RECT, &window);
			this->status = GetString(STR_TUBE_GEOTHERMAL_PICK_PLANT);
			break;
		case WID_UC_TUBE_CONNECT:
			if (this->QuoteConnection().Failed()) break;
			if (this->geothermal_route ? Command<Commands::ConnectTubeGeothermalPower>::Post(STR_ERROR_TUBE_POWER_ACTION, this->generator, this->geothermal_inlet, this->generator_output) :
				Command<Commands::ConnectTubePower>::Post(STR_ERROR_TUBE_POWER_ACTION, this->portal, this->path.cells[this->path.count - 1], this->path)) {
				ResetObjectToPlace();
				this->status = GetString(STR_TUBE_SUBMITTED);
			}
			break;
		case WID_UC_TUBE_CLOSE:
			if (this->geothermal_source.base() != UINT32_MAX) Command<Commands::CloseTubeGeothermalPower>::Post(STR_ERROR_TUBE_POWER_ACTION, this->geothermal_source);
			else Command<Commands::CloseTubePower>::Post(STR_ERROR_TUBE_POWER_ACTION, this->source);
			break;
		case WID_UC_TUBE_BIND:
			if (this->QuoteBinding().Failed()) break;
			if (this->geothermal_source.base() != UINT32_MAX) Command<Commands::SetTubeVehicleGeothermalPower>::Post(STR_ERROR_TUBE_POWER_ACTION,
				this->vehicle, this->geothermal_source, GetTubePowerVehicle(this->vehicle) != nullptr);
			else Command<Commands::SetTubeVehiclePower>::Post(STR_ERROR_TUBE_POWER_ACTION,
				this->vehicle, this->source, GetTubePowerVehicle(this->vehicle) != nullptr);
			break;
		case WID_UC_TUBE_VIEW:
			if (const auto *train = this->SelectedVehicle()) ShowVehicleViewWindow(train);
			break;
		case WID_UC_TUBE_FLEET: {
			DropDownList list;
			// Rebuild from actual native fronts on every opening; no retained UI
			// row can manufacture a fleet member, stock account or order list.
			for (VehicleID id : BuildUndergroundTubeFleetFilter(this->layer, _local_company)) {
				const Train *train = Train::Get(id);
				list.push_back(MakeDropDownListStringItem(GetString(STR_TUBE_FLEET_ENTRY, id.base() + 1, train->GetEngine()->name), id.base()));
			}
			if (list.empty()) { this->status = GetString(STR_TUBE_NO_CAPSULES); break; }
			ShowDropDownList(&window, std::move(list), -1, widget);
			break;
		}
		case WID_UC_TUBE_SOURCE: {
			DropDownList list;
			// Offer only actual active owned inlets on this explicit rock plane.
			// Selecting an inlet is UI state and grants no electricity.
			for (const auto &connection : CaptureTubePowerConnections()) {
				// A current local active physical source alone may enter this menu;
				// callbacks still revalidate permissions before posting any command.
				if (!connection.active || connection.owner != _local_company || GetTileLayer(connection.interchange) != this->layer) continue;
				list.push_back(MakeDropDownListStringItem(GetString(STR_TUBE_SOURCE_ENTRY, connection.producer.base(),
					TileX(connection.interchange), TilePlaneY(connection.interchange)), connection.producer.base()));
			}
			// Nonreused original inlet IDs occupy a separate UI-only namespace.
			// Two cable endpoints on one generator remain two explicit choices.
			for (const auto &source : CaptureTubeGeothermalPowerState().sources) {
				const auto label = this->SourceText(UtilityProducerID{UINT32_MAX}, source.id);
				if (label.has_value()) list.push_back(MakeDropDownListStringItem(std::string{*label}, TUBE_GEOTHERMAL_SOURCE_MENU_BASE + source.id.base()));
			}
			if (list.empty()) { this->status = GetString(STR_TUBE_NO_SOURCES); break; }
			ShowDropDownList(&window, std::move(list), -1, widget);
			break;
		}
		case WID_UC_TUBE_MODELS: {
			DropDownList list;
			// Future designs remain inspectable with their actual introduction date;
			// this menu never allocates an engine or bypasses the depot purchase list.
			for (const auto &design : GetMultiLayerVehicleCapabilities()) {
				if (design.transport != MultiLayerVehicleTransport::Tube) continue;
				list.push_back(MakeDropDownListStringItem(GetString(STR_UNDERGROUND_MODEL_DATE, std::string(design.name),
					design.introduction_year.base()), design.model_id));
			}
			ShowDropDownList(&window, std::move(list), this->model, widget);
			break;
		}
		default: break;
	}
	window.SetDirty();
	return true;
}

/** @copydoc UndergroundTubeControls::OnDropdownSelect */
bool UndergroundTubeControls::OnDropdownSelect(Window &window, WidgetID widget, int index)
{
	if (index < 0) return false;
	switch (widget) {
		case WID_UC_TUBE_FLEET: this->vehicle = VehicleID{static_cast<uint32_t>(index)}; break;
		case WID_UC_TUBE_SOURCE: {
			const bool native = index >= TUBE_GEOTHERMAL_SOURCE_MENU_BASE;
			const auto external = UtilityProducerID{native ? UINT32_MAX : static_cast<uint32_t>(index)};
			const auto geothermal = TubeGeothermalSourceID{native ? static_cast<uint32_t>(index - TUBE_GEOTHERMAL_SOURCE_MENU_BASE) : UINT32_MAX};
			if (!this->SourceText(external, geothermal).has_value()) {
				this->source = UtilityProducerID{UINT32_MAX}; this->geothermal_source = TubeGeothermalSourceID{UINT32_MAX};
				this->status = GetString(STR_TUBE_SOURCE_STALE); window.SetDirty(); return true;
			}
			this->source = external; this->geothermal_source = geothermal;
			break;
		}
		case WID_UC_TUBE_MODELS:
			if (GetTubeVehicleDefinition(static_cast<uint16_t>(index)) != nullptr) this->model = static_cast<uint16_t>(index);
			break;
		default: return false;
	}
	// Source/fleet choices update the proposed binding and its native admission
	// explanation. Inspecting a design must preserve unrelated power guidance.
	if (widget != WID_UC_TUBE_MODELS) {
		if (const auto *train = this->SelectedVehicle()) {
			const auto quote = this->QuoteBinding();
			this->status = GetString(quote.Failed() ? quote.GetErrorMessage() : STR_TUBE_BIND_READY);
		}
	}
	window.SetDirty();
	return true;
}

/** @copydoc UndergroundTubeControls::OnPlaceObject */
bool UndergroundTubeControls::OnPlaceObject(Window &window, TileIndex tile)
{
	if (!this->placing) return false;
	// A viewport switch never moves this request to a different plane or lets
	// an out-of-map click choose an endpoint by a coincident coordinate.
	if (!IsValidTile(tile) || GetTileLayer(tile) != this->layer) {
		this->status = GetString(STR_UNDERGROUND_WRONG_LAYER);
	} else if (this->geothermal_route) {
		if (this->generator == IndustryID::Invalid()) {
			const auto id = IsTileType(tile, TileType::Industry) ? GetIndustryIndex(tile) : IndustryID::Invalid();
			const auto *plant = GetGeothermalGeneratorState(id);
			// Only existing completed owned machinery with its acquired original
			// output may start a request; a survey or cold blueprint is insufficient.
			if (plant == nullptr || plant->owner != _local_company || !IsGeothermalGeneratorConstructionComplete(id) || plant->electrical_producer.base() == UINT32_MAX) {
				this->status = GetString(STR_TUBE_GEOTHERMAL_PICK_PLANT);
			} else {
				this->generator = id; this->generator_output = plant->electrical_producer;
				this->status = GetString(STR_TUBE_GEOTHERMAL_PICK_INLET);
			}
		} else {
			this->geothermal_inlet = tile;
			const auto quote = this->QuoteConnection();
			this->status = GetString(quote.Succeeded() ? STR_TUBE_GEOTHERMAL_READY : STR_TUBE_GEOTHERMAL_PATH);
		}
	} else if (this->portal == INVALID_TILE) {
		const auto *record = GetLayerPortalWorkRecord(tile);
		// Only a real owned commissioned rock endpoint can start this explicit
		// source request. Native command validation remains the final authority.
		if (record == nullptr || record->company != _local_company || record->state != LayerPortalWorkState::Connected ||
			GetLayerPortalTransportType(tile) != TransportType::Rail) {
			this->status = GetString(STR_TUBE_PICK_PORTAL);
		} else {
			this->portal = tile;
			this->status = GetString(STR_TUBE_PICK_ROUTE);
		}
	} else {
		const auto *record = GetLayerPortalWorkRecord(this->portal);
		// Demolition, sealing or takeover can invalidate a draft between clicks.
		// Recheck the actual endpoint before asking its tile owner for the door.
		if (record == nullptr || record->state != LayerPortalWorkState::Connected || record->company != _local_company) {
			this->Abort();
			ResetObjectToPlace(); // A lost endpoint cannot leave a cursor that silently falls through to the parent's survey tool.
			this->status = GetString(STR_TUBE_PICK_PORTAL);
			window.SetDirty();
			return true;
		}
		const auto floor = CheckTubeConstruction(std::array{tile});
		const TileIndex previous = this->path.count == 0 ? GetLayerPortalDoorTile(this->portal) : this->path.cells[this->path.count - 1];
		const uint distance = abs(static_cast<int>(TileX(tile)) - static_cast<int>(TileX(previous))) +
			abs(static_cast<int>(TilePlaneY(tile)) - static_cast<int>(TilePlaneY(previous)));
		// The first wire cell is the real door; later clicks advance exactly one
		// orthogonal cell. Repeated cells and over-limit routes cannot inflate reach.
		if (this->path.count >= this->path.cells.size() || (this->path.count == 0 ? tile != previous : distance != 1) ||
			std::ranges::find(std::span{this->path.cells}.first(this->path.count), tile) != std::span{this->path.cells}.first(this->path.count).end()) {
			this->status = GetString(STR_TUBE_ROUTE_ADJACENT);
		} else if (floor.Failed()) {
			this->status = GetString(floor.GetErrorMessage());
		} else {
			this->path.cells[this->path.count++] = tile;
			const auto quote = this->QuoteConnection();
			const auto &terms = GetMultiLayerTubePowerContract();
			this->status = quote.Succeeded() ? GetString(STR_TUBE_ROUTE_READY, Money{terms.lease_per_day}, Money{1}, terms.joules_per_money, this->path.count) :
				GetString(STR_TUBE_ROUTE_PROGRESS, this->path.count, GetString(quote.GetErrorMessage()));
		}
	}
	window.SetDirty();
	return true;
}

/** @copydoc UndergroundTubeControls::Abort */
void UndergroundTubeControls::Abort()
{
	if (this->placing) this->status = GetString(STR_TUBE_HELP);
	this->placing = false; this->portal = INVALID_TILE; this->path = {};
	this->geothermal_route = false; this->generator = IndustryID::Invalid(); this->generator_output = UtilityProducerID{UINT32_MAX}; this->geothermal_inlet = INVALID_TILE;
}

/** @copydoc UndergroundTubeControls::GetWidgetText */
std::optional<std::string> UndergroundTubeControls::GetWidgetText(WidgetID widget) const
{
	if (widget == WID_UC_TUBE_STATUS) {
		std::string current = this->status;
		// A completed draft can lose its plant, cable or recipient while paused;
		// refresh its explanation with the same pure admission as confirmation.
		if (this->geothermal_route && this->geothermal_inlet != INVALID_TILE) current = GetString(this->QuoteConnection().Succeeded() ? STR_TUBE_GEOTHERMAL_READY : STR_TUBE_GEOTHERMAL_PATH);
		const auto *connection = GetTubePowerConnection(this->source);
		// The source's actual closing estimate supplies a live funds/permission
		// explanation; this view never recomputes an invoice or settles its carry.
		if (connection != nullptr && connection->active && connection->owner == _local_company) {
			AutoRestoreBackup actor{_current_company, _local_company};
			const auto quote = CmdCloseTubePower({}, this->source);
			if (quote.Failed()) current += " " + GetString(quote.GetErrorMessage());
		}
		const auto *train = this->SelectedVehicle();
		if (train == nullptr) return current;
		const auto *binding = GetTubePowerVehicle(train->index);
		auto wait = binding == nullptr ? TubePowerWait::NoConnection : binding->wait;
		if (binding != nullptr) {
			// A closed source is visible immediately, even while paused. A newly
			// bound capsule awaiting its first tick has an inlet but no allocation;
			// presenting either case must not advance the native power owner.
			if (binding->geothermal_source.base() != UINT32_MAX) {
				const auto *source = GetTubeGeothermalSource(binding->geothermal_source);
				// Original machinery keeps its model/date gate. Physical interruption
				// and unaffordable service need distinct guidance, without a fake tick.
				if (source == nullptr || !source->active) wait = TubePowerWait::Topology;
				else if (!IsGeothermalGeneratorConstructionComplete(source->generator)) wait = TubePowerWait::Unavailable;
				else if (!IsTubeGeothermalPowerConnected(binding->geothermal_source, *train)) wait = TubePowerWait::Topology;
				else if (!HasGeothermalGeneratorServiceFunds(source->generator, 1)) return GetString(STR_TUBE_SELECTED_STATUS, current,
					train->cargo.StoredCount(), train->cargo_cap, GetString(CargoSpec::Get(train->cargo_type)->name), GetString(STR_TUBE_GEOTHERMAL_FUNDS));
				// An acquired binding before its first simulation opportunity has
				// physical permission, but its initial sentinel is not a date refusal.
				else if (wait == TubePowerWait::NoConnection || binding->prepared_tick == UINT64_MAX) wait = TubePowerWait::Grant;
			} else {
				if (!IsTubePowerConnected(binding->source, *train)) wait = TubePowerWait::Topology;
				else if (wait == TubePowerWait::NoConnection) wait = TubePowerWait::Grant;
			}
		}
		const std::array waits{STR_TUBE_WAIT_NONE, STR_TUBE_WAIT_CONNECTION, STR_TUBE_WAIT_TOPOLOGY, STR_TUBE_WAIT_GRANT, STR_TUBE_WAIT_FUNDS, STR_TUBE_WAIT_UNAVAILABLE};
		return GetString(STR_TUBE_SELECTED_STATUS, current, train->cargo.StoredCount(), train->cargo_cap,
			GetString(CargoSpec::Get(train->cargo_type)->name), GetString(waits.at(to_underlying(wait))));
	}
	if (widget == WID_UC_TUBE_BOUND) {
		const auto *train = this->SelectedVehicle();
		const auto *binding = train == nullptr ? nullptr : GetTubePowerVehicle(train->index);
		if (binding == nullptr) return GetString(STR_TUBE_BOUND, GetString(STR_TUBE_WAIT_CONNECTION));
		return GetString(STR_TUBE_BOUND, this->SourceText(binding->source, binding->geothermal_source).value_or(GetString(STR_TUBE_SOURCE_STALE)));
	}
	if (widget == WID_UC_TUBE_FLEET) return GetString(STR_TUBE_FLEET, BuildUndergroundTubeFleetFilter(this->layer, _local_company).size());
	// A proposed selection changes the action caption only when this actual
	// capsule already has its own active binding; it never binds by inspection.
	if (widget == WID_UC_TUBE_BIND) return GetString(this->SelectedVehicle() != nullptr && GetTubePowerVehicle(this->vehicle) != nullptr ? STR_TUBE_RECONNECT : STR_TUBE_BIND);
	if (widget == WID_UC_TUBE_SOURCE) {
		return this->SourceText(this->source, this->geothermal_source).value_or(GetString(STR_TUBE_SOURCE));
	}
	if (widget == WID_UC_TUBE_CONNECT) return GetString(this->geothermal_route ? STR_TUBE_GEOTHERMAL_CONNECT : STR_TUBE_CONNECT);
	if (widget != WID_UC_TUBE_PREVIEW) return std::nullopt;
	const auto designs = GetMultiLayerVehicleCapabilities();
	const auto design = std::ranges::find(designs, this->model, &LayerVehicleCapability::model_id);
	if (design == designs.end()) return std::string{};
	std::string cargos;
	// Cargo labels and physical forms come from the one immutable catalogue;
	// this preview cannot add a refit accepted by the actual native depot window.
	for (CargoLabel label : design->cargo_labels) {
		const CargoType cargo = GetCargoTypeByLabel(label);
		if (cargo == INVALID_CARGO) continue;
		if (!cargos.empty()) cargos += ", ";
		cargos += GetString(CargoSpec::Get(cargo)->name);
	}
	return GetString(STR_TUBE_MODEL_PREVIEW, std::string(design->name), design->introduction_year.base(),
		GetTubeVehicleDefinition(this->model)->capacity, cargos);
}
