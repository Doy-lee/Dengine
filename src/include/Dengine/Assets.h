#ifndef DENGINE_ASSETS_H
#define DENGINE_ASSETS_H

#include "Dengine/Math.h"
#include "Dengine/Texture.h"

enum TexList
{
	texlist_empty,
	texlist_hero,
	texlist_terrain,
	texlist_font,
	texlist_count,
};

enum ShaderList
{
	shaderlist_sprite,
	shaderlist_count,
};

enum TerrainCoords
{
	terraincoords_ground,
	terraincoords_count,
};

enum HeroCoords
{
	herocoords_idle,
	herocoords_walkA,
	herocoords_walkB,
	herocoords_head,
	herocoords_waveA,
	herocoords_waveB,
	herocoords_count,
};

typedef struct TexAtlas
{
	// TODO(doyle): String hash based lookup
	v4 texRect[128];
} TexAtlas;

// TODO(doyle): We only use the offset and advance metric at the moment, remove?
typedef struct FontMetrics
{
	i32 ascent;
	i32 descent;
	i32 lineGap;
} FontMetrics;

typedef struct CharMetrics
{
	i32 advance;
	i32 leftSideBearing;
	// TODO(doyle): Utilise kerning
	i32 *kerning;
	v2i offset;
	v2i trueSize;
} CharMetrics;

typedef struct Font
{
	TexAtlas *atlas;
	Texture *tex;

	FontMetrics metrics;
	CharMetrics *charMetrics;

	v2i codepointRange;
	v2i maxSize;

} Font;
#endif
