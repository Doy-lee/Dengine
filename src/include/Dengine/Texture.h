#ifndef DENGINE_TEXTURE_H
#define DENGINE_TEXTURE_H

#include <Dengine\OpenGL.h>
#include <Dengine\Common.h>

namespace Dengine
{
class Texture
{
public:
	// Constructor (sets default texture modes)
	Texture();

	// Generates texture from image data
	void generate(const GLuint width, const GLuint height,
	              const u8 *const image);

	// Binds the texture as the current active GL_TEXTURE_2D texture object
	void bind() const;

	inline const GLuint getWidth() const { return this->width; }
	inline const GLuint getHeight() const { return this->height; }
	inline const GLuint getID() const { return this->id; }

private:
	// Holds the ID of the texture object, used for all texture operations to
	// reference to this particlar texture
	GLuint id;

	// Texture image dimensions
	GLuint width;
	GLuint height;

	// Texture Format
	GLuint internalFormat; // Format of texture object
	GLuint imageFormat;    // Format of loaded image

	// Texture configuration
	GLuint wrapS;     // Wrapping mode on S axis
	GLuint wrapT;     // Wrapping mode on T axis

	// Filtering mode if texture pixels < screen pixels
	GLuint filterMinification;
	// Filtering mode if texture pixels > screen pixels
	GLuint filterMagnification;

};
}
#endif
