#ifndef DENGINE_ASSET_MANAGER_H
#define DENGINE_ASSET_MANAGER_H

#include "Dengine/Shader.h"
#include "Dengine/Texture.h"

#define MAX_TEXTURE_SIZE 1024

enum TexList
{
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

// TODO(doyle): Switch to hash based lookup
typedef struct AssetManager
{
	Texture textures[256];
	TexAtlas texAtlas[256];
	Shader shaders[256];
	Font font;
} AssetManager;

GLOBAL_VAR AssetManager assetManager;

/* Texture */
Texture *asset_getTexture(AssetManager *assetManager, const enum TexList type);
TexAtlas *asset_getTextureAtlas(AssetManager *assetManager,
                                const enum TexList type);
const i32 asset_loadTextureImage(AssetManager *assetManager,
                                 const char *const path,
                                 const enum TexList type);

/* Shaders */
Shader *asset_getShader(AssetManager *assetManager, const enum ShaderList type);
const i32 asset_loadShaderFiles(AssetManager *assetManager,
                                const char *const vertexPath,
                                const char *const fragmentPath,
                                const enum ShaderList type);

const i32 asset_loadTTFont(AssetManager *assetManager, const char *filePath);

inline i32 asset_getVFontSpacing(FontMetrics metrics)
{
	i32 result =
	    metrics.ascent - metrics.descent + metrics.lineGap;
	return result;
}

#endif
