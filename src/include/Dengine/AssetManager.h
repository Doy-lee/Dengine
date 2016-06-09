#ifndef DENGINE_ASSET_MANAGER_H
#define DENGINE_ASSET_MANAGER_H

#include <Dengine\Common.h>
#include <Dengine\Texture.h>
#include <Dengine\Shader.h>

#include <map>
#include <string>
#include <iostream>

namespace Dengine
{
class AssetManager
{
public:
	static std::map<std::string, Texture> textures;
	static std::map<std::string, Shader> shaders;

	/* Texture */
	static Texture *getTexture(const std::string name);
	static const i32 loadTextureImage(const std::string path, const std::string name);

	/* Shaders */
	static Shader *getShader(const std::string name);
	static const i32 loadShaderFiles(const std::string vertexPath,
	                          const std::string fragmentPath,
	                          const std::string name);
};
}
#endif
