/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file mock_fontcache.h Mock font cache implementation definition. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, actual window layout/paint fixtures */

#ifndef MOCK_FONTCACHE_H
#define MOCK_FONTCACHE_H

#include "../fontcache.h"
#include "../string_func.h"

/** Font cache for mocking basic use of fonts. */
class MockFontCache : public FontCache {
	alignas(Sprite) std::array<std::byte, sizeof(Sprite)> empty_glyph{}; ///< Sprite's trailing variable data requires raw storage; zero-sized metadata supplies no raster artwork.
public:
	/** @param fs Font category whose normal height remains available to real widget/text layout. */
	MockFontCache(FontSize fs) : FontCache(fs)
	{
		::new (this->empty_glyph.data()) Sprite; // Start the implicit header object's lifetime without materializing its flexible bitmap array.
		this->height = FontCache::GetDefaultFontHeight(this->fs);
	}

	/** Clear the fixture's immutable cache; no allocation or glyph state needs resetting. */
	void ClearFontCache() override {}
	/** @param glyph Requested fixture glyph. @return Non-null metadata required by real multiline paint; empty clipping supplies no bitmap acceptance. */
	const Sprite *GetGlyph([[maybe_unused]] GlyphID glyph) override { return reinterpret_cast<const Sprite *>(this->empty_glyph.data()); }
	/** @param glyph Requested fixture glyph. @return Deterministic width used by actual text/widget layout. */
	uint GetGlyphWidth([[maybe_unused]] GlyphID glyph) override { return this->height / 2; }
	/** @return False because the zero-raster fixture draws no shadow. */
	bool GetDrawGlyphShadow() override { return false; }
	/** @param key Character to measure. @param allow_fallback Standard interface policy, unused by the fixture. @return Stable matching fixture glyph. */
	GlyphID MapCharToGlyph(char32_t key, [[maybe_unused]] bool allow_fallback = true) override { return key; }
	/** @return Fixture font identity for diagnostics. */
	std::string GetFontName() override { return "mock"; }
	/** @return True to keep the fixture independent of filesystem font discovery. */
	bool IsBuiltInFont() override { return true; }

	/** Initialize only missing real font categories, keeping an already supplied test environment's ownership. */
	static void InitializeFontCaches()
	{
		// Every normal font category participates in widget sizing; never replace an existing caller-owned cache.
		for (FontSize fs : EnumRange(FontSize::End)) {
			if (FontCache::Get(fs) != nullptr) continue;
			FontCache::Register(std::make_unique<MockFontCache>(fs));
		}
	}
};

#endif /* MOCK_FONTCACHE_H */
