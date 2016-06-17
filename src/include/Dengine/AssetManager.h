#ifndef DENGINE_ASSET_MANAGER_H
#define DENGINE_ASSET_MANAGER_H

#include <Dengine/Platform.h>
#include <Dengine/Shader.h>
#include <Dengine/Texture.h>

#include <map>
#include <string>

extern std::map<std::string, Texture> textures;
extern std::map<std::string, Shader> shaders;

/* Texture */
Texture *asset_getTexture(const char *const name);
const i32 asset_loadTextureImage(const char *const path,
                                 const char *const name);

/* Shaders */
Shader *asset_getShader(const char *const name);
const i32 asset_loadShaderFiles(const char *const vertexPath,
                                const char *const fragmentPath,
                                const char *const name);

#endif
