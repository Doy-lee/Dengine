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
	texlist_claude,
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
	audiolist_tackle,
	audiolist_count,
	audiolist_invalid,
};

typedef struct AudioVorbis
{
	enum AudioList type;
	stb_vorbis *file;
	stb_vorbis_info info;

	u32 lengthInSamples;
	f32 lengthInSeconds;

	u8 *data;
	i32 size;
} AudioVorbis;

typedef struct AtlasSubTexture
{
	// NOTE(doyle): Key used to arrive to hash entry
	char *key;
	Rect rect;
	
	// NOTE(doyle): For hashing collisions
	struct AtlasSubTexture *next;
} AtlasSubTexture;

typedef struct TexAtlas
{
	char *key;
	Texture *tex;
	AtlasSubTexture subTex[512];

	struct TexAtlas *next;
} TexAtlas;

typedef struct Animation
{
	union {
		char *name;
		char *key;
	};

	TexAtlas *atlas;
	char **frameList;

	i32 numFrames;
	f32 frameDuration;

	struct Animation *next;
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
	FontMetrics metrics;

	// NOTE(doyle): Array of character's by ASCII value starting from
	// codepointRange and their metrics
	CharMetrics *charMetrics;

	i32 verticalSpacing;

	v2 codepointRange;
	v2 maxSize;

} Font;
#endif
