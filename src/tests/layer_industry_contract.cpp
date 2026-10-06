/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_industry_contract.cpp Public ownership and type/signature fixture; actual production is implemented in Group 7. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Group 6 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_industry.h"

#include "../safeguards.h"

/** Invalid defaults cannot authorize a mine claim, recipe inventory mutation or future orbital/utility operation. */
TEST_CASE("Layer industry public handoff keeps invalid identities and actual inventory ownership explicit", "[layer-industry-contract]")
{
	LayerMineRequest request;
	LayerMineState mine;
	LayerMineAdmission admission;
	LayerRecipeInventorySnapshot inventory;
	LayerRecipeBatchProposal proposal;
	LayerRecipeBatchReceipt receipt;
	CHECK(request.tile == INVALID_TILE);
	CHECK(request.type == IT_INVALID);
	CHECK(request.deposit == GeologyDepositID{UINT32_MAX});
	CHECK(request.company == CompanyID::Invalid());
	CHECK(mine.industry == IndustryID::Invalid());
	CHECK(mine.deposit == GeologyDepositID{UINT32_MAX});
	CHECK(mine.company == CompanyID::Invalid());
	CHECK(admission.error == LayerIndustryError::InvalidDefinition);
	CHECK(inventory.layer == MapLayer::Invalid);
	CHECK_FALSE(inventory.active);
	CHECK(inventory.inputs.empty());
	CHECK(inventory.outputs.empty());
	CHECK(inventory.heat_j == 0);
	CHECK(inventory.power_j == 0);
	CHECK(proposal.owner == nullptr);
	CHECK(proposal.definition == nullptr);
	CHECK(proposal.plan.batches == 0);
	CHECK(proposal.inputs.empty());
	CHECK(proposal.outputs.empty());
	CHECK(receipt.error == LayerRecipeCommitError::InvalidProposal);
	CHECK(receipt.batches == 0);
	STATIC_REQUIRE(std::is_abstract_v<LayerRecipeInventoryView>);
	STATIC_REQUIRE(std::is_abstract_v<LayerRecipeCommitParticipant>);
	using ClaimSignature = LayerMineAdmission (*)(const LayerMineRequest &);
	using ProduceSignature = LayerMineProduction (*)(Industry &, uint32_t, bool);
	using PrepareSignature = LayerRecipeBatchProposal (*)(const MultiLayerRecipe &, const LayerRecipeInventoryView &);
	using CommitSignature = LayerRecipeBatchReceipt (*)(const MultiLayerRecipe &, const LayerRecipeInventoryView &, LayerRecipeCommitParticipant &, const LayerRecipeBatchProposal &);
	STATIC_REQUIRE(std::is_same_v<decltype(&ValidateMineDepositClaim), ClaimSignature>);
	STATIC_REQUIRE(std::is_same_v<decltype(&ProduceLayerMineCargo), ProduceSignature>);
	STATIC_REQUIRE(std::is_same_v<decltype(&PrepareLayerRecipeBatch), PrepareSignature>);
	STATIC_REQUIRE(std::is_same_v<decltype(&CommitLayerRecipeBatch), CommitSignature>);
}
