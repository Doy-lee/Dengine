#ifndef DENGINE_ASSET_MANAGER_H
#define DENGINE_ASSET_MANAGER_H

#include "Dengine/Assets.h"
#include "Dengine/Shader.h"
#include "Dengine/Texture.h"

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
const SubTexture asset_getAtlasSubTex(TexAtlas *const atlas,
                                      const char *const key);
Texture *asset_getTex(AssetManager *const assetManager, const char *const key);
TexAtlas *asset_getFreeTexAtlasSlot(AssetManager *const assetManager,
                                    MemoryArena_ *arena, const char *const key,
                                    i32 numSubTex);
TexAtlas *asset_getTexAtlas(AssetManager *const assetManager,
                            const char *const key);
Texture *asset_getFreeTexSlot(AssetManager *const assetManager,
                              MemoryArena_ *const arena, const char *const key);
Texture *asset_loadTextureImage(AssetManager *assetManager, MemoryArena_ *arena,
                                const char *const path, const char *const key);

/*
 *********************************
 * Animation Asset Managing
 *********************************
 */
void asset_addAnimation(AssetManager *const assetManager,
                        MemoryArena_ *const arena, const char *const animName,
                        TexAtlas *const atlas, char **const subTextureNames,
                        const i32 numSubTextures, const f32 frameDuration);
Animation *asset_getAnim(AssetManager *const assetManager,
                         const char *const key);

/*
 *********************************
 * Audio
 *********************************
 */
AudioVorbis *const asset_getVorbis(AssetManager *const assetManager,
                                   const char *const key);
const i32 asset_loadVorbis(AssetManager *assetManager, MemoryArena_ *arena,
                           const char *const path, const char *const key);

/*
 *********************************
 * Everything else
 *********************************
 */
const i32 asset_loadXmlFile(AssetManager *const assetManager,
                            MemoryArena_ *const arena,
                            const PlatformFileRead *const fileRead);

u32 asset_getShader(AssetManager *assetManager, const enum ShaderList type);
const i32 asset_loadShaderFiles(AssetManager *assetManager, MemoryArena_ *arena,
                                const char *const vertexPath,
                                const char *const fragmentPath,
                                const enum ShaderList type);

Font *asset_getFontCreateSizeOnDemand(AssetManager *assetManager,
                                      MemoryArena_ *persistentArena,
                                      MemoryArena_ *transientArena, char *name,
                                      i32 size);
Font *asset_getFont(AssetManager *assetManager, char *name,
                    i32 size);
const i32 asset_loadTTFont(AssetManager *assetManager,
                           MemoryArena_ *persistentArena,
                           MemoryArena_ *transientArena, char *filePath,
                           char *name, i32 targetFontHeight);

const v2 asset_stringDimInPixels(const Font *const font,
                                 const char *const string);

void asset_unitTest(MemoryArena_ *arena);

#endif
