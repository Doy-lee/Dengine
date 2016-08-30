#ifndef DENGINE_ASSET_MANAGER_H
#define DENGINE_ASSET_MANAGER_H

#include "Dengine/Assets.h"
#include "Dengine/Shader.h"
#include "Dengine/Texture.h"

/* Forward declaration */
typedef struct MemoryArena MemoryArena;
typedef struct PlatformFileRead PlatformFileRead;

typedef struct AssetManager
{
	/* Hash Tables */
	HashTable texAtlas;
	HashTable textures;
	HashTable anims;

	/* Primitive Array */
	Shader shaders[32];
	AudioVorbis audio[32];
	Font font;
} AssetManager;

#define MAX_TEXTURE_SIZE 1024
/*
 *********************************
 * Texture Operations
 *********************************
 */
Rect *asset_getAtlasSubTex(TexAtlas *const atlas, const char *const key);
Texture *asset_getTex(AssetManager *const assetManager, const char *const key);
TexAtlas *asset_getFreeTexAtlasSlot(AssetManager *const assetManager,
                                    MemoryArena *arena, const char *const key,
                                    i32 numSubTex);
TexAtlas *asset_getTexAtlas(AssetManager *const assetManager,
                            const char *const key);
Texture *asset_getFreeTexSlot(AssetManager *const assetManager,
                              MemoryArena *const arena, const char *const key);
Texture *asset_loadTextureImage(AssetManager *assetManager, MemoryArena *arena,
                                const char *const path, const char *const key);

/*
 *********************************
 * Animation Asset Managing
 *********************************
 */
void asset_addAnimation(AssetManager *assetManager, MemoryArena *arena,
                        char *animName, TexAtlas *atlas, char **subTextureNames,
                        i32 numSubTextures, f32 frameDuration);
Animation *asset_getAnim(AssetManager *const assetManager,
                         const char *const key);

/*
 *********************************
 * Everything else
 *********************************
 */
i32 asset_loadXmlFile(AssetManager *assetManager, MemoryArena *arena,
                      PlatformFileRead *fileRead);

AudioVorbis *asset_getVorbis(AssetManager *assetManager,
                             const enum AudioList type);
const i32 asset_loadVorbis(AssetManager *assetManager, MemoryArena *arena,
                           const char *const path, const enum AudioList type);

Shader *asset_getShader(AssetManager *assetManager, const enum ShaderList type);
const i32 asset_loadShaderFiles(AssetManager *assetManager, MemoryArena *arena,
                                const char *const vertexPath,
                                const char *const fragmentPath,
                                const enum ShaderList type);

const i32 asset_loadTTFont(AssetManager *assetManager, MemoryArena *arena,
                           const char *filePath);
v2 asset_stringDimInPixels(const Font *const font, const char *const string);

void asset_unitTest(MemoryArena *arena);

#endif
