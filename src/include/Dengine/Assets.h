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

enum HeroRects
{
	herorects_idle,
	herorects_walkA,
	herorects_walkB,
	herorects_head,
	herorects_waveA,
	herorects_waveB,
	herorects_battlePose,
	herorects_castA,
	herorects_castB,
	herorects_castC,
	herorects_count,
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
	v2 offset;
	v2 trueSize;
} CharMetrics;

typedef struct Font
{
	TexAtlas *atlas;
	Texture *tex;

	FontMetrics metrics;
	CharMetrics *charMetrics;

	v2 codepointRange;
	v2 maxSize;

} Font;
#endif
