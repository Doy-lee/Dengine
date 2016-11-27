#ifndef DENGINE_ASSET_MANAGER_H
#define DENGINE_ASSET_MANAGER_H

#include "Dengine/Assets.h"

/* Forward declaration */
typedef struct MemoryArena MemoryArena_;
typedef struct PlatformFileRead PlatformFileRead;

typedef struct AssetManager
{
	/* Hash Tables */
	HashTable texAtlas;
	HashTable textures;
	HashTable anims;
	HashTable audio;

	/* Primitive Array */
	u32 shaders[shaderlist_count];
	FontPack fontPack[4];
	i32 fontPackIndex;

} AssetManager;

#define MAX_TEXTURE_SIZE 1024

void asset_init(AssetManager *assetManager, MemoryArena_ *arena);

/*
 *********************************
 * Texture Operations
 *********************************
 */
const SubTexture asset_atlasGetSubTex(TexAtlas *const atlas,
                                      const char *const key);
Texture *asset_texGet(AssetManager *const assetManager, const char *const key);
TexAtlas *asset_atlasGetFreeSlot(AssetManager *const assetManager,
                                    MemoryArena_ *arena, const char *const key,
                                    i32 numSubTex);
TexAtlas *asset_atlasGet(AssetManager *const assetManager,
                            const char *const key);
Texture *asset_texGetFreeSlot(AssetManager *const assetManager,
                              MemoryArena_ *const arena, const char *const key);
Texture *asset_loadTextureImage(AssetManager *assetManager, MemoryArena_ *arena,
                                const char *const path, const char *const key);

/*
 *********************************
 * Animation Asset Managing
 *********************************
 */
void asset_animAdd(AssetManager *const assetManager,
                        MemoryArena_ *const arena, const char *const animName,
                        TexAtlas *const atlas, char **const subTextureNames,
                        const i32 numSubTextures, const f32 frameDuration);
Animation *asset_animGet(AssetManager *const assetManager,
                         const char *const key);

/*
 *********************************
 * Audio
 *********************************
 */
AudioVorbis *const asset_vorbisGet(AssetManager *const assetManager,
                                   const char *const key);
const i32 asset_vorbisLoad(AssetManager *assetManager, MemoryArena_ *arena,
                           const char *const path, const char *const key);

/*
 *********************************
 * Everything else
 *********************************
 */
const i32 asset_xmlLoad(AssetManager *const assetManager,
                            MemoryArena_ *const arena,
                            const PlatformFileRead *const fileRead);

u32 asset_shaderGet(AssetManager *assetManager, const enum ShaderList type);
const i32 asset_shaderLoad(AssetManager *assetManager, MemoryArena_ *arena,
                                const char *const vertexPath,
                                const char *const fragmentPath,
                                const enum ShaderList type);

Font *asset_fontGetOrCreateOnDemand(AssetManager *assetManager,
                                      MemoryArena_ *persistentArena,
                                      MemoryArena_ *transientArena, char *name,
                                      i32 size);
Font *asset_fontGet(AssetManager *assetManager, char *name,
                    i32 size);
const i32 asset_fontLoadTTF(AssetManager *assetManager,
                           MemoryArena_ *persistentArena,
                           MemoryArena_ *transientArena, char *filePath,
                           char *name, i32 targetFontHeight);

const v2 asset_fontStringDimInPixels(const Font *const font,
                                 const char *const string);

void asset_unitTest(MemoryArena_ *arena);

#endif
