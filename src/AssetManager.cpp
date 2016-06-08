#include <Dengine\AssetManager.h>

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>
#include <fstream>

namespace Dengine
{

AssetManager::AssetManager() {}
AssetManager::~AssetManager() {}

const Texture *AssetManager::getTexture(const std::string name)
{
	// NOTE(doyle): Since we're using a map, the count of an object can
	// only be 1 or 0
	if (mTextures.count(name) == 1)
		return &mTextures[name];

	return nullptr;
}

const i32 AssetManager::loadTextureImage(const std::string path,
                                         const std::string name)
{
	/* Open the texture image */
	i32 imgWidth, imgHeight, bytesPerPixel;
	stbi_set_flip_vertically_on_load(TRUE);
	u8 *image =
	    stbi_load(path.c_str(), &imgWidth, &imgHeight, &bytesPerPixel, 0);

	if (!image)
	{
		std::cerr << "stdbi_load() failed: " << stbi_failure_reason()
		          << std::endl;
		return -1;
	}

	Texture tex;
	tex.generate(imgWidth, imgHeight, image);
	stbi_image_free(image);

	mTextures[name] = tex;
	return 0;
}

const Shader *AssetManager::getShader(const std::string name)
{
	if (mShaders.count(name) == 1)
		return &mShaders[name];

	return nullptr;
}

INTERNAL std::string stringFromFile(const std::string &filename)
{
	std::ifstream file;
	file.open(filename.c_str(), std::ios::in | std::ios::binary);

	std::string output;
	std::string line;

	if (!file.is_open())
	{
		std::runtime_error(std::string("Failed to open file: ") + filename);
	}
	else
	{
		while (file.good())
		{
			std::getline(file, line);
			output.append(line + "\n");
		}
	}
	file.close();
	return output;
}

INTERNAL GLuint createShaderFromPath(std::string path, GLuint shadertype)
{
	std::string shaderSource = stringFromFile(path);
	const GLchar *source     = shaderSource.c_str();

	GLuint result = glCreateShader(shadertype);
	glShaderSource(result, 1, &source, NULL);
	glCompileShader(result);

	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(result, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(result, 512, NULL, infoLog);
		std::cout << "glCompileShader failed: " << infoLog << std::endl;
	}

	return result;
}

const i32 AssetManager::loadShaderFiles(const std::string vertexPath,
                                        const std::string fragmentPath,
                                        const std::string name)
{

	GLuint vertexShader = createShaderFromPath(vertexPath, GL_VERTEX_SHADER);
	GLuint fragmentShader =
	    createShaderFromPath(fragmentPath, GL_FRAGMENT_SHADER);

	Shader shader;
	i32 result = shader.loadProgram(vertexShader, fragmentShader);
	if (result)
		return result;

	mShaders[name] = shader;
	return 0;
}
}
