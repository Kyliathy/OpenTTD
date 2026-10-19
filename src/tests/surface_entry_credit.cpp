/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_entry_credit.cpp Native bank borrowing from the authentic new-game company.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BC.1.
 * Actual IncreaseLoan owns both cash and debt. No fixture balance, refund,
 * cargo grant, new company or parallel financial account is introduced.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../console_internal.h"
#include "../company_base.h"
#include "../company_func.h"
#include "../command_func.h"
#include "../misc_cmd.h"
#include "../cargopacket.h"
#include "../industry.h"
#include "../openttd.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../saveload/saveload_func.h"
#include "layer_save_fixture.h"
#include "test_artifact_paths.h"
#include "../safeguards.h"

static std::function<void()> *surface_credit_observer = nullptr; ///< Non-owning invocation callback; never a financial owner.

/** Inspect only the actual restored world in this invocation.
 * @param args Parameter-free original console callback arguments.
 * @return True after invoking the current real-world observer.
 */
static bool ObserveSurfaceCredit(std::span<std::string_view> args)
{
	REQUIRE(args.size() == 1); REQUIRE(surface_credit_observer != nullptr);
	(*surface_credit_observer)(); return true;
}

/** Save exact original native financial/physical owners without ticking.
 * @param path Fresh invocation-owned checkpoint, never an original input.
 */
static void SaveSurfaceCredit(const std::filesystem::path &path)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{PauseMode::Normal});
	AutoRestoreBackup compression(_savegame_format, std::string{"lzma"});
	REQUIRE(SaveWithFilter(std::make_shared<LayerSaveFileSink>(path), false) == SaveLoadResult::Ok);
}

/** Request actual original company credit through synchronized command finance.
 * @param estimate True queries without changing cash/debt; false performs the real bank operation.
 * @return Native result; the bank command already owns both ledger mutations and returns zero expense.
 */
static CommandCost BorrowSurfaceConstructionCredit(bool estimate)
{
	// Match the normal coordinate-free Post path: zero means no map action.
	// INVALID_TILE would make the generic kernel replace the company's last
	// construction location even though borrowing changes no physical site.
	return Command<Commands::IncreaseLoan>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, estimate,
		TileIndex{0}, std::make_tuple(LoanCommand::Amount, Money{1900000}));
}

/** A real starting bank loan, pure refusal and actual additional credit preserve every nonfinancial owner. */
TEST_CASE("Authentic Surface company acquires construction cash only with matching native bank debt", "[.surface-entry-credit]")
{
	const char *source = std::getenv("OTTD_SURFACE_ENTRY_SOURCE"); REQUIRE(source != nullptr);
	const auto paths = GetTestCaseArtifactPaths("space", "surface-entry-credit");
	const auto config = GetTestRuntimeConfig(); bool observed = false; std::exception_ptr failure;
	/** Preserve native assertion failures through normal loaded-game shutdown; no gameplay state is fabricated on failure. */
	std::function<void()> inspect = [&] {
		try {
			AutoRestoreBackup actor(_current_company, _local_company);
			REQUIRE(Company::IsValidID(_current_company));
			const auto *company = Company::Get(_current_company);
			// This action is intentionally bound to the untouched original bank
			// state. Native loan commands are repeatable requests, not idempotent
			// purchase receipts; replaying a borrowed output must not borrow again.
			REQUIRE(company->money == 100000); REQUIRE(company->current_loan == 100000);
			REQUIRE(company->GetMaxLoan() == 5000000);
			const auto expenses = company->yearly_expenses;
			const auto random = _random;
			const auto industries = Industry::GetNumItems(), packets = CargoPacket::GetNumItems();
			SaveSurfaceCredit(paths.saves / "surface-entry-before-credit-440.sav");
			REQUIRE(BorrowSurfaceConstructionCredit(true).Succeeded());
			CHECK(company->money == 100000); CHECK(company->current_loan == 100000);
			CHECK(CmdIncreaseLoan({}, LoanCommand::Amount, Money{5000000}).Failed());
			CHECK(company->money == 100000); CHECK(company->current_loan == 100000);
			SaveSurfaceCredit(paths.saves / "surface-entry-credit-query-440.sav");
			const auto paid = BorrowSurfaceConstructionCredit(false); REQUIRE(paid.Succeeded()); CHECK(paid.GetCost() == 0);
			CHECK(company->money == 2000000); CHECK(company->current_loan == 2000000);
			CHECK(company->yearly_expenses == expenses); CHECK(company->cur_economy.income == 0);
			CHECK(Industry::GetNumItems() == industries); CHECK(CargoPacket::GetNumItems() == packets);
			CHECK(_random.state[0] == random.state[0]); CHECK(_random.state[1] == random.state[1]);
			SaveSurfaceCredit(paths.saves / "surface-entry-credit-440.sav");
			fmt::print("SURFACE_ENTRY_CREDIT_COMPLETE before=100000 borrowed=1900000 cash={} debt={} maximum={} expense=0 native_bank=true\n",
				company->money.base(), company->current_loan.base(), company->GetMaxLoan().base()); observed = true;
		} catch (...) { failure = std::current_exception(); }
		_pause_mode.Set(PauseMode::Normal); _exit_game = true;
	};
	AutoRestoreBackup observer(surface_credit_observer, &inspect);
	IConsole::CmdRegister("surface_entry_credit", ObserveSurfaceCredit);
	const std::string config_arg = config.string();
	std::array<std::string_view, 17> args = {"openttd_surface_supply_test.exe", "-v", "null:ticks=2", "-b", "null", "-s", "null", "-m", "null", "-x", "-X", "-I", "original_dos", "-c", config_arg, "-g", source};
	CHECK(openttd_main(args) == 0); if (failure) std::rethrow_exception(failure); CHECK(observed);
}
