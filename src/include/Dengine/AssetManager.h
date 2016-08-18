#ifndef DENGINE_ASSET_MANAGER_H
#define DENGINE_ASSET_MANAGER_H

#include "Dengine/Assets.h"
#include "Dengine/Shader.h"
#include "Dengine/Texture.h"

/* Forward declaration */
typedef struct MemoryArena MemoryArena;

typedef struct AssetManager
{
	Texture textures[32];
	TexAtlas texAtlas[32];
	Shader shaders[32];
	Animation anims[32];
	AudioVorbis audio[32];
	Font font;
} AssetManager;

#define MAX_TEXTURE_SIZE 1024

AudioVorbis *asset_getVorbis(AssetManager *assetManager,
                             const enum AudioList type);
Texture *asset_getTexture(AssetManager *const assetManager,
                          const enum TexList type);
Shader *asset_getShader(AssetManager *assetManager, const enum ShaderList type);
TexAtlas *asset_getTextureAtlas(AssetManager *assetManager,
                                const enum TexList type);
Animation *asset_getAnim(AssetManager *assetManager, i32 type);

const i32 asset_loadVorbis(AssetManager *assetManager, MemoryArena *arena,
                           const char *const path, const enum AudioList type);
const i32 asset_loadTextureImage(AssetManager *assetManager,
                                 const char *const path,
                                 const enum TexList type);

const i32 asset_loadShaderFiles(AssetManager *assetManager, MemoryArena *arena,
                                const char *const vertexPath,
                                const char *const fragmentPath,
                                const enum ShaderList type);

const i32 asset_loadTTFont(AssetManager *assetManager, MemoryArena *arena,
                           const char *filePath);

void asset_addAnimation(AssetManager *assetManager, MemoryArena *arena,
                        i32 texId, i32 animId, i32 *atlasIndexes, i32 numFrames,
                        f32 frameDuration);

v2 asset_stringDimInPixels(const Font *const font, const char *const string);

#endif
