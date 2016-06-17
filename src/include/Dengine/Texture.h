#ifndef DENGINE_TEXTURE_H
#define DENGINE_TEXTURE_H

#include <Dengine/Common.h>
#include <Dengine/OpenGL.h>

struct Texture
{
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
	GLuint wrapS; // Wrapping mode on S axis
	GLuint wrapT; // Wrapping mode on T axis

	// Filtering mode if texture pixels < screen pixels
	GLuint filterMinification;
	// Filtering mode if texture pixels > screen pixels
	GLuint filterMagnification;
};

// Generates texture from image data
Texture genTexture(const GLuint width, const GLuint height,
                   const GLint bytesPerPixel, const u8 *const image);

#endif
