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
	~AssetManager();

	/* Texture */
	Texture *getTexture(std::string name);
	i32 loadTextureImage(std::string path, std::string name);

	/* Shaders */
	Shader *getShader(std::string name);
	i32 loadShaderFiles(std::string vertexPath, std::string fragmentPath,
	                    std::string name);

private:
	std::map<std::string, Texture> mTextures;
	std::map<std::string, Shader> mShaders;
};
}
#endif
