#include <Dengine/AssetManager.h>

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>

std::map<std::string, Texture> textures;
std::map<std::string, Shader> shaders;

Texture *asset_getTexture(const char *const name)
{
	// NOTE(doyle): Since we're using a map, the count of an object can
	// only be 1 or 0
	if (textures.count(name) == 1)
		return &textures[name];

	return NULL;
}

const i32 asset_loadTextureImage(const char *const path, const char *const name)
{
	/* Open the texture image */
	i32 imgWidth, imgHeight, bytesPerPixel;
	stbi_set_flip_vertically_on_load(TRUE);
	u8 *image =
	    stbi_load(path, &imgWidth, &imgHeight, &bytesPerPixel, 0);

	if (!image)
	{
		printf("stdbi_load() failed: %s\n", stbi_failure_reason());
		return -1;
	}

	Texture tex = genTexture(CAST(GLuint)(imgWidth), CAST(GLuint)(imgHeight),
	                         CAST(GLint)(bytesPerPixel), image);
	glCheckError();
	stbi_image_free(image);

	textures[name] = tex;
	return 0;
}

Shader *asset_getShader(const char *const name)
{
	if (shaders.count(name) == 1)
		return &shaders[name];

	return nullptr;
}

INTERNAL GLuint createShaderFromPath(const char *const path, GLuint shadertype)
{
	PlatformFileReadResult file = {0};

	i32 status = platform_readFileToBuffer(path, &file);
	if (status)
		return status;

	const GLchar *source = CAST(char *)file.buffer;

	GLuint result = glCreateShader(shadertype);
	glShaderSource(result, 1, &source, NULL);
	glCompileShader(result);

	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(result, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(result, 512, NULL, infoLog);
		printf("glCompileShader() failed: %s\n", infoLog);
	}

	platform_closeFileReadResult(&file);

	return result;
}

const i32 asset_loadShaderFiles(const char *const vertexPath,
                                const char *const fragmentPath,
                                const char *const name)
{
	GLuint vertexShader = createShaderFromPath(vertexPath, GL_VERTEX_SHADER);
	GLuint fragmentShader =
	    createShaderFromPath(fragmentPath, GL_FRAGMENT_SHADER);

	Shader shader;
	i32 result = shader_loadProgram(&shader, vertexShader, fragmentShader);
	if (result)
		return result;

	shaders[name] = shader;
	return 0;
}
