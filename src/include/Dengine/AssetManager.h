#ifndef DENGINE_ASSET_MANAGER_H
#define DENGINE_ASSET_MANAGER_H

#include <Dengine/Common.h>
#include <Dengine/Shader.h>
#include <Dengine/Texture.h>

#include <iostream>
#include <map>
#include <string>

enum Assets
{
	asset_vertShader,
	asset_fragShader,
	asset_hero,
};

struct AssetManager
{
	Textures textures[256];
	Shaders shaders[256];
};
extern std::map<std::string, Texture> textures;
extern std::map<std::string, Shader> shaders;

/* Texture */
Texture *asset_getTexture(const std::string name);
const i32 asset_loadTextureImage(const std::string path,
                                 const std::string name);

/* Shaders */
Shader *asset_getShader(const std::string name);
const i32 asset_loadShaderFiles(const std::string vertexPath,
                                const std::string fragmentPath,
                                const std::string name);

#endif
