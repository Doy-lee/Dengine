#ifndef DENGINE_ASSETS_H
#define DENGINE_ASSETS_H

#define STB_VORBIS_HEADER_ONLY
#include <STB/stb_vorbis.c>

#include "Dengine/Common.h"
#include "Dengine/Math.h"

/* Forward Declaration */
typedef struct Texture Texture;

/*
 *********************************
 * XML
 *********************************
 */
// TODO(doyle): We only expose this to for debug.h to print out xml trees
typedef struct XmlAttribute
{
	b32 init;
	char *name;
	char *value;

	struct XmlAttribute *next;
} XmlAttribute;

typedef struct XmlNode
{
	char *name;
	XmlAttribute attribute;

	// NOTE(doyle): Track if node has more children
	b32 isClosed;

	// NOTE(doyle): Direct child/parent
	struct XmlNode *parent;
	struct XmlNode *child;

	// NOTE(doyle): Else all other nodes
	struct XmlNode *next;
} XmlNode;


enum ShaderList
{
	shaderlist_sprite,
	shaderlist_count,
};

/*
 *********************************
 * Audio
 *********************************
 */
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

/*
 *********************************
 * Hash Table
 *********************************
 */
typedef struct HashTableEntry
{
	void *data;
	char *key;

	void *next;
} HashTableEntry;

typedef struct HashTable
{
	HashTableEntry *entries;
	i32 size;
} HashTable;

/*
 *********************************
 * Texture Assets
 *********************************
 */
typedef struct TexAtlas
{
	Texture *tex;

	HashTable subTex;
	i32 numSubTex;
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
} Animation;

/*
 *********************************
 * Font
 *********************************
 */
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
