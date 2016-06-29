#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>

#define SBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <STB/stb_truetype.h>

#include "Dengine/Platform.h"
#include "Dengine/AssetManager.h"

Texture *asset_getTexture(AssetManager *assetManager, const enum TexList type)
{
	if (type < texlist_count)
		return &assetManager->textures[type];

	return NULL;
}

TexAtlas *asset_getTextureAtlas(AssetManager *assetManager, const enum TexList type)
{
	if (type < texlist_count)
		return &assetManager->texAtlas[type];

	return NULL;
}

const i32 asset_loadTextureImage(AssetManager *assetManager,
                                 const char *const path, const enum TexList type)
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

	assetManager->textures[type] = tex;
	return 0;
}

Shader *asset_getShader(AssetManager *assetManager, const enum ShaderList type)
{
	if (type < shaderlist_count)
		return &assetManager->shaders[type];

	return NULL;
}

INTERNAL GLuint createShaderFromPath(const char *const path, GLuint shadertype)
{
	PlatformFileRead file = {0};

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

	platform_closeFileRead(&file);

	return result;
}

const i32 asset_loadShaderFiles(AssetManager *assetManager,
                                const char *const vertexPath,
                                const char *const fragmentPath,
                                const enum ShaderList type)
{
	GLuint vertexShader = createShaderFromPath(vertexPath, GL_VERTEX_SHADER);
	GLuint fragmentShader =
	    createShaderFromPath(fragmentPath, GL_FRAGMENT_SHADER);

	Shader shader;
	i32 result = shader_loadProgram(&shader, vertexShader, fragmentShader);
	if (result)
		return result;

	assetManager->shaders[type] = shader;
	return 0;
}

/* Individual glyph bitmap generated from STB used for creating a font sheet */
typedef struct GlyphBitmap
{
	v2i dimensions;
	u32 *pixels;
	i32 codepoint;
} GlyphBitmap;

const i32 asset_loadTTFont(AssetManager *assetManager, const char *filePath)
{
	PlatformFileRead fontFileRead = {0};
	platform_readFileToBuffer(filePath, &fontFileRead);

	stbtt_fontinfo fontInfo = {0};
	stbtt_InitFont(&fontInfo, fontFileRead.buffer,
	               stbtt_GetFontOffsetForIndex(fontFileRead.buffer, 0));

	assetManager->codepointRange = V2i(32, 127);
	v2i codepointRange = assetManager->codepointRange;
	const i32 numGlyphs = codepointRange.y - codepointRange.x;

	i32 glyphIndex = 0;
	GlyphBitmap *glyphBitmaps =
	    CAST(GlyphBitmap *) calloc(numGlyphs, sizeof(GlyphBitmap));
	v2i largestGlyphDimension = V2i(0, 0);

	const f32 targetFontHeight = 64.0f;
	f32 scaleY = stbtt_ScaleForPixelHeight(&fontInfo, targetFontHeight);

	/* Use STB_TrueType to generate a series of bitmap characters */
	for (i32 codepoint = codepointRange.x; codepoint < codepointRange.y;
	     codepoint++)
	{
		// NOTE(doyle): ScaleX if not specified, is then calculated based on the
		// ScaleY component
		i32 width, height, xOffset, yOffset;
		u8 *const monoBitmap =
		    stbtt_GetCodepointBitmap(&fontInfo, 0, scaleY, codepoint, &width,
		                             &height, &xOffset, &yOffset);

		u8 *source       = monoBitmap;
		u32 *colorBitmap = calloc(width * height, sizeof(u32));
		u32 *dest        = colorBitmap;

		// NOTE(doyle): STB generates 1 byte per pixel bitmaps, we use 4bpp, so
		// duplicate the alpha byte (essentially) for all RGBA components
		for (i32 y = 0; y < height; y++)
		{
			for (i32 x = 0; x < width; x++)
			{
				u8 monoByte = *source++;
				*dest++     = (monoByte << 24) | (monoByte << 16) |
				              (monoByte << 8) | (monoByte << 0);
			}
		}

		stbtt_FreeBitmap(monoBitmap, NULL);
		glyphBitmaps[glyphIndex].dimensions = V2i(width, height);
		glyphBitmaps[glyphIndex].codepoint  = codepoint;
		glyphBitmaps[glyphIndex++].pixels   = colorBitmap;

		if (height > largestGlyphDimension.h)
			largestGlyphDimension.h = height;
		if (width > largestGlyphDimension.w)
			largestGlyphDimension.w = width;

#ifdef WT_DEBUG
		if ((largestGlyphDimension.h - CAST(i32)targetFontHeight) >= 50)
		{
			printf(
			    "asset_loadTTFont() warning: The loaded font file has a glyph "
			    "considerably larger than our target .. font packing is "
			    "unoptimal\n");
		}
#endif
	}

	/*
	 * NOTE(doyle): Use rasterised TTF bitmap-characters combine them all into
	 * one bitmap as an atlas. We determine how many glyphs we can fit per row
	 * by determining the largest glyph size we have. Rounding it to the nearest
	 * multiple of 2 and dividing by our target texture size.
	 *
	 * For the amount of glyphs we fit per row, we iterate through them and
	 * write each row of the glyph adjacent to the next until we finish writing
	 * all pixels for the glyphs, then move onto the next set of glyphs.
	 */
	if ((largestGlyphDimension.w & 1) == 1)
		largestGlyphDimension.w += 1;

	if ((largestGlyphDimension.h & 1) == 1)
		largestGlyphDimension.h += 1;

	i32 glyphsPerRow = (MAX_TEXTURE_SIZE / largestGlyphDimension.w) + 1;

#ifdef WT_DEBUG
	i32 glyphsPerCol = MAX_TEXTURE_SIZE / largestGlyphDimension.h;
	if ((glyphsPerRow * glyphsPerCol) <= numGlyphs)
	{
		printf(
		    "asset_loadTTFont() warning: The target font height creates a "
		    "glyph sheet that exceeds the available space!");
		ASSERT(1);
	}
#endif

#if 1
	u32 *fontBitmap = CAST(u32 *)calloc(
	    squared(TARGET_TEXTURE_SIZE) * TARGET_BYTES_PER_PIXEL, sizeof(u32));
	const i32 pitch = MAX_TEXTURE_SIZE * TARGET_BYTES_PER_PIXEL;

	// Check value to determine when a row of glyphs is completely printed
	i32 verticalPixelsBlitted = 0;

	i32 startingGlyphIndex = 0;
	i32 glyphsRemaining = numGlyphs;
	i32 glyphsOnCurrRow = glyphsPerRow;

	i32 atlasIndex = 0;
	for (i32 row = 0; row < MAX_TEXTURE_SIZE; row++)
	{
		u32 *destRow = fontBitmap + (row * MAX_TEXTURE_SIZE);
		for (i32 glyphIndex = 0; glyphIndex < glyphsOnCurrRow;
		     glyphIndex++)
		{
			i32 activeGlyphIndex = startingGlyphIndex + glyphIndex;

			GlyphBitmap activeGlyph = glyphBitmaps[activeGlyphIndex];

			/* Store the location of glyph into atlas */
			if (verticalPixelsBlitted == 0)
			{
				TexAtlas *fontAtlas = &assetManager->texAtlas[texlist_font];
#ifdef WT_DEBUG
				printf("codepoint: %d\n", activeGlyph.codepoint);
				printf("atlasIndex: %d\n", atlasIndex);
				ASSERT(activeGlyph.codepoint < ARRAY_COUNT(fontAtlas->texRect));
#endif

				v2 origin = V2(CAST(f32)(glyphIndex * largestGlyphDimension.w),
				               CAST(f32) row);
				fontAtlas->texRect[atlasIndex++] =
				    getRect(origin, V2(CAST(f32) largestGlyphDimension.w,
				                       CAST(f32) largestGlyphDimension.h));
			}

			/* Copy over exactly one row of pixels */
			i32 numPixelsToPad = largestGlyphDimension.w;
			if (verticalPixelsBlitted < activeGlyph.dimensions.h)
			{
				const i32 srcPitch =
				    activeGlyph.dimensions.w * verticalPixelsBlitted;
				const u32 *src = activeGlyph.pixels + srcPitch;

				const i32 numPixelsToCopy = activeGlyph.dimensions.w;

				for (i32 count = 0; count < numPixelsToCopy; count++)
					*destRow++ = *src++;

				/*
				 * NOTE(doyle): If the glyph is smaller than largest glyph
				 * available size, don't advance src pointer any further
				 * (NULL/mixes up rows), instead just advance the final bitmap
				 * pointer by the remaining distance
				 */
				numPixelsToPad =
				    largestGlyphDimension.w - activeGlyph.dimensions.w;
			}
			destRow += numPixelsToPad;
		}

		/* A row of glyphs has been fully formed on the atlas */
		if (verticalPixelsBlitted++ >= largestGlyphDimension.h)
		{
			verticalPixelsBlitted = 0;
			startingGlyphIndex += glyphsPerRow;

			glyphsRemaining -= glyphsPerRow;

			if (glyphsRemaining <= 0)
				break;
			else if (glyphsRemaining <= glyphsPerRow)
			{
				// NOTE(doyle): This allows us to modify the glyph iterator to
				// prevent over-running of the available glyphs
				glyphsOnCurrRow = glyphsRemaining;
			}
		}
	}

	Texture tex = genTexture(MAX_TEXTURE_SIZE, MAX_TEXTURE_SIZE, 4,
	                         CAST(u8 *) fontBitmap);
	assetManager->textures[texlist_font] = tex;
#else
	i32 letter = 1;
	Texture tex = genTexture(glyphBitmaps[letter].dimensions.w, glyphBitmaps[letter].dimensions.h, 4,
	                         CAST(u8 *)glyphBitmaps[letter].pixels);
	assetManager.textures[texlist_font] = tex;
#endif

	for (i32 i = 0; i < numGlyphs; i++)
		free(glyphBitmaps[i].pixels);

	free(glyphBitmaps);
	platform_closeFileRead(&fontFileRead);

	return 0;
}
