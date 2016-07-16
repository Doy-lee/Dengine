#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>

#define SBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <STB/stb_truetype.h>

#include "Dengine/Platform.h"
#include "Dengine/AssetManager.h"
#include "Dengine/Debug.h"

//#define WT_RENDER_FONT_FILE
#ifdef WT_RENDER_FONT_FILE
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <STB/stb_image_write.h>
#endif

Texture *asset_getTexture(AssetManager *assetManager, const enum TexList type)
{
	if (type < texlist_count)
		return &assetManager->textures[type];

#ifdef DENGINE_DEBUG
	ASSERT(INVALID_CODE_PATH);
#endif

	return NULL;
}

TexAtlas *asset_getTextureAtlas(AssetManager *assetManager, const enum TexList type)
{
	if (type < texlist_count)
		return &assetManager->texAtlas[type];

#ifdef DENGINE_DEBUG
	ASSERT(INVALID_CODE_PATH);
#endif
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

#ifdef DENGINE_DEBUG
	if (imgWidth != imgHeight)
	{
		printf(
		    "worldTraveller_gameInit() warning: Sprite sheet is not square: "
		    "%dx%dpx\n", imgWidth, imgHeight);
	}
#endif

	if (!image)
	{
		printf("stdbi_load() failed: %s\n", stbi_failure_reason());
		return -1;
	}

	Texture tex = texture_gen(CAST(GLuint)(imgWidth), CAST(GLuint)(imgHeight),
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

#ifdef DENGINE_DEBUG
	ASSERT(INVALID_CODE_PATH);
#endif
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

INTERNAL i32 shaderLoadProgram(Shader *const shader, const GLuint vertexShader,
                               const GLuint fragmentShader)
{
	shader->id = glCreateProgram();
	glAttachShader(shader->id, vertexShader);
	glAttachShader(shader->id, fragmentShader);
	glLinkProgram(shader->id);

	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	GLint success;
	GLchar infoLog[512];
	glGetProgramiv(shader->id, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shader->id, 512, NULL, infoLog);
		printf("glLinkProgram failed: %s\n", infoLog);
		return -1;
	}

	return 0;
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
	i32 result = shaderLoadProgram(&shader, vertexShader, fragmentShader);
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

	/* Initialise Assetmanager Font */
	Font *font = &assetManager->font;
	font->codepointRange = V2i(32, 127);
	v2i codepointRange = font->codepointRange;
	const i32 numGlyphs = codepointRange.y - codepointRange.x;

	GlyphBitmap *glyphBitmaps = PLATFORM_MEM_ALLOC(numGlyphs, GlyphBitmap);
	v2i largestGlyphDimension = V2i(0, 0);

	const f32 targetFontHeight = 15.0f;
	f32 scaleY = stbtt_ScaleForPixelHeight(&fontInfo, targetFontHeight);

	i32 ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

	ascent  = CAST(i32)(ascent * scaleY);
	descent = CAST(i32)(descent * scaleY);
	lineGap = CAST(i32)(lineGap * scaleY);

	font->metrics = CAST(FontMetrics){ascent, descent, lineGap};

	font->charMetrics = PLATFORM_MEM_ALLOC(numGlyphs, CharMetrics);

	/* Use STB_TrueType to generate a series of bitmap characters */
	i32 glyphIndex = 0;
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
		u32 *colorBitmap = PLATFORM_MEM_ALLOC(width * height, u32);
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

		/* Get individual character metrics */
		i32 advance, leftSideBearing;
		stbtt_GetCodepointHMetrics(&fontInfo, codepoint, &advance,
		                           &leftSideBearing);

		advance = CAST(i32)(advance * scaleY);
		leftSideBearing = CAST(i32)(leftSideBearing * scaleY);

		font->charMetrics[glyphIndex] =
		    CAST(CharMetrics){advance, leftSideBearing, NULL,
		                      V2i(xOffset, yOffset), V2i(width, height)};

		/* Store bitmap into intermediate storage */
		stbtt_FreeBitmap(monoBitmap, NULL);

		// TODO(doyle): Dimensions is used twice in font->trueSize and this
		glyphBitmaps[glyphIndex].dimensions = V2i(width, height);
		glyphBitmaps[glyphIndex].codepoint  = codepoint;
		glyphBitmaps[glyphIndex++].pixels   = colorBitmap;

		if (height > largestGlyphDimension.h)
			largestGlyphDimension.h = height;
		if (width > largestGlyphDimension.w)
			largestGlyphDimension.w = width;

#ifdef DENGINE_DEBUG
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
	 * by determining the largest glyph size we have.
	 *
	 * For the amount of glyphs we fit per row, we iterate through them and
	 * write each row of the glyph adjacent to the next until we finish writing
	 * all pixels for the glyphs, then move onto the next set of glyphs.
	 */
	font->maxSize = largestGlyphDimension;
	i32 glyphsPerRow = (MAX_TEXTURE_SIZE / font->maxSize.w);

#ifdef DENGINE_DEBUG
	i32 glyphsPerCol = MAX_TEXTURE_SIZE / font->maxSize.h;
	if ((glyphsPerRow * glyphsPerCol) <= numGlyphs)
	{
		printf(
		    "asset_loadTTFont() warning: The target font height creates a "
		    "glyph sheet that exceeds the available space!");

		ASSERT(INVALID_CODE_PATH);
	}
#endif

	i32 bitmapSize = SQUARED(TARGET_TEXTURE_SIZE) * TARGET_BYTES_PER_PIXEL;
	u32 *fontBitmap = PLATFORM_MEM_ALLOC(bitmapSize, u32);
	const i32 pitch = MAX_TEXTURE_SIZE * TARGET_BYTES_PER_PIXEL;

	// Check value to determine when a row of glyphs is completely printed
	i32 verticalPixelsBlitted = 0;

	i32 startingGlyphIndex = 0;
	i32 glyphsRemaining = numGlyphs;
	i32 glyphsOnCurrRow =
	    (glyphsPerRow < glyphsRemaining) ? glyphsPerRow : glyphsRemaining;

	// TODO(doyle): We copy over the bitmap direct to the font sheet, should we
	// align the baselines up so we don't need to do baseline adjusting at
	// render?
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
#ifdef DENGINE_DEBUG
				ASSERT(activeGlyph.codepoint < ARRAY_COUNT(fontAtlas->texRect));
#endif

				v2 origin =
				    V2(CAST(f32)(glyphIndex * font->maxSize.w), CAST(f32) row);
#if 1
				fontAtlas->texRect[atlasIndex++] =
				    math_getRect(origin, V2(CAST(f32) font->maxSize.w,
				                            CAST(f32) font->maxSize.h));
#else
				v2i fontSize =
				    font->charMetrics[activeGlyph.codepoint - 32].trueSize;
				fontAtlas->texRect[atlasIndex++] = math_getRect(
				    origin, V2(CAST(f32) fontSize.x, CAST(f32) fontSize.y));
#endif
			}

			/* Copy over exactly one row of pixels */
			i32 numPixelsToPad = font->maxSize.w;
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
				numPixelsToPad = font->maxSize.w - activeGlyph.dimensions.w;
			}
			destRow += numPixelsToPad;
		}

		/* A row of glyphs has been fully formed on the atlas */
		if (verticalPixelsBlitted++ >= font->maxSize.h)
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

	Texture tex = texture_gen(MAX_TEXTURE_SIZE, MAX_TEXTURE_SIZE, 4,
	                          CAST(u8 *) fontBitmap);
	assetManager->textures[texlist_font] = tex;

#ifdef WT_RENDER_FONT_FILE
	/* save out a 4 channel image */
	stbi_write_png("out.png", MAX_TEXTURE_SIZE, MAX_TEXTURE_SIZE, 4, fontBitmap,
	               MAX_TEXTURE_SIZE * 4);
#endif
	PLATFORM_MEM_FREE(fontBitmap, bitmapSize);

	font->tex = &assetManager->textures[texlist_font];
	font->atlas = &assetManager->texAtlas[texlist_font];

	for (i32 i = 0; i < numGlyphs; i++)
	{
		i32 glyphBitmapSizeInBytes = glyphBitmaps[i].dimensions.w *
		                             glyphBitmaps[i].dimensions.h * sizeof(u32);
		PLATFORM_MEM_FREE(glyphBitmaps[i].pixels, glyphBitmapSizeInBytes);
	}

	PLATFORM_MEM_FREE(glyphBitmaps, numGlyphs * sizeof(GlyphBitmap));
	platform_closeFileRead(&fontFileRead);

	return 0;
}
