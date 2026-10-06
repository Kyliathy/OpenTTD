/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_save_fixture.h Real save-writer sink and native package/engine transport setup. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Groups 16/16B */

#ifndef TESTS_LAYER_SAVE_FIXTURE_H
#define TESTS_LAYER_SAVE_FIXTURE_H

#include <fstream>
#include "../saveload/saveload_filter.h"
#include "../engine_base.h"
#include "../newgrf_config.h"

/** Repository-owned save sink uses the real save writer without initializing user-wide content search paths. */
class LayerSaveFileSink : public SaveFilter {
	std::ofstream output; ///< Actual evidence bytes written beside the owning plan.
public:
	/** @param path Repository-local evidence destination validated by the fixture. */
	explicit LayerSaveFileSink(const std::filesystem::path &path) : SaveFilter(nullptr), output(path, std::ios::binary)
	{
		this->output.exceptions(std::ios::failbit | std::ios::badbit);
	}
	/** @param bytes Actual savegame writer output. @param length Number of exact bytes to persist. */
	void Write(const uint8_t *bytes, size_t length) override { this->output.write(reinterpret_cast<const char *>(bytes), length); }
	/** Flush actual bytes before the test or external loader inspects the resulting save. */
	void Finish() override { this->output.flush(); }
};

/** Save the fixture's actual activated package descriptor and restore the caller's current content list afterward. */
class LayerSaveContent {
	GRFConfigList original = std::move(_grfconfig); ///< Caller package ownership restored without copying private loader state.
	decltype(_engine_mngr.mappings) original_engines = _engine_mngr.mappings; ///< Caller engine identities restored after the ordinary save.
public:
	/** @param activated Actual public profile fixture descriptors, already checked against the required identity. */
	explicit LayerSaveContent(const GRFConfigList &activated)
	{
		CopyGRFConfigList(_grfconfig, activated, false);
		REQUIRE(_grfconfig.size() == 1);
		_grfconfig.front()->filename = "multilayer_industry.grf"; // Distributed native package name from PrepareMultiLayerProfileContent.
		_engine_mngr.ResetToDefaultMapping(); // A full-world save also needs the normal original-engine mapping published during new-game initialization.
	}
	/** Restore the caller's actual package list after writing the test-owned world. */
	~LayerSaveContent()
	{
		_grfconfig = std::move(this->original);
		_engine_mngr.mappings = std::move(this->original_engines);
	}
};


#endif /* TESTS_LAYER_SAVE_FIXTURE_H */
