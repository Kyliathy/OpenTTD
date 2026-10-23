/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file script_window.hpp Everything to handle window interaction. */

#ifndef SCRIPT_WINDOW_HPP
#define SCRIPT_WINDOW_HPP

#include "script_object.hpp"

#include "../../window_type.h"
#include "../../widgets/ai_widget.h"
#include "../../widgets/airport_widget.h"
#include "../../widgets/autoreplace_widget.h"
#include "../../widgets/bootstrap_widget.h"
#include "../../widgets/bridge_widget.h"
#include "../../widgets/build_vehicle_widget.h"
#include "../../widgets/cheat_widget.h"
#include "../../widgets/company_widget.h"
#include "../../widgets/console_widget.h"
#include "../../widgets/date_widget.h"
#include "../../widgets/depot_widget.h"
#include "../../widgets/dock_widget.h"
#include "../../widgets/dropdown_widget.h"
#include "../../widgets/engine_widget.h"
#include "../../widgets/error_widget.h"
#include "../../widgets/fios_widget.h"
#include "../../widgets/framerate_widget.h"
#include "../../widgets/game_widget.h"
#include "../../widgets/genworld_widget.h"
#include "../../widgets/goal_widget.h"
#include "../../widgets/graph_widget.h"
#include "../../widgets/group_widget.h"
#include "../../widgets/help_widget.h"
#include "../../widgets/highscore_widget.h"
#include "../../widgets/industry_widget.h"
#include "../../widgets/intro_widget.h"
#include "../../widgets/layer_portal_widget.h"
#include "../../widgets/league_widget.h"
#include "../../widgets/link_graph_legend_widget.h"
#include "../../widgets/main_widget.h"
#include "../../widgets/misc_widget.h"
#include "../../widgets/music_widget.h"
#include "../../widgets/network_chat_widget.h"
#include "../../widgets/network_content_widget.h"
#include "../../widgets/network_widget.h"
#include "../../widgets/newgrf_debug_widget.h"
#include "../../widgets/newgrf_widget.h"
#include "../../widgets/news_widget.h"
#include "../../widgets/object_widget.h"
#include "../../widgets/order_widget.h"
#include "../../widgets/osk_widget.h"
#include "../../widgets/picker_widget.h"
#include "../../widgets/rail_widget.h"
#include "../../widgets/road_widget.h"
#include "../../widgets/screenshot_widget.h"
#include "../../widgets/script_widget.h"
#include "../../widgets/settings_widget.h"
#include "../../widgets/sign_widget.h"
#include "../../widgets/smallmap_widget.h"
#include "../../widgets/station_widget.h"
#include "../../widgets/statusbar_widget.h"
#include "../../widgets/story_widget.h"
#include "../../widgets/subsidy_widget.h"
#include "../../widgets/tbm_widget.h"
#include "../../widgets/terraform_widget.h"
#include "../../widgets/timetable_widget.h"
#include "../../widgets/toolbar_widget.h"
#include "../../widgets/town_widget.h"
#include "../../widgets/transparency_widget.h"
#include "../../widgets/tree_widget.h"
#include "../../widgets/underground_widget.h"
#include "../../widgets/vehicle_widget.h"
#include "../../widgets/viewport_widget.h"
#include "../../widgets/waypoint_widget.h"

/**
 * Class that handles window interaction. A Window in OpenTTD has two important
 *  values. The WindowClass, and a Window number. The first indicates roughly
 *  which window it is. WindowClass::TownView for example, is the view of a town.
 * The Window number is a bit more complex, as it depends mostly on the
 *  WindowClass. For example for WindowClass::TownView it is the TownID. In general a
 *  good rule of thumb is: either the number is always 0, or the ID of the
 *  object in question.
 * In the comment at the widget enum, it is mentioned how the number is used.
 *
 * Note, that the detailed window layout is very version specific.
 * Enum values might be added, changed or removed in future versions without notice
 * in the changelog, and there won't be any means of compatibility.
 *
 * @api game
 */
class ScriptWindow : public ScriptObject {
public:
	// @enum .*WindowNumber ../../window_type.h
	/* automatically generated from ../../window_type.h */
	/** Window numbers for GameOptions windows. */
	enum class GameOptionsWindowNumber {
		AI                                           = to_underlying(::GameOptionsWindowNumber::AI),                                           ///< AI settings.
		GS                                           = to_underlying(::GameOptionsWindowNumber::GS),                                           ///< GS settings.
		About                                        = to_underlying(::GameOptionsWindowNumber::About),                                        ///< About window.
		NewGRFState                                  = to_underlying(::GameOptionsWindowNumber::NewGRFState),                                  ///< NewGRF settings.
		GameOptions                                  = to_underlying(::GameOptionsWindowNumber::GameOptions),                                  ///< Game options.
	};

	/** Window numbers for QueryString windows. */
	enum class QueryStringWindowNumber {
		Default                                      = to_underlying(::QueryStringWindowNumber::Default),                                      ///< Query string.
		Sign                                         = to_underlying(::QueryStringWindowNumber::Sign),                                         ///< Query string for signs.
	};

	/** Window numbers for PopupQuery windows. */
	enum class ConfirmPopupQueryWindowNumber {
		Default                                      = to_underlying(::ConfirmPopupQueryWindowNumber::Default),                                      ///< Query popup confirm.
		Bootstrap                                    = to_underlying(::ConfirmPopupQueryWindowNumber::Bootstrap),                                    ///< Query popup confirm for bootstrap.
	};

	/** Window numbers for network windows. */
	enum class NetworkWindowNumber {
		Game                                         = to_underlying(::NetworkWindowNumber::Game),                                         ///< Network game window.
		ContentList                                  = to_underlying(::NetworkWindowNumber::ContentList),                                  ///< Network content list.
		StartServer                                  = to_underlying(::NetworkWindowNumber::StartServer),                                  ///< Network start server.
	};

	/** Window number for network status windows. */
	enum class NetworkStatusWindowNumber {
		Join                                         = to_underlying(::NetworkStatusWindowNumber::Join),                                         ///< Network join status.
		ContentDownload                              = to_underlying(::NetworkStatusWindowNumber::ContentDownload),                              ///< Network content download status.
	};

	// @endenum

	// @enum WindowClass ../../window_type.h
	/* automatically generated from ../../window_type.h */
	/** %Window classes. */
	enum class WindowClass {
		None                                         = to_underlying(::WindowClass::None),                                         ///< No window, redirects to WindowClass::MainWindow.

		/**
		 * Main window; %Window numbers:
		 *   - 0 = #MainWidgets
		 */
		MainWindow                                   = to_underlying(::WindowClass::MainWindow),

		/**
		 * Main toolbar (the long bar at the top); %Window numbers:
		 *   - 0 = #ToolbarNormalWidgets
		 *   - 0 = #ToolbarEditorWidgets
		 */
		MainToolbar                                  = to_underlying(::WindowClass::MainToolbar),

		/**
		 * Statusbar (at the bottom of your screen); %Window numbers:
		 *   - 0 = #StatusbarWidgets
		 */
		Statusbar                                    = to_underlying(::WindowClass::Statusbar),

		/**
		 * Build toolbar; %Window numbers:
		 *   - TransportType::Rail = #RailToolbarWidgets
		 *   - TransportType::Air = #AirportToolbarWidgets
		 *   - TransportType::Water = #DockToolbarWidgets
		 *   - TransportType::Road = #RoadToolbarWidgets
		 */
		BuildToolbar                                 = to_underlying(::WindowClass::BuildToolbar),

		/**
		 * Scenario build toolbar; %Window numbers:
		 *   - TransportType::Water = #DockToolbarWidgets
		 *   - TransportType::Road = #RoadToolbarWidgets
		 */
		ScenarioBuildToolbar                         = to_underlying(::WindowClass::ScenarioBuildToolbar),

		/**
		 * Build trees toolbar; %Window numbers:
		 *   - 0 = #BuildTreesWidgets
		 */
		BuildTrees                                   = to_underlying(::WindowClass::BuildTrees),

		/**
		 * Transparency toolbar; %Window numbers:
		 *   - 0 = #TransparencyToolbarWidgets
		 */
		TransparencyToolbar                          = to_underlying(::WindowClass::TransparencyToolbar),

		/**
		 * Build signal toolbar; %Window numbers:
		 *   - TransportType::Rail = #BuildSignalWidgets
		 */
		BuildSignal                                  = to_underlying(::WindowClass::BuildSignal),

		/**
		 * Small map; %Window numbers:
		 *   - 0 = #SmallMapWidgets
		 */
		SmallMap                                     = to_underlying(::WindowClass::SmallMap),

		/**
		 * Error message; %Window numbers:
		 *   - 0 = #ErrorMessageWidgets
		 */
		ErrorMessage                                 = to_underlying(::WindowClass::ErrorMessage),

		/**
		 * Tooltip window; %Window numbers:
		 *   - 0 = #ToolTipsWidgets
		 */
		ToolTips                                     = to_underlying(::WindowClass::ToolTips),

		/**
		 * Query string window; %Window numbers:
		 *   - QueryStringWindowNumber::Default = #QueryStringWidgets
		 *   - QueryStringWindowNumber::Sign = #QueryEditSignWidgets
		 */
		QueryString                                  = to_underlying(::WindowClass::QueryString),

		/**
		 * Popup with confirm question; %Window numbers:
		 *   - ConfirmPopupQueryWindowNumber::Default = #QueryWidgets
		 *   - ConfirmPopupQueryWindowNumber::Bootstrap = #BootstrapAskForDownloadWidgets
		 */
		ConfirmPopupQuery                            = to_underlying(::WindowClass::ConfirmPopupQuery),

		/**
		 * Popup with a set of buttons, designed to ask the user a question
		 *  from a GameScript. %Window numbers:
		 *   - uniqueid = #GoalQuestionWidgets
		 */
		GoalQuestion                                 = to_underlying(::WindowClass::GoalQuestion),


		/**
		 * Saveload window; %Window numbers:
		 *   - 0 = #SaveLoadWidgets
		 */
		SaveLoad                                     = to_underlying(::WindowClass::SaveLoad),

		/**
		 * Land info window; %Window numbers:
		 *   - 0 = #LandInfoWidgets
		 */
		LandInfo                                     = to_underlying(::WindowClass::LandInfo),

		/**
		 * Drop down menu; %Window numbers:
		 *   - 0 = #DropdownMenuWidgets
		 */
		DropdownMenu                                 = to_underlying(::WindowClass::DropdownMenu),

		/**
		 * On Screen Keyboard; %Window numbers:
		 *   - 0 = #OnScreenKeyboardWidgets
		 */
		OnScreenKeyboard                             = to_underlying(::WindowClass::OnScreenKeyboard),

		/**
		 * Set date; %Window numbers:
		 *   - VehicleID = #SetDateWidgets
		 */
		SetDate                                      = to_underlying(::WindowClass::SetDate),


		/**
		 * Script settings; %Window numbers:
		 *   - 0 = #ScriptSettingsWidgets
		 */
		ScriptSettings                               = to_underlying(::WindowClass::ScriptSettings),

		/**
		 * NewGRF parameters; %Window numbers:
		 *   - 0 = #NewGRFParametersWidgets
		 */
		NewGRFParameters                             = to_underlying(::WindowClass::NewGRFParameters),

		/**
		 * textfile; %Window numbers:
		 *   - 0 = #TextfileWidgets
		 */
		Textfile                                     = to_underlying(::WindowClass::Textfile),


		/**
		 * Town authority; %Window numbers:
		 *   - TownID = #TownAuthorityWidgets
		 */
		TownAuthority                                = to_underlying(::WindowClass::TownAuthority),

		/**
		 * Vehicle details; %Window numbers:
		 *   - VehicleID = #VehicleDetailsWidgets
		 */
		VehicleDetails                               = to_underlying(::WindowClass::VehicleDetails),

		/**
		 * Vehicle refit; %Window numbers:
		 *   - VehicleID = #VehicleRefitWidgets
		 */
		VehicleRefit                                 = to_underlying(::WindowClass::VehicleRefit),

		/**
		 * Vehicle orders; %Window numbers:
		 *   - VehicleID = #OrderWidgets
		 */
		VehicleOrders                                = to_underlying(::WindowClass::VehicleOrders),

		/**
		 * Replace vehicle window; %Window numbers:
		 *   - VehicleType = #ReplaceVehicleWidgets
		 */
		ReplaceVehicle                               = to_underlying(::WindowClass::ReplaceVehicle),

		/**
		 * Vehicle timetable; %Window numbers:
		 *   - VehicleID = #VehicleTimetableWidgets
		 */
		VehicleTimetable                             = to_underlying(::WindowClass::VehicleTimetable),

		/**
		 * Company colour selection; %Window numbers:
		 *   - CompanyID = #SelectCompanyLiveryWidgets
		 */
		CompanyLivery                                = to_underlying(::WindowClass::CompanyLivery),

		/**
		 * Alter company face window; %Window numbers:
		 *   - CompanyID = #SelectCompanyManagerFaceWidgets
		 */
		CompanyManagerFace                           = to_underlying(::WindowClass::CompanyManagerFace),

		/**
		 * Select station (when joining stations); %Window numbers:
		 *   - 0 = #JoinStationWidgets
		 */
		JoinStation                                  = to_underlying(::WindowClass::JoinStation),

		/**
		 * News window; %Window numbers:
		 *   - 0 = #NewsWidgets
		 */
		News                                         = to_underlying(::WindowClass::News),

		/**
		 * Town directory; %Window numbers:
		 *   - 0 = #TownDirectoryWidgets
		 */
		TownDirectory                                = to_underlying(::WindowClass::TownDirectory),

		/**
		 * Subsidies list; %Window numbers:
		 *   - 0 = #SubsidyListWidgets
		 */
		SubsidyList                                  = to_underlying(::WindowClass::SubsidyList),

		/**
		 * Industry directory; %Window numbers:
		 *   - 0 = #IndustryDirectoryWidgets
		 */
		IndustryDirectory                            = to_underlying(::WindowClass::IndustryDirectory),

		/**
		 * News history list; %Window numbers:
		 *   - 0 = #MessageHistoryWidgets
		 */
		MessageHistory                               = to_underlying(::WindowClass::MessageHistory),

		/**
		 * Sign list; %Window numbers:
		 *   - 0 = #SignListWidgets
		 */
		SignList                                     = to_underlying(::WindowClass::SignList),

		/**
		 * Scripts list; %Window numbers:
		 *   - 0 = #ScriptListWidgets
		 */
		ScriptList                                   = to_underlying(::WindowClass::ScriptList),

		/**
		 * Goals list; %Window numbers:
		 *   - 0 ; #GoalListWidgets
		 */
		GoalList                                     = to_underlying(::WindowClass::GoalList),

		/**
		 * Story book; %Window numbers:
		 *   - CompanyID = #StoryBookWidgets
		 */
		StoryBook                                    = to_underlying(::WindowClass::StoryBook),

		/**
		 * Station list; %Window numbers:
		 *   - CompanyID = #StationListWidgets
		 */
		StationList                                  = to_underlying(::WindowClass::StationList),

		/**
		 * Trains list; %Window numbers:
		 *   - Packed value = #GroupListWidgets / #VehicleListWidgets
		 */
		TrainList                                    = to_underlying(::WindowClass::TrainList),

		/**
		 * Road vehicle list; %Window numbers:
		 *   - Packed value = #GroupListWidgets / #VehicleListWidgets
		 */
		RoadVehicleList                              = to_underlying(::WindowClass::RoadVehicleList),

		/**
		 * Ships list; %Window numbers:
		 *   - Packed value = #GroupListWidgets / #VehicleListWidgets
		 */
		ShipList                                     = to_underlying(::WindowClass::ShipList),

		/**
		 * Aircraft list; %Window numbers:
		 *   - Packed value = #GroupListWidgets / #VehicleListWidgets
		 */
		AircraftList                                 = to_underlying(::WindowClass::AircraftList),


		/**
		 * Town view; %Window numbers:
		 *   - TownID = #TownViewWidgets
		 */
		TownView                                     = to_underlying(::WindowClass::TownView),

		/**
		 * Vehicle view; %Window numbers:
		 *   - VehicleID = #VehicleViewWidgets
		 */
		VehicleView                                  = to_underlying(::WindowClass::VehicleView),

		/**
		 * Station view; %Window numbers:
		 *   - StationID = #StationViewWidgets
		 */
		StationView                                  = to_underlying(::WindowClass::StationView),

		/**
		 * Depot view; %Window numbers:
		 *   - TileIndex = #DepotWidgets
		 */
		VehicleDepot                                 = to_underlying(::WindowClass::VehicleDepot),

		/**
		 * Waypoint view; %Window numbers:
		 *   - WaypointID = #WaypointWidgets
		 */
		WaypointView                                 = to_underlying(::WindowClass::WaypointView),

		/**
		 * Industry view; %Window numbers:
		 *   - IndustryID = #IndustryViewWidgets
		 */
		IndustryView                                 = to_underlying(::WindowClass::IndustryView),

		/**
		 * Company view; %Window numbers:
		 *   - CompanyID = #CompanyWidgets
		 */
		Company                                      = to_underlying(::WindowClass::Company),


		/**
		 * Build object; %Window numbers:
		 *   - 0 = #BuildObjectWidgets
		 */
		BuildObject                                  = to_underlying(::WindowClass::BuildObject),

		/**
		 * Build house; %Window numbers:
		 *   - 0 = #BuildHouseWidgets
		 */
		BuildHouse                                   = to_underlying(::WindowClass::BuildHouse),

		/**
		 * Build vehicle; %Window numbers:
		 *   - VehicleType = #BuildVehicleWidgets
		 *   - TileIndex = #BuildVehicleWidgets
		 */
		BuildVehicle                                 = to_underlying(::WindowClass::BuildVehicle),

		/**
		 * Build bridge; %Window numbers:
		 *   - TransportType = #BuildBridgeSelectionWidgets
		 */
		BuildBridge                                  = to_underlying(::WindowClass::BuildBridge),

		/**
		 * Build station; %Window numbers:
		 *   - TransportType::Air = #AirportPickerWidgets
		 *   - TransportType::Water = #DockToolbarWidgets
		 *   - TransportType::Rail = #BuildRailStationWidgets
		 */
		BuildStation                                 = to_underlying(::WindowClass::BuildStation),

		/**
		 * Build bus station; %Window numbers:
		 *   - TransportType::Road = #BuildRoadStationWidgets
		 */
		BuildBusStation                              = to_underlying(::WindowClass::BuildBusStation),

		/**
		 * Build truck station; %Window numbers:
		 *   - TransportType::Road = #BuildRoadStationWidgets
		 */
		BuildTruckStation                            = to_underlying(::WindowClass::BuildTruckStation),

		/**
		 * Build depot; %Window numbers:
		 *   - TransportType::Water = #BuildDockDepotWidgets
		 *   - TransportType::Rail = #BuildRailDepotWidgets
		 *   - TransportType::Road = #BuildRoadDepotWidgets
		 */
		BuildDepot                                   = to_underlying(::WindowClass::BuildDepot),

		/**
		 * Build waypoint; %Window numbers:
		 *   - TransportType::Rail = #BuildRailWaypointWidgets
		 */
		BuildWaypoint                                = to_underlying(::WindowClass::BuildWaypoint),

		/**
		 * Found a town; %Window numbers:
		 *   - 0 = #TownFoundingWidgets
		 */
		FoundTown                                    = to_underlying(::WindowClass::FoundTown),

		/**
		 * Build industry; %Window numbers:
		 *   - 0 = #DynamicPlaceIndustriesWidgets
		 */
		BuildIndustry                                = to_underlying(::WindowClass::BuildIndustry),


		/**
		 * Select game window; %Window numbers:
		 *   - 0 = #SelectGameIntroWidgets
		 */
		SelectGame                                   = to_underlying(::WindowClass::SelectGame),

		/**
		 * Landscape generation (in Scenario Editor); %Window numbers:
		 *   - 0 = #TerraformToolbarWidgets
		 *   - 0 = #EditorTerraformToolbarWidgets
		 */
		ScenarioGenerateLandscape                    = to_underlying(::WindowClass::ScenarioGenerateLandscape),

		/**
		 * Generate landscape (newgame); %Window numbers:
		 *   - GLWM_SCENARIO = #CreateScenarioWidgets
		 *   - GenerateLandscapeWindowMode = #GenerateLandscapeWidgets
		 */
		GenerateLandscape                            = to_underlying(::WindowClass::GenerateLandscape),

		/**
		 * Progress report of landscape generation; %Window numbers:
		 *   - 0 = #GenerationProgressWidgets
		 *   - 1 = #ScanProgressWidgets
		 */
		ModalProgress                                = to_underlying(::WindowClass::ModalProgress),


		/**
		 * Network window; %Window numbers:
		 *   - NetworkWindowNumber::Game = #NetworkGameWidgets
		 *   - NetworkWindowNumber::ContentList = #NetworkContentListWidgets
		 *   - NetworkWindowNumber::StartServer = #NetworkStartServerWidgets
		 */
		Network                                      = to_underlying(::WindowClass::Network),

		/**
		 * Client list; %Window numbers:
		 *   - 0 = #ClientListWidgets
		 */
		NetworkClientList                            = to_underlying(::WindowClass::NetworkClientList),

		/**
		 * Network status window; %Window numbers:
		 *   - NetworkStatusWindowNumber::Join = #NetworkJoinStatusWidgets
		 *   - NetworkStatusWindowNumber::ContentDownload = #NetworkContentDownloadStatusWidgets
		 */
		NetworkStatus                                = to_underlying(::WindowClass::NetworkStatus),

		/**
		 * Network ask relay window; %Window numbers:
		 *   - 0 - #NetworkAskRelayWidgets
		 */
		NetworkAskRelay                              = to_underlying(::WindowClass::NetworkAskRelay),

		/**
		 * Network ask survey window; %Window numbers:
		 *  - 0 - #NetworkAskSurveyWidgets
		 */
		NetworkAskSurvey                             = to_underlying(::WindowClass::NetworkAskSurvey),

		/**
		 * Chatbox; %Window numbers:
		 *   - NetworkChatDestinationType = #NetWorkChatWidgets
		 */
		NetworkChat                                  = to_underlying(::WindowClass::NetworkChat),

		/**
		 * Industry cargoes chain; %Window numbers:
		 *   - 0 = #IndustryCargoesWidgets
		 */
		IndustryCargoes                              = to_underlying(::WindowClass::IndustryCargoes),

		/**
		 * Legend for graphs; %Window numbers:
		 *   - 0 = #GraphLegendWidgets
		 */
		GraphLegend                                  = to_underlying(::WindowClass::GraphLegend),

		/**
		 * Finances of a company; %Window numbers:
		 *   - CompanyID = #CompanyWidgets
		 */
		Finances                                     = to_underlying(::WindowClass::Finances),

		/**
		 * Income graph; %Window numbers:
		 *   - 0 = #CompanyValueWidgets
		 */
		IncomeGraph                                  = to_underlying(::WindowClass::IncomeGraph),

		/**
		 * Operating profit graph; %Window numbers:
		 *   - 0 = #CompanyValueWidgets
		 */
		OperatingProfitGraph                         = to_underlying(::WindowClass::OperatingProfitGraph),

		/**
		 * Delivered cargo graph; %Window numbers:
		 *   - 0 = #CompanyValueWidgets
		 */
		DeliveredCargoGraph                          = to_underlying(::WindowClass::DeliveredCargoGraph),

		/**
		 * Performance history graph; %Window numbers:
		 *   - 0 = #PerformanceHistoryGraphWidgets
		 */
		PerformanceGraph                             = to_underlying(::WindowClass::PerformanceGraph),

		/**
		 * Company value graph; %Window numbers:
		 *   - 0 = #CompanyValueWidgets
		 */
		CompanyValueGraph                            = to_underlying(::WindowClass::CompanyValueGraph),

		/**
		 * Company league window; %Window numbers:
		 *   - 0 = #CompanyLeagueWidgets
		 */
		CompanyLeague                                = to_underlying(::WindowClass::CompanyLeague),

		/**
		 * Payment rates graph; %Window numbers:
		 *   - 0 = #CargoPaymentRatesWidgets
		 */
		CargoPaymentRatesGraph                       = to_underlying(::WindowClass::CargoPaymentRatesGraph),

		/**
		 * Performance detail window; %Window numbers:
		 *   - 0 = #PerformanceRatingDetailsWidgets
		 */
		PerformanceDetail                            = to_underlying(::WindowClass::PerformanceDetail),

		/**
		 * Industry production history graph; %Window numbers:
		 *   - IndustryID = #IndustryProductionGraphWidgets
		 */
		IndustryProductionGraph                      = to_underlying(::WindowClass::IndustryProductionGraph),

		/**
		 * Town cargo history graph; %Window numbers:
		 *   - TownID = #GraphWidgets
		 */
		TownCargoGraph                               = to_underlying(::WindowClass::TownCargoGraph),

		/**
		 * Company infrastructure overview; %Window numbers:
		 *   - CompanyID = #CompanyInfrastructureWidgets
		 */
		CompanyInfrastructure                        = to_underlying(::WindowClass::CompanyInfrastructure),


		/**
		 * Buyout company (merger); %Window numbers:
		 *   - CompanyID = #BuyCompanyWidgets
		 */
		BuyCompany                                   = to_underlying(::WindowClass::BuyCompany),

		/**
		 * Engine preview window; %Window numbers:
		 *   - 0 = #EnginePreviewWidgets
		 */
		EnginePreview                                = to_underlying(::WindowClass::EnginePreview),


		/**
		 * Music window; %Window numbers:
		 *   - 0 = #MusicWidgets
		 */
		Music                                        = to_underlying(::WindowClass::Music),

		/**
		 * Music track selection; %Window numbers:
		 *   - 0 = MusicTrackSelectionWidgets
		 */
		MusicTrackSelection                          = to_underlying(::WindowClass::MusicTrackSelection),

		/**
		 * Game options window; %Window numbers:
		 *   - GameOptionsWindowNumber::AI = #AIConfigWidgets
		 *   - GameOptionsWindowNumber::GS = #GSConfigWidgets
		 *   - GameOptionsWindowNumber::About = #AboutWidgets
		 *   - GameOptionsWindowNumber::NewGRFState = #NewGRFStateWidgets
		 *   - GameOptionsWindowNumber::GameOptions = #GameOptionsWidgets
		 */
		GameOptions                                  = to_underlying(::WindowClass::GameOptions),

		/**
		 * Custom currency; %Window numbers:
		 *   - 0 = #CustomCurrencyWidgets
		 */
		CustomCurrenty                               = to_underlying(::WindowClass::CustomCurrenty),

		/**
		 * Cheat window; %Window numbers:
		 *   - 0 = #CheatWidgets
		 */
		Cheat                                        = to_underlying(::WindowClass::Cheat),

		/**
		 * Extra viewport; %Window numbers:
		 *   - Ascending value = #ExtraViewportWidgets
		 */
		ExtraViewport                                = to_underlying(::WindowClass::ExtraViewport),


		/**
		 * Console; %Window numbers:
		 *   - 0 = #ConsoleWidgets
		 */
		Console                                      = to_underlying(::WindowClass::Console),

		/**
		 * Bootstrap; %Window numbers:
		 *   - 0 = #BootstrapBackgroundWidgets
		 */
		Bootstrap                                    = to_underlying(::WindowClass::Bootstrap),

		/**
		 * Highscore; %Window numbers:
		 *   - 0 = #HighscoreWidgets
		 */
		Highscore                                    = to_underlying(::WindowClass::Highscore),

		/**
		 * Endscreen; %Window numbers:
		 *   - 0 = #HighscoreWidgets
		 */
		Endscreen                                    = to_underlying(::WindowClass::Endscreen),


		/**
		 * Script debug window; %Window numbers:
		 *   - Ascending value = #ScriptDebugWidgets
		 */
		ScriptDebug                                  = to_underlying(::WindowClass::ScriptDebug),

		/**
		 * NewGRF inspect (debug); %Window numbers:
		 *   - Packed value = #NewGRFInspectWidgets
		 */
		NewGRFInspect                                = to_underlying(::WindowClass::NewGRFInspect),

		/**
		 * Sprite aligner (debug); %Window numbers:
		 *   - 0 = #SpriteAlignerWidgets
		 */
		SpriteAligner                                = to_underlying(::WindowClass::SpriteAligner),

		/**
		 * Linkgraph legend; %Window numbers:
		 *   - 0 = #LinkGraphWidgets
		 */
		LinkGraphLegend                              = to_underlying(::WindowClass::LinkGraphLegend),

		/**
		 * Save preset; %Window numbers:
		 *   - 0 = #SavePresetWidgets
		 */
		SavePreset                                   = to_underlying(::WindowClass::SavePreset),

		/**
		 * Framerate display; %Window numbers:
		 *   - 0 = #FramerateDisplayWidgets
		 */
		FramerateDisplay                             = to_underlying(::WindowClass::FramerateDisplay),

		/**
		 * Frame time graph; %Window numbers:
		 *   - 0 = #FrametimeGraphWindowWidgets
		 */
		FrametimeGraph                               = to_underlying(::WindowClass::FrametimeGraph),

		/**
		 * Screenshot window; %Window numbers:
		 *   - 0 = #ScreenshotWidgets
		 */
		Screenshot                                   = to_underlying(::WindowClass::Screenshot),

		/**
		 * Help and manuals window; %Window numbers:
		 *   - 0 = #HelpWindowWidgets
		 */
		Help                                         = to_underlying(::WindowClass::Help),

		LayerPortal                                  = to_underlying(::WindowClass::LayerPortal),                                  ///< Native pair status/connection controls; window number is the actual canonical pair tile. Appended to preserve existing classes.
		UndergroundConstruction                      = to_underlying(::WindowClass::UndergroundConstruction),                      ///< Explicit rock-layer tool context; window number is the requested layer.
		TBMProjects                                  = to_underlying(::WindowClass::TBMProjects),                                  ///< Actual owned project browser; explicit work-layer context (r2tbmf-tbm-fixes.md Group2).

		SpaceSurvey                                  = to_underlying(::WindowClass::SpaceSurvey),                                  ///< Read-only persistent Space field and staged survey inspector.
		LaunchAssembly                               = to_underlying(::WindowClass::LaunchAssembly),                               ///< Native Surface hull assembly and retained paid work.
		SpaceConstruction                            = to_underlying(::WindowClass::SpaceConstruction),                            ///< Native orbital blueprints and delivered construction jobs.
		Invalid                                      = to_underlying(::WindowClass::Invalid),                                      ///< Invalid window.
	};

	// @endenum

	/**
	 * The colours in the game which you can use for text and highlights.
	 */
	enum TextColour {
		/* Note: these values represent part of the in-game TextColour enum */
		TC_BLUE = to_underlying(::TextColour::Blue), ///< Blue colour.
		TC_SILVER = to_underlying(::TextColour::Silver), ///< Silver colour.
		TC_GOLD = to_underlying(::TextColour::Gold), ///< Gold colour.
		TC_RED = to_underlying(::TextColour::Red), ///< Red colour.
		TC_PURPLE = to_underlying(::TextColour::Purple), ///< Purple colour.
		TC_LIGHT_BROWN = to_underlying(::TextColour::LightBrown), ///< Light brown colour.
		TC_ORANGE = to_underlying(::TextColour::Orange), ///< Orange colour.
		TC_GREEN = to_underlying(::TextColour::Green), ///< Green colour.
		TC_YELLOW = to_underlying(::TextColour::Yellow), ///< Yellow colour.
		TC_DARK_GREEN = to_underlying(::TextColour::DarkGreen), ///< Dark green colour.
		TC_CREAM = to_underlying(::TextColour::Cream), ///< Cream colour.
		TC_BROWN = to_underlying(::TextColour::Brown), ///< Brown colour.
		TC_WHITE = to_underlying(::TextColour::White), ///< White colour.
		TC_LIGHT_BLUE = to_underlying(::TextColour::LightBlue), ///< Light blue colour.
		TC_GREY = to_underlying(::TextColour::Grey), ///< Grey colour.
		TC_DARK_BLUE = to_underlying(::TextColour::DarkBlue), ///< Dark blue colour.
		TC_BLACK = to_underlying(::TextColour::Black), ///< Black colour.
		TC_INVALID = to_underlying(::TextColour::Invalid), ///< Invalid colour.
	};

	/**
	 * Special number values.
	 */
	enum NumberType {
		NUMBER_ALL = -1, ///< Value to select all windows of a class.
	};

	/**
	 * Special widget values.
	 */
	enum WidgetType {
		WIDGET_ALL = -1, ///< Value to select all widgets of a window.
	};

	/**
	 * Close a window.
	 * @param window The class of the window to close.
	 * @param number The number of the window to close, or NUMBER_ALL to close all of this class.
	 *               The value will be clamped to 0 .. MAX(int32_t) when value is not NUMBER_ALL.
	 * @pre !ScriptGame::IsMultiplayer().
	 */
	static void Close(WindowClass window, SQInteger number);

	/**
	 * Check if a window is open.
	 * @param window The class of the window to check for.
	 * @param number The number of the window to check for, or NUMBER_ALL to check for any in the class.
	 *               The value will be clamped to 0 .. MAX(int32_t) when value is not NUMBER_ALL.
	 * @pre !ScriptGame::IsMultiplayer().
	 * @return True if the window is open.
	 */
	static bool IsOpen(WindowClass window, SQInteger number);

	/**
	 * Highlight a widget in a window.
	 * @param window The class of the window to highlight a widget in.
	 * @param number The number of the window to highlight a widget in.
	 *               The value will be clamped to 0 .. MAX(int32_t) when value is not NUMBER_ALL.
	 * @param widget The widget in the window to highlight, or WIDGET_ALL (in combination with TC_INVALID) to disable all widget highlighting on this window.
	 *               The value will be clamped to 0 .. MAX(uint8_t) when value is not WIDGET_ALL.
	 * @param colour The colour of the highlight, or TC_INVALID for disabling.
	 * @pre !ScriptGame::IsMultiplayer().
	 * @pre number != NUMBER_ALL.
	 * @pre colour < TC_END || (widget == WIDGET_ALL && colour == TC_INVALID).
	 * @pre IsOpen(window, number).
	 */
	static void Highlight(WindowClass window, SQInteger number, SQInteger widget, TextColour colour);

	// @enum .*Widgets ../../widgets/*_widget.h
	/* automatically generated from ../../widgets/ai_widget.h */
	/** Widgets of the AIConfigWindow class. */
	enum AIConfigWidgets {
		WID_AIC_BACKGROUND                           = to_underlying(::AIConfigWidgets::WID_AIC_BACKGROUND),                           ///< Window background.
		WID_AIC_DECREASE_NUMBER                      = to_underlying(::AIConfigWidgets::WID_AIC_DECREASE_NUMBER),                      ///< Decrease the number of AIs.
		WID_AIC_INCREASE_NUMBER                      = to_underlying(::AIConfigWidgets::WID_AIC_INCREASE_NUMBER),                      ///< Increase the number of AIs.
		WID_AIC_NUMBER                               = to_underlying(::AIConfigWidgets::WID_AIC_NUMBER),                               ///< Number of AIs.
		WID_AIC_DECREASE_INTERVAL                    = to_underlying(::AIConfigWidgets::WID_AIC_DECREASE_INTERVAL),                    ///< Decrease the interval.
		WID_AIC_INCREASE_INTERVAL                    = to_underlying(::AIConfigWidgets::WID_AIC_INCREASE_INTERVAL),                    ///< Increase the interval.
		WID_AIC_INTERVAL                             = to_underlying(::AIConfigWidgets::WID_AIC_INTERVAL),                             ///< Interval between time AIs start.
		WID_AIC_LIST                                 = to_underlying(::AIConfigWidgets::WID_AIC_LIST),                                 ///< List with currently selected AIs.
		WID_AIC_SCROLLBAR                            = to_underlying(::AIConfigWidgets::WID_AIC_SCROLLBAR),                            ///< Scrollbar to scroll through the selected AIs.
		WID_AIC_MOVE_UP                              = to_underlying(::AIConfigWidgets::WID_AIC_MOVE_UP),                              ///< Move up button.
		WID_AIC_MOVE_DOWN                            = to_underlying(::AIConfigWidgets::WID_AIC_MOVE_DOWN),                            ///< Move down button.
		WID_AIC_CHANGE                               = to_underlying(::AIConfigWidgets::WID_AIC_CHANGE),                               ///< Select another AI button.
		WID_AIC_CONFIGURE                            = to_underlying(::AIConfigWidgets::WID_AIC_CONFIGURE),                            ///< Change AI settings button.
		WID_AIC_OPEN_URL                             = to_underlying(::AIConfigWidgets::WID_AIC_OPEN_URL),                             ///< Open AI URL.
		WID_AIC_TEXTFILE                             = to_underlying(::AIConfigWidgets::WID_AIC_TEXTFILE),                             ///< Open AI readme, changelog (+1) or license (+2).
		WID_AIC_CONTENT_DOWNLOAD                     = to_underlying(::AIConfigWidgets::WID_AIC_CONTENT_DOWNLOAD),                     ///< Download content button.
	};

	/* automatically generated from ../../widgets/airport_widget.h */
	/** Widgets of the BuildAirToolbarWindow class. */
	enum AirportToolbarWidgets {
		WID_AT_AIRPORT                               = to_underlying(::AirportToolbarWidgets::WID_AT_AIRPORT),                               ///< Build airport button.
		WID_AT_DEMOLISH                              = to_underlying(::AirportToolbarWidgets::WID_AT_DEMOLISH),                              ///< Demolish button.
		WID_AT_LAUNCH_CENTER                         = to_underlying(::AirportToolbarWidgets::WID_AT_LAUNCH_CENTER),                         ///< Distinct Space launch facility.
		WID_AT_LAUNCH_ROTATE                         = to_underlying(::AirportToolbarWidgets::WID_AT_LAUNCH_ROTATE),                         ///< Rotate its land footprint.
	};

	/** Widgets of the BuildAirportWindow class. */
	enum AirportPickerWidgets {
		WID_AP_CLASS_DROPDOWN                        = to_underlying(::AirportPickerWidgets::WID_AP_CLASS_DROPDOWN),                        ///< Dropdown of airport classes.
		WID_AP_AIRPORT_LIST                          = to_underlying(::AirportPickerWidgets::WID_AP_AIRPORT_LIST),                          ///< List of airports.
		WID_AP_SCROLLBAR                             = to_underlying(::AirportPickerWidgets::WID_AP_SCROLLBAR),                             ///< Scrollbar of the list.
		WID_AP_LAYOUT_NUM                            = to_underlying(::AirportPickerWidgets::WID_AP_LAYOUT_NUM),                            ///< Current number of the layout.
		WID_AP_LAYOUT_DECREASE                       = to_underlying(::AirportPickerWidgets::WID_AP_LAYOUT_DECREASE),                       ///< Decrease the layout number.
		WID_AP_LAYOUT_INCREASE                       = to_underlying(::AirportPickerWidgets::WID_AP_LAYOUT_INCREASE),                       ///< Increase the layout number.
		WID_AP_AIRPORT_SPRITE                        = to_underlying(::AirportPickerWidgets::WID_AP_AIRPORT_SPRITE),                        ///< A visual display of the airport currently selected.
		WID_AP_EXTRA_TEXT                            = to_underlying(::AirportPickerWidgets::WID_AP_EXTRA_TEXT),                            ///< Additional text about the airport.
		WID_AP_COVERAGE_LABEL                        = to_underlying(::AirportPickerWidgets::WID_AP_COVERAGE_LABEL),                        ///< Label if you want to see the coverage.
		WID_AP_BTN_DONTHILIGHT                       = to_underlying(::AirportPickerWidgets::WID_AP_BTN_DONTHILIGHT),                       ///< Don't show the coverage button.
		WID_AP_BTN_DOHILIGHT                         = to_underlying(::AirportPickerWidgets::WID_AP_BTN_DOHILIGHT),                         ///< Show the coverage button.
		WID_AP_ACCEPTANCE                            = to_underlying(::AirportPickerWidgets::WID_AP_ACCEPTANCE),                            ///< Acceptance info.
	};

	/* automatically generated from ../../widgets/autoreplace_widget.h */
	/** Widgets of the ReplaceVehicleWindow class. */
	enum ReplaceVehicleWidgets {
		WID_RV_CAPTION                               = to_underlying(::ReplaceVehicleWidgets::WID_RV_CAPTION),                               ///< Caption of the window.

		/* Sort dropdown at the right. */
		WID_RV_SORT_ASCENDING_DESCENDING             = to_underlying(::ReplaceVehicleWidgets::WID_RV_SORT_ASCENDING_DESCENDING),             ///< Ascending/descending sort order button.
		WID_RV_SHOW_HIDDEN_ENGINES                   = to_underlying(::ReplaceVehicleWidgets::WID_RV_SHOW_HIDDEN_ENGINES),                   ///< Toggle whether to display the hidden vehicles.
		WID_RV_SORT_DROPDOWN                         = to_underlying(::ReplaceVehicleWidgets::WID_RV_SORT_DROPDOWN),                         ///< Dropdown for the sort criteria.

		/* Left and right matrix + details. */
		WID_RV_LEFT_MATRIX                           = to_underlying(::ReplaceVehicleWidgets::WID_RV_LEFT_MATRIX),                           ///< The matrix on the left.
		WID_RV_LEFT_SCROLLBAR                        = to_underlying(::ReplaceVehicleWidgets::WID_RV_LEFT_SCROLLBAR),                        ///< The scrollbar for the matrix on the left.
		WID_RV_RIGHT_MATRIX                          = to_underlying(::ReplaceVehicleWidgets::WID_RV_RIGHT_MATRIX),                          ///< The matrix on the right.
		WID_RV_RIGHT_SCROLLBAR                       = to_underlying(::ReplaceVehicleWidgets::WID_RV_RIGHT_SCROLLBAR),                       ///< The scrollbar for the matrix on the right.
		WID_RV_LEFT_DETAILS                          = to_underlying(::ReplaceVehicleWidgets::WID_RV_LEFT_DETAILS),                          ///< Details of the entry on the left.
		WID_RV_RIGHT_DETAILS                         = to_underlying(::ReplaceVehicleWidgets::WID_RV_RIGHT_DETAILS),                         ///< Details of the entry on the right.

		/* Button row. */
		WID_RV_START_REPLACE                         = to_underlying(::ReplaceVehicleWidgets::WID_RV_START_REPLACE),                         ///< Start Replacing button.
		WID_RV_INFO_TAB                              = to_underlying(::ReplaceVehicleWidgets::WID_RV_INFO_TAB),                              ///< Info tab.
		WID_RV_STOP_REPLACE                          = to_underlying(::ReplaceVehicleWidgets::WID_RV_STOP_REPLACE),                          ///< Stop Replacing button.

		/* Train only widgets. */
		WID_RV_RAIL_TYPE_DROPDOWN                    = to_underlying(::ReplaceVehicleWidgets::WID_RV_RAIL_TYPE_DROPDOWN),                    ///< Dropdown to select railtype.
		WID_RV_TRAIN_ENGINEWAGON_DROPDOWN            = to_underlying(::ReplaceVehicleWidgets::WID_RV_TRAIN_ENGINEWAGON_DROPDOWN),            ///< Dropdown to select engines and/or wagons.
		WID_RV_TRAIN_WAGONREMOVE_TOGGLE              = to_underlying(::ReplaceVehicleWidgets::WID_RV_TRAIN_WAGONREMOVE_TOGGLE),              ///< Button to toggle removing wagons.

		/* Road only widgets. */
		WID_RV_ROAD_TYPE_DROPDOWN                    = to_underlying(::ReplaceVehicleWidgets::WID_RV_ROAD_TYPE_DROPDOWN),                    ///< Dropdown to select roadtype.
	};

	/* automatically generated from ../../widgets/bootstrap_widget.h */
	/** Widgets of the BootstrapBackground class. */
	enum BootstrapBackgroundWidgets {
		WID_BB_BACKGROUND                            = to_underlying(::BootstrapBackgroundWidgets::WID_BB_BACKGROUND),                            ///< Background of the window.
	};

	/** Widgets of the BootstrapErrmsgWindow class. */
	enum BootstrapErrorMessageWidgets {
		WID_BEM_CAPTION                              = to_underlying(::BootstrapErrorMessageWidgets::WID_BEM_CAPTION),                              ///< Caption of the window.
		WID_BEM_MESSAGE                              = to_underlying(::BootstrapErrorMessageWidgets::WID_BEM_MESSAGE),                              ///< Error message.
		WID_BEM_QUIT                                 = to_underlying(::BootstrapErrorMessageWidgets::WID_BEM_QUIT),                                 ///< Quit button.
	};

	/** Widgets of the BootstrapContentDownloadStatusWindow class. */
	enum BootstrapAskForDownloadWidgets {
		WID_BAFD_QUESTION                            = to_underlying(::BootstrapAskForDownloadWidgets::WID_BAFD_QUESTION),                            ///< The question whether to download.
		WID_BAFD_YES                                 = to_underlying(::BootstrapAskForDownloadWidgets::WID_BAFD_YES),                                 ///< An affirmative answer to the question.
		WID_BAFD_NO                                  = to_underlying(::BootstrapAskForDownloadWidgets::WID_BAFD_NO),                                  ///< An negative answer to the question.
	};

	/* automatically generated from ../../widgets/bridge_widget.h */
	/** Widgets of the BuildBridgeWindow class. */
	enum BuildBridgeSelectionWidgets {
		WID_BBS_CAPTION                              = to_underlying(::BuildBridgeSelectionWidgets::WID_BBS_CAPTION),                              ///< Caption of the window.
		WID_BBS_DROPDOWN_ORDER                       = to_underlying(::BuildBridgeSelectionWidgets::WID_BBS_DROPDOWN_ORDER),                       ///< Direction of sort dropdown.
		WID_BBS_DROPDOWN_CRITERIA                    = to_underlying(::BuildBridgeSelectionWidgets::WID_BBS_DROPDOWN_CRITERIA),                    ///< Criteria of sort dropdown.
		WID_BBS_BRIDGE_LIST                          = to_underlying(::BuildBridgeSelectionWidgets::WID_BBS_BRIDGE_LIST),                          ///< List of bridges.
		WID_BBS_SCROLLBAR                            = to_underlying(::BuildBridgeSelectionWidgets::WID_BBS_SCROLLBAR),                            ///< Scrollbar of the list.
	};

	/* automatically generated from ../../widgets/build_vehicle_widget.h */
	/** Widgets of the BuildVehicleWindow class. */
	enum BuildVehicleWidgets {
		WID_BV_CAPTION                               = to_underlying(::BuildVehicleWidgets::WID_BV_CAPTION),                               ///< Caption of window.
		WID_BV_SORT_ASCENDING_DESCENDING             = to_underlying(::BuildVehicleWidgets::WID_BV_SORT_ASCENDING_DESCENDING),             ///< Sort direction.
		WID_BV_SORT_DROPDOWN                         = to_underlying(::BuildVehicleWidgets::WID_BV_SORT_DROPDOWN),                         ///< Criteria of sorting dropdown.
		WID_BV_CARGO_FILTER_DROPDOWN                 = to_underlying(::BuildVehicleWidgets::WID_BV_CARGO_FILTER_DROPDOWN),                 ///< Cargo filter dropdown.
		WID_BV_FILTER                                = to_underlying(::BuildVehicleWidgets::WID_BV_FILTER),                                ///< Filter by name.
		WID_BV_SHOW_HIDDEN_ENGINES                   = to_underlying(::BuildVehicleWidgets::WID_BV_SHOW_HIDDEN_ENGINES),                   ///< Toggle whether to display the hidden vehicles.
		WID_BV_LIST                                  = to_underlying(::BuildVehicleWidgets::WID_BV_LIST),                                  ///< List of vehicles.
		WID_BV_SCROLLBAR                             = to_underlying(::BuildVehicleWidgets::WID_BV_SCROLLBAR),                             ///< Scrollbar of list.
		WID_BV_PANEL                                 = to_underlying(::BuildVehicleWidgets::WID_BV_PANEL),                                 ///< Button panel.
		WID_BV_BUILD                                 = to_underlying(::BuildVehicleWidgets::WID_BV_BUILD),                                 ///< Build panel.
		WID_BV_SHOW_HIDE                             = to_underlying(::BuildVehicleWidgets::WID_BV_SHOW_HIDE),                             ///< Button to hide or show the selected engine.
		WID_BV_BUILD_SEL                             = to_underlying(::BuildVehicleWidgets::WID_BV_BUILD_SEL),                             ///< Build button.
		WID_BV_RENAME                                = to_underlying(::BuildVehicleWidgets::WID_BV_RENAME),                                ///< Rename button.
		WID_BV_CONFIGURE_BADGES                      = to_underlying(::BuildVehicleWidgets::WID_BV_CONFIGURE_BADGES),                      ///< Button to configure badges.
		WID_BV_BADGE_FILTER                          = to_underlying(::BuildVehicleWidgets::WID_BV_BADGE_FILTER),                          ///< Container for dropdown badge filters.
	};

	/* automatically generated from ../../widgets/cheat_widget.h */
	/** Widgets of the CheatWindow class. */
	enum CheatWidgets {
		WID_C_NOTE                                   = to_underlying(::CheatWidgets::WID_C_NOTE),                                   ///< Note on top of panel for use of cheat.
		WID_C_PANEL                                  = to_underlying(::CheatWidgets::WID_C_PANEL),                                  ///< Panel where all cheats are shown in.
		WID_C_SETTINGS                               = to_underlying(::CheatWidgets::WID_C_SETTINGS),                               ///< Panel where sandbox settings are shown.
	};

	/* automatically generated from ../../widgets/company_widget.h */
	/** Widgets of the CompanyWindow class. */
	enum CompanyWidgets {
		WID_C_CAPTION                                = to_underlying(::CompanyWidgets::WID_C_CAPTION),                                ///< Caption of the window.

		WID_C_FACE                                   = to_underlying(::CompanyWidgets::WID_C_FACE),                                   ///< View of the face.
		WID_C_FACE_TITLE                             = to_underlying(::CompanyWidgets::WID_C_FACE_TITLE),                             ///< Title for the face.

		WID_C_DESC_INAUGURATION                      = to_underlying(::CompanyWidgets::WID_C_DESC_INAUGURATION),                      ///< Inauguration.
		WID_C_DESC_COLOUR_SCHEME                     = to_underlying(::CompanyWidgets::WID_C_DESC_COLOUR_SCHEME),                     ///< Colour scheme.
		WID_C_DESC_COLOUR_SCHEME_EXAMPLE             = to_underlying(::CompanyWidgets::WID_C_DESC_COLOUR_SCHEME_EXAMPLE),             ///< Colour scheme example.
		WID_C_DESC_VEHICLE                           = to_underlying(::CompanyWidgets::WID_C_DESC_VEHICLE),                           ///< Vehicles.
		WID_C_DESC_VEHICLE_COUNTS                    = to_underlying(::CompanyWidgets::WID_C_DESC_VEHICLE_COUNTS),                    ///< Vehicle count.
		WID_C_DESC_COMPANY_VALUE                     = to_underlying(::CompanyWidgets::WID_C_DESC_COMPANY_VALUE),                     ///< Company value.
		WID_C_DESC_INFRASTRUCTURE                    = to_underlying(::CompanyWidgets::WID_C_DESC_INFRASTRUCTURE),                    ///< Infrastructure.
		WID_C_DESC_INFRASTRUCTURE_COUNTS             = to_underlying(::CompanyWidgets::WID_C_DESC_INFRASTRUCTURE_COUNTS),             ///< Infrastructure count.

		WID_C_SELECT_BUTTONS                         = to_underlying(::CompanyWidgets::WID_C_SELECT_BUTTONS),                         ///< Selection widget for the button bar.
		WID_C_NEW_FACE                               = to_underlying(::CompanyWidgets::WID_C_NEW_FACE),                               ///< Button to make new face.
		WID_C_COLOUR_SCHEME                          = to_underlying(::CompanyWidgets::WID_C_COLOUR_SCHEME),                          ///< Button to change colour scheme.
		WID_C_PRESIDENT_NAME                         = to_underlying(::CompanyWidgets::WID_C_PRESIDENT_NAME),                         ///< Button to change president name.
		WID_C_COMPANY_NAME                           = to_underlying(::CompanyWidgets::WID_C_COMPANY_NAME),                           ///< Button to change company name.

		WID_C_SELECT_VIEW_BUILD_HQ                   = to_underlying(::CompanyWidgets::WID_C_SELECT_VIEW_BUILD_HQ),                   ///< Panel about HQ.
		WID_C_VIEW_HQ                                = to_underlying(::CompanyWidgets::WID_C_VIEW_HQ),                                ///< Button to view the HQ.
		WID_C_BUILD_HQ                               = to_underlying(::CompanyWidgets::WID_C_BUILD_HQ),                               ///< Button to build the HQ.

		WID_C_SELECT_RELOCATE                        = to_underlying(::CompanyWidgets::WID_C_SELECT_RELOCATE),                        ///< Panel about 'Relocate HQ'.
		WID_C_RELOCATE_HQ                            = to_underlying(::CompanyWidgets::WID_C_RELOCATE_HQ),                            ///< Button to relocate the HQ.

		WID_C_VIEW_INFRASTRUCTURE                    = to_underlying(::CompanyWidgets::WID_C_VIEW_INFRASTRUCTURE),                    ///< Panel about infrastructure.

		WID_C_SELECT_GIVE_MONEY                      = to_underlying(::CompanyWidgets::WID_C_SELECT_GIVE_MONEY),                      ///< Selection widget for the give money button.
		WID_C_GIVE_MONEY                             = to_underlying(::CompanyWidgets::WID_C_GIVE_MONEY),                             ///< Button to give money.

		WID_C_SELECT_HOSTILE_TAKEOVER                = to_underlying(::CompanyWidgets::WID_C_SELECT_HOSTILE_TAKEOVER),                ///< Selection widget for the hostile takeover button.
		WID_C_HOSTILE_TAKEOVER                       = to_underlying(::CompanyWidgets::WID_C_HOSTILE_TAKEOVER),                       ///< Button to hostile takeover another company.

		WID_C_SELECT_MULTIPLAYER                     = to_underlying(::CompanyWidgets::WID_C_SELECT_MULTIPLAYER),                     ///< Multiplayer selection panel.
		WID_C_COMPANY_JOIN                           = to_underlying(::CompanyWidgets::WID_C_COMPANY_JOIN),                           ///< Button to join company.
	};

	/** Widgets of the CompanyFinancesWindow class. */
	enum CompanyFinancesWidgets {
		WID_CF_CAPTION                               = to_underlying(::CompanyFinancesWidgets::WID_CF_CAPTION),                               ///< Caption of the window.
		WID_CF_TOGGLE_SIZE                           = to_underlying(::CompanyFinancesWidgets::WID_CF_TOGGLE_SIZE),                           ///< Toggle windows size.
		WID_CF_SEL_PANEL                             = to_underlying(::CompanyFinancesWidgets::WID_CF_SEL_PANEL),                             ///< Select panel or nothing.
		WID_CF_EXPS_CATEGORY                         = to_underlying(::CompanyFinancesWidgets::WID_CF_EXPS_CATEGORY),                         ///< Column for expenses category strings.
		WID_CF_EXPS_PRICE1                           = to_underlying(::CompanyFinancesWidgets::WID_CF_EXPS_PRICE1),                           ///< Column for year Y-2 expenses.
		WID_CF_EXPS_PRICE2                           = to_underlying(::CompanyFinancesWidgets::WID_CF_EXPS_PRICE2),                           ///< Column for year Y-1 expenses.
		WID_CF_EXPS_PRICE3                           = to_underlying(::CompanyFinancesWidgets::WID_CF_EXPS_PRICE3),                           ///< Column for year Y expenses.
		WID_CF_TOTAL_PANEL                           = to_underlying(::CompanyFinancesWidgets::WID_CF_TOTAL_PANEL),                           ///< Panel for totals.
		WID_CF_SEL_MAXLOAN                           = to_underlying(::CompanyFinancesWidgets::WID_CF_SEL_MAXLOAN),                           ///< Selection of maxloan column.
		WID_CF_BALANCE_VALUE                         = to_underlying(::CompanyFinancesWidgets::WID_CF_BALANCE_VALUE),                         ///< Bank balance value.
		WID_CF_LOAN_VALUE                            = to_underlying(::CompanyFinancesWidgets::WID_CF_LOAN_VALUE),                            ///< Loan.
		WID_CF_BALANCE_LINE                          = to_underlying(::CompanyFinancesWidgets::WID_CF_BALANCE_LINE),                          ///< Available cash.
		WID_CF_OWN_VALUE                             = to_underlying(::CompanyFinancesWidgets::WID_CF_OWN_VALUE),                             ///< Own funds, not including loan.
		WID_CF_INTEREST_RATE                         = to_underlying(::CompanyFinancesWidgets::WID_CF_INTEREST_RATE),                         ///< Loan interest rate.
		WID_CF_MAXLOAN_VALUE                         = to_underlying(::CompanyFinancesWidgets::WID_CF_MAXLOAN_VALUE),                         ///< Max loan widget.
		WID_CF_SEL_BUTTONS                           = to_underlying(::CompanyFinancesWidgets::WID_CF_SEL_BUTTONS),                           ///< Selection of buttons.
		WID_CF_INCREASE_LOAN                         = to_underlying(::CompanyFinancesWidgets::WID_CF_INCREASE_LOAN),                         ///< Increase loan.
		WID_CF_REPAY_LOAN                            = to_underlying(::CompanyFinancesWidgets::WID_CF_REPAY_LOAN),                            ///< Decrease loan..
		WID_CF_INFRASTRUCTURE                        = to_underlying(::CompanyFinancesWidgets::WID_CF_INFRASTRUCTURE),                        ///< View company infrastructure.
	};

	/** Widgets of the SelectCompanyLiveryWindow class. */
	enum SelectCompanyLiveryWidgets {
		WID_SCL_CAPTION                              = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_CAPTION),                              ///< Caption of window.
		WID_SCL_CLASS_GENERAL                        = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_CLASS_GENERAL),                        ///< Class general.
		WID_SCL_CLASS_RAIL                           = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_CLASS_RAIL),                           ///< Class rail.
		WID_SCL_CLASS_ROAD                           = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_CLASS_ROAD),                           ///< Class road.
		WID_SCL_CLASS_SHIP                           = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_CLASS_SHIP),                           ///< Class ship.
		WID_SCL_CLASS_AIRCRAFT                       = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_CLASS_AIRCRAFT),                       ///< Class aircraft.
		WID_SCL_GROUPS_RAIL                          = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_GROUPS_RAIL),                          ///< Rail groups.
		WID_SCL_GROUPS_ROAD                          = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_GROUPS_ROAD),                          ///< Road groups.
		WID_SCL_GROUPS_SHIP                          = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_GROUPS_SHIP),                          ///< Ship groups.
		WID_SCL_GROUPS_AIRCRAFT                      = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_GROUPS_AIRCRAFT),                      ///< Aircraft groups.
		WID_SCL_SPACER_DROPDOWN                      = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_SPACER_DROPDOWN),                      ///< Spacer for dropdown.
		WID_SCL_PRI_COL_DROPDOWN                     = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_PRI_COL_DROPDOWN),                     ///< Dropdown for primary colour.
		WID_SCL_SEC_COL_DROPDOWN                     = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_SEC_COL_DROPDOWN),                     ///< Dropdown for secondary colour.
		WID_SCL_SEC_COL_DROP_SEL                     = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_SEC_COL_DROP_SEL),                     ///< Container for secondary color dropdown, which can be hidden.
		WID_SCL_MATRIX                               = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_MATRIX),                               ///< Matrix.
		WID_SCL_MATRIX_SCROLLBAR                     = to_underlying(::SelectCompanyLiveryWidgets::WID_SCL_MATRIX_SCROLLBAR),                     ///< Matrix scrollbar.
	};

	/**
	 * Widgets of the SelectCompanyManagerFaceWindow class.
	 */
	enum SelectCompanyManagerFaceWidgets {
		WID_SCMF_CAPTION                             = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_CAPTION),                             ///< Caption of window.
		WID_SCMF_TOGGLE_LARGE_SMALL                  = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_TOGGLE_LARGE_SMALL),                  ///< Toggle for large or small.
		WID_SCMF_SELECT_FACE                         = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_SELECT_FACE),                         ///< Select face.
		WID_SCMF_CANCEL                              = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_CANCEL),                              ///< Cancel.
		WID_SCMF_ACCEPT                              = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_ACCEPT),                              ///< Accept.
		WID_SCMF_SEL_LOADSAVE                        = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_SEL_LOADSAVE),                        ///< Selection to display the load/save/number buttons in the advanced view.
		WID_SCMF_SEL_PARTS                           = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_SEL_PARTS),                           ///< Selection to display the buttons for setting each part of the face in the advanced view.
		WID_SCMF_SEL_RESIZE                          = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_SEL_RESIZE),                          ///< Selection to display the resize button.
		WID_SCMF_RANDOM_NEW_FACE                     = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_RANDOM_NEW_FACE),                     ///< Create random new face.
		WID_SCMF_TOGGLE_LARGE_SMALL_BUTTON           = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_TOGGLE_LARGE_SMALL_BUTTON),           ///< Toggle for large or small.
		WID_SCMF_FACE                                = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_FACE),                                ///< Current face.
		WID_SCMF_LOAD                                = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_LOAD),                                ///< Load face.
		WID_SCMF_FACECODE                            = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_FACECODE),                            ///< Get the face code.
		WID_SCMF_SAVE                                = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_SAVE),                                ///< Save face.
		WID_SCMF_STYLE                               = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_STYLE),                               ///< Style selector widget.
		WID_SCMF_PARTS                               = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_PARTS),                               ///< Face configuration parts widget.
		WID_SCMF_PARTS_SCROLLBAR                     = to_underlying(::SelectCompanyManagerFaceWidgets::WID_SCMF_PARTS_SCROLLBAR),                     ///< Scrollbar for configuration parts widget.
	};

	/** Widgets of the CompanyInfrastructureWindow class. */
	enum CompanyInfrastructureWidgets {
		WID_CI_CAPTION                               = to_underlying(::CompanyInfrastructureWidgets::WID_CI_CAPTION),                               ///< Caption of window.
		WID_CI_LIST                                  = to_underlying(::CompanyInfrastructureWidgets::WID_CI_LIST),                                  ///< Infrastructure list.
		WID_CI_SCROLLBAR                             = to_underlying(::CompanyInfrastructureWidgets::WID_CI_SCROLLBAR),                             ///< Infrastructure list scrollbar.
	};

	/** Widgets of the BuyCompanyWindow class. */
	enum BuyCompanyWidgets {
		WID_BC_CAPTION                               = to_underlying(::BuyCompanyWidgets::WID_BC_CAPTION),                               ///< Caption of window.
		WID_BC_FACE                                  = to_underlying(::BuyCompanyWidgets::WID_BC_FACE),                                  ///< Face button.
		WID_BC_QUESTION                              = to_underlying(::BuyCompanyWidgets::WID_BC_QUESTION),                              ///< Question text.
		WID_BC_NO                                    = to_underlying(::BuyCompanyWidgets::WID_BC_NO),                                    ///< No button.
		WID_BC_YES                                   = to_underlying(::BuyCompanyWidgets::WID_BC_YES),                                   ///< Yes button.
	};

	/* automatically generated from ../../widgets/console_widget.h */
	/** Widgets of the IConsoleWindow class. */
	enum ConsoleWidgets {
		WID_C_BACKGROUND                             = to_underlying(::ConsoleWidgets::WID_C_BACKGROUND),                             ///< Background of the console.
	};

	/* automatically generated from ../../widgets/date_widget.h */
	/** Widgets of the SetDateWindow class. */
	enum SetDateWidgets {
		WID_SD_DAY                                   = to_underlying(::SetDateWidgets::WID_SD_DAY),                                   ///< Dropdown for the day.
		WID_SD_MONTH                                 = to_underlying(::SetDateWidgets::WID_SD_MONTH),                                 ///< Dropdown for the month.
		WID_SD_YEAR                                  = to_underlying(::SetDateWidgets::WID_SD_YEAR),                                  ///< Dropdown for the year.
		WID_SD_SET_DATE                              = to_underlying(::SetDateWidgets::WID_SD_SET_DATE),                              ///< Actually set the date.
	};

	/* automatically generated from ../../widgets/depot_widget.h */
	/** Widgets of the DepotWindow class. */
	enum DepotWidgets {
		WID_D_CAPTION                                = to_underlying(::DepotWidgets::WID_D_CAPTION),                                ///< Caption of window.
		WID_D_SELL                                   = to_underlying(::DepotWidgets::WID_D_SELL),                                   ///< Sell button.
		WID_D_SHOW_SELL_CHAIN                        = to_underlying(::DepotWidgets::WID_D_SHOW_SELL_CHAIN),                        ///< Show sell chain panel.
		WID_D_SELL_CHAIN                             = to_underlying(::DepotWidgets::WID_D_SELL_CHAIN),                             ///< Sell chain button.
		WID_D_SELL_ALL                               = to_underlying(::DepotWidgets::WID_D_SELL_ALL),                               ///< Sell all button.
		WID_D_AUTOREPLACE                            = to_underlying(::DepotWidgets::WID_D_AUTOREPLACE),                            ///< Autoreplace button.
		WID_D_MATRIX                                 = to_underlying(::DepotWidgets::WID_D_MATRIX),                                 ///< Matrix of vehicles.
		WID_D_V_SCROLL                               = to_underlying(::DepotWidgets::WID_D_V_SCROLL),                               ///< Vertical scrollbar.
		WID_D_SHOW_H_SCROLL                          = to_underlying(::DepotWidgets::WID_D_SHOW_H_SCROLL),                          ///< Show horizontal scrollbar panel.
		WID_D_H_SCROLL                               = to_underlying(::DepotWidgets::WID_D_H_SCROLL),                               ///< Horizontal scrollbar.
		WID_D_BUILD                                  = to_underlying(::DepotWidgets::WID_D_BUILD),                                  ///< Build button.
		WID_D_CLONE                                  = to_underlying(::DepotWidgets::WID_D_CLONE),                                  ///< Clone button.
		WID_D_LOCATION                               = to_underlying(::DepotWidgets::WID_D_LOCATION),                               ///< Location button.
		WID_D_SHOW_RENAME                            = to_underlying(::DepotWidgets::WID_D_SHOW_RENAME),                            ///< Show rename panel.
		WID_D_RENAME                                 = to_underlying(::DepotWidgets::WID_D_RENAME),                                 ///< Rename button.
		WID_D_VEHICLE_LIST                           = to_underlying(::DepotWidgets::WID_D_VEHICLE_LIST),                           ///< List of vehicles.
		WID_D_STOP_ALL                               = to_underlying(::DepotWidgets::WID_D_STOP_ALL),                               ///< Stop all button.
		WID_D_START_ALL                              = to_underlying(::DepotWidgets::WID_D_START_ALL),                              ///< Start all button.
	};

	/* automatically generated from ../../widgets/dock_widget.h */
	/** Widgets of the BuildDocksDepotWindow class. */
	enum BuildDockDepotWidgets {
		WID_BDD_BACKGROUND                           = to_underlying(::BuildDockDepotWidgets::WID_BDD_BACKGROUND),                           ///< Background of the window.
		WID_BDD_X                                    = to_underlying(::BuildDockDepotWidgets::WID_BDD_X),                                    ///< X-direction button.
		WID_BDD_Y                                    = to_underlying(::BuildDockDepotWidgets::WID_BDD_Y),                                    ///< Y-direction button.
	};

	/** Widgets of the BuildDocksStationWindow window. */
	enum BuildDocksStationWidgets {
		WID_BDSW_BACKGROUND                          = to_underlying(::BuildDocksStationWidgets::WID_BDSW_BACKGROUND),                          ///< Background panel.
		WID_BDSW_LT_OFF                              = to_underlying(::BuildDocksStationWidgets::WID_BDSW_LT_OFF),                              ///< 'Off' button of coverage high light.
		WID_BDSW_LT_ON                               = to_underlying(::BuildDocksStationWidgets::WID_BDSW_LT_ON),                               ///< 'On' button of coverage high light.
		WID_BDSW_INFO                                = to_underlying(::BuildDocksStationWidgets::WID_BDSW_INFO),                                ///< 'Coverage highlight' label.
		WID_BDSW_ACCEPTANCE                          = to_underlying(::BuildDocksStationWidgets::WID_BDSW_ACCEPTANCE),                          ///< Acceptance info.
	};

	/** Widgets of the BuildDocksToolbarWindow class. */
	enum DockToolbarWidgets {
		WID_DT_CANAL                                 = to_underlying(::DockToolbarWidgets::WID_DT_CANAL),                                 ///< Build canal button.
		WID_DT_LOCK                                  = to_underlying(::DockToolbarWidgets::WID_DT_LOCK),                                  ///< Build lock button.
		WID_DT_DEMOLISH                              = to_underlying(::DockToolbarWidgets::WID_DT_DEMOLISH),                              ///< Demolish aka dynamite button.
		WID_DT_DEPOT                                 = to_underlying(::DockToolbarWidgets::WID_DT_DEPOT),                                 ///< Build depot button.
		WID_DT_STATION                               = to_underlying(::DockToolbarWidgets::WID_DT_STATION),                               ///< Build station button.
		WID_DT_BUOY                                  = to_underlying(::DockToolbarWidgets::WID_DT_BUOY),                                  ///< Build buoy button.
		WID_DT_RIVER                                 = to_underlying(::DockToolbarWidgets::WID_DT_RIVER),                                 ///< Build river button (in scenario editor).
		WID_DT_BUILD_AQUEDUCT                        = to_underlying(::DockToolbarWidgets::WID_DT_BUILD_AQUEDUCT),                        ///< Build aqueduct button.
	};

	/* automatically generated from ../../widgets/dropdown_widget.h */
	/** Widgets of the DropdownWindow class. */
	enum DropdownMenuWidgets {
		WID_DM_FILTER_SEL                            = to_underlying(::DropdownMenuWidgets::WID_DM_FILTER_SEL),                            ///< Selection for item filter.
		WID_DM_FILTER_PANEL                          = to_underlying(::DropdownMenuWidgets::WID_DM_FILTER_PANEL),                          ///< Panel for item filter.
		WID_DM_FILTER                                = to_underlying(::DropdownMenuWidgets::WID_DM_FILTER),                                ///< Item filter.
		WID_DM_ITEMS                                 = to_underlying(::DropdownMenuWidgets::WID_DM_ITEMS),                                 ///< Panel showing the dropdown items.
		WID_DM_SHOW_SCROLL                           = to_underlying(::DropdownMenuWidgets::WID_DM_SHOW_SCROLL),                           ///< Hide scrollbar if too few items.
		WID_DM_SCROLL                                = to_underlying(::DropdownMenuWidgets::WID_DM_SCROLL),                                ///< Scrollbar.
	};

	/* automatically generated from ../../widgets/engine_widget.h */
	/** Widgets of the EnginePreviewWindow class. */
	enum EnginePreviewWidgets {
		WID_EP_CAPTION                               = to_underlying(::EnginePreviewWidgets::WID_EP_CAPTION),                               ///< The caption for the question.
		WID_EP_QUESTION                              = to_underlying(::EnginePreviewWidgets::WID_EP_QUESTION),                              ///< The container for the question.
		WID_EP_NO                                    = to_underlying(::EnginePreviewWidgets::WID_EP_NO),                                    ///< No button.
		WID_EP_YES                                   = to_underlying(::EnginePreviewWidgets::WID_EP_YES),                                   ///< Yes button.
		WID_EP_PREV                                  = to_underlying(::EnginePreviewWidgets::WID_EP_PREV),                                  ///< Previous button.
		WID_EP_LIST                                  = to_underlying(::EnginePreviewWidgets::WID_EP_LIST),                                  ///< Dropdown menu to jump to entries.
		WID_EP_NEXT                                  = to_underlying(::EnginePreviewWidgets::WID_EP_NEXT),                                  ///< Next button.
	};

	/* automatically generated from ../../widgets/error_widget.h */
	/** Widgets of the ErrmsgWindow class. */
	enum ErrorMessageWidgets {
		WID_EM_CAPTION                               = to_underlying(::ErrorMessageWidgets::WID_EM_CAPTION),                               ///< Caption of the window.
		WID_EM_FACE                                  = to_underlying(::ErrorMessageWidgets::WID_EM_FACE),                                  ///< Error title.
		WID_EM_MESSAGE                               = to_underlying(::ErrorMessageWidgets::WID_EM_MESSAGE),                               ///< Error message.
	};

	/* automatically generated from ../../widgets/fios_widget.h */
	/** Widgets of the SaveLoadWindow class. */
	enum SaveLoadWidgets {
		WID_SL_CAPTION                               = to_underlying(::SaveLoadWidgets::WID_SL_CAPTION),                               ///< Caption of the window.
		WID_SL_SORT_BYNAME                           = to_underlying(::SaveLoadWidgets::WID_SL_SORT_BYNAME),                           ///< Sort by name button.
		WID_SL_SORT_BYDATE                           = to_underlying(::SaveLoadWidgets::WID_SL_SORT_BYDATE),                           ///< Sort by date button.
		WID_SL_FILTER                                = to_underlying(::SaveLoadWidgets::WID_SL_FILTER),                                ///< Filter list of files
		WID_SL_BACKGROUND                            = to_underlying(::SaveLoadWidgets::WID_SL_BACKGROUND),                            ///< Background of window.
		WID_SL_FILE_BACKGROUND                       = to_underlying(::SaveLoadWidgets::WID_SL_FILE_BACKGROUND),                       ///< Background of file selection.
		WID_SL_HOME_BUTTON                           = to_underlying(::SaveLoadWidgets::WID_SL_HOME_BUTTON),                           ///< Home button.
		WID_SL_DRIVES_DIRECTORIES_LIST               = to_underlying(::SaveLoadWidgets::WID_SL_DRIVES_DIRECTORIES_LIST),               ///< Drives list.
		WID_SL_SCROLLBAR                             = to_underlying(::SaveLoadWidgets::WID_SL_SCROLLBAR),                             ///< Scrollbar of the file list.
		WID_SL_CONTENT_DOWNLOAD                      = to_underlying(::SaveLoadWidgets::WID_SL_CONTENT_DOWNLOAD),                      ///< Content download button, only available for play scenario/heightmap.
		WID_SL_SAVE_OSK_TITLE                        = to_underlying(::SaveLoadWidgets::WID_SL_SAVE_OSK_TITLE),                        ///< Title textbox, only available for save operations.
		WID_SL_DELETE_SELECTION                      = to_underlying(::SaveLoadWidgets::WID_SL_DELETE_SELECTION),                      ///< Delete button, only available for save operations.
		WID_SL_SAVE_GAME                             = to_underlying(::SaveLoadWidgets::WID_SL_SAVE_GAME),                             ///< Save button, only available for save operations.
		WID_SL_NEW_DIRECTORY                         = to_underlying(::SaveLoadWidgets::WID_SL_NEW_DIRECTORY),                         ///< Button to create a new directory in the current path.
		WID_SL_CONTENT_DOWNLOAD_SEL                  = to_underlying(::SaveLoadWidgets::WID_SL_CONTENT_DOWNLOAD_SEL),                  ///< Selection 'stack' to 'hide' the content download.
		WID_SL_DETAILS                               = to_underlying(::SaveLoadWidgets::WID_SL_DETAILS),                               ///< Panel with game details.
		WID_SL_NEWGRF_INFO                           = to_underlying(::SaveLoadWidgets::WID_SL_NEWGRF_INFO),                           ///< Button to open NewGgrf configuration.
		WID_SL_LOAD_BUTTON                           = to_underlying(::SaveLoadWidgets::WID_SL_LOAD_BUTTON),                           ///< Button to load game/scenario.
		WID_SL_MISSING_NEWGRFS                       = to_underlying(::SaveLoadWidgets::WID_SL_MISSING_NEWGRFS),                       ///< Button to find missing NewGRFs online.
	};

	/* automatically generated from ../../widgets/framerate_widget.h */
	/** Widgets of the FramerateWindow class. */
	enum FramerateWindowWidgets {
		WID_FRW_CAPTION                              = to_underlying(::FramerateWindowWidgets::WID_FRW_CAPTION),
		WID_FRW_RATE_GAMELOOP                        = to_underlying(::FramerateWindowWidgets::WID_FRW_RATE_GAMELOOP),
		WID_FRW_RATE_DRAWING                         = to_underlying(::FramerateWindowWidgets::WID_FRW_RATE_DRAWING),
		WID_FRW_RATE_FACTOR                          = to_underlying(::FramerateWindowWidgets::WID_FRW_RATE_FACTOR),
		WID_FRW_INFO_DATA_POINTS                     = to_underlying(::FramerateWindowWidgets::WID_FRW_INFO_DATA_POINTS),
		WID_FRW_TIMES_NAMES                          = to_underlying(::FramerateWindowWidgets::WID_FRW_TIMES_NAMES),
		WID_FRW_TIMES_CURRENT                        = to_underlying(::FramerateWindowWidgets::WID_FRW_TIMES_CURRENT),
		WID_FRW_TIMES_AVERAGE                        = to_underlying(::FramerateWindowWidgets::WID_FRW_TIMES_AVERAGE),
		WID_FRW_ALLOCSIZE                            = to_underlying(::FramerateWindowWidgets::WID_FRW_ALLOCSIZE),
		WID_FRW_SCROLLBAR                            = to_underlying(::FramerateWindowWidgets::WID_FRW_SCROLLBAR),
	};

	/** Widgets of the FrametimeGraphWindow class. */
	enum FrametimeGraphWindowWidgets {
		WID_FGW_CAPTION                              = to_underlying(::FrametimeGraphWindowWidgets::WID_FGW_CAPTION),
		WID_FGW_GRAPH                                = to_underlying(::FrametimeGraphWindowWidgets::WID_FGW_GRAPH),
	};

	/* automatically generated from ../../widgets/game_widget.h */
	/** Widgets of the GSConfigWindow class. */
	enum GSConfigWidgets {
		WID_GSC_BACKGROUND                           = to_underlying(::GSConfigWidgets::WID_GSC_BACKGROUND),                           ///< Window background.
		WID_GSC_GSLIST                               = to_underlying(::GSConfigWidgets::WID_GSC_GSLIST),                               ///< List with current selected Game Script.
		WID_GSC_SETTINGS                             = to_underlying(::GSConfigWidgets::WID_GSC_SETTINGS),                             ///< Panel to draw the Game Script settings on
		WID_GSC_SCROLLBAR                            = to_underlying(::GSConfigWidgets::WID_GSC_SCROLLBAR),                            ///< Scrollbar to scroll through the selected AIs.
		WID_GSC_CHANGE                               = to_underlying(::GSConfigWidgets::WID_GSC_CHANGE),                               ///< Select another Game Script button.
		WID_GSC_OPEN_URL                             = to_underlying(::GSConfigWidgets::WID_GSC_OPEN_URL),                             ///< Open GS URL.
		WID_GSC_TEXTFILE                             = to_underlying(::GSConfigWidgets::WID_GSC_TEXTFILE),                             ///< Open GS readme, changelog (+1) or license (+2).
		WID_GSC_CONTENT_DOWNLOAD                     = to_underlying(::GSConfigWidgets::WID_GSC_CONTENT_DOWNLOAD),                     ///< Download content button.
		WID_GSC_RESET                                = to_underlying(::GSConfigWidgets::WID_GSC_RESET),                                ///< Reset button.

		WID_GSC_SETTING_DROPDOWN                     = to_underlying(::GSConfigWidgets::WID_GSC_SETTING_DROPDOWN),                     ///< Dynamically created dropdown for changing setting value.
	};

	/* automatically generated from ../../widgets/genworld_widget.h */
	/** Widgets of the GenerateLandscapeWindow class. */
	enum GenerateLandscapeWidgets {
		WID_GL_TEMPERATE                             = to_underlying(::GenerateLandscapeWidgets::WID_GL_TEMPERATE),                             ///< Button with icon "Temperate".
		WID_GL_ARCTIC                                = to_underlying(::GenerateLandscapeWidgets::WID_GL_ARCTIC),                                ///< Button with icon "Arctic".
		WID_GL_TROPICAL                              = to_underlying(::GenerateLandscapeWidgets::WID_GL_TROPICAL),                              ///< Button with icon "Tropical".
		WID_GL_TOYLAND                               = to_underlying(::GenerateLandscapeWidgets::WID_GL_TOYLAND),                               ///< Button with icon "Toyland".

		WID_GL_MAPSIZE_X_PULLDOWN                    = to_underlying(::GenerateLandscapeWidgets::WID_GL_MAPSIZE_X_PULLDOWN),                    ///< Dropdown 'map X size'.
		WID_GL_MAPSIZE_Y_PULLDOWN                    = to_underlying(::GenerateLandscapeWidgets::WID_GL_MAPSIZE_Y_PULLDOWN),                    ///< Dropdown 'map Y size'.

		WID_GL_TOWN_PULLDOWN                         = to_underlying(::GenerateLandscapeWidgets::WID_GL_TOWN_PULLDOWN),                         ///< Dropdown 'No. of towns'.
		WID_GL_TOWNNAME_DROPDOWN                     = to_underlying(::GenerateLandscapeWidgets::WID_GL_TOWNNAME_DROPDOWN),                     ///< Dropdown 'Townnames'.
		WID_GL_INDUSTRY_PULLDOWN                     = to_underlying(::GenerateLandscapeWidgets::WID_GL_INDUSTRY_PULLDOWN),                     ///< Dropdown 'No. of industries'.

		WID_GL_GENERATE_BUTTON                       = to_underlying(::GenerateLandscapeWidgets::WID_GL_GENERATE_BUTTON),                       ///< 'Generate' button.

		WID_GL_HEIGHTMAP_HEIGHT_DOWN                 = to_underlying(::GenerateLandscapeWidgets::WID_GL_HEIGHTMAP_HEIGHT_DOWN),                 ///< Decrease heightmap highest mountain
		WID_GL_HEIGHTMAP_HEIGHT_TEXT                 = to_underlying(::GenerateLandscapeWidgets::WID_GL_HEIGHTMAP_HEIGHT_TEXT),                 ///< Max. heightmap highest mountain
		WID_GL_HEIGHTMAP_HEIGHT_UP                   = to_underlying(::GenerateLandscapeWidgets::WID_GL_HEIGHTMAP_HEIGHT_UP),                   ///< Increase max. heightmap highest mountain

		WID_GL_START_DATE_DOWN                       = to_underlying(::GenerateLandscapeWidgets::WID_GL_START_DATE_DOWN),                       ///< Decrease start year.
		WID_GL_START_DATE_TEXT                       = to_underlying(::GenerateLandscapeWidgets::WID_GL_START_DATE_TEXT),                       ///< Start year.
		WID_GL_START_DATE_UP                         = to_underlying(::GenerateLandscapeWidgets::WID_GL_START_DATE_UP),                         ///< Increase start year.

		WID_GL_SNOW_COVERAGE_DOWN                    = to_underlying(::GenerateLandscapeWidgets::WID_GL_SNOW_COVERAGE_DOWN),                    ///< Decrease snow coverage.
		WID_GL_SNOW_COVERAGE_TEXT                    = to_underlying(::GenerateLandscapeWidgets::WID_GL_SNOW_COVERAGE_TEXT),                    ///< Snow coverage.
		WID_GL_SNOW_COVERAGE_UP                      = to_underlying(::GenerateLandscapeWidgets::WID_GL_SNOW_COVERAGE_UP),                      ///< Increase snow coverage.

		WID_GL_DESERT_COVERAGE_DOWN                  = to_underlying(::GenerateLandscapeWidgets::WID_GL_DESERT_COVERAGE_DOWN),                  ///< Decrease desert coverage.
		WID_GL_DESERT_COVERAGE_TEXT                  = to_underlying(::GenerateLandscapeWidgets::WID_GL_DESERT_COVERAGE_TEXT),                  ///< Desert coverage.
		WID_GL_DESERT_COVERAGE_UP                    = to_underlying(::GenerateLandscapeWidgets::WID_GL_DESERT_COVERAGE_UP),                    ///< Increase desert coverage.

		WID_GL_LANDSCAPE_PULLDOWN                    = to_underlying(::GenerateLandscapeWidgets::WID_GL_LANDSCAPE_PULLDOWN),                    ///< Dropdown 'Land generator'.

		WID_GL_HEIGHTMAP_NAME_TEXT                   = to_underlying(::GenerateLandscapeWidgets::WID_GL_HEIGHTMAP_NAME_TEXT),                   ///< Heightmap name.
		WID_GL_HEIGHTMAP_SIZE_TEXT                   = to_underlying(::GenerateLandscapeWidgets::WID_GL_HEIGHTMAP_SIZE_TEXT),                   ///< Size of heightmap.
		WID_GL_HEIGHTMAP_ROTATION_PULLDOWN           = to_underlying(::GenerateLandscapeWidgets::WID_GL_HEIGHTMAP_ROTATION_PULLDOWN),           ///< Dropdown 'Heightmap rotation'.

		WID_GL_MAX_HEIGHT_PULLDOWN                   = to_underlying(::GenerateLandscapeWidgets::WID_GL_MAX_HEIGHT_PULLDOWN),                   ///< Dropdown 'Maximum height'.
		WID_GL_WATER_PULLDOWN                        = to_underlying(::GenerateLandscapeWidgets::WID_GL_WATER_PULLDOWN),                        ///< Dropdown 'Sea level'.
		WID_GL_RIVER_PULLDOWN                        = to_underlying(::GenerateLandscapeWidgets::WID_GL_RIVER_PULLDOWN),                        ///< Dropdown 'Rivers'.
		WID_GL_SMOOTHNESS_PULLDOWN                   = to_underlying(::GenerateLandscapeWidgets::WID_GL_SMOOTHNESS_PULLDOWN),                   ///< Dropdown 'Smoothness'.
		WID_GL_VARIETY_PULLDOWN                      = to_underlying(::GenerateLandscapeWidgets::WID_GL_VARIETY_PULLDOWN),                      ///< Dropdown 'Variety distribution'.
		WID_GL_AVERAGE_HEIGHT_PULLDOWN               = to_underlying(::GenerateLandscapeWidgets::WID_GL_AVERAGE_HEIGHT_PULLDOWN),               ///< Dropdown 'Average height'.

		WID_GL_BORDERS_PULLDOWN                      = to_underlying(::GenerateLandscapeWidgets::WID_GL_BORDERS_PULLDOWN),                      ///< Dropdown 'Map edges'.
		WID_GL_WATER_NW                              = to_underlying(::GenerateLandscapeWidgets::WID_GL_WATER_NW),                              ///< NW 'Water'/'Freeform'.
		WID_GL_WATER_NE                              = to_underlying(::GenerateLandscapeWidgets::WID_GL_WATER_NE),                              ///< NE 'Water'/'Freeform'.
		WID_GL_WATER_SE                              = to_underlying(::GenerateLandscapeWidgets::WID_GL_WATER_SE),                              ///< SE 'Water'/'Freeform'.
		WID_GL_WATER_SW                              = to_underlying(::GenerateLandscapeWidgets::WID_GL_WATER_SW),                              ///< SW 'Water'/'Freeform'.

		WID_GL_AI_BUTTON                             = to_underlying(::GenerateLandscapeWidgets::WID_GL_AI_BUTTON),                             ///< 'AI Settings' button.
		WID_GL_GS_BUTTON                             = to_underlying(::GenerateLandscapeWidgets::WID_GL_GS_BUTTON),                             ///< 'Game Script Settings' button.
		WID_GL_NEWGRF_BUTTON                         = to_underlying(::GenerateLandscapeWidgets::WID_GL_NEWGRF_BUTTON),                         ///< 'NewGRF Settings' button.

		WID_GL_CLIMATE_SEL_LABEL                     = to_underlying(::GenerateLandscapeWidgets::WID_GL_CLIMATE_SEL_LABEL),                     ///< NWID_SELECTION for snow or desert coverage label
		WID_GL_CLIMATE_SEL_SELECTOR                  = to_underlying(::GenerateLandscapeWidgets::WID_GL_CLIMATE_SEL_SELECTOR),                  ///< NWID_SELECTION for snow or desert coverage selector
		WID_GL_PROFILE_PULLDOWN                      = to_underlying(::GenerateLandscapeWidgets::WID_GL_PROFILE_PULLDOWN),                      ///< Appended explicit profile selector, preserving all original widget IDs.
		WID_GL_ASTEROID_PULLDOWN                     = to_underlying(::GenerateLandscapeWidgets::WID_GL_ASTEROID_PULLDOWN),                     ///< Saved Sparse/Normal/Dense asteroid population.
	};

	/** Widgets of the CreateScenarioWindow class. */
	enum CreateScenarioWidgets {
		WID_CS_TEMPERATE                             = to_underlying(::CreateScenarioWidgets::WID_CS_TEMPERATE),                             ///< Select temperate landscape style.
		WID_CS_ARCTIC                                = to_underlying(::CreateScenarioWidgets::WID_CS_ARCTIC),                                ///< Select arctic landscape style.
		WID_CS_TROPICAL                              = to_underlying(::CreateScenarioWidgets::WID_CS_TROPICAL),                              ///< Select tropical landscape style.
		WID_CS_TOYLAND                               = to_underlying(::CreateScenarioWidgets::WID_CS_TOYLAND),                               ///< Select toy-land landscape style.
		WID_CS_EMPTY_WORLD                           = to_underlying(::CreateScenarioWidgets::WID_CS_EMPTY_WORLD),                           ///< Generate an empty flat world.
		WID_CS_RANDOM_WORLD                          = to_underlying(::CreateScenarioWidgets::WID_CS_RANDOM_WORLD),                          ///< Generate random land button
		WID_CS_MAPSIZE_X_PULLDOWN                    = to_underlying(::CreateScenarioWidgets::WID_CS_MAPSIZE_X_PULLDOWN),                    ///< Pull-down arrow for x map size.
		WID_CS_MAPSIZE_Y_PULLDOWN                    = to_underlying(::CreateScenarioWidgets::WID_CS_MAPSIZE_Y_PULLDOWN),                    ///< Pull-down arrow for y map size.
		WID_CS_START_DATE_DOWN                       = to_underlying(::CreateScenarioWidgets::WID_CS_START_DATE_DOWN),                       ///< Decrease start year (start earlier).
		WID_CS_START_DATE_TEXT                       = to_underlying(::CreateScenarioWidgets::WID_CS_START_DATE_TEXT),                       ///< Clickable start date value.
		WID_CS_START_DATE_UP                         = to_underlying(::CreateScenarioWidgets::WID_CS_START_DATE_UP),                         ///< Increase start year (start later).
		WID_CS_FLAT_LAND_HEIGHT_DOWN                 = to_underlying(::CreateScenarioWidgets::WID_CS_FLAT_LAND_HEIGHT_DOWN),                 ///< Decrease flat land height.
		WID_CS_FLAT_LAND_HEIGHT_TEXT                 = to_underlying(::CreateScenarioWidgets::WID_CS_FLAT_LAND_HEIGHT_TEXT),                 ///< Clickable flat land height value.
		WID_CS_FLAT_LAND_HEIGHT_UP                   = to_underlying(::CreateScenarioWidgets::WID_CS_FLAT_LAND_HEIGHT_UP),                   ///< Increase flat land height.
	};

	/** Widgets of the GenerateProgressWindow class. */
	enum GenerationProgressWidgets {
		WID_GP_PROGRESS_BAR                          = to_underlying(::GenerationProgressWidgets::WID_GP_PROGRESS_BAR),                          ///< Progress bar.
		WID_GP_PROGRESS_TEXT                         = to_underlying(::GenerationProgressWidgets::WID_GP_PROGRESS_TEXT),                         ///< Text with the progress bar.
		WID_GP_ABORT                                 = to_underlying(::GenerationProgressWidgets::WID_GP_ABORT),                                 ///< Abort button.
	};

	/* automatically generated from ../../widgets/goal_widget.h */
	/** Widgets of the GoalListWindow class. */
	enum GoalListWidgets {
		WID_GOAL_CAPTION                             = to_underlying(::GoalListWidgets::WID_GOAL_CAPTION),                             ///< Caption of the window.
		WID_GOAL_SELECT_BUTTONS                      = to_underlying(::GoalListWidgets::WID_GOAL_SELECT_BUTTONS),                      ///< Selection widget for the title bar button.
		WID_GOAL_GLOBAL_BUTTON                       = to_underlying(::GoalListWidgets::WID_GOAL_GLOBAL_BUTTON),                       ///< Button to show global goals.
		WID_GOAL_COMPANY_BUTTON                      = to_underlying(::GoalListWidgets::WID_GOAL_COMPANY_BUTTON),                      ///< Button to show company goals.
		WID_GOAL_LIST                                = to_underlying(::GoalListWidgets::WID_GOAL_LIST),                                ///< Goal list.
		WID_GOAL_SCROLLBAR                           = to_underlying(::GoalListWidgets::WID_GOAL_SCROLLBAR),                           ///< Scrollbar of the goal list.
	};

	/** Widgets of the GoalQuestionWindow class. */
	enum GoalQuestionWidgets {
		WID_GQ_CAPTION                               = to_underlying(::GoalQuestionWidgets::WID_GQ_CAPTION),                               ///< Caption of the window.
		WID_GQ_QUESTION                              = to_underlying(::GoalQuestionWidgets::WID_GQ_QUESTION),                              ///< Question text.
		WID_GQ_BUTTONS                               = to_underlying(::GoalQuestionWidgets::WID_GQ_BUTTONS),                               ///< Buttons selection (between 1, 2 or 3).
		WID_GQ_BUTTON_1                              = to_underlying(::GoalQuestionWidgets::WID_GQ_BUTTON_1),                              ///< First button.
		WID_GQ_BUTTON_2                              = to_underlying(::GoalQuestionWidgets::WID_GQ_BUTTON_2),                              ///< Second button.
		WID_GQ_BUTTON_3                              = to_underlying(::GoalQuestionWidgets::WID_GQ_BUTTON_3),                              ///< Third button.
	};

	/* automatically generated from ../../widgets/graph_widget.h */
	/** Widgets of the GraphLegendWindow class. */
	enum GraphLegendWidgets {
		WID_GL_BACKGROUND                            = to_underlying(::GraphLegendWidgets::WID_GL_BACKGROUND),                            ///< Background of the window.

		WID_GL_FIRST_COMPANY                         = to_underlying(::GraphLegendWidgets::WID_GL_FIRST_COMPANY),                         ///< First company in the legend.
		WID_GL_LAST_COMPANY                          = to_underlying(::GraphLegendWidgets::WID_GL_LAST_COMPANY),                          ///< Last company in the legend.
	};

	/** Widgets of the BaseGraphWindow class and derived classes. */
	enum GraphWidgets {
		WID_GRAPH_KEY_BUTTON                         = to_underlying(::GraphWidgets::WID_GRAPH_KEY_BUTTON),                         ///< Key button.
		WID_GRAPH_BACKGROUND                         = to_underlying(::GraphWidgets::WID_GRAPH_BACKGROUND),                         ///< Background of the window.
		WID_GRAPH_CAPTION                            = to_underlying(::GraphWidgets::WID_GRAPH_CAPTION),                            ///< Caption.
		WID_GRAPH_GRAPH                              = to_underlying(::GraphWidgets::WID_GRAPH_GRAPH),                              ///< Graph itself.
		WID_GRAPH_RESIZE                             = to_underlying(::GraphWidgets::WID_GRAPH_RESIZE),                             ///< Resize button.
		WID_GRAPH_HEADER                             = to_underlying(::GraphWidgets::WID_GRAPH_HEADER),                             ///< Header.
		WID_GRAPH_FOOTER                             = to_underlying(::GraphWidgets::WID_GRAPH_FOOTER),                             ///< Footer.

		WID_GRAPH_ENABLE_CARGOES                     = to_underlying(::GraphWidgets::WID_GRAPH_ENABLE_CARGOES),                     ///< Enable cargoes button.
		WID_GRAPH_DISABLE_CARGOES                    = to_underlying(::GraphWidgets::WID_GRAPH_DISABLE_CARGOES),                    ///< Disable cargoes button.
		WID_GRAPH_MATRIX                             = to_underlying(::GraphWidgets::WID_GRAPH_MATRIX),                             ///< Cargo list.
		WID_GRAPH_MATRIX_SCROLLBAR                   = to_underlying(::GraphWidgets::WID_GRAPH_MATRIX_SCROLLBAR),                   ///< Cargo list scrollbar.

		WID_GRAPH_RANGE_MATRIX                       = to_underlying(::GraphWidgets::WID_GRAPH_RANGE_MATRIX),                       ///< Range list.
		WID_GRAPH_SCALE_MATRIX                       = to_underlying(::GraphWidgets::WID_GRAPH_SCALE_MATRIX),                       ///< Horizontal axis scale list.

		WID_PHG_DETAILED_PERFORMANCE                 = to_underlying(::GraphWidgets::WID_PHG_DETAILED_PERFORMANCE),                 ///< Detailed performance.
	};

	/** Widget of the PerformanceRatingDetailWindow class. */
	enum PerformanceRatingDetailsWidgets {
		WID_PRD_SCORE_FIRST                          = to_underlying(::PerformanceRatingDetailsWidgets::WID_PRD_SCORE_FIRST),                          ///< First entry in the score list.
		WID_PRD_SCORE_LAST                           = to_underlying(::PerformanceRatingDetailsWidgets::WID_PRD_SCORE_LAST),                           ///< Last entry in the score list.

		WID_PRD_COMPANY_FIRST                        = to_underlying(::PerformanceRatingDetailsWidgets::WID_PRD_COMPANY_FIRST),                        ///< First company.
		WID_PRD_COMPANY_LAST                         = to_underlying(::PerformanceRatingDetailsWidgets::WID_PRD_COMPANY_LAST),                         ///< Last company.
	};

	/* automatically generated from ../../widgets/group_widget.h */
	/** Widgets of the VehicleGroupWindow class. */
	enum GroupListWidgets {
		WID_GL_CAPTION                               = to_underlying(::GroupListWidgets::WID_GL_CAPTION),                               ///< Caption of the window.
		WID_GL_GROUP_BY_ORDER                        = to_underlying(::GroupListWidgets::WID_GL_GROUP_BY_ORDER),                        ///< Group order.
		WID_GL_GROUP_BY_DROPDOWN                     = to_underlying(::GroupListWidgets::WID_GL_GROUP_BY_DROPDOWN),                     ///< Group by dropdown list.
		WID_GL_SORT_BY_ORDER                         = to_underlying(::GroupListWidgets::WID_GL_SORT_BY_ORDER),                         ///< Sort order.
		WID_GL_SORT_BY_DROPDOWN                      = to_underlying(::GroupListWidgets::WID_GL_SORT_BY_DROPDOWN),                      ///< Sort by dropdown list.
		WID_GL_FILTER_BY_CARGO                       = to_underlying(::GroupListWidgets::WID_GL_FILTER_BY_CARGO),                       ///< Filter vehicles by cargo type.
		WID_GL_LIST_VEHICLE                          = to_underlying(::GroupListWidgets::WID_GL_LIST_VEHICLE),                          ///< List of the vehicles.
		WID_GL_LIST_VEHICLE_SCROLLBAR                = to_underlying(::GroupListWidgets::WID_GL_LIST_VEHICLE_SCROLLBAR),                ///< Scrollbar for the list.
		WID_GL_AVAILABLE_VEHICLES                    = to_underlying(::GroupListWidgets::WID_GL_AVAILABLE_VEHICLES),                    ///< Available vehicles.
		WID_GL_MANAGE_VEHICLES_DROPDOWN              = to_underlying(::GroupListWidgets::WID_GL_MANAGE_VEHICLES_DROPDOWN),              ///< Manage vehicles dropdown list.
		WID_GL_STOP_ALL                              = to_underlying(::GroupListWidgets::WID_GL_STOP_ALL),                              ///< Stop all button.
		WID_GL_START_ALL                             = to_underlying(::GroupListWidgets::WID_GL_START_ALL),                             ///< Start all button.

		WID_GL_ALL_VEHICLES                          = to_underlying(::GroupListWidgets::WID_GL_ALL_VEHICLES),                          ///< All vehicles entry.
		WID_GL_DEFAULT_VEHICLES                      = to_underlying(::GroupListWidgets::WID_GL_DEFAULT_VEHICLES),                      ///< Default vehicles entry.
		WID_GL_LIST_GROUP                            = to_underlying(::GroupListWidgets::WID_GL_LIST_GROUP),                            ///< List of the groups.
		WID_GL_LIST_GROUP_SCROLLBAR                  = to_underlying(::GroupListWidgets::WID_GL_LIST_GROUP_SCROLLBAR),                  ///< Scrollbar for the list.
		WID_GL_CREATE_GROUP                          = to_underlying(::GroupListWidgets::WID_GL_CREATE_GROUP),                          ///< Create group button.
		WID_GL_DELETE_GROUP                          = to_underlying(::GroupListWidgets::WID_GL_DELETE_GROUP),                          ///< Delete group button.
		WID_GL_RENAME_GROUP                          = to_underlying(::GroupListWidgets::WID_GL_RENAME_GROUP),                          ///< Rename group button.
		WID_GL_LIVERY_GROUP                          = to_underlying(::GroupListWidgets::WID_GL_LIVERY_GROUP),                          ///< Group livery button.
		WID_GL_REPLACE_PROTECTION                    = to_underlying(::GroupListWidgets::WID_GL_REPLACE_PROTECTION),                    ///< Replace protection button.
		WID_GL_INFO                                  = to_underlying(::GroupListWidgets::WID_GL_INFO),                                  ///< Group info.
	};

	/* automatically generated from ../../widgets/help_widget.h */
	/** Widgets of the HelpWindow class. */
	enum HelpWindowWidgets {
		WID_HW_README                                = to_underlying(::HelpWindowWidgets::WID_HW_README),
		WID_HW_CHANGELOG                             = to_underlying(::HelpWindowWidgets::WID_HW_CHANGELOG),
		WID_HW_KNOWN_BUGS                            = to_underlying(::HelpWindowWidgets::WID_HW_KNOWN_BUGS),
		WID_HW_LICENSE                               = to_underlying(::HelpWindowWidgets::WID_HW_LICENSE),
		WID_HW_FONTS                                 = to_underlying(::HelpWindowWidgets::WID_HW_FONTS),
		WID_HW_WEBSITE                               = to_underlying(::HelpWindowWidgets::WID_HW_WEBSITE),
		WID_HW_WIKI                                  = to_underlying(::HelpWindowWidgets::WID_HW_WIKI),
		WID_HW_BUGTRACKER                            = to_underlying(::HelpWindowWidgets::WID_HW_BUGTRACKER),
		WID_HW_COMMUNITY                             = to_underlying(::HelpWindowWidgets::WID_HW_COMMUNITY),
	};

	/* automatically generated from ../../widgets/highscore_widget.h */
	/** Widgets of the EndGameHighScoreBaseWindow class and HighScoreWindow class. */
	enum HighscoreWidgets {
		WID_H_BACKGROUND                             = to_underlying(::HighscoreWidgets::WID_H_BACKGROUND),                             ///< Background of the window.
	};

	/* automatically generated from ../../widgets/industry_widget.h */
	/** Widgets of the BuildIndustryWindow class. */
	enum DynamicPlaceIndustriesWidgets {
		WID_DPI_SCENARIO_EDITOR_PANE                 = to_underlying(::DynamicPlaceIndustriesWidgets::WID_DPI_SCENARIO_EDITOR_PANE),                 ///< Pane containing SE-only widgets.
		WID_DPI_REMOVE_ALL_INDUSTRIES_WIDGET         = to_underlying(::DynamicPlaceIndustriesWidgets::WID_DPI_REMOVE_ALL_INDUSTRIES_WIDGET),         ///< Remove all industries button.
		WID_DPI_CREATE_RANDOM_INDUSTRIES_WIDGET      = to_underlying(::DynamicPlaceIndustriesWidgets::WID_DPI_CREATE_RANDOM_INDUSTRIES_WIDGET),      ///< Create random industries button.
		WID_DPI_MATRIX_WIDGET                        = to_underlying(::DynamicPlaceIndustriesWidgets::WID_DPI_MATRIX_WIDGET),                        ///< Matrix of the industries.
		WID_DPI_SCROLLBAR                            = to_underlying(::DynamicPlaceIndustriesWidgets::WID_DPI_SCROLLBAR),                            ///< Scrollbar of the matrix.
		WID_DPI_INFOPANEL                            = to_underlying(::DynamicPlaceIndustriesWidgets::WID_DPI_INFOPANEL),                            ///< Info panel about the industry.
		WID_DPI_DISPLAY_WIDGET                       = to_underlying(::DynamicPlaceIndustriesWidgets::WID_DPI_DISPLAY_WIDGET),                       ///< Display chain button.
		WID_DPI_FUND_WIDGET                          = to_underlying(::DynamicPlaceIndustriesWidgets::WID_DPI_FUND_WIDGET),                          ///< Fund button.
	};

	/** Widgets of the IndustryViewWindow class. */
	enum IndustryViewWidgets {
		WID_IV_CAPTION                               = to_underlying(::IndustryViewWidgets::WID_IV_CAPTION),                               ///< Caption of the window.
		WID_IV_VIEWPORT                              = to_underlying(::IndustryViewWidgets::WID_IV_VIEWPORT),                              ///< Viewport of the industry.
		WID_IV_INFO                                  = to_underlying(::IndustryViewWidgets::WID_IV_INFO),                                  ///< Info of the industry.
		WID_IV_GOTO                                  = to_underlying(::IndustryViewWidgets::WID_IV_GOTO),                                  ///< Goto button.
		WID_IV_DISPLAY                               = to_underlying(::IndustryViewWidgets::WID_IV_DISPLAY),                               ///< Display chain button.
		WID_IV_GRAPH                                 = to_underlying(::IndustryViewWidgets::WID_IV_GRAPH),                                 ///< Production history button.
		WID_IV_RECIPE                                = to_underlying(::IndustryViewWidgets::WID_IV_RECIPE),                                ///< Cycle the current native facility's supported, unlocked recipe alternatives.
	};

	/** Widgets of the IndustryDirectoryWindow class. */
	enum IndustryDirectoryWidgets {
		WID_ID_CAPTION                               = to_underlying(::IndustryDirectoryWidgets::WID_ID_CAPTION),                               ///< Caption of the window.
		WID_ID_DROPDOWN_ORDER                        = to_underlying(::IndustryDirectoryWidgets::WID_ID_DROPDOWN_ORDER),                        ///< Dropdown for the order of the sort.
		WID_ID_DROPDOWN_CRITERIA                     = to_underlying(::IndustryDirectoryWidgets::WID_ID_DROPDOWN_CRITERIA),                     ///< Dropdown for the criteria of the sort.
		WID_ID_FILTER_BY_ACC_CARGO                   = to_underlying(::IndustryDirectoryWidgets::WID_ID_FILTER_BY_ACC_CARGO),                   ///< Accepted cargo filter dropdown list.
		WID_ID_FILTER_BY_PROD_CARGO                  = to_underlying(::IndustryDirectoryWidgets::WID_ID_FILTER_BY_PROD_CARGO),                  ///< Produced cargo filter dropdown list.
		WID_ID_FILTER                                = to_underlying(::IndustryDirectoryWidgets::WID_ID_FILTER),                                ///< Textbox to filter industry name.
		WID_ID_INDUSTRY_LIST                         = to_underlying(::IndustryDirectoryWidgets::WID_ID_INDUSTRY_LIST),                         ///< Industry list.
		WID_ID_HSCROLLBAR                            = to_underlying(::IndustryDirectoryWidgets::WID_ID_HSCROLLBAR),                            ///< Horizontal scrollbar of the list.
		WID_ID_VSCROLLBAR                            = to_underlying(::IndustryDirectoryWidgets::WID_ID_VSCROLLBAR),                            ///< Vertical scrollbar of the list.
	};

	/** Widgets of the IndustryCargoesWindow class */
	enum IndustryCargoesWidgets {
		WID_IC_CAPTION                               = to_underlying(::IndustryCargoesWidgets::WID_IC_CAPTION),                               ///< Caption of the window.
		WID_IC_NOTIFY                                = to_underlying(::IndustryCargoesWidgets::WID_IC_NOTIFY),                                ///< Row of buttons at the bottom.
		WID_IC_PANEL                                 = to_underlying(::IndustryCargoesWidgets::WID_IC_PANEL),                                 ///< Panel that shows the chain.
		WID_IC_SCROLLBAR                             = to_underlying(::IndustryCargoesWidgets::WID_IC_SCROLLBAR),                             ///< Scrollbar of the panel.
		WID_IC_CARGO_DROPDOWN                        = to_underlying(::IndustryCargoesWidgets::WID_IC_CARGO_DROPDOWN),                        ///< Select cargo dropdown.
		WID_IC_IND_DROPDOWN                          = to_underlying(::IndustryCargoesWidgets::WID_IC_IND_DROPDOWN),                          ///< Select industry dropdown.
	};

	/* automatically generated from ../../widgets/intro_widget.h */
	/** Widgets of the SelectGameWindow class. */
	enum SelectGameIntroWidgets {
		WID_SGI_GENERATE_GAME                        = to_underlying(::SelectGameIntroWidgets::WID_SGI_GENERATE_GAME),                        ///< Generate game button.
		WID_SGI_LOAD_GAME                            = to_underlying(::SelectGameIntroWidgets::WID_SGI_LOAD_GAME),                            ///< Load game button.
		WID_SGI_PLAY_SCENARIO                        = to_underlying(::SelectGameIntroWidgets::WID_SGI_PLAY_SCENARIO),                        ///< Play scenario button.
		WID_SGI_PLAY_HEIGHTMAP                       = to_underlying(::SelectGameIntroWidgets::WID_SGI_PLAY_HEIGHTMAP),                       ///< Play heightmap button.
		WID_SGI_EDIT_SCENARIO                        = to_underlying(::SelectGameIntroWidgets::WID_SGI_EDIT_SCENARIO),                        ///< Edit scenario button.
		WID_SGI_PLAY_NETWORK                         = to_underlying(::SelectGameIntroWidgets::WID_SGI_PLAY_NETWORK),                         ///< Play network button.
		WID_SGI_BASESET_SELECTION                    = to_underlying(::SelectGameIntroWidgets::WID_SGI_BASESET_SELECTION),                    ///< Baseset selection.
		WID_SGI_BASESET                              = to_underlying(::SelectGameIntroWidgets::WID_SGI_BASESET),                              ///< Baseset errors.
		WID_SGI_TRANSLATION_SELECTION                = to_underlying(::SelectGameIntroWidgets::WID_SGI_TRANSLATION_SELECTION),                ///< Translation selection.
		WID_SGI_TRANSLATION                          = to_underlying(::SelectGameIntroWidgets::WID_SGI_TRANSLATION),                          ///< Translation errors.
		WID_SGI_OPTIONS                              = to_underlying(::SelectGameIntroWidgets::WID_SGI_OPTIONS),                              ///< Options button.
		WID_SGI_HIGHSCORE                            = to_underlying(::SelectGameIntroWidgets::WID_SGI_HIGHSCORE),                            ///< Highscore button.
		WID_SGI_HELP                                 = to_underlying(::SelectGameIntroWidgets::WID_SGI_HELP),                                 ///< Help and manuals button.
		WID_SGI_CONTENT_DOWNLOAD                     = to_underlying(::SelectGameIntroWidgets::WID_SGI_CONTENT_DOWNLOAD),                     ///< Content Download button.
		WID_SGI_EXIT                                 = to_underlying(::SelectGameIntroWidgets::WID_SGI_EXIT),                                 ///< Exit button.
	};

	/* automatically generated from ../../widgets/layer_portal_widget.h */
	/** Native pair window widgets; status always queries canonical saved authority. */
	enum PortalWidgets {
		PW_STATUS                                    = to_underlying(::PortalWidgets::PW_STATUS),                                    ///< Pair state line.
		PW_CONNECT                                   = to_underlying(::PortalWidgets::PW_CONNECT),                                   ///< Connect the pair for ordinary trains.
		PW_SEAL                                      = to_underlying(::PortalWidgets::PW_SEAL),                                      ///< Seal it again.
		PW_COMMISSION                                = to_underlying(::PortalWidgets::PW_COMMISSION),                                ///< Buy and commission a boring machine.
		PW_CORRIDOR_DOWN                             = to_underlying(::PortalWidgets::PW_CORRIDOR_DOWN),                             ///< Shorter corridor to commission.
		PW_CORRIDOR                                  = to_underlying(::PortalWidgets::PW_CORRIDOR),                                  ///< Corridor length to commission.
		PW_CORRIDOR_UP                               = to_underlying(::PortalWidgets::PW_CORRIDOR_UP),                               ///< Longer corridor to commission.
		PW_INFO                                      = to_underlying(::PortalWidgets::PW_INFO),                                      ///< Project lines: progress, hold, spoil dug, hauler and the next step.
		PW_SHOW_MACHINE                              = to_underlying(::PortalWidgets::PW_SHOW_MACHINE),                              ///< Open the machine's vehicle window.
		PW_START_STOP                                = to_underlying(::PortalWidgets::PW_START_STOP),                                ///< Start or stop the machine.
		PW_PAUSE                                     = to_underlying(::PortalWidgets::PW_PAUSE),                                     ///< Pause or resume the project.
		PW_CANCEL                                    = to_underlying(::PortalWidgets::PW_CANCEL),                                    ///< Cancel the project, after confirmation.
		PW_RETIRE                                    = to_underlying(::PortalWidgets::PW_RETIRE),                                    ///< Retire the finished project.
		PW_ASSIGN                                    = to_underlying(::PortalWidgets::PW_ASSIGN),                                    ///< Pick a train to assign as hauler.
		PW_RELEASE                                   = to_underlying(::PortalWidgets::PW_RELEASE),                                   ///< Release the hauler.
		PW_SELL                                      = to_underlying(::PortalWidgets::PW_SELL),                                      ///< Sell the withdrawn train in the portal.
		PW_CONVERT                                   = to_underlying(::PortalWidgets::PW_CONVERT),                                   ///< Convert the released temporary track.
		PW_PREVIEW                                   = to_underlying(::PortalWidgets::PW_PREVIEW),                                   ///< Actual saved-state views of both independent pair endpoints.
	};

	/* automatically generated from ../../widgets/league_widget.h */
	/** Widget of the PerformanceLeagueWindow class. */
	enum PerformanceLeagueWidgets {
		WID_PLT_BACKGROUND                           = to_underlying(::PerformanceLeagueWidgets::WID_PLT_BACKGROUND),                           ///< Background of the window.
	};

	/** Widget of the ScriptLeagueWindow class. */
	enum ScriptLeagueWidgets {
		WID_SLT_CAPTION                              = to_underlying(::ScriptLeagueWidgets::WID_SLT_CAPTION),                              ///< Caption of the window.
		WID_SLT_BACKGROUND                           = to_underlying(::ScriptLeagueWidgets::WID_SLT_BACKGROUND),                           ///< Background of the window.
	};

	/* automatically generated from ../../widgets/link_graph_legend_widget.h */
	/** Widgets of the WindowClass::LinkGraphLegend. */
	enum LinkGraphLegendWidgets {
		WID_LGL_CAPTION                              = to_underlying(::LinkGraphLegendWidgets::WID_LGL_CAPTION),                              ///< Caption widget.
		WID_LGL_SATURATION                           = to_underlying(::LinkGraphLegendWidgets::WID_LGL_SATURATION),                           ///< Saturation legend.
		WID_LGL_SATURATION_FIRST                     = to_underlying(::LinkGraphLegendWidgets::WID_LGL_SATURATION_FIRST),
		WID_LGL_SATURATION_LAST                      = to_underlying(::LinkGraphLegendWidgets::WID_LGL_SATURATION_LAST),
		WID_LGL_COMPANIES                            = to_underlying(::LinkGraphLegendWidgets::WID_LGL_COMPANIES),                            ///< Company selection widget.
		WID_LGL_COMPANY_FIRST                        = to_underlying(::LinkGraphLegendWidgets::WID_LGL_COMPANY_FIRST),
		WID_LGL_COMPANY_LAST                         = to_underlying(::LinkGraphLegendWidgets::WID_LGL_COMPANY_LAST),
		WID_LGL_COMPANIES_ALL                        = to_underlying(::LinkGraphLegendWidgets::WID_LGL_COMPANIES_ALL),
		WID_LGL_COMPANIES_NONE                       = to_underlying(::LinkGraphLegendWidgets::WID_LGL_COMPANIES_NONE),
		WID_LGL_CARGOES                              = to_underlying(::LinkGraphLegendWidgets::WID_LGL_CARGOES),                              ///< Cargo selection widget.
		WID_LGL_CARGO_FIRST                          = to_underlying(::LinkGraphLegendWidgets::WID_LGL_CARGO_FIRST),
		WID_LGL_CARGO_LAST                           = to_underlying(::LinkGraphLegendWidgets::WID_LGL_CARGO_LAST),
		WID_LGL_CARGOES_ALL                          = to_underlying(::LinkGraphLegendWidgets::WID_LGL_CARGOES_ALL),
		WID_LGL_CARGOES_NONE                         = to_underlying(::LinkGraphLegendWidgets::WID_LGL_CARGOES_NONE),
	};

	/* automatically generated from ../../widgets/main_widget.h */
	/** Widgets of the MainWindow class. */
	enum MainWidgets {
		WID_M_VIEWPORT                               = to_underlying(::MainWidgets::WID_M_VIEWPORT),                               ///< Main window viewport.
	};

	/* automatically generated from ../../widgets/misc_widget.h */
	/** Widgets of the LandInfoWindow class. */
	enum LandInfoWidgets {
		WID_LI_LOCATION                              = to_underlying(::LandInfoWidgets::WID_LI_LOCATION),                              ///< Scroll to location.
		WID_LI_BACKGROUND                            = to_underlying(::LandInfoWidgets::WID_LI_BACKGROUND),                            ///< Background of the window.
	};

	/** Widgets of the TooltipsWindow class. */
	enum ToolTipsWidgets {
		WID_TT_BACKGROUND                            = to_underlying(::ToolTipsWidgets::WID_TT_BACKGROUND),                            ///< Background of the window.
	};

	/** Widgets of the AboutWindow class. */
	enum AboutWidgets {
		WID_A_SCROLLING_TEXT                         = to_underlying(::AboutWidgets::WID_A_SCROLLING_TEXT),                         ///< The actually scrolling text.
		WID_A_WEBSITE                                = to_underlying(::AboutWidgets::WID_A_WEBSITE),                                ///< URL of OpenTTD website.
		WID_A_COPYRIGHT                              = to_underlying(::AboutWidgets::WID_A_COPYRIGHT),                              ///< Copyright string
	};

	/** Widgets of the QueryStringWindow class. */
	enum QueryStringWidgets {
		WID_QS_CAPTION                               = to_underlying(::QueryStringWidgets::WID_QS_CAPTION),                               ///< Caption of the window.
		WID_QS_TEXT                                  = to_underlying(::QueryStringWidgets::WID_QS_TEXT),                                  ///< Text of the query.
		WID_QS_DEFAULT                               = to_underlying(::QueryStringWidgets::WID_QS_DEFAULT),                               ///< Default button.
		WID_QS_DEFAULT_SEL                           = to_underlying(::QueryStringWidgets::WID_QS_DEFAULT_SEL),                           ///< Container for default button, which can be hidden.
		WID_QS_CANCEL                                = to_underlying(::QueryStringWidgets::WID_QS_CANCEL),                                ///< Cancel button.
		WID_QS_OK                                    = to_underlying(::QueryStringWidgets::WID_QS_OK),                                    ///< OK button.
		WID_QS_MOVE                                  = to_underlying(::QueryStringWidgets::WID_QS_MOVE),                                  ///< Move button.
		WID_QS_MOVE_SEL                              = to_underlying(::QueryStringWidgets::WID_QS_MOVE_SEL),                              ///< Container for move button, which can be hidden.
	};

	/** Widgets of the QueryWindow class. */
	enum QueryWidgets {
		WID_Q_CAPTION                                = to_underlying(::QueryWidgets::WID_Q_CAPTION),                                ///< Caption of the window.
		WID_Q_TEXT                                   = to_underlying(::QueryWidgets::WID_Q_TEXT),                                   ///< Text of the query.
		WID_Q_NO                                     = to_underlying(::QueryWidgets::WID_Q_NO),                                     ///< Yes button.
		WID_Q_YES                                    = to_underlying(::QueryWidgets::WID_Q_YES),                                    ///< No button.
	};

	/** Widgets of the TextfileWindow class. */
	enum TextfileWidgets {
		WID_TF_CAPTION                               = to_underlying(::TextfileWidgets::WID_TF_CAPTION),                               ///< The caption of the window.
		WID_TF_NAVBACK                               = to_underlying(::TextfileWidgets::WID_TF_NAVBACK),                               ///< Navigate back button.
		WID_TF_NAVFORWARD                            = to_underlying(::TextfileWidgets::WID_TF_NAVFORWARD),                            ///< Navigate forward button.
		WID_TF_WRAPTEXT                              = to_underlying(::TextfileWidgets::WID_TF_WRAPTEXT),                              ///< Whether or not to wrap the text.
		WID_TF_JUMPLIST                              = to_underlying(::TextfileWidgets::WID_TF_JUMPLIST),                              ///< List to jump around the file.
		WID_TF_SEL_JUMPLIST                          = to_underlying(::TextfileWidgets::WID_TF_SEL_JUMPLIST),                          ///< Selection to display the jump list or not.
		WID_TF_BACKGROUND                            = to_underlying(::TextfileWidgets::WID_TF_BACKGROUND),                            ///< Panel to draw the textfile on.
		WID_TF_VSCROLLBAR                            = to_underlying(::TextfileWidgets::WID_TF_VSCROLLBAR),                            ///< Vertical scrollbar to scroll through the textfile up-and-down.
		WID_TF_HSCROLLBAR                            = to_underlying(::TextfileWidgets::WID_TF_HSCROLLBAR),                            ///< Horizontal scrollbar to scroll through the textfile left-to-right.
	};

	/* automatically generated from ../../widgets/music_widget.h */
	/** Widgets of the MusicTrackSelectionWindow class. */
	enum MusicTrackSelectionWidgets {
		WID_MTS_CAPTION                              = to_underlying(::MusicTrackSelectionWidgets::WID_MTS_CAPTION),                              ///< Window caption.
		WID_MTS_LIST_LEFT                            = to_underlying(::MusicTrackSelectionWidgets::WID_MTS_LIST_LEFT),                            ///< Left button.
		WID_MTS_PLAYLIST                             = to_underlying(::MusicTrackSelectionWidgets::WID_MTS_PLAYLIST),                             ///< Playlist.
		WID_MTS_LIST_RIGHT                           = to_underlying(::MusicTrackSelectionWidgets::WID_MTS_LIST_RIGHT),                           ///< Right button.
		WID_MTS_MUSICSET                             = to_underlying(::MusicTrackSelectionWidgets::WID_MTS_MUSICSET),                             ///< Music set selection.
		WID_MTS_ALL                                  = to_underlying(::MusicTrackSelectionWidgets::WID_MTS_ALL),                                  ///< All button.
		WID_MTS_OLD                                  = to_underlying(::MusicTrackSelectionWidgets::WID_MTS_OLD),                                  ///< Old button.
		WID_MTS_NEW                                  = to_underlying(::MusicTrackSelectionWidgets::WID_MTS_NEW),                                  ///< New button.
		WID_MTS_EZY                                  = to_underlying(::MusicTrackSelectionWidgets::WID_MTS_EZY),                                  ///< Ezy button.
		WID_MTS_CUSTOM1                              = to_underlying(::MusicTrackSelectionWidgets::WID_MTS_CUSTOM1),                              ///< Custom1 button.
		WID_MTS_CUSTOM2                              = to_underlying(::MusicTrackSelectionWidgets::WID_MTS_CUSTOM2),                              ///< Custom2 button.
		WID_MTS_CLEAR                                = to_underlying(::MusicTrackSelectionWidgets::WID_MTS_CLEAR),                                ///< Clear button.
	};

	/** Widgets of the MusicWindow class. */
	enum MusicWidgets {
		WID_M_PREV                                   = to_underlying(::MusicWidgets::WID_M_PREV),                                   ///< Previous button.
		WID_M_NEXT                                   = to_underlying(::MusicWidgets::WID_M_NEXT),                                   ///< Next button.
		WID_M_STOP                                   = to_underlying(::MusicWidgets::WID_M_STOP),                                   ///< Stop button.
		WID_M_PLAY                                   = to_underlying(::MusicWidgets::WID_M_PLAY),                                   ///< Play button.
		WID_M_SLIDERS                                = to_underlying(::MusicWidgets::WID_M_SLIDERS),                                ///< Sliders.
		WID_M_MUSIC_VOL                              = to_underlying(::MusicWidgets::WID_M_MUSIC_VOL),                              ///< Music volume.
		WID_M_EFFECT_VOL                             = to_underlying(::MusicWidgets::WID_M_EFFECT_VOL),                             ///< Effect volume.
		WID_M_BACKGROUND                             = to_underlying(::MusicWidgets::WID_M_BACKGROUND),                             ///< Background of the window.
		WID_M_TRACK                                  = to_underlying(::MusicWidgets::WID_M_TRACK),                                  ///< Track playing.
		WID_M_TRACK_NR                               = to_underlying(::MusicWidgets::WID_M_TRACK_NR),                               ///< Track number.
		WID_M_TRACK_TITLE                            = to_underlying(::MusicWidgets::WID_M_TRACK_TITLE),                            ///< Track title.
		WID_M_TRACK_NAME                             = to_underlying(::MusicWidgets::WID_M_TRACK_NAME),                             ///< Track name.
		WID_M_SHUFFLE                                = to_underlying(::MusicWidgets::WID_M_SHUFFLE),                                ///< Shuffle button.
		WID_M_PROGRAMME                              = to_underlying(::MusicWidgets::WID_M_PROGRAMME),                              ///< Program button.
		WID_M_ALL                                    = to_underlying(::MusicWidgets::WID_M_ALL),                                    ///< All button.
		WID_M_OLD                                    = to_underlying(::MusicWidgets::WID_M_OLD),                                    ///< Old button.
		WID_M_NEW                                    = to_underlying(::MusicWidgets::WID_M_NEW),                                    ///< New button.
		WID_M_EZY                                    = to_underlying(::MusicWidgets::WID_M_EZY),                                    ///< Ezy button.
		WID_M_CUSTOM1                                = to_underlying(::MusicWidgets::WID_M_CUSTOM1),                                ///< Custom1 button.
		WID_M_CUSTOM2                                = to_underlying(::MusicWidgets::WID_M_CUSTOM2),                                ///< Custom2 button.
	};

	/* automatically generated from ../../widgets/network_chat_widget.h */
	/** Widgets of the NetworkChatWindow class. */
	enum NetWorkChatWidgets {
		WID_NC_CLOSE                                 = to_underlying(::NetWorkChatWidgets::WID_NC_CLOSE),                                 ///< Close button.
		WID_NC_BACKGROUND                            = to_underlying(::NetWorkChatWidgets::WID_NC_BACKGROUND),                            ///< Background of the window.
		WID_NC_DESTINATION                           = to_underlying(::NetWorkChatWidgets::WID_NC_DESTINATION),                           ///< Destination.
		WID_NC_TEXTBOX                               = to_underlying(::NetWorkChatWidgets::WID_NC_TEXTBOX),                               ///< Textbox.
		WID_NC_SENDBUTTON                            = to_underlying(::NetWorkChatWidgets::WID_NC_SENDBUTTON),                            ///< Send button.
	};

	/* automatically generated from ../../widgets/network_content_widget.h */
	/** Widgets of the NetworkContentDownloadStatusWindow class. */
	enum NetworkContentDownloadStatusWidgets {
		WID_NCDS_PROGRESS_BAR                        = to_underlying(::NetworkContentDownloadStatusWidgets::WID_NCDS_PROGRESS_BAR),                        ///< Simple progress bar.
		WID_NCDS_PROGRESS_TEXT                       = to_underlying(::NetworkContentDownloadStatusWidgets::WID_NCDS_PROGRESS_TEXT),                       ///< Text explaining what is happening.
		WID_NCDS_CANCELOK                            = to_underlying(::NetworkContentDownloadStatusWidgets::WID_NCDS_CANCELOK),                            ///< (Optional) Cancel/OK button.
	};

	/** Widgets of the NetworkContentListWindow class. */
	enum NetworkContentListWidgets {
		WID_NCL_BACKGROUND                           = to_underlying(::NetworkContentListWidgets::WID_NCL_BACKGROUND),                           ///< Resize button.

		WID_NCL_FILTER_CAPT                          = to_underlying(::NetworkContentListWidgets::WID_NCL_FILTER_CAPT),                          ///< Caption for the filter editbox.
		WID_NCL_FILTER                               = to_underlying(::NetworkContentListWidgets::WID_NCL_FILTER),                               ///< Filter editbox.

		WID_NCL_CHECKBOX                             = to_underlying(::NetworkContentListWidgets::WID_NCL_CHECKBOX),                             ///< Button above checkboxes.
		WID_NCL_TYPE                                 = to_underlying(::NetworkContentListWidgets::WID_NCL_TYPE),                                 ///< 'Type' button.
		WID_NCL_NAME                                 = to_underlying(::NetworkContentListWidgets::WID_NCL_NAME),                                 ///< 'Name' button.

		WID_NCL_MATRIX                               = to_underlying(::NetworkContentListWidgets::WID_NCL_MATRIX),                               ///< Panel with list of content.
		WID_NCL_SCROLLBAR                            = to_underlying(::NetworkContentListWidgets::WID_NCL_SCROLLBAR),                            ///< Scrollbar of matrix.

		WID_NCL_DETAILS                              = to_underlying(::NetworkContentListWidgets::WID_NCL_DETAILS),                              ///< Panel with content details.
		WID_NCL_TEXTFILE                             = to_underlying(::NetworkContentListWidgets::WID_NCL_TEXTFILE),                             ///< Open readme, changelog (+1) or license (+2) of a file in the content window.

		WID_NCL_SELECT_ALL                           = to_underlying(::NetworkContentListWidgets::WID_NCL_SELECT_ALL),                           ///< 'Select all' button.
		WID_NCL_SELECT_UPDATE                        = to_underlying(::NetworkContentListWidgets::WID_NCL_SELECT_UPDATE),                        ///< 'Select updates' button.
		WID_NCL_UNSELECT                             = to_underlying(::NetworkContentListWidgets::WID_NCL_UNSELECT),                             ///< 'Unselect all' button.
		WID_NCL_OPEN_URL                             = to_underlying(::NetworkContentListWidgets::WID_NCL_OPEN_URL),                             ///< 'Open url' button.
		WID_NCL_DOWNLOAD                             = to_underlying(::NetworkContentListWidgets::WID_NCL_DOWNLOAD),                             ///< 'Download' button.

		WID_NCL_SEL_ALL_UPDATE                       = to_underlying(::NetworkContentListWidgets::WID_NCL_SEL_ALL_UPDATE),                       ///< NWID_SELECTION widget for select all/update buttons..
		WID_NCL_SEARCH_EXTERNAL                      = to_underlying(::NetworkContentListWidgets::WID_NCL_SEARCH_EXTERNAL),                      ///< Search external sites for missing NewGRF.
	};

	/* automatically generated from ../../widgets/network_widget.h */
	/** Widgets of the NetworkGameWindow class. */
	enum NetworkGameWidgets {
		WID_NG_MAIN                                  = to_underlying(::NetworkGameWidgets::WID_NG_MAIN),                                  ///< Main panel.

		WID_NG_CLIENT_LABEL                          = to_underlying(::NetworkGameWidgets::WID_NG_CLIENT_LABEL),                          ///< Label in front of client name edit box.
		WID_NG_CLIENT                                = to_underlying(::NetworkGameWidgets::WID_NG_CLIENT),                                ///< Panel with editbox to set client name.
		WID_NG_FILTER_LABEL                          = to_underlying(::NetworkGameWidgets::WID_NG_FILTER_LABEL),                          ///< Label in front of the filter/search edit box.
		WID_NG_FILTER                                = to_underlying(::NetworkGameWidgets::WID_NG_FILTER),                                ///< Panel with the edit box to enter the search text.

		WID_NG_NAME                                  = to_underlying(::NetworkGameWidgets::WID_NG_NAME),                                  ///< 'Name' button.
		WID_NG_CLIENTS                               = to_underlying(::NetworkGameWidgets::WID_NG_CLIENTS),                               ///< 'Clients' button.
		WID_NG_MAPSIZE                               = to_underlying(::NetworkGameWidgets::WID_NG_MAPSIZE),                               ///< 'Map size' button.
		WID_NG_DATE                                  = to_underlying(::NetworkGameWidgets::WID_NG_DATE),                                  ///< 'Date' button.
		WID_NG_YEARS                                 = to_underlying(::NetworkGameWidgets::WID_NG_YEARS),                                 ///< 'Years' button.
		WID_NG_INFO                                  = to_underlying(::NetworkGameWidgets::WID_NG_INFO),                                  ///< Third button in the game list panel.

		WID_NG_MATRIX                                = to_underlying(::NetworkGameWidgets::WID_NG_MATRIX),                                ///< Panel with list of games.
		WID_NG_SCROLLBAR                             = to_underlying(::NetworkGameWidgets::WID_NG_SCROLLBAR),                             ///< Scrollbar of matrix.

		WID_NG_LASTJOINED_LABEL                      = to_underlying(::NetworkGameWidgets::WID_NG_LASTJOINED_LABEL),                      ///< Label "Last joined server:".
		WID_NG_LASTJOINED                            = to_underlying(::NetworkGameWidgets::WID_NG_LASTJOINED),                            ///< Info about the last joined server.
		WID_NG_LASTJOINED_SPACER                     = to_underlying(::NetworkGameWidgets::WID_NG_LASTJOINED_SPACER),                     ///< Spacer after last joined server panel.

		WID_NG_DETAILS                               = to_underlying(::NetworkGameWidgets::WID_NG_DETAILS),                               ///< Panel with game details.
		WID_NG_JOIN                                  = to_underlying(::NetworkGameWidgets::WID_NG_JOIN),                                  ///< 'Join game' button.
		WID_NG_REFRESH                               = to_underlying(::NetworkGameWidgets::WID_NG_REFRESH),                               ///< 'Refresh server' button.
		WID_NG_NEWGRF                                = to_underlying(::NetworkGameWidgets::WID_NG_NEWGRF),                                ///< 'NewGRF Settings' button.
		WID_NG_NEWGRF_SEL                            = to_underlying(::NetworkGameWidgets::WID_NG_NEWGRF_SEL),                            ///< Selection 'widget' to hide the NewGRF settings.
		WID_NG_NEWGRF_MISSING                        = to_underlying(::NetworkGameWidgets::WID_NG_NEWGRF_MISSING),                        ///< 'Find missing NewGRF online' button.
		WID_NG_NEWGRF_MISSING_SEL                    = to_underlying(::NetworkGameWidgets::WID_NG_NEWGRF_MISSING_SEL),                    ///< Selection widget for the above button.

		WID_NG_SEARCH_INTERNET                       = to_underlying(::NetworkGameWidgets::WID_NG_SEARCH_INTERNET),                       ///< 'Search internet server' button.
		WID_NG_SEARCH_LAN                            = to_underlying(::NetworkGameWidgets::WID_NG_SEARCH_LAN),                            ///< 'Search LAN server' button.
		WID_NG_ADD                                   = to_underlying(::NetworkGameWidgets::WID_NG_ADD),                                   ///< 'Add server' button.
		WID_NG_START                                 = to_underlying(::NetworkGameWidgets::WID_NG_START),                                 ///< 'Start server' button.
	};

	/** Widgets of the NetworkStartServerWindow class. */
	enum NetworkStartServerWidgets {
		WID_NSS_BACKGROUND                           = to_underlying(::NetworkStartServerWidgets::WID_NSS_BACKGROUND),                           ///< Background of the window.
		WID_NSS_GAMENAME_LABEL                       = to_underlying(::NetworkStartServerWidgets::WID_NSS_GAMENAME_LABEL),                       ///< Label for the game name.
		WID_NSS_GAMENAME                             = to_underlying(::NetworkStartServerWidgets::WID_NSS_GAMENAME),                             ///< Background for editbox to set game name.
		WID_NSS_SETPWD                               = to_underlying(::NetworkStartServerWidgets::WID_NSS_SETPWD),                               ///< 'Set password' button.
		WID_NSS_CONNTYPE_LABEL                       = to_underlying(::NetworkStartServerWidgets::WID_NSS_CONNTYPE_LABEL),                       ///< Label for 'connection type'.
		WID_NSS_CONNTYPE_BTN                         = to_underlying(::NetworkStartServerWidgets::WID_NSS_CONNTYPE_BTN),                         ///< 'Connection type' droplist button.
		WID_NSS_CLIENTS_LABEL                        = to_underlying(::NetworkStartServerWidgets::WID_NSS_CLIENTS_LABEL),                        ///< Label for 'max clients'.
		WID_NSS_CLIENTS_BTND                         = to_underlying(::NetworkStartServerWidgets::WID_NSS_CLIENTS_BTND),                         ///< 'Max clients' downarrow.
		WID_NSS_CLIENTS_TXT                          = to_underlying(::NetworkStartServerWidgets::WID_NSS_CLIENTS_TXT),                          ///< 'Max clients' text.
		WID_NSS_CLIENTS_BTNU                         = to_underlying(::NetworkStartServerWidgets::WID_NSS_CLIENTS_BTNU),                         ///< 'Max clients' uparrow.
		WID_NSS_COMPANIES_LABEL                      = to_underlying(::NetworkStartServerWidgets::WID_NSS_COMPANIES_LABEL),                      ///< Label for 'max companies'.
		WID_NSS_COMPANIES_BTND                       = to_underlying(::NetworkStartServerWidgets::WID_NSS_COMPANIES_BTND),                       ///< 'Max companies' downarrow.
		WID_NSS_COMPANIES_TXT                        = to_underlying(::NetworkStartServerWidgets::WID_NSS_COMPANIES_TXT),                        ///< 'Max companies' text.
		WID_NSS_COMPANIES_BTNU                       = to_underlying(::NetworkStartServerWidgets::WID_NSS_COMPANIES_BTNU),                       ///< 'Max companies' uparrow.

		WID_NSS_GENERATE_GAME                        = to_underlying(::NetworkStartServerWidgets::WID_NSS_GENERATE_GAME),                        ///< New game button.
		WID_NSS_LOAD_GAME                            = to_underlying(::NetworkStartServerWidgets::WID_NSS_LOAD_GAME),                            ///< Load game button.
		WID_NSS_PLAY_SCENARIO                        = to_underlying(::NetworkStartServerWidgets::WID_NSS_PLAY_SCENARIO),                        ///< Play scenario button.
		WID_NSS_PLAY_HEIGHTMAP                       = to_underlying(::NetworkStartServerWidgets::WID_NSS_PLAY_HEIGHTMAP),                       ///< Play heightmap button.

		WID_NSS_CANCEL                               = to_underlying(::NetworkStartServerWidgets::WID_NSS_CANCEL),                               ///< 'Cancel' button.
	};

	/** Widgets of the NetworkClientListWindow class. */
	enum ClientListWidgets {
		WID_CL_PANEL                                 = to_underlying(::ClientListWidgets::WID_CL_PANEL),                                 ///< Panel of the window.
		WID_CL_SERVER_SELECTOR                       = to_underlying(::ClientListWidgets::WID_CL_SERVER_SELECTOR),                       ///< Selector to hide the server frame.
		WID_CL_SERVER_NAME                           = to_underlying(::ClientListWidgets::WID_CL_SERVER_NAME),                           ///< Server name.
		WID_CL_SERVER_NAME_EDIT                      = to_underlying(::ClientListWidgets::WID_CL_SERVER_NAME_EDIT),                      ///< Edit button for server name.
		WID_CL_SERVER_VISIBILITY                     = to_underlying(::ClientListWidgets::WID_CL_SERVER_VISIBILITY),                     ///< Server visibility.
		WID_CL_SERVER_INVITE_CODE                    = to_underlying(::ClientListWidgets::WID_CL_SERVER_INVITE_CODE),                    ///< Invite code for this server.
		WID_CL_SERVER_CONNECTION_TYPE                = to_underlying(::ClientListWidgets::WID_CL_SERVER_CONNECTION_TYPE),                ///< The type of connection the Game Coordinator detected for this server.
		WID_CL_CLIENT_NAME                           = to_underlying(::ClientListWidgets::WID_CL_CLIENT_NAME),                           ///< Client name.
		WID_CL_CLIENT_NAME_EDIT                      = to_underlying(::ClientListWidgets::WID_CL_CLIENT_NAME_EDIT),                      ///< Edit button for client name.
		WID_CL_MATRIX                                = to_underlying(::ClientListWidgets::WID_CL_MATRIX),                                ///< Company/client list.
		WID_CL_SCROLLBAR                             = to_underlying(::ClientListWidgets::WID_CL_SCROLLBAR),                             ///< Scrollbar for company/client list.
		WID_CL_CLIENT_COMPANY_COUNT                  = to_underlying(::ClientListWidgets::WID_CL_CLIENT_COMPANY_COUNT),                  ///< Count of clients and companies.
	};

	/** Widgets of the NetworkJoinStatusWindow class. */
	enum NetworkJoinStatusWidgets {
		WID_NJS_PROGRESS_BAR                         = to_underlying(::NetworkJoinStatusWidgets::WID_NJS_PROGRESS_BAR),                         ///< Simple progress bar.
		WID_NJS_PROGRESS_TEXT                        = to_underlying(::NetworkJoinStatusWidgets::WID_NJS_PROGRESS_TEXT),                        ///< Text explaining what is happening.
		WID_NJS_CANCELOK                             = to_underlying(::NetworkJoinStatusWidgets::WID_NJS_CANCELOK),                             ///< Cancel / OK button.
	};

	/** Widgets of the NetworkAskRelayWindow class. */
	enum NetworkAskRelayWidgets {
		WID_NAR_CAPTION                              = to_underlying(::NetworkAskRelayWidgets::WID_NAR_CAPTION),                              ///< Caption of the window.
		WID_NAR_TEXT                                 = to_underlying(::NetworkAskRelayWidgets::WID_NAR_TEXT),                                 ///< Text in the window.
		WID_NAR_NO                                   = to_underlying(::NetworkAskRelayWidgets::WID_NAR_NO),                                   ///< "No" button.
		WID_NAR_YES_ONCE                             = to_underlying(::NetworkAskRelayWidgets::WID_NAR_YES_ONCE),                             ///< "Yes, once" button.
		WID_NAR_YES_ALWAYS                           = to_underlying(::NetworkAskRelayWidgets::WID_NAR_YES_ALWAYS),                           ///< "Yes, always" button.
	};

	/** Widgets of the NetworkAskSurveyWindow class. */
	enum NetworkAskSurveyWidgets {
		WID_NAS_CAPTION                              = to_underlying(::NetworkAskSurveyWidgets::WID_NAS_CAPTION),                              ///< Caption of the window.
		WID_NAS_TEXT                                 = to_underlying(::NetworkAskSurveyWidgets::WID_NAS_TEXT),                                 ///< Text in the window.
		WID_NAS_PREVIEW                              = to_underlying(::NetworkAskSurveyWidgets::WID_NAS_PREVIEW),                              ///< "Preview" button.
		WID_NAS_LINK                                 = to_underlying(::NetworkAskSurveyWidgets::WID_NAS_LINK),                                 ///< "Details & Privacy" button.
		WID_NAS_NO                                   = to_underlying(::NetworkAskSurveyWidgets::WID_NAS_NO),                                   ///< "No" button.
		WID_NAS_YES                                  = to_underlying(::NetworkAskSurveyWidgets::WID_NAS_YES),                                  ///< "Yes" button.
	};

	/* automatically generated from ../../widgets/newgrf_debug_widget.h */
	/** Widgets of the NewGRFInspectWindow class. */
	enum NewGRFInspectWidgets {
		WID_NGRFI_CAPTION                            = to_underlying(::NewGRFInspectWidgets::WID_NGRFI_CAPTION),                            ///< The caption bar of course.
		WID_NGRFI_PARENT                             = to_underlying(::NewGRFInspectWidgets::WID_NGRFI_PARENT),                             ///< Inspect the parent.
		WID_NGRFI_VEH_PREV                           = to_underlying(::NewGRFInspectWidgets::WID_NGRFI_VEH_PREV),                           ///< Go to previous vehicle in chain.
		WID_NGRFI_VEH_NEXT                           = to_underlying(::NewGRFInspectWidgets::WID_NGRFI_VEH_NEXT),                           ///< Go to next vehicle in chain.
		WID_NGRFI_VEH_CHAIN                          = to_underlying(::NewGRFInspectWidgets::WID_NGRFI_VEH_CHAIN),                          ///< Display for vehicle chain.
		WID_NGRFI_MAINPANEL                          = to_underlying(::NewGRFInspectWidgets::WID_NGRFI_MAINPANEL),                          ///< Panel widget containing the actual data.
		WID_NGRFI_SCROLLBAR                          = to_underlying(::NewGRFInspectWidgets::WID_NGRFI_SCROLLBAR),                          ///< Scrollbar.
	};

	/** Widgets of the SpriteAlignerWindow class. */
	enum SpriteAlignerWidgets {
		WID_SA_CAPTION                               = to_underlying(::SpriteAlignerWidgets::WID_SA_CAPTION),                               ///< Caption of the window.
		WID_SA_PREVIOUS                              = to_underlying(::SpriteAlignerWidgets::WID_SA_PREVIOUS),                              ///< Skip to the previous sprite.
		WID_SA_GOTO                                  = to_underlying(::SpriteAlignerWidgets::WID_SA_GOTO),                                  ///< Go to a given sprite.
		WID_SA_NEXT                                  = to_underlying(::SpriteAlignerWidgets::WID_SA_NEXT),                                  ///< Skip to the next sprite.
		WID_SA_UP                                    = to_underlying(::SpriteAlignerWidgets::WID_SA_UP),                                    ///< Move the sprite up.
		WID_SA_LEFT                                  = to_underlying(::SpriteAlignerWidgets::WID_SA_LEFT),                                  ///< Move the sprite to the left.
		WID_SA_RIGHT                                 = to_underlying(::SpriteAlignerWidgets::WID_SA_RIGHT),                                 ///< Move the sprite to the right.
		WID_SA_DOWN                                  = to_underlying(::SpriteAlignerWidgets::WID_SA_DOWN),                                  ///< Move the sprite down.
		WID_SA_SPRITE                                = to_underlying(::SpriteAlignerWidgets::WID_SA_SPRITE),                                ///< The actual sprite.
		WID_SA_OFFSETS_ABS                           = to_underlying(::SpriteAlignerWidgets::WID_SA_OFFSETS_ABS),                           ///< The sprite offsets (absolute).
		WID_SA_OFFSETS_REL                           = to_underlying(::SpriteAlignerWidgets::WID_SA_OFFSETS_REL),                           ///< The sprite offsets (relative).
		WID_SA_PICKER                                = to_underlying(::SpriteAlignerWidgets::WID_SA_PICKER),                                ///< Sprite picker.
		WID_SA_LIST                                  = to_underlying(::SpriteAlignerWidgets::WID_SA_LIST),                                  ///< Queried sprite list.
		WID_SA_SCROLLBAR                             = to_underlying(::SpriteAlignerWidgets::WID_SA_SCROLLBAR),                             ///< Scrollbar for sprite list.
		WID_SA_ZOOM                                  = to_underlying(::SpriteAlignerWidgets::WID_SA_ZOOM),                                  ///< Zoom level buttons (from ZoomLevel::Begin to ZoomLevel::Max).
		WID_SA_ZOOM_LAST                             = to_underlying(::SpriteAlignerWidgets::WID_SA_ZOOM_LAST),                             ///< Marker for last zoom level button.
		WID_SA_RESET_REL                             = to_underlying(::SpriteAlignerWidgets::WID_SA_RESET_REL),                             ///< Reset relative sprite offset
		WID_SA_CENTRE                                = to_underlying(::SpriteAlignerWidgets::WID_SA_CENTRE),                                ///< Toggle centre sprite.
		WID_SA_CROSSHAIR                             = to_underlying(::SpriteAlignerWidgets::WID_SA_CROSSHAIR),                             ///< Toggle crosshair.
	};

	/* automatically generated from ../../widgets/newgrf_widget.h */
	/** Widgets of the NewGRFParametersWindow class. */
	enum NewGRFParametersWidgets {
		WID_NP_CAPTION                               = to_underlying(::NewGRFParametersWidgets::WID_NP_CAPTION),                               ///< Caption of the window.
		WID_NP_SHOW_NUMPAR                           = to_underlying(::NewGRFParametersWidgets::WID_NP_SHOW_NUMPAR),                           ///< NWID_SELECTION to optionally display WID_NP_NUMPAR.
		WID_NP_NUMPAR_DEC                            = to_underlying(::NewGRFParametersWidgets::WID_NP_NUMPAR_DEC),                            ///< Button to decrease number of parameters.
		WID_NP_NUMPAR_INC                            = to_underlying(::NewGRFParametersWidgets::WID_NP_NUMPAR_INC),                            ///< Button to increase number of parameters.
		WID_NP_NUMPAR                                = to_underlying(::NewGRFParametersWidgets::WID_NP_NUMPAR),                                ///< Optional number of parameters.
		WID_NP_NUMPAR_TEXT                           = to_underlying(::NewGRFParametersWidgets::WID_NP_NUMPAR_TEXT),                           ///< Text description.
		WID_NP_BACKGROUND                            = to_underlying(::NewGRFParametersWidgets::WID_NP_BACKGROUND),                            ///< Panel to draw the settings on.
		WID_NP_SCROLLBAR                             = to_underlying(::NewGRFParametersWidgets::WID_NP_SCROLLBAR),                             ///< Scrollbar to scroll through all settings.
		WID_NP_RESET                                 = to_underlying(::NewGRFParametersWidgets::WID_NP_RESET),                                 ///< Reset button.
		WID_NP_SHOW_DESCRIPTION                      = to_underlying(::NewGRFParametersWidgets::WID_NP_SHOW_DESCRIPTION),                      ///< NWID_SELECTION to optionally display parameter descriptions.
		WID_NP_DESCRIPTION                           = to_underlying(::NewGRFParametersWidgets::WID_NP_DESCRIPTION),                           ///< Multi-line description of a parameter.

		WID_NP_SETTING_DROPDOWN                      = to_underlying(::NewGRFParametersWidgets::WID_NP_SETTING_DROPDOWN),                      ///< Dynamically created dropdown for changing setting value.
	};

	/** Widgets of the NewGRFWindow class. */
	enum NewGRFStateWidgets {
		WID_NS_PRESET_LIST                           = to_underlying(::NewGRFStateWidgets::WID_NS_PRESET_LIST),                           ///< Active NewGRF preset.
		WID_NS_PRESET_SAVE                           = to_underlying(::NewGRFStateWidgets::WID_NS_PRESET_SAVE),                           ///< Save list of active NewGRFs as presets.
		WID_NS_PRESET_DELETE                         = to_underlying(::NewGRFStateWidgets::WID_NS_PRESET_DELETE),                         ///< Delete active preset.
		WID_NS_ADD                                   = to_underlying(::NewGRFStateWidgets::WID_NS_ADD),                                   ///< Add NewGRF to active list.
		WID_NS_REMOVE                                = to_underlying(::NewGRFStateWidgets::WID_NS_REMOVE),                                ///< Remove NewGRF from active list.
		WID_NS_MOVE_UP                               = to_underlying(::NewGRFStateWidgets::WID_NS_MOVE_UP),                               ///< Move NewGRF up in active list.
		WID_NS_MOVE_DOWN                             = to_underlying(::NewGRFStateWidgets::WID_NS_MOVE_DOWN),                             ///< Move NewGRF down in active list.
		WID_NS_UPGRADE                               = to_underlying(::NewGRFStateWidgets::WID_NS_UPGRADE),                               ///< Upgrade NewGRFs that have a newer version available.
		WID_NS_FILTER                                = to_underlying(::NewGRFStateWidgets::WID_NS_FILTER),                                ///< Filter list of available NewGRFs.
		WID_NS_FILE_LIST                             = to_underlying(::NewGRFStateWidgets::WID_NS_FILE_LIST),                             ///< List window of active NewGRFs.
		WID_NS_SCROLLBAR                             = to_underlying(::NewGRFStateWidgets::WID_NS_SCROLLBAR),                             ///< Scrollbar for active NewGRF list.
		WID_NS_AVAIL_LIST                            = to_underlying(::NewGRFStateWidgets::WID_NS_AVAIL_LIST),                            ///< List window of available NewGRFs.
		WID_NS_SCROLL2BAR                            = to_underlying(::NewGRFStateWidgets::WID_NS_SCROLL2BAR),                            ///< Scrollbar for available NewGRF list.
		WID_NS_NEWGRF_INFO_TITLE                     = to_underlying(::NewGRFStateWidgets::WID_NS_NEWGRF_INFO_TITLE),                     ///< Title for Info on selected NewGRF.
		WID_NS_NEWGRF_INFO                           = to_underlying(::NewGRFStateWidgets::WID_NS_NEWGRF_INFO),                           ///< Panel for Info on selected NewGRF.
		WID_NS_OPEN_URL                              = to_underlying(::NewGRFStateWidgets::WID_NS_OPEN_URL),                              ///< Open URL of NewGRF.
		WID_NS_NEWGRF_TEXTFILE                       = to_underlying(::NewGRFStateWidgets::WID_NS_NEWGRF_TEXTFILE),                       ///< Open NewGRF readme, changelog (+1) or license (+2).
		WID_NS_SET_PARAMETERS                        = to_underlying(::NewGRFStateWidgets::WID_NS_SET_PARAMETERS),                        ///< Open Parameters Window for selected NewGRF for editing parameters.
		WID_NS_VIEW_PARAMETERS                       = to_underlying(::NewGRFStateWidgets::WID_NS_VIEW_PARAMETERS),                       ///< Open Parameters Window for selected NewGRF for viewing parameters.
		WID_NS_TOGGLE_PALETTE                        = to_underlying(::NewGRFStateWidgets::WID_NS_TOGGLE_PALETTE),                        ///< Toggle Palette of selected, active NewGRF.
		WID_NS_APPLY_CHANGES                         = to_underlying(::NewGRFStateWidgets::WID_NS_APPLY_CHANGES),                         ///< Apply changes to NewGRF config.
		WID_NS_RESCAN_FILES                          = to_underlying(::NewGRFStateWidgets::WID_NS_RESCAN_FILES),                          ///< Rescan files (available NewGRFs).
		WID_NS_RESCAN_FILES2                         = to_underlying(::NewGRFStateWidgets::WID_NS_RESCAN_FILES2),                         ///< Rescan files (active NewGRFs).
		WID_NS_CONTENT_DOWNLOAD                      = to_underlying(::NewGRFStateWidgets::WID_NS_CONTENT_DOWNLOAD),                      ///< Open content download (available NewGRFs).
		WID_NS_CONTENT_DOWNLOAD2                     = to_underlying(::NewGRFStateWidgets::WID_NS_CONTENT_DOWNLOAD2),                     ///< Open content download (active NewGRFs).
		WID_NS_SHOW_REMOVE                           = to_underlying(::NewGRFStateWidgets::WID_NS_SHOW_REMOVE),                           ///< Select active list buttons (0, 1 = simple layout).
		WID_NS_SHOW_EDIT                             = to_underlying(::NewGRFStateWidgets::WID_NS_SHOW_EDIT),                             ///< Select display of the buttons below the 'details'.
		WID_NS_SHOW_APPLY                            = to_underlying(::NewGRFStateWidgets::WID_NS_SHOW_APPLY),                            ///< Select display of the apply button.
	};

	/** Widgets of the SavePresetWindow class. */
	enum SavePresetWidgets {
		WID_SVP_PRESET_LIST                          = to_underlying(::SavePresetWidgets::WID_SVP_PRESET_LIST),                          ///< List with available preset names.
		WID_SVP_SCROLLBAR                            = to_underlying(::SavePresetWidgets::WID_SVP_SCROLLBAR),                            ///< Scrollbar for the list available preset names.
		WID_SVP_EDITBOX                              = to_underlying(::SavePresetWidgets::WID_SVP_EDITBOX),                              ///< Edit box for changing the preset name.
		WID_SVP_SAVE                                 = to_underlying(::SavePresetWidgets::WID_SVP_SAVE),                                 ///< Button to save the preset.
	};

	/** Widgets of the ScanProgressWindow class. */
	enum ScanProgressWidgets {
		WID_SP_PROGRESS_BAR                          = to_underlying(::ScanProgressWidgets::WID_SP_PROGRESS_BAR),                          ///< Simple progress bar.
		WID_SP_PROGRESS_TEXT                         = to_underlying(::ScanProgressWidgets::WID_SP_PROGRESS_TEXT),                         ///< Text explaining what is happening.
	};

	/* automatically generated from ../../widgets/news_widget.h */
	/** Widgets of the NewsWindow class. */
	enum NewsWidgets {
		WID_N_PANEL                                  = to_underlying(::NewsWidgets::WID_N_PANEL),                                  ///< Panel of the window.
		WID_N_TITLE                                  = to_underlying(::NewsWidgets::WID_N_TITLE),                                  ///< Title of the company news.
		WID_N_HEADLINE                               = to_underlying(::NewsWidgets::WID_N_HEADLINE),                               ///< The news headline.
		WID_N_CLOSEBOX                               = to_underlying(::NewsWidgets::WID_N_CLOSEBOX),                               ///< Close the window.
		WID_N_DATE                                   = to_underlying(::NewsWidgets::WID_N_DATE),                                   ///< Date of the news item.
		WID_N_CAPTION                                = to_underlying(::NewsWidgets::WID_N_CAPTION),                                ///< Title bar of the window. Only used in small news items.
		WID_N_INSET                                  = to_underlying(::NewsWidgets::WID_N_INSET),                                  ///< Inset around the viewport in the window. Only used in small news items.
		WID_N_VIEWPORT                               = to_underlying(::NewsWidgets::WID_N_VIEWPORT),                               ///< Viewport in the window.
		WID_N_COMPANY_MSG                            = to_underlying(::NewsWidgets::WID_N_COMPANY_MSG),                            ///< Message in company news items.
		WID_N_MESSAGE                                = to_underlying(::NewsWidgets::WID_N_MESSAGE),                                ///< Space for displaying the message. Only used in small news items.
		WID_N_MGR_FACE                               = to_underlying(::NewsWidgets::WID_N_MGR_FACE),                               ///< Face of the manager.
		WID_N_MGR_NAME                               = to_underlying(::NewsWidgets::WID_N_MGR_NAME),                               ///< Name of the manager.
		WID_N_VEH_TITLE                              = to_underlying(::NewsWidgets::WID_N_VEH_TITLE),                              ///< Vehicle new title.
		WID_N_VEH_BKGND                              = to_underlying(::NewsWidgets::WID_N_VEH_BKGND),                              ///< Dark background of new vehicle news.
		WID_N_VEH_NAME                               = to_underlying(::NewsWidgets::WID_N_VEH_NAME),                               ///< Name of the new vehicle.
		WID_N_VEH_SPR                                = to_underlying(::NewsWidgets::WID_N_VEH_SPR),                                ///< Graphical display of the new vehicle.
		WID_N_VEH_INFO                               = to_underlying(::NewsWidgets::WID_N_VEH_INFO),                               ///< Some technical data of the new vehicle.
		WID_N_SHOW_GROUP                             = to_underlying(::NewsWidgets::WID_N_SHOW_GROUP),                             ///< Show vehicle's group
		WID_N_SHOW_GROUP_SEL                         = to_underlying(::NewsWidgets::WID_N_SHOW_GROUP_SEL),                         ///< Selector for showing vehicle group, which can be hidden.
	};

	/** Widgets of the MessageHistoryWindow class. */
	enum MessageHistoryWidgets {
		WID_MH_STICKYBOX                             = to_underlying(::MessageHistoryWidgets::WID_MH_STICKYBOX),                             ///< Stickybox.
		WID_MH_BACKGROUND                            = to_underlying(::MessageHistoryWidgets::WID_MH_BACKGROUND),                            ///< Background of the window.
		WID_MH_SCROLLBAR                             = to_underlying(::MessageHistoryWidgets::WID_MH_SCROLLBAR),                             ///< Scrollbar for the list.
	};

	/* automatically generated from ../../widgets/object_widget.h */
	/** Widgets of the BuildObjectWindow class. */
	enum BuildObjectWidgets {
		WID_BO_OBJECT_MATRIX                         = to_underlying(::BuildObjectWidgets::WID_BO_OBJECT_MATRIX),                         ///< The matrix with preview sprites.
		WID_BO_OBJECT_SPRITE                         = to_underlying(::BuildObjectWidgets::WID_BO_OBJECT_SPRITE),                         ///< A preview sprite of the object.
		WID_BO_OBJECT_SIZE                           = to_underlying(::BuildObjectWidgets::WID_BO_OBJECT_SIZE),                           ///< The size of the selected object.
		WID_BO_INFO                                  = to_underlying(::BuildObjectWidgets::WID_BO_INFO),                                  ///< Other information about the object (from the NewGRF).
	};

	/* automatically generated from ../../widgets/order_widget.h */
	/** Widgets of the OrdersWindow class. */
	enum OrderWidgets {
		WID_O_CAPTION                                = to_underlying(::OrderWidgets::WID_O_CAPTION),                                ///< Caption of the window.
		WID_O_TIMETABLE_VIEW                         = to_underlying(::OrderWidgets::WID_O_TIMETABLE_VIEW),                         ///< Toggle timetable view.
		WID_O_ORDER_LIST                             = to_underlying(::OrderWidgets::WID_O_ORDER_LIST),                             ///< Order list panel.
		WID_O_SCROLLBAR                              = to_underlying(::OrderWidgets::WID_O_SCROLLBAR),                              ///< Order list scrollbar.
		WID_O_SKIP                                   = to_underlying(::OrderWidgets::WID_O_SKIP),                                   ///< Skip current order.
		WID_O_DELETE                                 = to_underlying(::OrderWidgets::WID_O_DELETE),                                 ///< Delete selected order.
		WID_O_STOP_SHARING                           = to_underlying(::OrderWidgets::WID_O_STOP_SHARING),                           ///< Stop sharing orders.
		WID_O_NON_STOP                               = to_underlying(::OrderWidgets::WID_O_NON_STOP),                               ///< Goto non-stop to destination.
		WID_O_DEPOT_UNBUNCHING                       = to_underlying(::OrderWidgets::WID_O_DEPOT_UNBUNCHING),                       ///< Toggle unbunching.
		WID_O_GOTO                                   = to_underlying(::OrderWidgets::WID_O_GOTO),                                   ///< Goto destination.
		WID_O_FULL_LOAD                              = to_underlying(::OrderWidgets::WID_O_FULL_LOAD),                              ///< Select full load.
		WID_O_UNLOAD                                 = to_underlying(::OrderWidgets::WID_O_UNLOAD),                                 ///< Select unload.
		WID_O_REFIT                                  = to_underlying(::OrderWidgets::WID_O_REFIT),                                  ///< Select refit.
		WID_O_DEPOT_ACTION                           = to_underlying(::OrderWidgets::WID_O_DEPOT_ACTION),                           ///< Dropdown to select the depot action (stop, service if needed, unbunch).
		WID_O_REFIT_DROPDOWN                         = to_underlying(::OrderWidgets::WID_O_REFIT_DROPDOWN),                         ///< Open refit options.
		WID_O_COND_VARIABLE                          = to_underlying(::OrderWidgets::WID_O_COND_VARIABLE),                          ///< Choose condition variable.
		WID_O_COND_COMPARATOR                        = to_underlying(::OrderWidgets::WID_O_COND_COMPARATOR),                        ///< Choose condition type.
		WID_O_COND_VALUE                             = to_underlying(::OrderWidgets::WID_O_COND_VALUE),                             ///< Choose condition value.
		WID_O_SEL_TOP_LEFT                           = to_underlying(::OrderWidgets::WID_O_SEL_TOP_LEFT),                           ///< NWID_SELECTION widget for left part of the top row of the 'your train' order window.
		WID_O_SEL_TOP_MIDDLE                         = to_underlying(::OrderWidgets::WID_O_SEL_TOP_MIDDLE),                         ///< NWID_SELECTION widget for middle part of the top row of the 'your train' order window.
		WID_O_SEL_TOP_RIGHT                          = to_underlying(::OrderWidgets::WID_O_SEL_TOP_RIGHT),                          ///< NWID_SELECTION widget for right part of the top row of the 'your train' order window.
		WID_O_SEL_TOP_ROW_GROUNDVEHICLE              = to_underlying(::OrderWidgets::WID_O_SEL_TOP_ROW_GROUNDVEHICLE),              ///< NWID_SELECTION widget for the top row of the 'your train' order window.
		WID_O_SEL_TOP_ROW                            = to_underlying(::OrderWidgets::WID_O_SEL_TOP_ROW),                            ///< NWID_SELECTION widget for the top row of the 'your non-trains' order window.
		WID_O_SEL_BOTTOM_MIDDLE                      = to_underlying(::OrderWidgets::WID_O_SEL_BOTTOM_MIDDLE),                      ///< NWID_SELECTION widget for the middle part of the bottom row of the 'your train' order window.
		WID_O_SHARED_ORDER_LIST                      = to_underlying(::OrderWidgets::WID_O_SHARED_ORDER_LIST),                      ///< Open list of shared vehicles.
	};

	/* automatically generated from ../../widgets/osk_widget.h */
	/** Widgets of the OskWindow class. */
	enum OnScreenKeyboardWidgets {
		WID_OSK_CAPTION                              = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_CAPTION),                              ///< Caption of window.
		WID_OSK_TEXT                                 = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_TEXT),                                 ///< Edit box.
		WID_OSK_CANCEL                               = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_CANCEL),                               ///< Cancel key.
		WID_OSK_OK                                   = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_OK),                                   ///< Ok key.
		WID_OSK_BACKSPACE                            = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_BACKSPACE),                            ///< Backspace key.
		WID_OSK_SPECIAL                              = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_SPECIAL),                              ///< Special key (at keyboards often used for tab key).
		WID_OSK_CAPS                                 = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_CAPS),                                 ///< Capslock key.
		WID_OSK_SHIFT                                = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_SHIFT),                                ///< Shift(lock) key.
		WID_OSK_SPACE                                = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_SPACE),                                ///< Space bar.
		WID_OSK_LEFT                                 = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_LEFT),                                 ///< Cursor left key.
		WID_OSK_RIGHT                                = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_RIGHT),                                ///< Cursor right key.
		WID_OSK_LETTERS                              = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_LETTERS),                              ///< First widget of the 'normal' keys.

		WID_OSK_NUMBERS_FIRST                        = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_NUMBERS_FIRST),                        ///< First widget of the numbers row.
		WID_OSK_NUMBERS_LAST                         = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_NUMBERS_LAST),                         ///< Last widget of the numbers row.

		WID_OSK_QWERTY_FIRST                         = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_QWERTY_FIRST),                         ///< First widget of the qwerty row.
		WID_OSK_QWERTY_LAST                          = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_QWERTY_LAST),                          ///< Last widget of the qwerty row.

		WID_OSK_ASDFG_FIRST                          = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_ASDFG_FIRST),                          ///< First widget of the asdfg row.
		WID_OSK_ASDFG_LAST                           = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_ASDFG_LAST),                           ///< Last widget of the asdfg row.

		WID_OSK_ZXCVB_FIRST                          = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_ZXCVB_FIRST),                          ///< First widget of the zxcvb row.
		WID_OSK_ZXCVB_LAST                           = to_underlying(::OnScreenKeyboardWidgets::WID_OSK_ZXCVB_LAST),                           ///< Last widget of the zxcvb row.
	};

	/* automatically generated from ../../widgets/picker_widget.h */
	/** Widgets of the PickerWindow class. */
	enum PickerClassWindowWidgets {
		WID_PW_START                                 = to_underlying(::PickerClassWindowWidgets::WID_PW_START),                                 ///< Dummy to ensure widgets don't overlap.

		WID_PW_CLASS_SEL                             = to_underlying(::PickerClassWindowWidgets::WID_PW_CLASS_SEL),                             ///< Stack to hide the class picker.
		WID_PW_CLASS_FILTER                          = to_underlying(::PickerClassWindowWidgets::WID_PW_CLASS_FILTER),                          ///< Editbox filter.
		WID_PW_CLASS_LIST                            = to_underlying(::PickerClassWindowWidgets::WID_PW_CLASS_LIST),                            ///< List of classes.
		WID_PW_CLASS_SCROLL                          = to_underlying(::PickerClassWindowWidgets::WID_PW_CLASS_SCROLL),                          ///< Scrollbar for list of classes.

		WID_PW_COLEC_LIST                            = to_underlying(::PickerClassWindowWidgets::WID_PW_COLEC_LIST),                            ///< List of collections.
		WID_PW_COLEC_ADD                             = to_underlying(::PickerClassWindowWidgets::WID_PW_COLEC_ADD),                             ///< Button to create a new collections.
		WID_PW_COLEC_RENAME                          = to_underlying(::PickerClassWindowWidgets::WID_PW_COLEC_RENAME),                          ///< Button to rename a collections.
		WID_PW_COLEC_DELETE                          = to_underlying(::PickerClassWindowWidgets::WID_PW_COLEC_DELETE),                          ///< Button to delete a collection.

		WID_PW_TYPE_SEL                              = to_underlying(::PickerClassWindowWidgets::WID_PW_TYPE_SEL),                              ///< Stack to hide the type picker.
		WID_PW_TYPE_FILTER                           = to_underlying(::PickerClassWindowWidgets::WID_PW_TYPE_FILTER),                           ///< Text filter.
		WID_PW_MODE_ALL                              = to_underlying(::PickerClassWindowWidgets::WID_PW_MODE_ALL),                              ///< Toggle "Show all" filter mode.
		WID_PW_MODE_USED                             = to_underlying(::PickerClassWindowWidgets::WID_PW_MODE_USED),                             ///< Toggle showing only used types.
		WID_PW_MODE_SAVED                            = to_underlying(::PickerClassWindowWidgets::WID_PW_MODE_SAVED),                            ///< Toggle showing only saved types.
		WID_PW_SHRINK                                = to_underlying(::PickerClassWindowWidgets::WID_PW_SHRINK),                                ///< Button to reduce preview image height.
		WID_PW_EXPAND                                = to_underlying(::PickerClassWindowWidgets::WID_PW_EXPAND),                                ///< Button to increase preview image height.
		WID_PW_TYPE_MATRIX                           = to_underlying(::PickerClassWindowWidgets::WID_PW_TYPE_MATRIX),                           ///< Matrix with items.
		WID_PW_TYPE_ITEM                             = to_underlying(::PickerClassWindowWidgets::WID_PW_TYPE_ITEM),                             ///< A single item.
		WID_PW_TYPE_SCROLL                           = to_underlying(::PickerClassWindowWidgets::WID_PW_TYPE_SCROLL),                           ///< Scrollbar for the matrix.
		WID_PW_TYPE_NAME                             = to_underlying(::PickerClassWindowWidgets::WID_PW_TYPE_NAME),                             ///< Name of selected item.
		WID_PW_TYPE_RESIZE                           = to_underlying(::PickerClassWindowWidgets::WID_PW_TYPE_RESIZE),                           ///< Type resize handle.
		WID_PW_CONFIGURE_BADGES                      = to_underlying(::PickerClassWindowWidgets::WID_PW_CONFIGURE_BADGES),                      ///< Button to configure badges.
		WID_PW_BADGE_FILTER                          = to_underlying(::PickerClassWindowWidgets::WID_PW_BADGE_FILTER),                          ///< Container for dropdown badge filters. Must be last in this list.
	};

	/* automatically generated from ../../widgets/rail_widget.h */
	/** Widgets of the BuildRailToolbarWindow class. */
	enum RailToolbarWidgets {
		/* Name starts with RA instead of R, because of collision with RoadToolbarWidgets */
		WID_RAT_CAPTION                              = to_underlying(::RailToolbarWidgets::WID_RAT_CAPTION),                              ///< Caption of the window.
		WID_RAT_BUILD_NS                             = to_underlying(::RailToolbarWidgets::WID_RAT_BUILD_NS),                             ///< Build rail along the game view Y axis.
		WID_RAT_BUILD_X                              = to_underlying(::RailToolbarWidgets::WID_RAT_BUILD_X),                              ///< Build rail along the game grid X axis.
		WID_RAT_BUILD_EW                             = to_underlying(::RailToolbarWidgets::WID_RAT_BUILD_EW),                             ///< Build rail along the game view X axis.
		WID_RAT_BUILD_Y                              = to_underlying(::RailToolbarWidgets::WID_RAT_BUILD_Y),                              ///< Build rail along the game grid Y axis.
		WID_RAT_AUTORAIL                             = to_underlying(::RailToolbarWidgets::WID_RAT_AUTORAIL),                             ///< Autorail tool.
		WID_RAT_DEMOLISH                             = to_underlying(::RailToolbarWidgets::WID_RAT_DEMOLISH),                             ///< Destroy something with dynamite!
		WID_RAT_BUILD_DEPOT                          = to_underlying(::RailToolbarWidgets::WID_RAT_BUILD_DEPOT),                          ///< Build a depot.
		WID_RAT_BUILD_WAYPOINT                       = to_underlying(::RailToolbarWidgets::WID_RAT_BUILD_WAYPOINT),                       ///< Build a waypoint.
		WID_RAT_BUILD_STATION                        = to_underlying(::RailToolbarWidgets::WID_RAT_BUILD_STATION),                        ///< Build a station.
		WID_RAT_BUILD_SIGNALS                        = to_underlying(::RailToolbarWidgets::WID_RAT_BUILD_SIGNALS),                        ///< Build signals.
		WID_RAT_BUILD_BRIDGE                         = to_underlying(::RailToolbarWidgets::WID_RAT_BUILD_BRIDGE),                         ///< Build a bridge.
		WID_RAT_BUILD_TUNNEL                         = to_underlying(::RailToolbarWidgets::WID_RAT_BUILD_TUNNEL),                         ///< Build a tunnel.
		WID_RAT_BUILD_LAYER_PORTAL                   = to_underlying(::RailToolbarWidgets::WID_RAT_BUILD_LAYER_PORTAL),                   ///< Build a rail portal to another map layer.
		WID_RAT_REMOVE                               = to_underlying(::RailToolbarWidgets::WID_RAT_REMOVE),                               ///< Bulldozer to remove rail.
		WID_RAT_CONVERT_RAIL                         = to_underlying(::RailToolbarWidgets::WID_RAT_CONVERT_RAIL),                         ///< Convert other rail to this type.
	};

	/** Widgets of the BuildRailStationWindow class. */
	enum BuildRailStationWidgets {
		/* Name starts with BRA instead of BR, because of collision with BuildRoadStationWidgets */
		WID_BRAS_PLATFORM_DIR_X                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_DIR_X),                      ///< Button to select '/' view.
		WID_BRAS_PLATFORM_DIR_Y                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_DIR_Y),                      ///< Button to select '\' view.

		WID_BRAS_PLATFORM_NUM_1                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_NUM_1),                      ///< Button to select stations with a single platform.
		WID_BRAS_PLATFORM_NUM_2                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_NUM_2),                      ///< Button to select stations with 2 platforms.
		WID_BRAS_PLATFORM_NUM_3                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_NUM_3),                      ///< Button to select stations with 3 platforms.
		WID_BRAS_PLATFORM_NUM_4                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_NUM_4),                      ///< Button to select stations with 4 platforms.
		WID_BRAS_PLATFORM_NUM_5                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_NUM_5),                      ///< Button to select stations with 5 platforms.
		WID_BRAS_PLATFORM_NUM_6                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_NUM_6),                      ///< Button to select stations with 6 platforms.
		WID_BRAS_PLATFORM_NUM_7                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_NUM_7),                      ///< Button to select stations with 7 platforms.

		WID_BRAS_PLATFORM_LEN_1                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_LEN_1),                      ///< Button to select single tile length station platforms.
		WID_BRAS_PLATFORM_LEN_2                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_LEN_2),                      ///< Button to select 2 tiles length station platforms.
		WID_BRAS_PLATFORM_LEN_3                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_LEN_3),                      ///< Button to select 3 tiles length station platforms.
		WID_BRAS_PLATFORM_LEN_4                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_LEN_4),                      ///< Button to select 4 tiles length station platforms.
		WID_BRAS_PLATFORM_LEN_5                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_LEN_5),                      ///< Button to select 5 tiles length station platforms.
		WID_BRAS_PLATFORM_LEN_6                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_LEN_6),                      ///< Button to select 6 tiles length station platforms.
		WID_BRAS_PLATFORM_LEN_7                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_LEN_7),                      ///< Button to select 7 tiles length station platforms.

		WID_BRAS_PLATFORM_DRAG_N_DROP                = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_DRAG_N_DROP),                ///< Button to enable drag and drop type station placement.

		WID_BRAS_HIGHLIGHT_OFF                       = to_underlying(::BuildRailStationWidgets::WID_BRAS_HIGHLIGHT_OFF),                       ///< Button for turning coverage highlighting off.
		WID_BRAS_HIGHLIGHT_ON                        = to_underlying(::BuildRailStationWidgets::WID_BRAS_HIGHLIGHT_ON),                        ///< Button for turning coverage highlighting on.
		WID_BRAS_COVERAGE_TEXTS                      = to_underlying(::BuildRailStationWidgets::WID_BRAS_COVERAGE_TEXTS),                      ///< Empty space for the coverage texts.

		WID_BRAS_PLATFORM_NUM_BEGIN                  = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_NUM_BEGIN),                  ///< Helper for determining the chosen platform width.
		WID_BRAS_PLATFORM_LEN_BEGIN                  = to_underlying(::BuildRailStationWidgets::WID_BRAS_PLATFORM_LEN_BEGIN),                  ///< Helper for determining the chosen platform length.
	};

	/** Widgets of the BuildSignalWindow class. */
	enum BuildSignalWidgets {
		WID_BS_CAPTION                               = to_underlying(::BuildSignalWidgets::WID_BS_CAPTION),                               ///< Caption for the Signal Selection window.
		WID_BS_SEMAPHORE_NORM                        = to_underlying(::BuildSignalWidgets::WID_BS_SEMAPHORE_NORM),                        ///< Build a semaphore normal block signal.
		WID_BS_SEMAPHORE_ENTRY                       = to_underlying(::BuildSignalWidgets::WID_BS_SEMAPHORE_ENTRY),                       ///< Build a semaphore entry block signal.
		WID_BS_SEMAPHORE_EXIT                        = to_underlying(::BuildSignalWidgets::WID_BS_SEMAPHORE_EXIT),                        ///< Build a semaphore exit block signal.
		WID_BS_SEMAPHORE_COMBO                       = to_underlying(::BuildSignalWidgets::WID_BS_SEMAPHORE_COMBO),                       ///< Build a semaphore combo block signal.
		WID_BS_SEMAPHORE_PBS                         = to_underlying(::BuildSignalWidgets::WID_BS_SEMAPHORE_PBS),                         ///< Build a semaphore path signal.
		WID_BS_SEMAPHORE_PBS_OWAY                    = to_underlying(::BuildSignalWidgets::WID_BS_SEMAPHORE_PBS_OWAY),                    ///< Build a semaphore one way path signal.
		WID_BS_ELECTRIC_NORM                         = to_underlying(::BuildSignalWidgets::WID_BS_ELECTRIC_NORM),                         ///< Build an electric normal block signal.
		WID_BS_ELECTRIC_ENTRY                        = to_underlying(::BuildSignalWidgets::WID_BS_ELECTRIC_ENTRY),                        ///< Build an electric entry block signal.
		WID_BS_ELECTRIC_EXIT                         = to_underlying(::BuildSignalWidgets::WID_BS_ELECTRIC_EXIT),                         ///< Build an electric exit block signal.
		WID_BS_ELECTRIC_COMBO                        = to_underlying(::BuildSignalWidgets::WID_BS_ELECTRIC_COMBO),                        ///< Build an electric combo block signal.
		WID_BS_ELECTRIC_PBS                          = to_underlying(::BuildSignalWidgets::WID_BS_ELECTRIC_PBS),                          ///< Build an electric path signal.
		WID_BS_ELECTRIC_PBS_OWAY                     = to_underlying(::BuildSignalWidgets::WID_BS_ELECTRIC_PBS_OWAY),                     ///< Build an electric one way path signal.
		WID_BS_CONVERT                               = to_underlying(::BuildSignalWidgets::WID_BS_CONVERT),                               ///< Convert the signal.
		WID_BS_DRAG_SIGNALS_DENSITY_LABEL            = to_underlying(::BuildSignalWidgets::WID_BS_DRAG_SIGNALS_DENSITY_LABEL),            ///< The current signal density.
		WID_BS_DRAG_SIGNALS_DENSITY_DECREASE         = to_underlying(::BuildSignalWidgets::WID_BS_DRAG_SIGNALS_DENSITY_DECREASE),         ///< Decrease the signal density.
		WID_BS_DRAG_SIGNALS_DENSITY_INCREASE         = to_underlying(::BuildSignalWidgets::WID_BS_DRAG_SIGNALS_DENSITY_INCREASE),         ///< Increase the signal density.
		WID_BS_BLOCK_SEL                             = to_underlying(::BuildSignalWidgets::WID_BS_BLOCK_SEL),                             ///< Container for the block signal group, which can be hidden.
		WID_BS_BLOCK_SPACER_SEL                      = to_underlying(::BuildSignalWidgets::WID_BS_BLOCK_SPACER_SEL),                      ///< Container for the spacer between block and path signal groups, which can be hidden.
	};

	/** Widgets of the BuildRailDepotWindow class. */
	enum BuildRailDepotWidgets {
		/* Name starts with BRA instead of BR, because of collision with BuildRoadDepotWidgets */
		WID_BRAD_DEPOT_NE                            = to_underlying(::BuildRailDepotWidgets::WID_BRAD_DEPOT_NE),                            ///< Build a depot with the entrance in the north east.
		WID_BRAD_DEPOT_SE                            = to_underlying(::BuildRailDepotWidgets::WID_BRAD_DEPOT_SE),                            ///< Build a depot with the entrance in the south east.
		WID_BRAD_DEPOT_SW                            = to_underlying(::BuildRailDepotWidgets::WID_BRAD_DEPOT_SW),                            ///< Build a depot with the entrance in the south west.
		WID_BRAD_DEPOT_NW                            = to_underlying(::BuildRailDepotWidgets::WID_BRAD_DEPOT_NW),                            ///< Build a depot with the entrance in the north west.
	};

	/** Widgets of the BuildRailLayerPortalWindow class. */
	enum BuildRailLayerPortalWidgets {
		WID_BRLP_CAPTION                             = to_underlying(::BuildRailLayerPortalWidgets::WID_BRLP_CAPTION),                             ///< Caption of the window.
		WID_BRLP_MOUTH_NE                            = to_underlying(::BuildRailLayerPortalWidgets::WID_BRLP_MOUTH_NE),                            ///< Mouth facing north east.
		WID_BRLP_MOUTH_SE                            = to_underlying(::BuildRailLayerPortalWidgets::WID_BRLP_MOUTH_SE),                            ///< Mouth facing south east.
		WID_BRLP_MOUTH_SW                            = to_underlying(::BuildRailLayerPortalWidgets::WID_BRLP_MOUTH_SW),                            ///< Mouth facing south west.
		WID_BRLP_MOUTH_NW                            = to_underlying(::BuildRailLayerPortalWidgets::WID_BRLP_MOUTH_NW),                            ///< Mouth facing north west.
		WID_BRLP_TARGET_SEL                          = to_underlying(::BuildRailLayerPortalWidgets::WID_BRLP_TARGET_SEL),                          ///< Native-only Underground destination selector.
		WID_BRLP_TARGET_SURFACE                      = to_underlying(::BuildRailLayerPortalWidgets::WID_BRLP_TARGET_SURFACE),                      ///< Existing Underground-to-Surface rail destination.
		WID_BRLP_TARGET_DEEP                         = to_underlying(::BuildRailLayerPortalWidgets::WID_BRLP_TARGET_DEEP),                         ///< Profile-specific Underground-to-Deep rail destination.
		WID_BRLP_QUOTE                               = to_underlying(::BuildRailLayerPortalWidgets::WID_BRLP_QUOTE),                               ///< Actual clicked-layer destination, company and complete cost/refusal.
	};

	/** Widgets of the BuildRailWaypointWindow class. */
	enum BuildRailWaypointWidgets {
		WID_BRW_FILTER                               = to_underlying(::BuildRailWaypointWidgets::WID_BRW_FILTER),                               ///< Text filter.
		WID_BRW_WAYPOINT_MATRIX                      = to_underlying(::BuildRailWaypointWidgets::WID_BRW_WAYPOINT_MATRIX),                      ///< Matrix with waypoints.
		WID_BRW_WAYPOINT                             = to_underlying(::BuildRailWaypointWidgets::WID_BRW_WAYPOINT),                             ///< A single waypoint.
		WID_BRW_SCROLL                               = to_underlying(::BuildRailWaypointWidgets::WID_BRW_SCROLL),                               ///< Scrollbar for the matrix.
		WID_BRW_NAME                                 = to_underlying(::BuildRailWaypointWidgets::WID_BRW_NAME),                                 ///< Name of selected waypoint.
	};

	/* automatically generated from ../../widgets/road_widget.h */
	/** Widgets of the BuildRoadToolbarWindow class. */
	enum RoadToolbarWidgets {
		/* Name starts with RO instead of R, because of collision with RailToolbarWidgets */
		WID_ROT_CAPTION                              = to_underlying(::RoadToolbarWidgets::WID_ROT_CAPTION),                              ///< Caption of the window
		WID_ROT_ROAD_X                               = to_underlying(::RoadToolbarWidgets::WID_ROT_ROAD_X),                               ///< Build road in x-direction.
		WID_ROT_ROAD_Y                               = to_underlying(::RoadToolbarWidgets::WID_ROT_ROAD_Y),                               ///< Build road in y-direction.
		WID_ROT_AUTOROAD                             = to_underlying(::RoadToolbarWidgets::WID_ROT_AUTOROAD),                             ///< Autorail.
		WID_ROT_DEMOLISH                             = to_underlying(::RoadToolbarWidgets::WID_ROT_DEMOLISH),                             ///< Demolish.
		WID_ROT_DEPOT                                = to_underlying(::RoadToolbarWidgets::WID_ROT_DEPOT),                                ///< Build depot.
		WID_ROT_BUILD_WAYPOINT                       = to_underlying(::RoadToolbarWidgets::WID_ROT_BUILD_WAYPOINT),                       ///< Build waypoint.
		WID_ROT_BUS_STATION                          = to_underlying(::RoadToolbarWidgets::WID_ROT_BUS_STATION),                          ///< Build bus station.
		WID_ROT_TRUCK_STATION                        = to_underlying(::RoadToolbarWidgets::WID_ROT_TRUCK_STATION),                        ///< Build truck station.
		WID_ROT_ONE_WAY                              = to_underlying(::RoadToolbarWidgets::WID_ROT_ONE_WAY),                              ///< Build one-way road.
		WID_ROT_BUILD_BRIDGE                         = to_underlying(::RoadToolbarWidgets::WID_ROT_BUILD_BRIDGE),                         ///< Build bridge.
		WID_ROT_BUILD_TUNNEL                         = to_underlying(::RoadToolbarWidgets::WID_ROT_BUILD_TUNNEL),                         ///< Build tunnel.
		WID_ROT_LAYER_TUNNEL                         = to_underlying(::RoadToolbarWidgets::WID_ROT_LAYER_TUNNEL),                         ///< Build road tunnel to another Map Layer.
		WID_ROT_LAYER_ELEVATOR                       = to_underlying(::RoadToolbarWidgets::WID_ROT_LAYER_ELEVATOR),                       ///< Build road elevator to another map layer.
		WID_ROT_REMOVE                               = to_underlying(::RoadToolbarWidgets::WID_ROT_REMOVE),                               ///< Remove road.
		WID_ROT_CONVERT_ROAD                         = to_underlying(::RoadToolbarWidgets::WID_ROT_CONVERT_ROAD),                         ///< Convert road.
	};

	/** Widgets of the BuildRoadDepotWindow class. */
	enum BuildRoadDepotWidgets {
		/* Name starts with BRO instead of BR, because of collision with BuildRailDepotWidgets */
		WID_BROD_CAPTION                             = to_underlying(::BuildRoadDepotWidgets::WID_BROD_CAPTION),                             ///< Caption of the window.
		WID_BROD_DEPOT_NE                            = to_underlying(::BuildRoadDepotWidgets::WID_BROD_DEPOT_NE),                            ///< Depot with NE entry.
		WID_BROD_DEPOT_SE                            = to_underlying(::BuildRoadDepotWidgets::WID_BROD_DEPOT_SE),                            ///< Depot with SE entry.
		WID_BROD_DEPOT_SW                            = to_underlying(::BuildRoadDepotWidgets::WID_BROD_DEPOT_SW),                            ///< Depot with SW entry.
		WID_BROD_DEPOT_NW                            = to_underlying(::BuildRoadDepotWidgets::WID_BROD_DEPOT_NW),                            ///< Depot with NW entry.
	};

	/** Widgets of the BuildRoadElevatorWindow class. */
	enum BuildRoadElevatorWidgets {
		WID_BREL_CAPTION                             = to_underlying(::BuildRoadElevatorWidgets::WID_BREL_CAPTION),                             ///< Caption of the window.
		WID_BREL_DOOR_NE                             = to_underlying(::BuildRoadElevatorWidgets::WID_BREL_DOOR_NE),                             ///< Door facing NE.
		WID_BREL_DOOR_SE                             = to_underlying(::BuildRoadElevatorWidgets::WID_BREL_DOOR_SE),                             ///< Door facing SE.
		WID_BREL_DOOR_SW                             = to_underlying(::BuildRoadElevatorWidgets::WID_BREL_DOOR_SW),                             ///< Door facing SW.
		WID_BREL_DOOR_NW                             = to_underlying(::BuildRoadElevatorWidgets::WID_BREL_DOOR_NW),                             ///< Door facing NW.
		WID_BREL_UPPER_SEL                           = to_underlying(::BuildRoadElevatorWidgets::WID_BREL_UPPER_SEL),                           ///< Selection that shows the upper-end row only for elevators.
		WID_BREL_UPPER_SURFACE                       = to_underlying(::BuildRoadElevatorWidgets::WID_BREL_UPPER_SURFACE),                       ///< Elevator from Deep Underground goes up to Surface.
		WID_BREL_UPPER_UNDERGROUND                   = to_underlying(::BuildRoadElevatorWidgets::WID_BREL_UPPER_UNDERGROUND),                   ///< Elevator from Deep Underground goes up to Underground.
	};

	/** Widgets of the BuildRoadStationWindow class. */
	enum BuildRoadStationWidgets {
		/* Name starts with BRO instead of BR, because of collision with BuildRailStationWidgets */
		WID_BROS_CAPTION                             = to_underlying(::BuildRoadStationWidgets::WID_BROS_CAPTION),                             ///< Caption of the window.
		WID_BROS_STATION_NE                          = to_underlying(::BuildRoadStationWidgets::WID_BROS_STATION_NE),                          ///< Terminal station with NE entry.
		WID_BROS_STATION_SE                          = to_underlying(::BuildRoadStationWidgets::WID_BROS_STATION_SE),                          ///< Terminal station with SE entry.
		WID_BROS_STATION_SW                          = to_underlying(::BuildRoadStationWidgets::WID_BROS_STATION_SW),                          ///< Terminal station with SW entry.
		WID_BROS_STATION_NW                          = to_underlying(::BuildRoadStationWidgets::WID_BROS_STATION_NW),                          ///< Terminal station with NW entry.
		WID_BROS_STATION_X                           = to_underlying(::BuildRoadStationWidgets::WID_BROS_STATION_X),                           ///< Drive-through station in x-direction.
		WID_BROS_STATION_Y                           = to_underlying(::BuildRoadStationWidgets::WID_BROS_STATION_Y),                           ///< Drive-through station in y-direction.
		WID_BROS_LT_OFF                              = to_underlying(::BuildRoadStationWidgets::WID_BROS_LT_OFF),                              ///< Turn off area highlight.
		WID_BROS_LT_ON                               = to_underlying(::BuildRoadStationWidgets::WID_BROS_LT_ON),                               ///< Turn on area highlight.
		WID_BROS_ACCEPTANCE                          = to_underlying(::BuildRoadStationWidgets::WID_BROS_ACCEPTANCE),                          ///< Station acceptance info.
		WID_BROS_AVAILABLE_ORIENTATIONS              = to_underlying(::BuildRoadStationWidgets::WID_BROS_AVAILABLE_ORIENTATIONS),              ///< Selection for selecting 6 or 2 orientations.
	};

	/* automatically generated from ../../widgets/screenshot_widget.h */
	/** Widgets of the ScreenshotWindow class. */
	enum ScreenshotWindowWidgets {
		WID_SC_TAKE                                  = to_underlying(::ScreenshotWindowWidgets::WID_SC_TAKE),                                  ///< Button for taking a normal screenshot
		WID_SC_TAKE_ZOOMIN                           = to_underlying(::ScreenshotWindowWidgets::WID_SC_TAKE_ZOOMIN),                           ///< Button for taking a zoomed in screenshot
		WID_SC_TAKE_DEFAULTZOOM                      = to_underlying(::ScreenshotWindowWidgets::WID_SC_TAKE_DEFAULTZOOM),                      ///< Button for taking a screenshot at normal zoom
		WID_SC_TAKE_WORLD                            = to_underlying(::ScreenshotWindowWidgets::WID_SC_TAKE_WORLD),                            ///< Button for taking a screenshot of the whole world
		WID_SC_TAKE_HEIGHTMAP                        = to_underlying(::ScreenshotWindowWidgets::WID_SC_TAKE_HEIGHTMAP),                        ///< Button for taking a heightmap "screenshot"
		WID_SC_TAKE_MINIMAP                          = to_underlying(::ScreenshotWindowWidgets::WID_SC_TAKE_MINIMAP),                          ///< Button for taking a minimap screenshot
	};

	/* automatically generated from ../../widgets/script_widget.h */
	/** Widgets of the ScriptListWindow class. */
	enum ScriptListWidgets {
		WID_SCRL_CAPTION                             = to_underlying(::ScriptListWidgets::WID_SCRL_CAPTION),                             ///< Caption of the window.
		WID_SCRL_LIST                                = to_underlying(::ScriptListWidgets::WID_SCRL_LIST),                                ///< The matrix with all available Scripts.
		WID_SCRL_SCROLLBAR                           = to_underlying(::ScriptListWidgets::WID_SCRL_SCROLLBAR),                           ///< Scrollbar next to the Script list.
		WID_SCRL_INFO_BG                             = to_underlying(::ScriptListWidgets::WID_SCRL_INFO_BG),                             ///< Panel to draw some Script information on.
		WID_SCRL_ACCEPT                              = to_underlying(::ScriptListWidgets::WID_SCRL_ACCEPT),                              ///< Accept button.
	};

	/** Widgets of the ScriptSettingsWindow class. */
	enum ScriptSettingsWidgets {
		WID_SCRS_CAPTION                             = to_underlying(::ScriptSettingsWidgets::WID_SCRS_CAPTION),                             ///< Caption of the window.
		WID_SCRS_BACKGROUND                          = to_underlying(::ScriptSettingsWidgets::WID_SCRS_BACKGROUND),                          ///< Panel to draw the settings on.
		WID_SCRS_SCROLLBAR                           = to_underlying(::ScriptSettingsWidgets::WID_SCRS_SCROLLBAR),                           ///< Scrollbar to scroll through all settings.
		WID_SCRS_RESET                               = to_underlying(::ScriptSettingsWidgets::WID_SCRS_RESET),                               ///< Reset button.

		WID_SCRS_SETTING_DROPDOWN                    = to_underlying(::ScriptSettingsWidgets::WID_SCRS_SETTING_DROPDOWN),                    ///< Dynamically created dropdown for changing setting value.
	};

	/** Widgets of the ScriptDebugWindow class. */
	enum ScriptDebugWidgets {
		WID_SCRD_VIEW                                = to_underlying(::ScriptDebugWidgets::WID_SCRD_VIEW),                                ///< The row of company buttons.
		WID_SCRD_NAME_TEXT                           = to_underlying(::ScriptDebugWidgets::WID_SCRD_NAME_TEXT),                           ///< Name of the current selected.
		WID_SCRD_SETTINGS                            = to_underlying(::ScriptDebugWidgets::WID_SCRD_SETTINGS),                            ///< Settings button.
		WID_SCRD_SCRIPT_GAME                         = to_underlying(::ScriptDebugWidgets::WID_SCRD_SCRIPT_GAME),                         ///< Game Script button.
		WID_SCRD_RELOAD_TOGGLE                       = to_underlying(::ScriptDebugWidgets::WID_SCRD_RELOAD_TOGGLE),                       ///< Reload button.
		WID_SCRD_LOG_PANEL                           = to_underlying(::ScriptDebugWidgets::WID_SCRD_LOG_PANEL),                           ///< Panel where the log is in.
		WID_SCRD_VSCROLLBAR                          = to_underlying(::ScriptDebugWidgets::WID_SCRD_VSCROLLBAR),                          ///< Vertical scrollbar of the log panel.
		WID_SCRD_COMPANY_BUTTON_START                = to_underlying(::ScriptDebugWidgets::WID_SCRD_COMPANY_BUTTON_START),                ///< Buttons in the VIEW.
		WID_SCRD_COMPANY_BUTTON_END                  = to_underlying(::ScriptDebugWidgets::WID_SCRD_COMPANY_BUTTON_END),                  ///< Last possible button in the VIEW.
		WID_SCRD_BREAK_STRING_WIDGETS                = to_underlying(::ScriptDebugWidgets::WID_SCRD_BREAK_STRING_WIDGETS),                ///< The panel to handle the breaking on string.
		WID_SCRD_BREAK_STR_ON_OFF_BTN                = to_underlying(::ScriptDebugWidgets::WID_SCRD_BREAK_STR_ON_OFF_BTN),                ///< Enable breaking on string.
		WID_SCRD_BREAK_STR_EDIT_BOX                  = to_underlying(::ScriptDebugWidgets::WID_SCRD_BREAK_STR_EDIT_BOX),                  ///< Edit box for the string to break on.
		WID_SCRD_MATCH_CASE_BTN                      = to_underlying(::ScriptDebugWidgets::WID_SCRD_MATCH_CASE_BTN),                      ///< Checkbox to use match caching or not.
		WID_SCRD_CONTINUE_BTN                        = to_underlying(::ScriptDebugWidgets::WID_SCRD_CONTINUE_BTN),                        ///< Continue button.
		WID_SCRD_HSCROLLBAR                          = to_underlying(::ScriptDebugWidgets::WID_SCRD_HSCROLLBAR),                          ///< Horizontal scrollbar of the log panel.
	};

	/* automatically generated from ../../widgets/settings_widget.h */
	/** Widgets of the GameOptionsWindow class. */
	enum GameOptionsWidgets {
		WID_GO_TAB_GENERAL                           = to_underlying(::GameOptionsWidgets::WID_GO_TAB_GENERAL),                           ///< General tab.
		WID_GO_TAB_GRAPHICS                          = to_underlying(::GameOptionsWidgets::WID_GO_TAB_GRAPHICS),                          ///< Graphics tab.
		WID_GO_TAB_SOUND                             = to_underlying(::GameOptionsWidgets::WID_GO_TAB_SOUND),                             ///< Sound tab.
		WID_GO_TAB_SOCIAL                            = to_underlying(::GameOptionsWidgets::WID_GO_TAB_SOCIAL),                            ///< Social tab.
		WID_GO_TAB_ADVANCED                          = to_underlying(::GameOptionsWidgets::WID_GO_TAB_ADVANCED),                          ///< Advanced tab.
		WID_GO_TAB_SELECTION                         = to_underlying(::GameOptionsWidgets::WID_GO_TAB_SELECTION),                         ///< Background of the tab selection.
		WID_GO_CURRENCY_DROPDOWN                     = to_underlying(::GameOptionsWidgets::WID_GO_CURRENCY_DROPDOWN),                     ///< Currency dropdown.
		WID_GO_DISTANCE_DROPDOWN                     = to_underlying(::GameOptionsWidgets::WID_GO_DISTANCE_DROPDOWN),                     ///< Measuring unit dropdown.
		WID_GO_AUTOSAVE_DROPDOWN                     = to_underlying(::GameOptionsWidgets::WID_GO_AUTOSAVE_DROPDOWN),                     ///< Dropdown to say how often to autosave.
		WID_GO_LANG_DROPDOWN                         = to_underlying(::GameOptionsWidgets::WID_GO_LANG_DROPDOWN),                         ///< Language dropdown.
		WID_GO_RESOLUTION_DROPDOWN                   = to_underlying(::GameOptionsWidgets::WID_GO_RESOLUTION_DROPDOWN),                   ///< Dropdown for the resolution.
		WID_GO_FULLSCREEN_BUTTON                     = to_underlying(::GameOptionsWidgets::WID_GO_FULLSCREEN_BUTTON),                     ///< Toggle fullscreen.
		WID_GO_FULLSCREEN_TEXT                       = to_underlying(::GameOptionsWidgets::WID_GO_FULLSCREEN_TEXT),                       ///< Text for toggle fullscreen.
		WID_GO_GUI_SCALE                             = to_underlying(::GameOptionsWidgets::WID_GO_GUI_SCALE),                             ///< GUI Scale slider.
		WID_GO_GUI_SCALE_AUTO                        = to_underlying(::GameOptionsWidgets::WID_GO_GUI_SCALE_AUTO),                        ///< Autodetect GUI scale button.
		WID_GO_GUI_SCALE_AUTO_TEXT                   = to_underlying(::GameOptionsWidgets::WID_GO_GUI_SCALE_AUTO_TEXT),                   ///< Text for Autodetect GUI scale.
		WID_GO_GUI_SCALE_BEVEL_BUTTON                = to_underlying(::GameOptionsWidgets::WID_GO_GUI_SCALE_BEVEL_BUTTON),                ///< Toggle for chunky bevels.
		WID_GO_GUI_SCALE_BEVEL_TEXT                  = to_underlying(::GameOptionsWidgets::WID_GO_GUI_SCALE_BEVEL_TEXT),                  ///< Text for chunky bevels.
		WID_GO_GUI_FONT_SPRITE                       = to_underlying(::GameOptionsWidgets::WID_GO_GUI_FONT_SPRITE),                       ///< Toggle whether to prefer the sprite font over TTF fonts.
		WID_GO_GUI_FONT_SPRITE_TEXT                  = to_underlying(::GameOptionsWidgets::WID_GO_GUI_FONT_SPRITE_TEXT),                  ///< Text for sprite font toggle.
		WID_GO_GUI_FONT_AA                           = to_underlying(::GameOptionsWidgets::WID_GO_GUI_FONT_AA),                           ///< Toggle whether to anti-alias fonts.
		WID_GO_GUI_FONT_AA_TEXT                      = to_underlying(::GameOptionsWidgets::WID_GO_GUI_FONT_AA_TEXT),                      ///< Text for anti-alias toggle.
		WID_GO_BASE_GRF_DROPDOWN                     = to_underlying(::GameOptionsWidgets::WID_GO_BASE_GRF_DROPDOWN),                     ///< Use to select a base GRF.
		WID_GO_BASE_GRF_PARAMETERS                   = to_underlying(::GameOptionsWidgets::WID_GO_BASE_GRF_PARAMETERS),                   ///< Base GRF parameters.
		WID_GO_BASE_GRF_CONTENT_DOWNLOAD             = to_underlying(::GameOptionsWidgets::WID_GO_BASE_GRF_CONTENT_DOWNLOAD),             ///< 'Get Content' button for base GRF.
		WID_GO_BASE_GRF_OPEN_URL                     = to_underlying(::GameOptionsWidgets::WID_GO_BASE_GRF_OPEN_URL),                     ///< Open base GRF URL.
		WID_GO_BASE_GRF_TEXTFILE                     = to_underlying(::GameOptionsWidgets::WID_GO_BASE_GRF_TEXTFILE),                     ///< Open base GRF readme, changelog (+1) or license (+2).
		WID_GO_BASE_GRF_DESCRIPTION                  = to_underlying(::GameOptionsWidgets::WID_GO_BASE_GRF_DESCRIPTION),                  ///< Description of selected base GRF.
		WID_GO_BASE_SFX_DROPDOWN                     = to_underlying(::GameOptionsWidgets::WID_GO_BASE_SFX_DROPDOWN),                     ///< Use to select a base SFX.
		WID_GO_BASE_SFX_CONTENT_DOWNLOAD             = to_underlying(::GameOptionsWidgets::WID_GO_BASE_SFX_CONTENT_DOWNLOAD),             ///< 'Get Content' button for base SFX.
		WID_GO_TEXT_SFX_VOLUME                       = to_underlying(::GameOptionsWidgets::WID_GO_TEXT_SFX_VOLUME),                       ///< Sound effects volume label.
		WID_GO_BASE_SFX_VOLUME                       = to_underlying(::GameOptionsWidgets::WID_GO_BASE_SFX_VOLUME),                       ///< Change sound effects volume.
		WID_GO_BASE_SFX_OPEN_URL                     = to_underlying(::GameOptionsWidgets::WID_GO_BASE_SFX_OPEN_URL),                     ///< Open base SFX URL.
		WID_GO_BASE_SFX_TEXTFILE                     = to_underlying(::GameOptionsWidgets::WID_GO_BASE_SFX_TEXTFILE),                     ///< Open base SFX readme, changelog (+1) or license (+2).
		WID_GO_BASE_SFX_DESCRIPTION                  = to_underlying(::GameOptionsWidgets::WID_GO_BASE_SFX_DESCRIPTION),                  ///< Description of selected base SFX.
		WID_GO_BASE_MUSIC_DROPDOWN                   = to_underlying(::GameOptionsWidgets::WID_GO_BASE_MUSIC_DROPDOWN),                   ///< Use to select a base music set.
		WID_GO_BASE_MUSIC_CONTENT_DOWNLOAD           = to_underlying(::GameOptionsWidgets::WID_GO_BASE_MUSIC_CONTENT_DOWNLOAD),           ///< 'Get Content' button for base music.
		WID_GO_TEXT_MUSIC_VOLUME                     = to_underlying(::GameOptionsWidgets::WID_GO_TEXT_MUSIC_VOLUME),                     ///< Music volume label.
		WID_GO_BASE_MUSIC_VOLUME                     = to_underlying(::GameOptionsWidgets::WID_GO_BASE_MUSIC_VOLUME),                     ///< Change music volume.
		WID_GO_BASE_MUSIC_JUKEBOX                    = to_underlying(::GameOptionsWidgets::WID_GO_BASE_MUSIC_JUKEBOX),                    ///< Open the jukebox.
		WID_GO_BASE_MUSIC_OPEN_URL                   = to_underlying(::GameOptionsWidgets::WID_GO_BASE_MUSIC_OPEN_URL),                   ///< Open base music URL.
		WID_GO_BASE_MUSIC_TEXTFILE                   = to_underlying(::GameOptionsWidgets::WID_GO_BASE_MUSIC_TEXTFILE),                   ///< Open base music readme, changelog (+1) or license (+2).
		WID_GO_BASE_MUSIC_DESCRIPTION                = to_underlying(::GameOptionsWidgets::WID_GO_BASE_MUSIC_DESCRIPTION),                ///< Description of selected base music set.
		WID_GO_VIDEO_ACCEL_BUTTON                    = to_underlying(::GameOptionsWidgets::WID_GO_VIDEO_ACCEL_BUTTON),                    ///< Toggle for video acceleration.
		WID_GO_VIDEO_ACCEL_TEXT                      = to_underlying(::GameOptionsWidgets::WID_GO_VIDEO_ACCEL_TEXT),                      ///< Text for video acceleration toggle.
		WID_GO_VIDEO_VSYNC_BUTTON                    = to_underlying(::GameOptionsWidgets::WID_GO_VIDEO_VSYNC_BUTTON),                    ///< Toggle for video vsync.
		WID_GO_VIDEO_VSYNC_TEXT                      = to_underlying(::GameOptionsWidgets::WID_GO_VIDEO_VSYNC_TEXT),                      ///< Text for video vsync toggle.
		WID_GO_REFRESH_RATE_DROPDOWN                 = to_underlying(::GameOptionsWidgets::WID_GO_REFRESH_RATE_DROPDOWN),                 ///< Dropdown for all available refresh rates.
		WID_GO_VIDEO_DRIVER_INFO                     = to_underlying(::GameOptionsWidgets::WID_GO_VIDEO_DRIVER_INFO),                     ///< Label showing details about the current video driver.
		WID_GO_SURVEY_SEL                            = to_underlying(::GameOptionsWidgets::WID_GO_SURVEY_SEL),                            ///< Selection to hide survey if no JSON library is compiled in.
		WID_GO_SURVEY_PARTICIPATE_BUTTON             = to_underlying(::GameOptionsWidgets::WID_GO_SURVEY_PARTICIPATE_BUTTON),             ///< Toggle for participating in the automated survey.
		WID_GO_SURVEY_PARTICIPATE_TEXT               = to_underlying(::GameOptionsWidgets::WID_GO_SURVEY_PARTICIPATE_TEXT),               ///< Text for automated survey toggle.
		WID_GO_SURVEY_LINK_BUTTON                    = to_underlying(::GameOptionsWidgets::WID_GO_SURVEY_LINK_BUTTON),                    ///< Button to open browser to go to the survey website.
		WID_GO_SURVEY_PREVIEW_BUTTON                 = to_underlying(::GameOptionsWidgets::WID_GO_SURVEY_PREVIEW_BUTTON),                 ///< Button to open a preview window with the survey results
		WID_GO_SOCIAL_PLUGINS                        = to_underlying(::GameOptionsWidgets::WID_GO_SOCIAL_PLUGINS),                        ///< Main widget handling the social plugins.
		WID_GO_SOCIAL_PLUGIN_TITLE                   = to_underlying(::GameOptionsWidgets::WID_GO_SOCIAL_PLUGIN_TITLE),                   ///< Title of the frame of the social plugin.
		WID_GO_SOCIAL_PLUGIN_PLATFORM                = to_underlying(::GameOptionsWidgets::WID_GO_SOCIAL_PLUGIN_PLATFORM),                ///< Platform of the social plugin.
		WID_GO_SOCIAL_PLUGIN_STATE                   = to_underlying(::GameOptionsWidgets::WID_GO_SOCIAL_PLUGIN_STATE),                   ///< State of the social plugin.

		WID_GO_FILTER                                = to_underlying(::GameOptionsWidgets::WID_GO_FILTER),                                ///< Text filter.
		WID_GO_OPTIONSPANEL                          = to_underlying(::GameOptionsWidgets::WID_GO_OPTIONSPANEL),                          ///< Panel widget containing the option lists.
		WID_GO_SCROLLBAR                             = to_underlying(::GameOptionsWidgets::WID_GO_SCROLLBAR),                             ///< Scrollbar.
		WID_GO_SETTING_PROPERTIES                    = to_underlying(::GameOptionsWidgets::WID_GO_SETTING_PROPERTIES),                    ///< Information area to display setting type and default value.
		WID_GO_HELP_TEXT                             = to_underlying(::GameOptionsWidgets::WID_GO_HELP_TEXT),                             ///< Information area to display help text of the selected option.
		WID_GO_HELP_TEXT_SCROLL                      = to_underlying(::GameOptionsWidgets::WID_GO_HELP_TEXT_SCROLL),                      ///< Scrollbar for setting description.
		WID_GO_EXPAND_ALL                            = to_underlying(::GameOptionsWidgets::WID_GO_EXPAND_ALL),                            ///< Expand all button.
		WID_GO_COLLAPSE_ALL                          = to_underlying(::GameOptionsWidgets::WID_GO_COLLAPSE_ALL),                          ///< Collapse all button.
		WID_GO_RESET_ALL                             = to_underlying(::GameOptionsWidgets::WID_GO_RESET_ALL),                             ///< Reset all button.
		WID_GO_RESTRICT_CATEGORY                     = to_underlying(::GameOptionsWidgets::WID_GO_RESTRICT_CATEGORY),                     ///< Label upfront to the category drop-down box to restrict the list of settings to show
		WID_GO_RESTRICT_TYPE                         = to_underlying(::GameOptionsWidgets::WID_GO_RESTRICT_TYPE),                         ///< Label upfront to the type drop-down box to restrict the list of settings to show
		WID_GO_RESTRICT_DROPDOWN                     = to_underlying(::GameOptionsWidgets::WID_GO_RESTRICT_DROPDOWN),                     ///< The drop down box to restrict the list of settings
		WID_GO_TYPE_DROPDOWN                         = to_underlying(::GameOptionsWidgets::WID_GO_TYPE_DROPDOWN),                         ///< The drop down box to choose client/game/company/all settings

		WID_GO_SETTING_DROPDOWN                      = to_underlying(::GameOptionsWidgets::WID_GO_SETTING_DROPDOWN),                      ///< Dynamically created dropdown for changing setting value.
	};

	/** Widgets of the CustomCurrencyWindow class. */
	enum CustomCurrencyWidgets {
		WID_CC_RATE_DOWN                             = to_underlying(::CustomCurrencyWidgets::WID_CC_RATE_DOWN),                             ///< Down button.
		WID_CC_RATE_UP                               = to_underlying(::CustomCurrencyWidgets::WID_CC_RATE_UP),                               ///< Up button.
		WID_CC_RATE                                  = to_underlying(::CustomCurrencyWidgets::WID_CC_RATE),                                  ///< Rate of currency.
		WID_CC_SEPARATOR_EDIT                        = to_underlying(::CustomCurrencyWidgets::WID_CC_SEPARATOR_EDIT),                        ///< Separator edit button.
		WID_CC_SEPARATOR                             = to_underlying(::CustomCurrencyWidgets::WID_CC_SEPARATOR),                             ///< Current separator.
		WID_CC_PREFIX_EDIT                           = to_underlying(::CustomCurrencyWidgets::WID_CC_PREFIX_EDIT),                           ///< Prefix edit button.
		WID_CC_PREFIX                                = to_underlying(::CustomCurrencyWidgets::WID_CC_PREFIX),                                ///< Current prefix.
		WID_CC_SUFFIX_EDIT                           = to_underlying(::CustomCurrencyWidgets::WID_CC_SUFFIX_EDIT),                           ///< Suffix edit button.
		WID_CC_SUFFIX                                = to_underlying(::CustomCurrencyWidgets::WID_CC_SUFFIX),                                ///< Current suffix.
		WID_CC_YEAR_DOWN                             = to_underlying(::CustomCurrencyWidgets::WID_CC_YEAR_DOWN),                             ///< Down button.
		WID_CC_YEAR_UP                               = to_underlying(::CustomCurrencyWidgets::WID_CC_YEAR_UP),                               ///< Up button.
		WID_CC_YEAR                                  = to_underlying(::CustomCurrencyWidgets::WID_CC_YEAR),                                  ///< Year of introduction.
		WID_CC_PREVIEW                               = to_underlying(::CustomCurrencyWidgets::WID_CC_PREVIEW),                               ///< Preview.
	};

	/* automatically generated from ../../widgets/sign_widget.h */
	/** Widgets of the SignListWindow class. */
	enum SignListWidgets {
		/* Name starts with SI instead of S, because of collision with SaveLoadWidgets */
		WID_SIL_CAPTION                              = to_underlying(::SignListWidgets::WID_SIL_CAPTION),                              ///< Caption of the window.
		WID_SIL_LIST                                 = to_underlying(::SignListWidgets::WID_SIL_LIST),                                 ///< List of signs.
		WID_SIL_SCROLLBAR                            = to_underlying(::SignListWidgets::WID_SIL_SCROLLBAR),                            ///< Scrollbar of list.
		WID_SIL_FILTER_TEXT                          = to_underlying(::SignListWidgets::WID_SIL_FILTER_TEXT),                          ///< Text box for typing a filter string.
		WID_SIL_FILTER_MATCH_CASE_BTN                = to_underlying(::SignListWidgets::WID_SIL_FILTER_MATCH_CASE_BTN),                ///< Button to toggle if case sensitive filtering should be used.
	};

	/** Widgets of the SignWindow class. */
	enum QueryEditSignWidgets {
		WID_QES_CAPTION                              = to_underlying(::QueryEditSignWidgets::WID_QES_CAPTION),                              ///< Caption of the window.
		WID_QES_LOCATION                             = to_underlying(::QueryEditSignWidgets::WID_QES_LOCATION),                             ///< Scroll to sign location.
		WID_QES_TEXT                                 = to_underlying(::QueryEditSignWidgets::WID_QES_TEXT),                                 ///< Text of the query.
		WID_QES_OK                                   = to_underlying(::QueryEditSignWidgets::WID_QES_OK),                                   ///< OK button.
		WID_QES_CANCEL                               = to_underlying(::QueryEditSignWidgets::WID_QES_CANCEL),                               ///< Cancel button.
		WID_QES_DELETE                               = to_underlying(::QueryEditSignWidgets::WID_QES_DELETE),                               ///< Delete button.
		WID_QES_COLOUR_PANE                          = to_underlying(::QueryEditSignWidgets::WID_QES_COLOUR_PANE),                          ///< Pane to show/hide the color dropdown.
		WID_QES_COLOUR                               = to_underlying(::QueryEditSignWidgets::WID_QES_COLOUR),                               ///< Colour selection dropdown.
		WID_QES_MOVE                                 = to_underlying(::QueryEditSignWidgets::WID_QES_MOVE),                                 ///< Move Sign button.
		WID_QES_PREVIOUS                             = to_underlying(::QueryEditSignWidgets::WID_QES_PREVIOUS),                             ///< Previous button.
		WID_QES_NEXT                                 = to_underlying(::QueryEditSignWidgets::WID_QES_NEXT),                                 ///< Next button.
	};

	/* automatically generated from ../../widgets/smallmap_widget.h */
	/** Widgets of the SmallMapWindow class. */
	enum SmallMapWidgets {
		WID_SM_CAPTION                               = to_underlying(::SmallMapWidgets::WID_SM_CAPTION),                               ///< Caption of the window.
		WID_SM_MAP_BORDER                            = to_underlying(::SmallMapWidgets::WID_SM_MAP_BORDER),                            ///< Border around the smallmap.
		WID_SM_MAP                                   = to_underlying(::SmallMapWidgets::WID_SM_MAP),                                   ///< Panel containing the smallmap.
		WID_SM_LEGEND                                = to_underlying(::SmallMapWidgets::WID_SM_LEGEND),                                ///< Bottom panel to display smallmap legends.
		WID_SM_BLANK                                 = to_underlying(::SmallMapWidgets::WID_SM_BLANK),                                 ///< Empty button as placeholder.
		WID_SM_ZOOM_IN                               = to_underlying(::SmallMapWidgets::WID_SM_ZOOM_IN),                               ///< Button to zoom in one step.
		WID_SM_ZOOM_OUT                              = to_underlying(::SmallMapWidgets::WID_SM_ZOOM_OUT),                              ///< Button to zoom out one step.
		WID_SM_CONTOUR                               = to_underlying(::SmallMapWidgets::WID_SM_CONTOUR),                               ///< Button to select the contour view (height map).
		WID_SM_VEHICLES                              = to_underlying(::SmallMapWidgets::WID_SM_VEHICLES),                              ///< Button to select the vehicles view.
		WID_SM_INDUSTRIES                            = to_underlying(::SmallMapWidgets::WID_SM_INDUSTRIES),                            ///< Button to select the industries view.
		WID_SM_LINKSTATS                             = to_underlying(::SmallMapWidgets::WID_SM_LINKSTATS),                             ///< Button to select the link stats view.
		WID_SM_ROUTES                                = to_underlying(::SmallMapWidgets::WID_SM_ROUTES),                                ///< Button to select the routes view.
		WID_SM_VEGETATION                            = to_underlying(::SmallMapWidgets::WID_SM_VEGETATION),                            ///< Button to select the vegetation view.
		WID_SM_OWNERS                                = to_underlying(::SmallMapWidgets::WID_SM_OWNERS),                                ///< Button to select the owners view.
		WID_SM_CENTERMAP                             = to_underlying(::SmallMapWidgets::WID_SM_CENTERMAP),                             ///< Button to move smallmap center to main window center.
		WID_SM_MAP_LAYER                             = to_underlying(::SmallMapWidgets::WID_SM_MAP_LAYER),                             ///< Cycle the layer this smallmap shows.
		WID_SM_TOGGLETOWNNAME                        = to_underlying(::SmallMapWidgets::WID_SM_TOGGLETOWNNAME),                        ///< Toggle button to display town names.
		WID_SM_SELECT_BUTTONS                        = to_underlying(::SmallMapWidgets::WID_SM_SELECT_BUTTONS),                        ///< Selection widget for the buttons present in some smallmap modes.
		WID_SM_ENABLE_ALL                            = to_underlying(::SmallMapWidgets::WID_SM_ENABLE_ALL),                            ///< Button to enable display of all legend entries.
		WID_SM_DISABLE_ALL                           = to_underlying(::SmallMapWidgets::WID_SM_DISABLE_ALL),                           ///< Button to disable display of all legend entries.
		WID_SM_SHOW_HEIGHT                           = to_underlying(::SmallMapWidgets::WID_SM_SHOW_HEIGHT),                           ///< Show heightmap toggle button.
		WID_SM_SHOW_IND_NAMES                        = to_underlying(::SmallMapWidgets::WID_SM_SHOW_IND_NAMES),                        ///< Show industry names toggle button.
		WID_SM_SHOW_IND_NAMES_SEL                    = to_underlying(::SmallMapWidgets::WID_SM_SHOW_IND_NAMES_SEL),                    ///< Container for the 'show industry names' button, which can be hidden.
	};

	/* automatically generated from ../../widgets/station_widget.h */
	/** Widgets of the StationViewWindow class. */
	enum StationViewWidgets {
		WID_SV_CAPTION                               = to_underlying(::StationViewWidgets::WID_SV_CAPTION),                               ///< Caption of the window.
		WID_SV_GROUP                                 = to_underlying(::StationViewWidgets::WID_SV_GROUP),                                 ///< label for "group by"
		WID_SV_GROUP_BY                              = to_underlying(::StationViewWidgets::WID_SV_GROUP_BY),                              ///< 'Group by' button
		WID_SV_SORT_ORDER                            = to_underlying(::StationViewWidgets::WID_SV_SORT_ORDER),                            ///< 'Sort order' button
		WID_SV_SORT_BY                               = to_underlying(::StationViewWidgets::WID_SV_SORT_BY),                               ///< 'Sort by' button
		WID_SV_WAITING                               = to_underlying(::StationViewWidgets::WID_SV_WAITING),                               ///< List of waiting cargo.
		WID_SV_SCROLLBAR                             = to_underlying(::StationViewWidgets::WID_SV_SCROLLBAR),                             ///< Scrollbar.
		WID_SV_ACCEPT_RATING_LIST                    = to_underlying(::StationViewWidgets::WID_SV_ACCEPT_RATING_LIST),                    ///< List of accepted cargoes / rating of cargoes.
		WID_SV_LOCATION                              = to_underlying(::StationViewWidgets::WID_SV_LOCATION),                              ///< 'Location' button.
		WID_SV_ACCEPTS_RATINGS                       = to_underlying(::StationViewWidgets::WID_SV_ACCEPTS_RATINGS),                       ///< 'Accepts' / 'Ratings' button.
		WID_SV_RENAME                                = to_underlying(::StationViewWidgets::WID_SV_RENAME),                                ///< 'Rename' button.
		WID_SV_CLOSE_AIRPORT                         = to_underlying(::StationViewWidgets::WID_SV_CLOSE_AIRPORT),                         ///< 'Close airport' button.
		WID_SV_CLOSE_AIRPORT_SEL                     = to_underlying(::StationViewWidgets::WID_SV_CLOSE_AIRPORT_SEL),                     ///< Container for 'close airport' button, which can be hidden.
		WID_SV_TRAINS                                = to_underlying(::StationViewWidgets::WID_SV_TRAINS),                                ///< List of scheduled trains button.
		WID_SV_ROADVEHS                              = to_underlying(::StationViewWidgets::WID_SV_ROADVEHS),                              ///< List of scheduled road vehs button.
		WID_SV_SHIPS                                 = to_underlying(::StationViewWidgets::WID_SV_SHIPS),                                 ///< List of scheduled ships button.
		WID_SV_PLANES                                = to_underlying(::StationViewWidgets::WID_SV_PLANES),                                ///< List of scheduled planes button.
		WID_SV_CATCHMENT                             = to_underlying(::StationViewWidgets::WID_SV_CATCHMENT),                             ///< Toggle catchment area highlight.
		WID_SV_LAUNCH_HUB                            = to_underlying(::StationViewWidgets::WID_SV_LAUNCH_HUB),                            ///< Explicit Surface freight hub selector.
		WID_SV_LAUNCH_COLLECTION                     = to_underlying(::StationViewWidgets::WID_SV_LAUNCH_COLLECTION),                     ///< Separate collection-circle overlay.
		WID_SV_LAUNCH_HUB_SEL                        = to_underlying(::StationViewWidgets::WID_SV_LAUNCH_HUB_SEL),                        ///< Hide hub controls outside supported Surface stations.
		WID_SV_LAUNCH_ASSEMBLY                       = to_underlying(::StationViewWidgets::WID_SV_LAUNCH_ASSEMBLY),                       ///< Open the owned native launch workshop.
	};

	/** Widgets of the CompanyStationsWindow class. */
	enum StationListWidgets {
		/* Name starts with ST instead of S, because of collision with SaveLoadWidgets */
		WID_STL_CAPTION                              = to_underlying(::StationListWidgets::WID_STL_CAPTION),                              ///< Caption of the window.
		WID_STL_LIST                                 = to_underlying(::StationListWidgets::WID_STL_LIST),                                 ///< The main panel, list of stations.
		WID_STL_SCROLLBAR                            = to_underlying(::StationListWidgets::WID_STL_SCROLLBAR),                            ///< Scrollbar next to the main panel.

		/* Vehicletypes need to be in order of StationFacility due to bit magic */
		WID_STL_TRAIN                                = to_underlying(::StationListWidgets::WID_STL_TRAIN),                                ///< 'TRAIN' button - list only facilities where is a railroad station.
		WID_STL_TRUCK                                = to_underlying(::StationListWidgets::WID_STL_TRUCK),                                ///< 'TRUCK' button - list only facilities where is a truck stop.
		WID_STL_BUS                                  = to_underlying(::StationListWidgets::WID_STL_BUS),                                  ///< 'BUS' button - list only facilities where is a bus stop.
		WID_STL_AIRPLANE                             = to_underlying(::StationListWidgets::WID_STL_AIRPLANE),                             ///< 'AIRPLANE' button - list only facilities where is an airport.
		WID_STL_SHIP                                 = to_underlying(::StationListWidgets::WID_STL_SHIP),                                 ///< 'SHIP' button - list only facilities where is a dock.
		WID_STL_FACILALL                             = to_underlying(::StationListWidgets::WID_STL_FACILALL),                             ///< 'ALL' button - list all facilities.

		WID_STL_SORTBY                               = to_underlying(::StationListWidgets::WID_STL_SORTBY),                               ///< 'Sort by' button - reverse sort direction.
		WID_STL_SORTDROPBTN                          = to_underlying(::StationListWidgets::WID_STL_SORTDROPBTN),                          ///< Dropdown button.
		WID_STL_FILTER                               = to_underlying(::StationListWidgets::WID_STL_FILTER),                               ///< Filter of name.

		WID_STL_CARGODROPDOWN                        = to_underlying(::StationListWidgets::WID_STL_CARGODROPDOWN),                        ///< Cargo type dropdown list.
	};

	/** Widgets of the SelectStationWindow class. */
	enum JoinStationWidgets {
		WID_JS_CAPTION                               = to_underlying(::JoinStationWidgets::WID_JS_CAPTION),                               ///< Caption of the window.
		WID_JS_PANEL                                 = to_underlying(::JoinStationWidgets::WID_JS_PANEL),                                 ///< Main panel.
		WID_JS_SCROLLBAR                             = to_underlying(::JoinStationWidgets::WID_JS_SCROLLBAR),                             ///< Scrollbar of the panel.
	};

	/* automatically generated from ../../widgets/statusbar_widget.h */
	/** Widgets of the StatusBarWindow class. */
	enum StatusbarWidgets {
		WID_S_LEFT                                   = to_underlying(::StatusbarWidgets::WID_S_LEFT),                                   ///< Left part of the statusbar; date is shown there.
		WID_S_MIDDLE                                 = to_underlying(::StatusbarWidgets::WID_S_MIDDLE),                                 ///< Middle part; current news or company name or *** SAVING *** or *** PAUSED ***.
		WID_S_RIGHT                                  = to_underlying(::StatusbarWidgets::WID_S_RIGHT),                                  ///< Right part; bank balance.
	};

	/* automatically generated from ../../widgets/story_widget.h */
	/** Widgets of the GoalListWindow class. */
	enum StoryBookWidgets {
		WID_SB_CAPTION                               = to_underlying(::StoryBookWidgets::WID_SB_CAPTION),                               ///< Caption of the window.
		WID_SB_SEL_PAGE                              = to_underlying(::StoryBookWidgets::WID_SB_SEL_PAGE),                              ///< Page selector.
		WID_SB_PAGE_PANEL                            = to_underlying(::StoryBookWidgets::WID_SB_PAGE_PANEL),                            ///< Page body.
		WID_SB_SCROLLBAR                             = to_underlying(::StoryBookWidgets::WID_SB_SCROLLBAR),                             ///< Scrollbar of the goal list.
		WID_SB_PREV_PAGE                             = to_underlying(::StoryBookWidgets::WID_SB_PREV_PAGE),                             ///< Prev button.
		WID_SB_NEXT_PAGE                             = to_underlying(::StoryBookWidgets::WID_SB_NEXT_PAGE),                             ///< Next button.
	};

	/* automatically generated from ../../widgets/subsidy_widget.h */
	/** Widgets of the SubsidyListWindow class. */
	enum SubsidyListWidgets {
		/* Name starts with SU instead of S, because of collision with SaveLoadWidgets. */
		WID_SUL_PANEL                                = to_underlying(::SubsidyListWidgets::WID_SUL_PANEL),                                ///< Main panel of window.
		WID_SUL_SCROLLBAR                            = to_underlying(::SubsidyListWidgets::WID_SUL_SCROLLBAR),                            ///< Scrollbar of panel.
	};

	/* automatically generated from ../../widgets/tbm_widget.h */
	/** Actual project selection and native vehicle/portal navigation; these actions allocate no work. */
	enum TBMProjectsWidgets {
		WID_TP_CAPTION                               = to_underlying(::TBMProjectsWidgets::WID_TP_CAPTION),                               ///< Explicit work-layer context.
		WID_TP_LIST                                  = to_underlying(::TBMProjectsWidgets::WID_TP_LIST),                                  ///< Scrollable actual owned project rows.
		WID_TP_SCROLL                                = to_underlying(::TBMProjectsWidgets::WID_TP_SCROLL),                                ///< Native scrollbar; project count does not enlarge the screen.
		WID_TP_SELECT                                = to_underlying(::TBMProjectsWidgets::WID_TP_SELECT),                                ///< Inspect the selected project's current geological footprint.
		WID_TP_CENTRE                                = to_underlying(::TBMProjectsWidgets::WID_TP_CENTRE),                                ///< Centre on the actual work face or original anchor.
		WID_TP_VEHICLE                               = to_underlying(::TBMProjectsWidgets::WID_TP_VEHICLE),                               ///< Open the same real native cutter vehicle.
		WID_TP_CONTROLS                              = to_underlying(::TBMProjectsWidgets::WID_TP_CONTROLS),                              ///< Open the existing real commissioning/retirement controls.
		WID_TP_EXTEND                                = to_underlying(::TBMProjectsWidgets::WID_TP_EXTEND),                                ///< Pause through the ordinary command, then plan additional work for this same cutter.
		WID_TP_PARK_DESTINATION                      = to_underlying(::TBMProjectsWidgets::WID_TP_PARK_DESTINATION),                      ///< Actual current original hangar/exchange location, independent of camera layer.
		WID_TP_PARK                                  = to_underlying(::TBMProjectsWidgets::WID_TP_PARK),                                  ///< Withdraw, settle the load and park the whole native machine without cancelling work.
		WID_TP_RESUME                                = to_underlying(::TBMProjectsWidgets::WID_TP_RESUME),                                ///< Resume a paused or physically parked project through the controller command.
		WID_TP_RELEASE                               = to_underlying(::TBMProjectsWidgets::WID_TP_RELEASE),                               ///< Finish/cancel and release only after complete physical withdrawal and real haulage settlement.
		WID_TP_RELEASE_REASON                        = to_underlying(::TBMProjectsWidgets::WID_TP_RELEASE_REASON),                        ///< Actual release prerequisite and blocking train, derived from the owning preflight.
		WID_TP_HANGAR                                = to_underlying(::TBMProjectsWidgets::WID_TP_HANGAR),                                ///< Choose another real compatible destination through ordinary native depot orders.
	};

	/* automatically generated from ../../widgets/terraform_widget.h */
	/** Widgets of the TerraformToolbarWindow class. */
	enum TerraformToolbarWidgets {
		WID_TT_SHOW_PLACE_OBJECT                     = to_underlying(::TerraformToolbarWidgets::WID_TT_SHOW_PLACE_OBJECT),                     ///< Should the place object button be shown?
		WID_TT_BUTTONS_START                         = to_underlying(::TerraformToolbarWidgets::WID_TT_BUTTONS_START),                         ///< Start of pushable buttons.
		WID_TT_LOWER_LAND                            = to_underlying(::TerraformToolbarWidgets::WID_TT_LOWER_LAND),                            ///< Lower land button.
		WID_TT_RAISE_LAND                            = to_underlying(::TerraformToolbarWidgets::WID_TT_RAISE_LAND),                            ///< Raise land button.
		WID_TT_LEVEL_LAND                            = to_underlying(::TerraformToolbarWidgets::WID_TT_LEVEL_LAND),                            ///< Level land button.
		WID_TT_DEMOLISH                              = to_underlying(::TerraformToolbarWidgets::WID_TT_DEMOLISH),                              ///< Demolish aka dynamite button.
		WID_TT_BUY_LAND                              = to_underlying(::TerraformToolbarWidgets::WID_TT_BUY_LAND),                              ///< Buy land button.
		WID_TT_PLANT_TREES                           = to_underlying(::TerraformToolbarWidgets::WID_TT_PLANT_TREES),                           ///< Plant trees button (note: opens separate window, no place-push-button).
		WID_TT_PLACE_SIGN                            = to_underlying(::TerraformToolbarWidgets::WID_TT_PLACE_SIGN),                            ///< Place sign button.
		WID_TT_PLACE_OBJECT                          = to_underlying(::TerraformToolbarWidgets::WID_TT_PLACE_OBJECT),                          ///< Place object button.
	};

	/** Widgets of the ScenarioEditorLandscapeGenerationWindow class. */
	enum EditorTerraformToolbarWidgets {
		WID_ETT_SHOW_PLACE_DESERT                    = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_SHOW_PLACE_DESERT),                    ///< Should the place desert button be shown?
		WID_ETT_START                                = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_START),                                ///< Used for iterations.
		WID_ETT_DOTS                                 = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_DOTS),                                 ///< Invisible widget for rendering the terraform size on.
		WID_ETT_BUTTONS_START                        = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_BUTTONS_START),                        ///< Start of pushable buttons.
		WID_ETT_DEMOLISH                             = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_DEMOLISH),                             ///< Demolish aka dynamite button.
		WID_ETT_LOWER_LAND                           = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_LOWER_LAND),                           ///< Lower land button.
		WID_ETT_RAISE_LAND                           = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_RAISE_LAND),                           ///< Raise land button.
		WID_ETT_LEVEL_LAND                           = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_LEVEL_LAND),                           ///< Level land button.
		WID_ETT_PLACE_ROCKS                          = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_PLACE_ROCKS),                          ///< Place rocks button.
		WID_ETT_PLACE_ROUGH                          = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_PLACE_ROUGH),                          ///< Place rough land button.
		WID_ETT_PLACE_DESERT                         = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_PLACE_DESERT),                         ///< Place desert button (in tropical climate).
		WID_ETT_PLACE_OBJECT                         = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_PLACE_OBJECT),                         ///< Place transmitter button.
		WID_ETT_BUTTONS_END                          = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_BUTTONS_END),                          ///< End of pushable buttons.
		WID_ETT_INCREASE_SIZE                        = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_INCREASE_SIZE),                        ///< Upwards arrow button to increase terraforming size.
		WID_ETT_DECREASE_SIZE                        = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_DECREASE_SIZE),                        ///< Downwards arrow button to decrease terraforming size.
		WID_ETT_NEW_SCENARIO                         = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_NEW_SCENARIO),                         ///< Button for generating a new scenario.
		WID_ETT_RESET_LANDSCAPE                      = to_underlying(::EditorTerraformToolbarWidgets::WID_ETT_RESET_LANDSCAPE),                      ///< Button for removing all company-owned property.
	};

	/* automatically generated from ../../widgets/timetable_widget.h */
	/** Widgets of the TimetableWindow class. */
	enum VehicleTimetableWidgets {
		WID_VT_CAPTION                               = to_underlying(::VehicleTimetableWidgets::WID_VT_CAPTION),                               ///< Caption of the window.
		WID_VT_ORDER_VIEW                            = to_underlying(::VehicleTimetableWidgets::WID_VT_ORDER_VIEW),                            ///< Order view.
		WID_VT_TIMETABLE_PANEL                       = to_underlying(::VehicleTimetableWidgets::WID_VT_TIMETABLE_PANEL),                       ///< Timetable panel.
		WID_VT_ARRIVAL_DEPARTURE_PANEL               = to_underlying(::VehicleTimetableWidgets::WID_VT_ARRIVAL_DEPARTURE_PANEL),               ///< Panel with the expected/scheduled arrivals.
		WID_VT_SCROLLBAR                             = to_underlying(::VehicleTimetableWidgets::WID_VT_SCROLLBAR),                             ///< Scrollbar for the panel.
		WID_VT_SUMMARY_PANEL                         = to_underlying(::VehicleTimetableWidgets::WID_VT_SUMMARY_PANEL),                         ///< Summary panel.
		WID_VT_START_DATE                            = to_underlying(::VehicleTimetableWidgets::WID_VT_START_DATE),                            ///< Start date button.
		WID_VT_CHANGE_TIME                           = to_underlying(::VehicleTimetableWidgets::WID_VT_CHANGE_TIME),                           ///< Change time button.
		WID_VT_CLEAR_TIME                            = to_underlying(::VehicleTimetableWidgets::WID_VT_CLEAR_TIME),                            ///< Clear time button.
		WID_VT_RESET_LATENESS                        = to_underlying(::VehicleTimetableWidgets::WID_VT_RESET_LATENESS),                        ///< Reset lateness button.
		WID_VT_AUTOFILL                              = to_underlying(::VehicleTimetableWidgets::WID_VT_AUTOFILL),                              ///< Autofill button.
		WID_VT_EXPECTED                              = to_underlying(::VehicleTimetableWidgets::WID_VT_EXPECTED),                              ///< Toggle between expected and scheduled arrivals.
		WID_VT_SHARED_ORDER_LIST                     = to_underlying(::VehicleTimetableWidgets::WID_VT_SHARED_ORDER_LIST),                     ///< Show the shared order list.
		WID_VT_ARRIVAL_DEPARTURE_SELECTION           = to_underlying(::VehicleTimetableWidgets::WID_VT_ARRIVAL_DEPARTURE_SELECTION),           ///< Disable/hide the arrival departure panel.
		WID_VT_EXPECTED_SELECTION                    = to_underlying(::VehicleTimetableWidgets::WID_VT_EXPECTED_SELECTION),                    ///< Disable/hide the expected selection button.
		WID_VT_CHANGE_SPEED                          = to_underlying(::VehicleTimetableWidgets::WID_VT_CHANGE_SPEED),                          ///< Change speed limit button.
		WID_VT_CLEAR_SPEED                           = to_underlying(::VehicleTimetableWidgets::WID_VT_CLEAR_SPEED),                           ///< Clear speed limit button.
	};

	/* automatically generated from ../../widgets/toolbar_widget.h */
	/** Widgets of the MainToolbarWindow class. */
	enum ToolbarNormalWidgets {
		WID_TN_PAUSE                                 = to_underlying(::ToolbarNormalWidgets::WID_TN_PAUSE),                                 ///< Pause the game.
		WID_TN_FAST_FORWARD                          = to_underlying(::ToolbarNormalWidgets::WID_TN_FAST_FORWARD),                          ///< Fast forward the game.
		WID_TN_SETTINGS                              = to_underlying(::ToolbarNormalWidgets::WID_TN_SETTINGS),                              ///< Settings menu.
		WID_TN_SAVE                                  = to_underlying(::ToolbarNormalWidgets::WID_TN_SAVE),                                  ///< Save menu.
		WID_TN_SMALL_MAP                             = to_underlying(::ToolbarNormalWidgets::WID_TN_SMALL_MAP),                             ///< Small map menu.
		WID_TN_MAP_LAYER                             = to_underlying(::ToolbarNormalWidgets::WID_TN_MAP_LAYER),                             ///< Cycle the map layer shown by the main viewport.
		WID_TN_TOWNS                                 = to_underlying(::ToolbarNormalWidgets::WID_TN_TOWNS),                                 ///< Town menu.
		WID_TN_SUBSIDIES                             = to_underlying(::ToolbarNormalWidgets::WID_TN_SUBSIDIES),                             ///< Subsidy menu.
		WID_TN_STATIONS                              = to_underlying(::ToolbarNormalWidgets::WID_TN_STATIONS),                              ///< Station menu.
		WID_TN_FINANCES                              = to_underlying(::ToolbarNormalWidgets::WID_TN_FINANCES),                              ///< Finance menu.
		WID_TN_COMPANIES                             = to_underlying(::ToolbarNormalWidgets::WID_TN_COMPANIES),                             ///< Company menu.
		WID_TN_STORY                                 = to_underlying(::ToolbarNormalWidgets::WID_TN_STORY),                                 ///< Story menu.
		WID_TN_GOAL                                  = to_underlying(::ToolbarNormalWidgets::WID_TN_GOAL),                                  ///< Goal menu.
		WID_TN_GRAPHS                                = to_underlying(::ToolbarNormalWidgets::WID_TN_GRAPHS),                                ///< Graph menu.
		WID_TN_LEAGUE                                = to_underlying(::ToolbarNormalWidgets::WID_TN_LEAGUE),                                ///< Company league menu.
		WID_TN_INDUSTRIES                            = to_underlying(::ToolbarNormalWidgets::WID_TN_INDUSTRIES),                            ///< Industry menu.
		WID_TN_VEHICLE_START                         = to_underlying(::ToolbarNormalWidgets::WID_TN_VEHICLE_START),                         ///< Helper for the offset of the vehicle menus.
		WID_TN_TRAINS                                = to_underlying(::ToolbarNormalWidgets::WID_TN_TRAINS),                                ///< Train menu.
		WID_TN_ROADVEHS                              = to_underlying(::ToolbarNormalWidgets::WID_TN_ROADVEHS),                              ///< Road vehicle menu.
		WID_TN_SHIPS                                 = to_underlying(::ToolbarNormalWidgets::WID_TN_SHIPS),                                 ///< Ship menu.
		WID_TN_AIRCRAFT                              = to_underlying(::ToolbarNormalWidgets::WID_TN_AIRCRAFT),                              ///< Aircraft menu.
		WID_TN_ZOOM_IN                               = to_underlying(::ToolbarNormalWidgets::WID_TN_ZOOM_IN),                               ///< Zoom in the main viewport.
		WID_TN_ZOOM_OUT                              = to_underlying(::ToolbarNormalWidgets::WID_TN_ZOOM_OUT),                              ///< Zoom out the main viewport.
		WID_TN_BUILDING_TOOLS_START                  = to_underlying(::ToolbarNormalWidgets::WID_TN_BUILDING_TOOLS_START),                  ///< Helper for the offset of the building tools
		WID_TN_RAILS                                 = to_underlying(::ToolbarNormalWidgets::WID_TN_RAILS),                                 ///< Rail building menu.
		WID_TN_ROADS                                 = to_underlying(::ToolbarNormalWidgets::WID_TN_ROADS),                                 ///< Road building menu.
		WID_TN_TRAMS                                 = to_underlying(::ToolbarNormalWidgets::WID_TN_TRAMS),                                 ///< Tram building menu.
		WID_TN_WATER                                 = to_underlying(::ToolbarNormalWidgets::WID_TN_WATER),                                 ///< Water building toolbar.
		WID_TN_AIR                                   = to_underlying(::ToolbarNormalWidgets::WID_TN_AIR),                                   ///< Airport building toolbar.
		WID_TN_LANDSCAPE                             = to_underlying(::ToolbarNormalWidgets::WID_TN_LANDSCAPE),                             ///< Landscaping toolbar.
		WID_TN_MUSIC_SOUND                           = to_underlying(::ToolbarNormalWidgets::WID_TN_MUSIC_SOUND),                           ///< Music/sound configuration menu.
		WID_TN_MESSAGES                              = to_underlying(::ToolbarNormalWidgets::WID_TN_MESSAGES),                              ///< Messages menu.
		WID_TN_HELP                                  = to_underlying(::ToolbarNormalWidgets::WID_TN_HELP),                                  ///< Help menu.
		WID_TN_SWITCH_BAR                            = to_underlying(::ToolbarNormalWidgets::WID_TN_SWITCH_BAR),                            ///< Only available when toolbar has been split to switch between different subsets.
		WID_TN_END                                   = to_underlying(::ToolbarNormalWidgets::WID_TN_END),                                   ///< Helper for knowing the amount of widgets.
	};

	/** Widgets of the ScenarioEditorToolbarWindow class. */
	enum ToolbarEditorWidgets {
		WID_TE_PAUSE                                 = to_underlying(::ToolbarEditorWidgets::WID_TE_PAUSE),                                 ///< Pause the game.
		WID_TE_FAST_FORWARD                          = to_underlying(::ToolbarEditorWidgets::WID_TE_FAST_FORWARD),                          ///< Fast forward the game.
		WID_TE_SETTINGS                              = to_underlying(::ToolbarEditorWidgets::WID_TE_SETTINGS),                              ///< Settings menu.
		WID_TE_SAVE                                  = to_underlying(::ToolbarEditorWidgets::WID_TE_SAVE),                                  ///< Save menu.
		WID_TE_SPACER                                = to_underlying(::ToolbarEditorWidgets::WID_TE_SPACER),                                ///< Spacer with "scenario editor" text.
		WID_TE_DATE                                  = to_underlying(::ToolbarEditorWidgets::WID_TE_DATE),                                  ///< The date of the scenario.
		WID_TE_DATE_BACKWARD                         = to_underlying(::ToolbarEditorWidgets::WID_TE_DATE_BACKWARD),                         ///< Reduce the date of the scenario.
		WID_TE_DATE_FORWARD                          = to_underlying(::ToolbarEditorWidgets::WID_TE_DATE_FORWARD),                          ///< Increase the date of the scenario.
		WID_TE_SMALL_MAP                             = to_underlying(::ToolbarEditorWidgets::WID_TE_SMALL_MAP),                             ///< Small map menu.
		WID_TE_MAP_LAYER                             = to_underlying(::ToolbarEditorWidgets::WID_TE_MAP_LAYER),                             ///< Cycle the map layer shown by the main viewport.
		WID_TE_ZOOM_IN                               = to_underlying(::ToolbarEditorWidgets::WID_TE_ZOOM_IN),                               ///< Zoom in the main viewport.
		WID_TE_ZOOM_OUT                              = to_underlying(::ToolbarEditorWidgets::WID_TE_ZOOM_OUT),                              ///< Zoom out the main viewport.
		WID_TE_LAND_GENERATE                         = to_underlying(::ToolbarEditorWidgets::WID_TE_LAND_GENERATE),                         ///< Land generation.
		WID_TE_TOWN_GENERATE                         = to_underlying(::ToolbarEditorWidgets::WID_TE_TOWN_GENERATE),                         ///< Town building window.
		WID_TE_INDUSTRY                              = to_underlying(::ToolbarEditorWidgets::WID_TE_INDUSTRY),                              ///< Industry building window.
		WID_TE_ROADS                                 = to_underlying(::ToolbarEditorWidgets::WID_TE_ROADS),                                 ///< Road building menu.
		WID_TE_TRAMS                                 = to_underlying(::ToolbarEditorWidgets::WID_TE_TRAMS),                                 ///< Tram building menu.
		WID_TE_WATER                                 = to_underlying(::ToolbarEditorWidgets::WID_TE_WATER),                                 ///< Water building toolbar.
		WID_TE_TREES                                 = to_underlying(::ToolbarEditorWidgets::WID_TE_TREES),                                 ///< Tree building toolbar.
		WID_TE_SIGNS                                 = to_underlying(::ToolbarEditorWidgets::WID_TE_SIGNS),                                 ///< Sign building.
		WID_TE_DATE_PANEL                            = to_underlying(::ToolbarEditorWidgets::WID_TE_DATE_PANEL),                            ///< Container for the date widgets.
		WID_TE_MUSIC_SOUND                           = to_underlying(::ToolbarEditorWidgets::WID_TE_MUSIC_SOUND),                           ///< Music/sound configuration menu.
		WID_TE_HELP                                  = to_underlying(::ToolbarEditorWidgets::WID_TE_HELP),                                  ///< Help menu.
		WID_TE_SWITCH_BAR                            = to_underlying(::ToolbarEditorWidgets::WID_TE_SWITCH_BAR),                            ///< Only available when toolbar has been split to switch between different subsets.
	};

	/* automatically generated from ../../widgets/town_widget.h */
	/** Widgets of the TownDirectoryWindow class. */
	enum TownDirectoryWidgets {
		WID_TD_CAPTION                               = to_underlying(::TownDirectoryWidgets::WID_TD_CAPTION),                               ///< Caption of the window.
		WID_TD_SORT_ORDER                            = to_underlying(::TownDirectoryWidgets::WID_TD_SORT_ORDER),                            ///< Direction of sort dropdown.
		WID_TD_SORT_CRITERIA                         = to_underlying(::TownDirectoryWidgets::WID_TD_SORT_CRITERIA),                         ///< Criteria of sort dropdown.
		WID_TD_FILTER                                = to_underlying(::TownDirectoryWidgets::WID_TD_FILTER),                                ///< Filter of name.
		WID_TD_LIST                                  = to_underlying(::TownDirectoryWidgets::WID_TD_LIST),                                  ///< List of towns.
		WID_TD_SCROLLBAR                             = to_underlying(::TownDirectoryWidgets::WID_TD_SCROLLBAR),                             ///< Scrollbar for the town list.
		WID_TD_WORLD_POPULATION                      = to_underlying(::TownDirectoryWidgets::WID_TD_WORLD_POPULATION),                      ///< The world's population.
	};

	/** Widgets of the TownAuthorityWindow class. */
	enum TownAuthorityWidgets {
		WID_TA_CAPTION                               = to_underlying(::TownAuthorityWidgets::WID_TA_CAPTION),                               ///< Caption of window.
		WID_TA_ZONE_BUTTON                           = to_underlying(::TownAuthorityWidgets::WID_TA_ZONE_BUTTON),                           ///< Turn on/off showing local authority zone.
		WID_TA_RATING_INFO                           = to_underlying(::TownAuthorityWidgets::WID_TA_RATING_INFO),                           ///< Overview with ratings for each company.
		WID_TA_COMMAND_LIST                          = to_underlying(::TownAuthorityWidgets::WID_TA_COMMAND_LIST),                          ///< List of commands for the player.
		WID_TA_SCROLLBAR                             = to_underlying(::TownAuthorityWidgets::WID_TA_SCROLLBAR),                             ///< Scrollbar of the list of commands.
		WID_TA_ACTION_INFO                           = to_underlying(::TownAuthorityWidgets::WID_TA_ACTION_INFO),                           ///< Additional information about the action.
		WID_TA_EXECUTE                               = to_underlying(::TownAuthorityWidgets::WID_TA_EXECUTE),                               ///< Do-it button.
	};

	/** Widgets of the TownViewWindow class. */
	enum TownViewWidgets {
		WID_TV_CAPTION                               = to_underlying(::TownViewWidgets::WID_TV_CAPTION),                               ///< Caption of window.
		WID_TV_VIEWPORT                              = to_underlying(::TownViewWidgets::WID_TV_VIEWPORT),                              ///< View of the center of the town.
		WID_TV_INFO                                  = to_underlying(::TownViewWidgets::WID_TV_INFO),                                  ///< General information about the town.
		WID_TV_CENTER_VIEW                           = to_underlying(::TownViewWidgets::WID_TV_CENTER_VIEW),                           ///< Center the main view on this town.
		WID_TV_SHOW_AUTHORITY                        = to_underlying(::TownViewWidgets::WID_TV_SHOW_AUTHORITY),                        ///< Show the town authority window.
		WID_TV_CHANGE_NAME                           = to_underlying(::TownViewWidgets::WID_TV_CHANGE_NAME),                           ///< Change the name of this town.
		WID_TV_CATCHMENT                             = to_underlying(::TownViewWidgets::WID_TV_CATCHMENT),                             ///< Toggle catchment area highlight.
		WID_TV_EXPAND                                = to_underlying(::TownViewWidgets::WID_TV_EXPAND),                                ///< Expand this town (scenario editor only).
		WID_TV_EXPAND_BUILDINGS                      = to_underlying(::TownViewWidgets::WID_TV_EXPAND_BUILDINGS),                      ///< Expand number of buildings this town (scenario editor only).
		WID_TV_EXPAND_ROADS                          = to_underlying(::TownViewWidgets::WID_TV_EXPAND_ROADS),                          ///< Expand roads of this town (scenario editor only).
		WID_TV_DELETE                                = to_underlying(::TownViewWidgets::WID_TV_DELETE),                                ///< Delete this town (scenario editor only).
		WID_TV_GRAPH                                 = to_underlying(::TownViewWidgets::WID_TV_GRAPH),
	};

	/** Widgets of the FoundTownWindow class. */
	enum TownFoundingWidgets {
		WID_TF_NEW_TOWN                              = to_underlying(::TownFoundingWidgets::WID_TF_NEW_TOWN),                              ///< Create a new town.
		WID_TF_TOWN_ACTION_SEL                       = to_underlying(::TownFoundingWidgets::WID_TF_TOWN_ACTION_SEL),                       ///< Container of town action buttons.
		WID_TF_RANDOM_TOWN                           = to_underlying(::TownFoundingWidgets::WID_TF_RANDOM_TOWN),                           ///< Randomly place a town.
		WID_TF_MANY_RANDOM_TOWNS                     = to_underlying(::TownFoundingWidgets::WID_TF_MANY_RANDOM_TOWNS),                     ///< Randomly place many towns.
		WID_TF_LOAD_FROM_FILE                        = to_underlying(::TownFoundingWidgets::WID_TF_LOAD_FROM_FILE),                        ///< Load town data from file.
		WID_TF_EXPAND_ALL_TOWNS                      = to_underlying(::TownFoundingWidgets::WID_TF_EXPAND_ALL_TOWNS),                      ///< Make all towns grow slightly.
		WID_TF_TOWN_NAME_EDITBOX                     = to_underlying(::TownFoundingWidgets::WID_TF_TOWN_NAME_EDITBOX),                     ///< Editor for the town name.
		WID_TF_TOWN_NAME_RANDOM                      = to_underlying(::TownFoundingWidgets::WID_TF_TOWN_NAME_RANDOM),                      ///< Generate a random town name.
		WID_TF_SIZE_SEL                              = to_underlying(::TownFoundingWidgets::WID_TF_SIZE_SEL),                              ///< Container of town size buttons.
		WID_TF_SIZE_SMALL                            = to_underlying(::TownFoundingWidgets::WID_TF_SIZE_SMALL),                            ///< Selection for a small town.
		WID_TF_SIZE_MEDIUM                           = to_underlying(::TownFoundingWidgets::WID_TF_SIZE_MEDIUM),                           ///< Selection for a medium town.
		WID_TF_SIZE_LARGE                            = to_underlying(::TownFoundingWidgets::WID_TF_SIZE_LARGE),                            ///< Selection for a large town.
		WID_TF_SIZE_RANDOM                           = to_underlying(::TownFoundingWidgets::WID_TF_SIZE_RANDOM),                           ///< Selection for a randomly sized town.
		WID_TF_CITY                                  = to_underlying(::TownFoundingWidgets::WID_TF_CITY),                                  ///< Selection for the town's city state.
		WID_TF_ROAD_LAYOUT_SEL                       = to_underlying(::TownFoundingWidgets::WID_TF_ROAD_LAYOUT_SEL),                       ///< Container of town road layout buttons.
		WID_TF_LAYOUT_ORIGINAL                       = to_underlying(::TownFoundingWidgets::WID_TF_LAYOUT_ORIGINAL),                       ///< Selection for the original town layout.
		WID_TF_LAYOUT_BETTER                         = to_underlying(::TownFoundingWidgets::WID_TF_LAYOUT_BETTER),                         ///< Selection for the better town layout.
		WID_TF_LAYOUT_GRID2                          = to_underlying(::TownFoundingWidgets::WID_TF_LAYOUT_GRID2),                          ///< Selection for the 2x2 grid town layout.
		WID_TF_LAYOUT_GRID3                          = to_underlying(::TownFoundingWidgets::WID_TF_LAYOUT_GRID3),                          ///< Selection for the 3x3 grid town layout.
		WID_TF_LAYOUT_RANDOM                         = to_underlying(::TownFoundingWidgets::WID_TF_LAYOUT_RANDOM),                         ///< Selection for a randomly chosen town layout.
		WID_TF_TOWN_EXPAND_SEL                       = to_underlying(::TownFoundingWidgets::WID_TF_TOWN_EXPAND_SEL),                       ///< Container of town expansion buttons.
		WID_TF_EXPAND_BUILDINGS                      = to_underlying(::TownFoundingWidgets::WID_TF_EXPAND_BUILDINGS),                      ///< Expand buildings toggle.
		WID_TF_EXPAND_ROADS                          = to_underlying(::TownFoundingWidgets::WID_TF_EXPAND_ROADS),                          ///< Expand roads toggle.
	};

	/** Widgets of the BuildHouseWindow class. */
	enum BuildHouseWidgets {
		WID_BH_INFO                                  = to_underlying(::BuildHouseWidgets::WID_BH_INFO),                                  ///< Information panel of selected house.
		WID_BH_PROTECT_TOGGLE                        = to_underlying(::BuildHouseWidgets::WID_BH_PROTECT_TOGGLE),                        ///< Button to toggle protecting the next house built.
		WID_BH_REPLACE_TOGGLE                        = to_underlying(::BuildHouseWidgets::WID_BH_REPLACE_TOGGLE),                        ///< Button to toggle replacing existing houses.
	};

	/* automatically generated from ../../widgets/transparency_widget.h */
	/** Widgets of the TransparenciesWindow class. */
	enum TransparencyToolbarWidgets {
		/* Button row. */
		WID_TT_BEGIN                                 = to_underlying(::TransparencyToolbarWidgets::WID_TT_BEGIN),                                 ///< First toggle button.
		WID_TT_SIGNS                                 = to_underlying(::TransparencyToolbarWidgets::WID_TT_SIGNS),                                 ///< Signs background transparency toggle button.
		WID_TT_TREES                                 = to_underlying(::TransparencyToolbarWidgets::WID_TT_TREES),                                 ///< Trees transparency toggle button.
		WID_TT_HOUSES                                = to_underlying(::TransparencyToolbarWidgets::WID_TT_HOUSES),                                ///< Houses transparency toggle button.
		WID_TT_INDUSTRIES                            = to_underlying(::TransparencyToolbarWidgets::WID_TT_INDUSTRIES),                            ///< industries transparency toggle button.
		WID_TT_BUILDINGS                             = to_underlying(::TransparencyToolbarWidgets::WID_TT_BUILDINGS),                             ///< Company buildings and structures transparency toggle button.
		WID_TT_BRIDGES                               = to_underlying(::TransparencyToolbarWidgets::WID_TT_BRIDGES),                               ///< Bridges transparency toggle button.
		WID_TT_STRUCTURES                            = to_underlying(::TransparencyToolbarWidgets::WID_TT_STRUCTURES),                            ///< Object structure transparency toggle button.
		WID_TT_CATENARY                              = to_underlying(::TransparencyToolbarWidgets::WID_TT_CATENARY),                              ///< Catenary transparency toggle button.
		WID_TT_TEXT                                  = to_underlying(::TransparencyToolbarWidgets::WID_TT_TEXT),                                  ///< Loading and cost/income text transparency toggle button.
		WID_TT_END                                   = to_underlying(::TransparencyToolbarWidgets::WID_TT_END),                                   ///< End of toggle buttons.

		/* Panel with buttons for invisibility */
		WID_TT_BUTTONS                               = to_underlying(::TransparencyToolbarWidgets::WID_TT_BUTTONS),                               ///< Panel with 'invisibility' buttons.
	};

	/* automatically generated from ../../widgets/tree_widget.h */
	/** Widgets of the BuildTreesWindow class. */
	enum BuildTreesWidgets {
		WID_BT_TYPE_RANDOM                           = to_underlying(::BuildTreesWidgets::WID_BT_TYPE_RANDOM),                           ///< Button to build random type of tree.
		WID_BT_SE_PANE                               = to_underlying(::BuildTreesWidgets::WID_BT_SE_PANE),                               ///< Selection pane to show/hide scenario editor tools.
		WID_BT_MODE_NORMAL                           = to_underlying(::BuildTreesWidgets::WID_BT_MODE_NORMAL),                           ///< Select normal/rectangle planting mode.
		WID_BT_MODE_FOREST_SM                        = to_underlying(::BuildTreesWidgets::WID_BT_MODE_FOREST_SM),                        ///< Select small forest planting mode.
		WID_BT_MODE_FOREST_LG                        = to_underlying(::BuildTreesWidgets::WID_BT_MODE_FOREST_LG),                        ///< Select large forest planting mode.
		WID_BT_MANY_RANDOM                           = to_underlying(::BuildTreesWidgets::WID_BT_MANY_RANDOM),                           ///< Button to build many random trees.
		WID_BT_TYPE_BUTTON_FIRST                     = to_underlying(::BuildTreesWidgets::WID_BT_TYPE_BUTTON_FIRST),                     ///< First tree type selection button. (This must be last in the enum.)
	};

	/* automatically generated from ../../widgets/underground_widget.h */
	/** Widgets of the underground construction window; feature commands retain all mutation authority. */
	enum UndergroundConstructionWidgets {
		WID_UC_CAPTION                               = to_underlying(::UndergroundConstructionWidgets::WID_UC_CAPTION),                               ///< Names the explicitly requested layer.
		WID_UC_REQUIREMENTS                          = to_underlying(::UndergroundConstructionWidgets::WID_UC_REQUIREMENTS),                          ///< Calendar and whole-footprint construction requirements.
		WID_UC_SURVEY                                = to_underlying(::UndergroundConstructionWidgets::WID_UC_SURVEY),                                ///< Inspect one world tile's public geological planning information.
		WID_UC_PROJECT                               = to_underlying(::UndergroundConstructionWidgets::WID_UC_PROJECT),                               ///< Browse actual owned projects and select their vehicle or footprint.
		WID_UC_MINE                                  = to_underlying(::UndergroundConstructionWidgets::WID_UC_MINE),                                  ///< Open the native industry's funding controls.
		WID_UC_EXCHANGE                              = to_underlying(::UndergroundConstructionWidgets::WID_UC_EXCHANGE),                              ///< Select a real project, then place its exchange at a validated bay.
		WID_UC_RAIL                                  = to_underlying(::UndergroundConstructionWidgets::WID_UC_RAIL),                                  ///< Open conventional rail construction.
		WID_UC_ROAD                                  = to_underlying(::UndergroundConstructionWidgets::WID_UC_ROAD),                                  ///< Open conventional road construction.
		WID_UC_RAIL_STATION                          = to_underlying(::UndergroundConstructionWidgets::WID_UC_RAIL_STATION),                          ///< Open the ordinary railway station picker.
		WID_UC_ROAD_TERMINAL                         = to_underlying(::UndergroundConstructionWidgets::WID_UC_ROAD_TERMINAL),                         ///< Open the ordinary freight road-stop picker.
		WID_UC_RAIL_SERVICE                          = to_underlying(::UndergroundConstructionWidgets::WID_UC_RAIL_SERVICE),                          ///< Open the ordinary rail service-depot picker.
		WID_UC_ROAD_SERVICE                          = to_underlying(::UndergroundConstructionWidgets::WID_UC_ROAD_SERVICE),                          ///< Open the ordinary road service-depot picker.
		WID_UC_STATUS                                = to_underlying(::UndergroundConstructionWidgets::WID_UC_STATUS),                                ///< Selected tile information or a visible placement refusal.
		WID_UC_AREA                                  = to_underlying(::UndergroundConstructionWidgets::WID_UC_AREA),                                  ///< Select a real portal, then drag a requested excavation floor.
		WID_UC_AREA_CONFIRM                          = to_underlying(::UndergroundConstructionWidgets::WID_UC_AREA_CONFIRM),                          ///< Submit the owning commissioning command after a complete current quote.
		WID_UC_AREA_PRICE                            = to_underlying(::UndergroundConstructionWidgets::WID_UC_AREA_PRICE),                            ///< Separate purchase and future geological-work charges.
		WID_UC_AREA_MATERIAL                         = to_underlying(::UndergroundConstructionWidgets::WID_UC_AREA_MATERIAL),                         ///< Canonical Earth, Rock, support and work requirements.
		WID_UC_AREA_FLOOR                            = to_underlying(::UndergroundConstructionWidgets::WID_UC_AREA_FLOOR),                            ///< Final floor and actual additional swept work space.
		WID_UC_MODEL_DATES                           = to_underlying(::UndergroundConstructionWidgets::WID_UC_MODEL_DATES),                           ///< Compatible cutter names and introduction years from the canonical catalogue.
		WID_UC_JOB_PROGRESS                          = to_underlying(::UndergroundConstructionWidgets::WID_UC_JOB_PROGRESS),                          ///< Live selected job cut/partial-work progress; geology retains its sole cursor.
		WID_UC_DETAILS                               = to_underlying(::UndergroundConstructionWidgets::WID_UC_DETAILS),                               ///< Toggle optional construction requirements without changing the active tool.
		WID_UC_DETAILS_STACK                         = to_underlying(::UndergroundConstructionWidgets::WID_UC_DETAILS_STACK),                         ///< Requirements/calendar are hidden until requested.
		WID_UC_QUOTE_STACK                           = to_underlying(::UndergroundConstructionWidgets::WID_UC_QUOTE_STACK),                           ///< Actual quote/material/confirmation rows, hidden before a floor exists.
		WID_UC_PROJECT_STACK                         = to_underlying(::UndergroundConstructionWidgets::WID_UC_PROJECT_STACK),                         ///< Selected project progress, hidden until a real project is inspected.
		WID_UC_INTENT_STACK                          = to_underlying(::UndergroundConstructionWidgets::WID_UC_INTENT_STACK),                          ///< Work intent is shown only while the area/tunnel placement tool owns the cursor.
		WID_UC_INTENT                                = to_underlying(::UndergroundConstructionWidgets::WID_UC_INTENT),                                ///< Choose area coverage or exact straight cutter-centre continuation.
		WID_UC_AREA_TIME                             = to_underlying(::UndergroundConstructionWidgets::WID_UC_AREA_TIME),                             ///< Approximate uninterrupted cutting time at the actual cutter model's work rate.
	};

	/* automatically generated from ../../widgets/vehicle_widget.h */
	/** Widgets of the VehicleViewWindow class. */
	enum VehicleViewWidgets {
		WID_VV_CAPTION                               = to_underlying(::VehicleViewWidgets::WID_VV_CAPTION),                               ///< Caption of window.
		WID_VV_VIEWPORT                              = to_underlying(::VehicleViewWidgets::WID_VV_VIEWPORT),                              ///< Viewport widget.
		WID_VV_START_STOP                            = to_underlying(::VehicleViewWidgets::WID_VV_START_STOP),                            ///< Start or stop this vehicle, and show information about the current state.
		WID_VV_RENAME                                = to_underlying(::VehicleViewWidgets::WID_VV_RENAME),                                ///< Rename vehicle
		WID_VV_LOCATION                              = to_underlying(::VehicleViewWidgets::WID_VV_LOCATION),                              ///< Center the main view on this vehicle.
		WID_VV_ORDER_LOCATION                        = to_underlying(::VehicleViewWidgets::WID_VV_ORDER_LOCATION),                        ///< Center the main view on the order's target location.
		WID_VV_GOTO_DEPOT                            = to_underlying(::VehicleViewWidgets::WID_VV_GOTO_DEPOT),                            ///< Order this vehicle to go to the depot.
		WID_VV_REFIT                                 = to_underlying(::VehicleViewWidgets::WID_VV_REFIT),                                 ///< Open the refit window.
		WID_VV_SHOW_ORDERS                           = to_underlying(::VehicleViewWidgets::WID_VV_SHOW_ORDERS),                           ///< Show the orders of this vehicle.
		WID_VV_SHOW_DETAILS                          = to_underlying(::VehicleViewWidgets::WID_VV_SHOW_DETAILS),                          ///< Show details of this vehicle.
		WID_VV_CLONE                                 = to_underlying(::VehicleViewWidgets::WID_VV_CLONE),                                 ///< Clone this vehicle.
		WID_VV_SELECT_DEPOT_CLONE                    = to_underlying(::VehicleViewWidgets::WID_VV_SELECT_DEPOT_CLONE),                    ///< Selection widget between 'goto depot', and 'clone vehicle' buttons.
		WID_VV_SELECT_REFIT_TURN                     = to_underlying(::VehicleViewWidgets::WID_VV_SELECT_REFIT_TURN),                     ///< Selection widget between 'refit' and 'turn around' buttons.
		WID_VV_TURN_AROUND                           = to_underlying(::VehicleViewWidgets::WID_VV_TURN_AROUND),                           ///< Turn this vehicle around.
		WID_VV_FORCE_PROCEED                         = to_underlying(::VehicleViewWidgets::WID_VV_FORCE_PROCEED),                         ///< Force this vehicle to pass a signal at danger.
		WID_VV_FORCE_PROCEED_SEL                     = to_underlying(::VehicleViewWidgets::WID_VV_FORCE_PROCEED_SEL),                     ///< Container for 'force proceed' button, which can be hidden.
		WID_VV_HONK_HORN                             = to_underlying(::VehicleViewWidgets::WID_VV_HONK_HORN),                             ///< Honk the vehicles horn (not drawn on UI, only used for hotkey).
	};

	/** Widgets of the RefitWindow class. */
	enum VehicleRefitWidgets {
		WID_VR_CAPTION                               = to_underlying(::VehicleRefitWidgets::WID_VR_CAPTION),                               ///< Caption of window.
		WID_VR_VEHICLE_PANEL_DISPLAY                 = to_underlying(::VehicleRefitWidgets::WID_VR_VEHICLE_PANEL_DISPLAY),                 ///< Display with a representation of the vehicle to refit.
		WID_VR_SHOW_HSCROLLBAR                       = to_underlying(::VehicleRefitWidgets::WID_VR_SHOW_HSCROLLBAR),                       ///< Selection widget for the horizontal scrollbar.
		WID_VR_HSCROLLBAR                            = to_underlying(::VehicleRefitWidgets::WID_VR_HSCROLLBAR),                            ///< Horizontal scrollbar or the vehicle display.
		WID_VR_SELECT_HEADER                         = to_underlying(::VehicleRefitWidgets::WID_VR_SELECT_HEADER),                         ///< Header with question about the cargo to carry.
		WID_VR_MATRIX                                = to_underlying(::VehicleRefitWidgets::WID_VR_MATRIX),                                ///< Options to refit to.
		WID_VR_SCROLLBAR                             = to_underlying(::VehicleRefitWidgets::WID_VR_SCROLLBAR),                             ///< Scrollbar for the refit options.
		WID_VR_INFO                                  = to_underlying(::VehicleRefitWidgets::WID_VR_INFO),                                  ///< Information about the currently selected refit option.
		WID_VR_REFIT                                 = to_underlying(::VehicleRefitWidgets::WID_VR_REFIT),                                 ///< Perform the refit.
	};

	/** Widgets of the VehicleDetailsWindow class. */
	enum VehicleDetailsWidgets {
		WID_VD_CAPTION                               = to_underlying(::VehicleDetailsWidgets::WID_VD_CAPTION),                               ///< Caption of window.
		WID_VD_TOP_DETAILS                           = to_underlying(::VehicleDetailsWidgets::WID_VD_TOP_DETAILS),                           ///< Panel with generic details.
		WID_VD_INCREASE_SERVICING_INTERVAL           = to_underlying(::VehicleDetailsWidgets::WID_VD_INCREASE_SERVICING_INTERVAL),           ///< Increase the servicing interval.
		WID_VD_DECREASE_SERVICING_INTERVAL           = to_underlying(::VehicleDetailsWidgets::WID_VD_DECREASE_SERVICING_INTERVAL),           ///< Decrease the servicing interval.
		WID_VD_SERVICE_INTERVAL_DROPDOWN             = to_underlying(::VehicleDetailsWidgets::WID_VD_SERVICE_INTERVAL_DROPDOWN),             ///< Dropdown to select default/days/percent service interval.
		WID_VD_SERVICING_INTERVAL                    = to_underlying(::VehicleDetailsWidgets::WID_VD_SERVICING_INTERVAL),                    ///< Information about the servicing interval.
		WID_VD_MIDDLE_DETAILS                        = to_underlying(::VehicleDetailsWidgets::WID_VD_MIDDLE_DETAILS),                        ///< Details for non-trains.
		WID_VD_MATRIX                                = to_underlying(::VehicleDetailsWidgets::WID_VD_MATRIX),                                ///< List of details for trains.
		WID_VD_SCROLLBAR                             = to_underlying(::VehicleDetailsWidgets::WID_VD_SCROLLBAR),                             ///< Scrollbar for train details.
		WID_VD_DETAILS_CARGO_CARRIED                 = to_underlying(::VehicleDetailsWidgets::WID_VD_DETAILS_CARGO_CARRIED),                 ///< Show carried cargo and capacity per part of the train.
		WID_VD_DETAILS_TRAIN_VEHICLES                = to_underlying(::VehicleDetailsWidgets::WID_VD_DETAILS_TRAIN_VEHICLES),                ///< Show all parts of the train with their description.
		WID_VD_DETAILS_TOTALS                        = to_underlying(::VehicleDetailsWidgets::WID_VD_DETAILS_TOTALS),                        ///< Show the total cargo, total capacity, and length of the train.
	};

	/** Widgets of the VehicleListWindow class. */
	enum VehicleListWidgets {
		WID_VL_CAPTION                               = to_underlying(::VehicleListWidgets::WID_VL_CAPTION),                               ///< Caption of window (for non shared orders windows).
		WID_VL_CAPTION_SHARED_ORDERS                 = to_underlying(::VehicleListWidgets::WID_VL_CAPTION_SHARED_ORDERS),                 ///< Caption of window (for shared orders windows).
		WID_VL_CAPTION_SELECTION                     = to_underlying(::VehicleListWidgets::WID_VL_CAPTION_SELECTION),                     ///< Selection for caption.
		WID_VL_ORDER_VIEW                            = to_underlying(::VehicleListWidgets::WID_VL_ORDER_VIEW),                            ///< Button to open order window (for shared orders windows).
		WID_VL_GROUP_ORDER                           = to_underlying(::VehicleListWidgets::WID_VL_GROUP_ORDER),                           ///< Group order.
		WID_VL_GROUP_BY_PULLDOWN                     = to_underlying(::VehicleListWidgets::WID_VL_GROUP_BY_PULLDOWN),                     ///< Group by dropdown list.
		WID_VL_SORT_ORDER                            = to_underlying(::VehicleListWidgets::WID_VL_SORT_ORDER),                            ///< Sort order.
		WID_VL_SORT_BY_PULLDOWN                      = to_underlying(::VehicleListWidgets::WID_VL_SORT_BY_PULLDOWN),                      ///< Sort by dropdown list.
		WID_VL_FILTER_BY_CARGO                       = to_underlying(::VehicleListWidgets::WID_VL_FILTER_BY_CARGO),                       ///< Cargo filter dropdown list.
		WID_VL_FILTER_BY_CARGO_SEL                   = to_underlying(::VehicleListWidgets::WID_VL_FILTER_BY_CARGO_SEL),                   ///< Cargo filter dropdown list panel selector.
		WID_VL_LIST                                  = to_underlying(::VehicleListWidgets::WID_VL_LIST),                                  ///< List of the vehicles.
		WID_VL_SCROLLBAR                             = to_underlying(::VehicleListWidgets::WID_VL_SCROLLBAR),                             ///< Scrollbar for the list.
		WID_VL_HIDE_BUTTONS                          = to_underlying(::VehicleListWidgets::WID_VL_HIDE_BUTTONS),                          ///< Selection to hide the buttons.
		WID_VL_AVAILABLE_VEHICLES                    = to_underlying(::VehicleListWidgets::WID_VL_AVAILABLE_VEHICLES),                    ///< Available vehicles.
		WID_VL_MANAGE_VEHICLES_DROPDOWN              = to_underlying(::VehicleListWidgets::WID_VL_MANAGE_VEHICLES_DROPDOWN),              ///< Manage vehicles dropdown list.
		WID_VL_STOP_ALL                              = to_underlying(::VehicleListWidgets::WID_VL_STOP_ALL),                              ///< Stop all button.
		WID_VL_START_ALL                             = to_underlying(::VehicleListWidgets::WID_VL_START_ALL),                             ///< Start all button.
	};

	/* automatically generated from ../../widgets/viewport_widget.h */
	/** Widgets of the ExtraViewportWindow class. */
	enum ExtraViewportWidgets {
		WID_EV_CAPTION                               = to_underlying(::ExtraViewportWidgets::WID_EV_CAPTION),                               ///< Caption of window.
		WID_EV_VIEWPORT                              = to_underlying(::ExtraViewportWidgets::WID_EV_VIEWPORT),                              ///< The viewport.
		WID_EV_ZOOM_IN                               = to_underlying(::ExtraViewportWidgets::WID_EV_ZOOM_IN),                               ///< Zoom in.
		WID_EV_ZOOM_OUT                              = to_underlying(::ExtraViewportWidgets::WID_EV_ZOOM_OUT),                              ///< Zoom out.
		WID_EV_MAP_LAYER                             = to_underlying(::ExtraViewportWidgets::WID_EV_MAP_LAYER),                             ///< Cycle the layer this extra viewport shows.
		WID_EV_MAIN_TO_VIEW                          = to_underlying(::ExtraViewportWidgets::WID_EV_MAIN_TO_VIEW),                          ///< Center the view of this viewport on the main view.
		WID_EV_VIEW_TO_MAIN                          = to_underlying(::ExtraViewportWidgets::WID_EV_VIEW_TO_MAIN),                          ///< Center the main view on the view of this viewport.
	};

	/* automatically generated from ../../widgets/waypoint_widget.h */
	/** Widgets of the WaypointWindow class. */
	enum WaypointWidgets {
		WID_W_CAPTION                                = to_underlying(::WaypointWidgets::WID_W_CAPTION),                                ///< Caption of window.
		WID_W_VIEWPORT                               = to_underlying(::WaypointWidgets::WID_W_VIEWPORT),                               ///< The viewport on this waypoint.
		WID_W_CENTER_VIEW                            = to_underlying(::WaypointWidgets::WID_W_CENTER_VIEW),                            ///< Center the main view on this waypoint.
		WID_W_RENAME                                 = to_underlying(::WaypointWidgets::WID_W_RENAME),                                 ///< Rename this waypoint.
		WID_W_SHOW_VEHICLES                          = to_underlying(::WaypointWidgets::WID_W_SHOW_VEHICLES),                          ///< Show the vehicles visiting this waypoint.
		WID_W_CATCHMENT                              = to_underlying(::WaypointWidgets::WID_W_CATCHMENT),                              ///< Coverage button.
	};

	// @endenum
};

#endif /* SCRIPT_WINDOW_HPP */
