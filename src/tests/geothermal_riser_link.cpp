/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file geothermal_riser_link.cpp Actual paid machinery admission, exclusive native electrical identity and retained loss/reload boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs17-geothermal-energy-and-industry.md Group5
 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "geothermal_riser_fixture.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** Actual registered electrical reservations only; this builder boundary creates neither a generator nor energy or assumed local cable connectivity. */
struct RiserElectricalParticipants {
	UtilityConsumerID source{UINT32_MAX}, destination{UINT32_MAX}; ///< Shared native reservation identities on the actual declared sending/receiving planes.
	/** @param rig Actual original machinery/pair. @param source_layer Optional wrong sending layer for whole-admission refusal. @param destination_layer Optional wrong receiving layer. Register real bounded accounts without inventing generation or demand. */
	explicit RiserElectricalParticipants(const NativeRiserRig &rig, std::optional<MapLayer> source_layer = {}, std::optional<MapLayer> destination_layer = {})
	{
		const auto from = RegisterUtilityPowerConsumer(rig.request.owner, source_layer.value_or(GetTileLayer(rig.request.portal)), UtilityConsumerEffect::AssemblyWork, 1000000, {});
		const auto to = RegisterUtilityPowerConsumer(rig.request.owner, destination_layer.value_or(GetTileLayer(rig.destination)), UtilityConsumerEffect::TubeWork, 1000000, {});
		REQUIRE(from.has_value()); REQUIRE(to.has_value()); this->source = *from; this->destination = *to;
	}
};

/** Complete actual paid machinery supplies one frozen physical direction; estimates and duplicate numerical accounts cannot multiply it or credit electricity. */
TEST_CASE("Paid riser registers one shared native electrical edge after real construction", "[geothermal-riser-link]")
{
	NativeRiserRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(296);
	const auto industry = rig.Riser().index; RiserElectricalParticipants accounts(rig);
	const auto before = ComputeUtilityPowerDigest(), construction = ComputeGeothermalRiserDigest(), geology = ComputeGeologyStateDigest(GetGeologyWorldState());
	const auto cash = Company::Get(rig.request.owner)->money;
	CHECK_FALSE(RegisterUtilityPowerLink(accounts.source, accounts.destination, rig.request.portal, 1000).has_value());
	CHECK(ComputeUtilityPowerDigest() == before);
	const auto estimate = RegisterGeothermalRiserLink(industry, accounts.source, accounts.destination, false);
	REQUIRE(estimate.has_value()); CHECK(ComputeUtilityPowerDigest() == before);
	const auto edge = RegisterGeothermalRiserLink(industry, accounts.source, accounts.destination);
	REQUIRE(edge == estimate); const auto *link = GetUtilityPowerLink(*edge); REQUIRE(link != nullptr);
	CHECK(link->kind == UtilityLinkKind::ElectricalRiser); CHECK(link->riser == industry); CHECK(link->riser_capacity_w == 200000);
	CHECK(link->source_endpoint == rig.request.portal); CHECK(link->destination_endpoint == rig.destination);
	CHECK(link->portal_revision == GetGeothermalRiserState(industry)->portal_revision); CHECK(link->capacity_j == 0); CHECK(link->flow_j == 0);
	CHECK(CaptureUtilityPowerState().producers.empty()); CHECK(GetUtilityPowerConsumer(accounts.destination)->meter.granted_energy_j == 0);
	CHECK(ComputeGeothermalRiserDigest() == construction); CHECK(ComputeGeologyStateDigest(GetGeologyWorldState()) == geology);
	CHECK(Company::Get(rig.request.owner)->money == cash);
	RiserElectricalParticipants other(rig); const auto bound = ComputeUtilityPowerDigest();
	CHECK_FALSE(RegisterGeothermalRiserLink(industry, accounts.source, accounts.destination).has_value());
	CHECK_FALSE(RegisterGeothermalRiserLink(industry, other.source, other.destination).has_value());
	CHECK_FALSE(RegisterUtilityPowerLink(other.source, other.destination, rig.request.portal, 1000).has_value());
	CHECK(ComputeUtilityPowerDigest() == bound);
	CHECK(ValidateUtilityPowerState(CaptureUtilityPowerState(), GetCurrentMultiLayerProfileSelection()) == UtilityPowerError::None);
}

/** Unfunded/partial/lost/foreign/locked or cross-domain participants refuse before the shared identity, sequence, native stock or capital changes. */
TEST_CASE("Paid riser electrical admission refuses incomplete actual participants atomically", "[geothermal-riser-link]")
{
	NativeRiserRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(295);
	const auto industry = rig.Riser().index;
	std::optional<MapLayer> source_layer, destination_layer;
	bool foreign_actor = false, foreign_endpoint = false, locked_date = false, heat_sender = false;
	SECTION("unfinished material labour") {}
	SECTION("wrong sending plane") { rig.Tick(); source_layer = MapLayer::Surface; }
	SECTION("wrong receiving plane") { rig.Tick(); destination_layer = MapLayer::Surface; }
	SECTION("completed but sealed") { rig.Tick(); REQUIRE(ChangeLayerPortalConnection(rig.request.portal, _current_company, false, true) == LayerPortalWorkError::None); }
	SECTION("original housing retired") { rig.Tick(); REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.destination).Succeeded()); }
	SECTION("native machinery removed") { rig.Tick(); REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Succeeded()); }
	SECTION("wrong company actor") { rig.Tick(); foreign_actor = true; }
	SECTION("foreign receiving housing") { rig.Tick(); foreign_endpoint = true; }
	SECTION("calendar locked after completion") { rig.Tick(); locked_date = true; }
	SECTION("Heat cannot supply an electrical sender") { rig.Tick(); heat_sender = true; }
	SECTION("native stock cannot hide behind a completed receipt") { rig.Tick(); rig.Riser().GetCargoAccepted(GetCargoTypeByLabel(CargoLabel{"STEL"}))->waiting = 1; }
	RiserElectricalParticipants accounts(rig, source_layer, destination_layer);
	if (heat_sender) {
		const auto heat = RegisterUtilityHeatConsumer(rig.request.owner, UtilityHeatUse::ElectricalGenerator, 1000000, {});
		REQUIRE(heat.has_value()); accounts.source = *heat;
	}
	// Register real accounts first, then change only the selected asking
	// actor/technology boundary; neither fault is hidden by failed setup.
	AutoRestoreBackup actor(_current_company, foreign_actor ? CompanyID{1} : _current_company);
	AutoRestoreBackup date(TimerGameCalendar::date, locked_date ? TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1979}, TimerGameCalendar::Month{0}, TimerGameCalendar::Day{1}) : TimerGameCalendar::date);
	const auto original_owner = GetTileOwner(rig.destination);
	if (foreign_endpoint) SetTileOwner(rig.destination, OWNER_NONE); // Deliberately invalid native endpoint ownership must refuse; restore the actual housing before fixture teardown.
	const auto before = ComputeUtilityPowerDigest(), construction = ComputeGeothermalRiserDigest();
	CHECK_FALSE(RegisterGeothermalRiserLink(industry, accounts.source, accounts.destination).has_value());
	CHECK(ComputeUtilityPowerDigest() == before); CHECK(ComputeGeothermalRiserDigest() == construction);
	if (foreign_endpoint) SetTileOwner(rig.destination, original_owner);
}

/** Actual sealing and machinery demolition retire original link identities; reopening/rebuilding must explicitly obtain new shared IDs without renewing any energy. */
TEST_CASE("Paid riser native loss retains retired edges through machinery and portal reuse", "[geothermal-riser-link]")
{
	NativeRiserRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(296);
	const auto industry = rig.Riser().index; RiserElectricalParticipants accounts(rig);
	const auto first = RegisterGeothermalRiserLink(industry, accounts.source, accounts.destination); REQUIRE(first.has_value());
	REQUIRE(ChangeLayerPortalConnection(rig.request.portal, _current_company, false, true) == LayerPortalWorkError::None);
	CHECK_FALSE(GetUtilityPowerLink(*first)->active); CHECK(GetUtilityPowerLink(*first)->riser == industry);
	REQUIRE(ChangeLayerPortalConnection(rig.request.portal, _current_company, true, true) == LayerPortalWorkError::None);
	const auto second = RegisterGeothermalRiserLink(industry, accounts.source, accounts.destination); REQUIRE(second.has_value()); CHECK(*second != *first);
	REQUIRE(Command<Commands::LandscapeClear>::Do(DoCommandFlag::Execute, rig.request.tile).Succeeded());
	CHECK_FALSE(GetUtilityPowerLink(*second)->active); CHECK_FALSE(RegisterGeothermalRiserLink(industry, accounts.source, accounts.destination).has_value());
	REQUIRE(rig.Fund().Succeeded()); CHECK(rig.Riser().index == industry);
	CHECK_FALSE(RegisterGeothermalRiserLink(industry, accounts.source, accounts.destination).has_value());
	rig.DeliverBill(); rig.Tick(296);
	const auto third = RegisterGeothermalRiserLink(rig.Riser().index, accounts.source, accounts.destination); REQUIRE(third.has_value()); CHECK(*third != *second);
	CHECK_FALSE(GetUtilityPowerLink(*first)->active); CHECK_FALSE(GetUtilityPowerLink(*second)->active);
	CHECK(CaptureUtilityPowerState().producers.empty());
}

/** Typed whole-state acceptance resolves original native machinery and immutable rate before replacing any published shared link owner. */
TEST_CASE("Paid riser typed native link reload preserves exact original identity and refuses forgery", "[geothermal-riser-link]")
{
	NativeRiserRig rig; REQUIRE(rig.Fund().Succeeded()); rig.DeliverBill(); rig.Tick(296);
	RiserElectricalParticipants accounts(rig);
	const auto edge = RegisterGeothermalRiserLink(rig.Riser().index, accounts.source, accounts.destination); REQUIRE(edge.has_value());
	const auto original = CaptureUtilityPowerState(); const auto digest = ComputeUtilityPowerDigest();
	ResetUtilityPowerState(); StageLoadedUtilityPowerState(original); REQUIRE(AcceptLoadedUtilityPowerState()); CHECK(ComputeUtilityPowerDigest() == digest);
	auto bad = original;
	SECTION("wrong machinery") { bad.links[0].riser = IndustryID{63999}; }
	SECTION("inflated physical rate") { ++bad.links[0].riser_capacity_w; }
	SECTION("invented interval credit") { bad.links[0].capacity_j = 1; }
	SECTION("invented transported flow") { bad.links[0].flow_j = 1; }
	SECTION("future portal incarnation") { ++bad.links[0].portal_revision; }
	SECTION("hidden native reference in an older role") { bad.links[0].kind = UtilityLinkKind::ElectricalPortal; bad.links[0].capacity_j = 1000; }
	SECTION("downgrade to canonical older portal role") { bad.links[0].kind = UtilityLinkKind::ElectricalPortal; bad.links[0].riser = IndustryID::Invalid(); bad.links[0].riser_capacity_w = 0; bad.links[0].capacity_j = 1000; }
	StageLoadedUtilityPowerState(std::move(bad)); CHECK_FALSE(AcceptLoadedUtilityPowerState()); CHECK(ComputeUtilityPowerDigest() == digest);
}
