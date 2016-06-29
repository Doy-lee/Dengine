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

// TODO(doyle): Switch to hash based lookup
typedef struct AssetManager
{
	Texture textures[256];
	TexAtlas texAtlas[256];
	Shader shaders[256];

	v2i codepointRange;
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
#endif
