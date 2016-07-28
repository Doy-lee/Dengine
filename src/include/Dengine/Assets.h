#ifndef DENGINE_ASSETS_H
#define DENGINE_ASSETS_H

#define STB_VORBIS_HEADER_ONLY
#include <STB/stb_vorbis.c>

#include "Dengine/Common.h"
#include "Dengine/Math.h"

/* Forward Declaration */
typedef struct Texture Texture;

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

enum TerrainRects
{
	terrainrects_ground,
	terrainrects_count,
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

enum AnimList
{
	animlist_hero_idle,
	animlist_hero_walk,
	animlist_hero_wave,
	animlist_hero_battlePose,
	animlist_hero_tackle,
	animlist_terrain,
	animlist_count,
	animlist_invalid,
};

enum AudioList
{
	audiolist_battle,
	audiolist_overworld,
	audiolist_count,
	audiolist_invalid,
};

typedef struct AudioVorbis
{
	stb_vorbis *file;
	stb_vorbis_info info;

	u32 lengthInSamples;
	f32 lengthInSeconds;
} AudioVorbis;

typedef struct TexAtlas
{
	// TODO(doyle): String hash based lookup
	v4 texRect[128];
} TexAtlas;

typedef struct Animation
{
	TexAtlas *atlas;
	i32 *frameIndex;
	i32 numFrames;
	f32 frameDuration;
} Animation;

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
