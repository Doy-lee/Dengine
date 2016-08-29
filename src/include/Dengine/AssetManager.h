#ifndef DENGINE_ASSET_MANAGER_H
#define DENGINE_ASSET_MANAGER_H

#include "Dengine/Assets.h"
#include "Dengine/Shader.h"
#include "Dengine/Texture.h"

/* Forward declaration */
typedef struct MemoryArena MemoryArena;
typedef struct PlatformFileRead PlatformFileRead;

enum HashTableType
{
	hashtabletype_unknown,
	hashtabletype_texture,
	hashtabletype_textureAtlas,
	hashtabletype_animation,
	hashtabletype_count,
};

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
	enum HashTableType type;
} HashTable;

typedef struct AssetManager
{
	/* Hash Tables */
	TexAtlas texAtlas[8];
	Texture textures[32];
	Animation anims[1024];

	/* Primitive Array */
	Shader shaders[32];
	AudioVorbis audio[32];
	Font font;
} AssetManager;

#define MAX_TEXTURE_SIZE 1024

/*
 *********************************
 * Texture Asset Managing
 *********************************
 */
Rect asset_getSubTexRect(TexAtlas *atlas, char *key);
Texture *asset_getTex(AssetManager *const assetManager, const char *const key);
TexAtlas *asset_makeTexAtlas(AssetManager *const assetManager,
                             MemoryArena *arena, const char *const key);
TexAtlas *asset_getTexAtlas(AssetManager *const assetManager,
                            const char *const key);
Texture *asset_getAndAllocFreeTexSlot(AssetManager *assetManager,
                                      MemoryArena *arena,
                                      const char *const key);
const i32 asset_loadTextureImage(AssetManager *assetManager, MemoryArena *arena,
                                 const char *const path,
                                 const char *const key);

/*
 *********************************
 * Animation Asset Managing
 *********************************
 */
void asset_addAnimation(AssetManager *assetManager, MemoryArena *arena,
                        char *animName, TexAtlas *atlas, char **subTextureNames,
                        i32 numSubTextures, f32 frameDuration);
Animation *asset_getAnim(AssetManager *assetManager, char *key);

/*
 *********************************
 * Everything else
 *********************************
 */
i32 asset_loadXmlFile(AssetManager *assetManager, MemoryArena *arena,
                      PlatformFileRead *fileRead);
AudioVorbis *asset_getVorbis(AssetManager *assetManager,
                             const enum AudioList type);
Shader *asset_getShader(AssetManager *assetManager, const enum ShaderList type);
const i32 asset_loadVorbis(AssetManager *assetManager, MemoryArena *arena,
                           const char *const path, const enum AudioList type);
const i32 asset_loadShaderFiles(AssetManager *assetManager, MemoryArena *arena,
                                const char *const vertexPath,
                                const char *const fragmentPath,
                                const enum ShaderList type);
const i32 asset_loadTTFont(AssetManager *assetManager, MemoryArena *arena,
                           const char *filePath);
v2 asset_stringDimInPixels(const Font *const font, const char *const string);
void asset_unitTest(MemoryArena *arena);

#endif
