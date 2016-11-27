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
	shaderlist_default,
	shaderlist_default_no_tex,
	shaderlist_count,
};

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
 * Audio
 *********************************
 */
typedef struct AudioVorbis
{
	union {
		char *key;
		char *name;
	};

	stb_vorbis *file;
	stb_vorbis_info info;

	u32 lengthInSamples;
	f32 lengthInSeconds;

	u8 *data;
	i32 size;
} AudioVorbis;

/*
 *********************************
 * Texture Assets
 *********************************
 */
typedef struct SubTexture
{
	Rect rect;
	v2 offset;
} SubTexture;

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

#define TARGET_TEXTURE_SIZE 1024
#define TARGET_BYTES_PER_PIXEL 4

// TODO(doyle): Look into merging into assets.h file ..
typedef struct Texture
{
	// Holds the ID of the texture object, used for all texture operations to
	// reference to this particlar texture
	u32 id;

	// Texture image dimensions
	u32 width;
	u32 height;

	// Texture Format
	u32 internalFormat; // Format of texture object
	u32 imageFormat;    // Format of loaded image

	// Texture configuration
	u32 wrapS; // Wrapping mode on S axis
	u32 wrapT; // Wrapping mode on T axis

	// Filtering mode if texture pixels < screen pixels
	u32 filterMinification;
	// Filtering mode if texture pixels > screen pixels
	u32 filterMagnification;
} Texture;

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
	// NOTE(doyle): In stb_font we scale the loaded font to this target height
	// and so this is used as a general font "size" notion
	union {
		i32 fontHeight;
		i32 size;
	};

	TexAtlas *atlas;
	FontMetrics metrics;

	// NOTE(doyle): Array of character's by ASCII value starting from
	// codepointRange and their metrics
	CharMetrics *charMetrics;

	i32 verticalSpacing;

	v2 codepointRange;
	v2 maxSize;
} Font;

// NOTE(doyle): A font pack is a singular font at different sizes
typedef struct FontPack
{
	char *name;
	char *filePath;
	Font font[4];
	i32 fontIndex;
} FontPack;
#endif
