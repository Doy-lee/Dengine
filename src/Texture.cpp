#include <Dengine\Texture.h>

namespace Dengine
{
Texture::Texture()
: mId(0)
, mWidth(0)
, mHeight(0)
, mInternalFormat(GL_RGB)
, mImageFormat(GL_RGB)
, mWrapS(GL_REPEAT)
, mWrapT(GL_REPEAT)
, mFilterMinification(GL_LINEAR)
, mFilterMagnification(GL_LINEAR)
{
	glGenTextures(1, &mId);
}

void Texture::generate(GLuint width, GLuint height, u8 *image)
{
	mWidth  = width;
	mHeight = height;

	glBindTexture(GL_TEXTURE_2D, mId);

	/* Load image into texture */
	glTexImage2D(GL_TEXTURE_2D, 0, mInternalFormat, mWidth, mHeight, 0,
	             mImageFormat, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);

	/* Set parameter of currently bound texture */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mWrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mWrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mFilterMinification);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mFilterMagnification);

	/* Unbind and clean up */
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::bind() const { glBindTexture(GL_TEXTURE_2D, mId); }
}
