/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file mock_spritecache.cpp Mock sprite cache implementation. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, actual window paint fixtures */

#include "../stdafx.h"

#include "../blitter/factory.hpp"
#include "../spritecache.h"
#include "../spritecache_internal.h"
#include "../table/sprites.h"

#include "../safeguards.h"

/**
 * Install valid sprite dimensions for real widget sizing and clipped paint without loading artwork.
 * @param load_index Sprite identity required by the standard UI definitions.
 * @return Whether further standard sprite identities remain to initialize.
 */
static bool MockLoadNextSprite(SpriteID load_index)
{
	UniquePtrSpriteAllocator allocator;
	Sprite *sprite = allocator.Allocate<Sprite>(sizeof(Sprite));
	// The renderer validates dimensions before clipping; a one-pixel header lets empty clips return safely.
	sprite->width = 1;
	sprite->height = 1;
	sprite->x_offs = 0;
	sprite->y_offs = 0;

	bool is_mapgen = IsMapgenSpriteID(load_index);

	SpriteCache *sc = AllocateSpriteCache(load_index);
	sc->file = nullptr;
	sc->file_pos = 0;
	sc->ptr = std::move(allocator.data);
	sc->length = static_cast<uint32_t>(allocator.size);
	sc->lru = 0;
	sc->id = 0;
	sc->type = is_mapgen ? SpriteType::MapGen : SpriteType::Normal;
	sc->warned = false;
	sc->control_flags = {};

	/* Fill with empty sprites up until the default sprite count. */
	return load_index < SPR_OPENTTD_BASE + OPENTTD_SPRITE_COUNT;
}

/** Initialize the null blitter and all standard UI sprite identities without filesystem artwork. */
void MockGfxLoadSprites()
{
	/* Force blitter 'null'. This is necessary for GfxInitSpriteMem() to function. */
	BlitterFactory::SelectBlitter("null");

	GfxInitSpriteMem();

	SpriteID load_index = 0;
	// Every standard identity must resolve for real cargo icons and controls, even when paint is clipped out.
	while (MockLoadNextSprite(load_index)) {
		load_index++;
	}
}
