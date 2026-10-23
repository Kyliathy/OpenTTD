/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_window_fixture.h Generated-language/screen setup for actual layer control windows. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, portal repair and Group 15 */

#ifndef TESTS_LAYER_WINDOW_FIXTURE_H
#define TESTS_LAYER_WINDOW_FIXTURE_H

#include "../core/backup_type.hpp"
#include "../company_func.h"
#include "../fileio_func.h"
#include "../gfx_func.h"
#include "../language.h"
#include "../strings_func.h"
#include "../window_func.h"
#include "../window_gui.h"

/**
 * Load generated English strings for real widget sizing, using the build directory where CTest runs.
 * Static metadata retains the lifetime required by the language owner's current-language pointer.
 */
inline void LoadGeneratedWindowTestLanguage()
{
	static LanguageMetadata metadata;
	metadata.file = std::filesystem::current_path() / "lang" / "english.lng";
	auto file = FileHandle::Open(metadata.file.string(), "rb");
	REQUIRE(file.has_value());
	REQUIRE(fread(static_cast<LanguagePackHeader *>(&metadata), sizeof(LanguagePackHeader), 1, *file) == 1);
	REQUIRE(metadata.IsValid());
	REQUIRE(ReadLanguagePack(&metadata));
}

/** Window setup restores screen and local-company state and closes UI before the fixture's map disappears. */
class LayerWindowTestUI {
	WindowClass window_class; ///< UI family closed before its test-owned map disappears.
	DrawPixelInfo saved_screen = _screen; ///< Caller screen geometry.
	DrawPixelInfo empty_clip{}; ///< Empty drawing area: exercise control refresh without raster assets.
	AutoRestoreBackup<DrawPixelInfo *> clip; ///< Caller drawing context.
	AutoRestoreBackup<CompanyID> local_company; ///< Caller player identity.

public:
	/** @param window_class Actual UI family under test. @param company Real fixture company's player identity. */
	LayerWindowTestUI(WindowClass window_class, CompanyID company) : window_class(window_class), clip(_cur_dpi, &this->empty_clip), local_company(_local_company, company)
	{
		_screen.width = 1280;
		_screen.height = 720;
		_screen.pitch = 1280;
		// Opening a window marks its area dirty, so allocate the real screen owner's dirty-block grid first.
		ScreenSizeChanged();
		SetupWidgetDimensions();
		NWidgetLeaf::InvalidateDimensionCache();
	}

	/** Close and delete this fixture's window while its real company and map still exist. */
	~LayerWindowTestUI()
	{
		CloseWindowByClass(this->window_class);
		Window::DeleteClosedWindows();
		_screen = this->saved_screen;
		ScreenSizeChanged();
	}
};


#endif /* TESTS_LAYER_WINDOW_FIXTURE_H */
