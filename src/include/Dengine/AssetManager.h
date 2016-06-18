#ifndef DENGINE_ASSET_MANAGER_H
#define DENGINE_ASSET_MANAGER_H

#include <Dengine/Shader.h>
#include <Dengine/Texture.h>

enum TexList
{
	texlist_hero,
	texlist_count,
};

enum ShaderList
{
	shaderlist_sprite,
	shaderlist_count,
};

// TODO(doyle): Switch to hash based lookup
typedef struct AssetManager
{
	Texture textures[256];
	Shader shaders[256];
} AssetManager;

extern AssetManager assetManager;

/* Texture */
Texture *asset_getTexture(const enum TexList type);
const i32 asset_loadTextureImage(const char *const path,
                                 const enum TexList type);

/* Shaders */
Shader *asset_getShader(const enum ShaderList type);
const i32 asset_loadShaderFiles(const char *const vertexPath,
                                const char *const fragmentPath,
                                const enum ShaderList type);

#endif
