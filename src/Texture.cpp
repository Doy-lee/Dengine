#include <Dengine\Texture.h>

namespace Dengine
{
Texture::Texture()
: id(0)
, width(0)
, height(0)
, internalFormat(GL_RGBA)
, imageFormat(GL_RGB)
, wrapS(GL_REPEAT)
, wrapT(GL_REPEAT)
, filterMinification(GL_LINEAR)
, filterMagnification(GL_LINEAR)
{
	glGenTextures(1, &this->id);
}

enum BytesPerPixel
{
	Greyscale      = 1,
	GreyscaleAlpha = 2,
	RGB            = 3,
	RGBA           = 4,
};

INTERNAL GLint getGLFormat(BytesPerPixel bytesPerPixel, b32 srgb)
{
	switch (bytesPerPixel)
	{
	case Greyscale:
		return GL_LUMINANCE;
	case GreyscaleAlpha:
		return GL_LUMINANCE_ALPHA;
	case RGB:
		return (srgb ? GL_SRGB : GL_RGB);
	case RGBA:
		return (srgb ? GL_SRGB_ALPHA : GL_RGBA);
	default:
		// TODO(doyle): Invalid
		// std::cout << "getGLFormat() invalid bytesPerPixel: "
		//          << bytesPerPixel << std::endl;
		return GL_LUMINANCE;
	}
}

void Texture::generate(const GLuint width, const GLuint height, const GLint bytesPerPixel,
                       const u8 *const image)
{
	// TODO(doyle): Let us set the parameters gl params as well
	this->width  = width;
	this->height = height;

	glBindTexture(GL_TEXTURE_2D, this->id);

	/* Load image into texture */
	// TODO(doyle) Figure out the gl format
	this->imageFormat = getGLFormat(static_cast<enum BytesPerPixel>(bytesPerPixel), FALSE);

	glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, this->width, this->height, 0,
	             this->imageFormat, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);

	/* Set parameter of currently bound texture */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, this->wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, this->wrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, this->filterMinification);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, this->filterMagnification);

	/* Unbind and clean up */
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::bind() const { glBindTexture(GL_TEXTURE_2D, this->id); }
}
