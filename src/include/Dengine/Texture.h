#ifndef DENGINE_TEXTURE_H
#define DENGINE_TEXTURE_H

#include <Dengine\OpenGL.h>
#include <Dengine\Common.h>

namespace Dengine
{
class Texture
{
public:
	// Holds the ID of the texture object, used for all texture operations to
	// reference to this particlar texture
	GLuint mId;

	// Texture image dimensions
	GLuint mWidth;
	GLuint mHeight;

	// Texture Format
	GLuint mInternalFormat; // Format of texture object
	GLuint mImageFormat;    // Format of loaded image

	// Texture configuration
	GLuint mWrapS;     // Wrapping mode on S axis
	GLuint mWrapT;     // Wrapping mode on T axis

	// Filtering mode if texture pixels < screen pixels
	GLuint mFilterMinification;
	// Filtering mode if texture pixels > screen pixels
	GLuint mFilterMagnification;

	// Constructor (sets default texture modes)
	Texture();

	// Generates texture from image data
	void generate(GLuint width, GLuint height, unsigned char *data);

	// Binds the texture as the current active GL_TEXTURE_2D texture object
	void bind() const;
};
}
#endif
