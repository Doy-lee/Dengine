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
	AssetManager();
	// TODO(doyle): Unload all resources in memory
	~AssetManager();

	/* Texture */
	const Texture *getTexture(const std::string name);
	const i32 loadTextureImage(const std::string path, const std::string name);

	/* Shaders */
	const Shader *getShader(const std::string name);
	const i32 loadShaderFiles(const std::string vertexPath,
	                          const std::string fragmentPath,
	                          const std::string name);

private:
	std::map<std::string, Texture> mTextures;
	std::map<std::string, Shader> mShaders;
};
}
#endif
