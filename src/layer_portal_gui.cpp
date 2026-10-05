/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_portal_gui.cpp Native pair status and validated ordinary connection controls. @see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md, Groups 8/13 */

#include "stdafx.h"
#include "layer_portal_cmd.h"
#include "layer_portal_map.h"
#include "map_layer_gui.h"
#include "company_func.h"
#include "command_func.h"
#include "window_func.h"
#include "window_gui.h"
#include "table/strings.h"
#include "safeguards.h"

/** Native pair window widgets; status always queries canonical saved authority. */
enum PortalWidgets : WidgetID { PW_STATUS, PW_CONNECT, PW_SEAL, PW_COMMISSION };

/** One status/control window per canonical pair, independent of the currently selected viewport layer. */
class LayerPortalWindow : public Window {
	TileIndex pair_key; ///< Actual lower housing; never a camera-relative address or duplicate work state.
public:
	/** @param desc Actual registered widget/window descriptor. @param key Actual canonical pair identity. */
	LayerPortalWindow(WindowDesc &desc, TileIndex key) : Window(desc), pair_key(key)
	{
		this->InitNested(key.base());
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
		this->SetWidgetDisabledState(PW_COMMISSION, true); // Actual purchase must remain unavailable until the real TBM construction/controller producer is installed; no substitute vehicle is created by this control.
		this->DrawWidgets();
	}
	/** @param pt Unused click coordinates. @param widget Actual selected control. @param click_count Unused repeated-click count. */
	void OnClick([[maybe_unused]] Point pt, WidgetID widget, [[maybe_unused]] int click_count) override
	{
		if (widget == PW_CONNECT || widget == PW_SEAL) { // Both controls submit the same synchronized backend; UI estimates never grant a private routing or excavation permission.
			Command<Commands::ChangeLayerPortalConnection>::Post(STR_ERROR_CAN_T_BUILD_LAYER_PORTAL, this->pair_key, widget == PW_CONNECT);
			this->SetDirty();
		}
	}
};

static constexpr std::initializer_list<NWidgetPart> _portal_widgets = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, Colours::DarkGreen),
		NWidget(WWT_CAPTION, Colours::DarkGreen), SetStringTip(STR_PORTAL_STATUS_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
	EndContainer(),
	NWidget(WWT_PANEL, Colours::DarkGreen),
		NWidget(NWID_VERTICAL), SetPadding(WidgetDimensions::unscaled.framerect),
			NWidget(WWT_TEXT, Colours::DarkGreen, PW_STATUS), SetStringTip(STR_PORTAL_STATE_SEALED, STR_PORTAL_STATUS_TOOLTIP), SetMinimalSize(300, 20), SetFill(1, 0),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_PUSHTXTBTN, Colours::Grey, PW_CONNECT), SetStringTip(STR_PORTAL_CONNECT, STR_PORTAL_CONNECT_TOOLTIP), SetFill(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::Grey, PW_SEAL), SetStringTip(STR_PORTAL_SEAL, STR_PORTAL_SEAL_TOOLTIP), SetFill(1, 0),
				NWidget(WWT_PUSHTXTBTN, Colours::Grey, PW_COMMISSION), SetStringTip(STR_PORTAL_COMMISSION, STR_PORTAL_COMMISSION_TOOLTIP), SetFill(1, 0),
			EndContainer(),
		EndContainer(),
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
