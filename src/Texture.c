#include "Dengine\Texture.h"

enum BytesPerPixel
{
	bytesPerPixel_Greyscale      = 1,
	bytesPerPixel_GreyscaleAlpha = 2,
	bytesPerPixel_RGB            = 3,
	bytesPerPixel_RGBA           = 4,
};

INTERNAL GLint getGLFormat(i32 bytesPerPixel, b32 srgb)
{
	switch (bytesPerPixel)
	{
	case bytesPerPixel_Greyscale:
		return GL_LUMINANCE;
	case bytesPerPixel_GreyscaleAlpha:
		return GL_LUMINANCE_ALPHA;
	case bytesPerPixel_RGB:
		return (srgb ? GL_SRGB : GL_RGB);
	case bytesPerPixel_RGBA:
		return (srgb ? GL_SRGB_ALPHA : GL_RGBA);
	default:
		// TODO(doyle): Invalid
		// std::cout << "getGLFormat() invalid bytesPerPixel: "
		//          << bytesPerPixel << std::endl;
		return GL_LUMINANCE;
	}
}

Texture texture_gen(const GLuint width, const GLuint height,
                    const GLint bytesPerPixel, const u8 *const image)
{
	// TODO(doyle): Let us set the parameters gl params as well
	glCheckError();
	Texture tex = {0};
	tex.width  = width;
	tex.height = height;
	tex.internalFormat = GL_RGBA;
	tex.wrapS = GL_REPEAT;
	tex.wrapT = GL_REPEAT;
	tex.filterMinification = GL_NEAREST;
	tex.filterMagnification = GL_NEAREST;

	glGenTextures(1, &tex.id);
	glCheckError();

	glBindTexture(GL_TEXTURE_2D, tex.id);
	glCheckError();

	/* Load image into texture */
	// TODO(doyle) Figure out the gl format
	tex.imageFormat = getGLFormat(bytesPerPixel, FALSE);
	ASSERT(tex.imageFormat == GL_RGBA);

	glCheckError();

	glTexImage2D(GL_TEXTURE_2D, 0, tex.internalFormat, tex.width, tex.height, 0,
	             tex.imageFormat, GL_UNSIGNED_BYTE, image);
	glCheckError();

	// TODO(doyle): Not needed for sprites? glGenerateMipmap(GL_TEXTURE_2D);

	/* Set parameter of currently bound texture */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex.wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex.wrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
	                tex.filterMinification);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
	                tex.filterMagnification);
	glCheckError();

	/* Unbind and clean up */
	glBindTexture(GL_TEXTURE_2D, 0);
	glCheckError();

	return tex;
}
