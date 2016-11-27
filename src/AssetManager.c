#define _CRT_SECURE_NO_WARNINGS

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <STB/stb_image.h>

#define SBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <STB/stb_truetype.h>

//#define WT_RENDER_FONT_FILE
#ifdef WT_RENDER_FONT_FILE
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <STB/stb_image_write.h>
#endif

#include <STB/stb_vorbis.c>

#include "Dengine/AssetManager.h"
#include "Dengine/Debug.h"
#include "Dengine/MemoryArena.h"
#include "Dengine/OpenGL.h"
#include "Dengine/Platform.h"

/*
 *********************************
 * Hash Table Operations
 *********************************
 */
INTERNAL HashTableEntry *const getFreeHashSlot(HashTable *const table,
                                               MemoryArena_ *const arena,
                                               const char *const key)
{
	u32 hashIndex = common_getHashIndex(key, table->size);
	HashTableEntry *result = &table->entries[hashIndex];
	if (result->key)
	{
		while (result->next)
		{
			if (common_strcmp(result->key, key) == 0)
			{
				// TODO(doyle): Error hash item already exists
				ASSERT(INVALID_CODE_PATH);
			}
			result = result->next;
		}

		result->next = MEMORY_PUSH_STRUCT(arena, HashTableEntry);
		result       = result->next;

	}

	/* Check if platform_mem_alloc failed(), otherwise always true if hash table
	 * entries initialised, assign key to hash entry */
	if (result)
	{
		// +1 Null terminator
		i32 keyLen  = common_strlen(key) + 1;
		result->key = memory_pushBytes(arena, keyLen * sizeof(char));
		common_strncpy(result->key, key, keyLen);
	}

	return result;
}

INTERNAL HashTableEntry *const getEntryFromHash(HashTable *const table,
                                          const char *const key)
{
	u32 hashIndex = common_getHashIndex(key, table->size);
	HashTableEntry *result = &table->entries[hashIndex];
	if (result->key)
	{
		while (result && common_strcmp(result->key, key) != 0)
			result = result->next;
	}

	return result;
}

/*
 *********************************
 * Texture Operations
 *********************************
 */
INTERNAL SubTexture *getFreeAtlasSubTexSlot(TexAtlas *const atlas,
                                            MemoryArena_ *const arena,
                                            const char *const key)
{
	HashTableEntry *entry = getFreeHashSlot(&atlas->subTex, arena, key);

	if (entry)
	{
		entry->data  = MEMORY_PUSH_STRUCT(arena, SubTexture);
		SubTexture *result = CAST(SubTexture *)entry->data;
		return result;
	}
	else
	{
		return NULL;
	}
}

const SubTexture asset_getAtlasSubTex(TexAtlas *const atlas, const char *const key)
{

	HashTableEntry *entry = getEntryFromHash(&atlas->subTex, key);

	SubTexture result       = {0};
	if (entry)
	{
		result = *(CAST(SubTexture *) entry->data);
		return result;
	}

	DEBUG_LOG("asset_getAtlasSubTex() failed: Sub texture does not exist");
	return result;
}

Texture *asset_getTex(AssetManager *const assetManager, const char *const key)
{
	HashTableEntry *entry = getEntryFromHash(&assetManager->textures, key);

	Texture *result = NULL;
	if (entry) result = CAST(Texture *)entry->data;

	return result;
}

TexAtlas *asset_getFreeTexAtlasSlot(AssetManager *const assetManager,
                                    MemoryArena_ *arena, const char *const key,
                                    i32 numSubTex)
{

	HashTableEntry *const entry =
	    getFreeHashSlot(&assetManager->texAtlas, arena, key);

	if (entry)
	{
		entry->data      = MEMORY_PUSH_STRUCT(arena, TexAtlas);
		TexAtlas *result = CAST(TexAtlas *) entry->data;

		if (result)
		{
			result->subTex.size = numSubTex;
			result->subTex.entries =
			    memory_pushBytes(arena, numSubTex * sizeof(HashTableEntry));

			if (!result->subTex.entries)
			{
				// TODO(doyle): Mem free
				/*
				PLATFORM_MEM_FREE(arena, result, sizeof(TexAtlas));
				result = NULL;
				*/
			}
		}

		return result;
	}
	else
	{
		return NULL;
	}
}

TexAtlas *asset_getTexAtlas(AssetManager *const assetManager,
                            const char *const key)
{

	HashTableEntry *entry = getEntryFromHash(&assetManager->texAtlas, key);

	TexAtlas *result = NULL;
	if (entry) result = CAST(TexAtlas *)entry->data;

	return result;
}


Texture *asset_getFreeTexSlot(AssetManager *const assetManager,
                              MemoryArena_ *const arena, const char *const key)
{

	HashTableEntry *const entry =
	    getFreeHashSlot(&assetManager->textures, arena, key);

	if (entry)
	{
		entry->data     = MEMORY_PUSH_STRUCT(arena, Texture);
		Texture *result = CAST(Texture *) entry->data;
		return result;
	}
	else
	{
		return NULL;
	}
}

Texture *asset_loadTextureImage(AssetManager *assetManager, MemoryArena_ *arena,
                                const char *const path, const char *const key)
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
		    "asset_loadTextureImage() warning: Sprite sheet is not square: "
		    "%dx%dpx\n", imgWidth, imgHeight);
	}
#endif

	if (!image)
	{
		printf("stdbi_load() failed: %s\n", stbi_failure_reason());
		return NULL;
	}

	Texture *result = asset_getFreeTexSlot(assetManager, arena, key);
	*result = texture_gen(CAST(GLuint)(imgWidth), CAST(GLuint)(imgHeight),
	                      CAST(GLint)(bytesPerPixel), image);

	GL_CHECK_ERROR();
	stbi_image_free(image);

	return result;
}

/*
 *********************************
 * Animation Asset Managing
 *********************************
 */
INTERNAL Animation *getFreeAnimationSlot(AssetManager *const assetManager,
                                         MemoryArena_ *const arena,
                                         const char *const key)
{
	HashTableEntry *entry = getFreeHashSlot(&assetManager->anims, arena, key);

	if (entry)
	{
		entry->data       = MEMORY_PUSH_STRUCT(arena, Animation);
		Animation *result = CAST(Animation *) entry->data;
		return result;
	}
	else
	{
		return NULL;
	}
}

void asset_addAnimation(AssetManager *const assetManager,
                        MemoryArena_ *const arena, const char *const animName,
                        TexAtlas *const atlas,
                        char **const subTextureNames,
                        const i32 numSubTextures, const f32 frameDuration)
{
	Animation *anim = getFreeAnimationSlot(assetManager, arena, animName);

	/* Use same animation ptr for name from entry key */
	HashTableEntry *entry = getEntryFromHash(&assetManager->anims, animName);
	anim->name            = entry->key;

	anim->atlas         = atlas;
	anim->frameDuration = frameDuration;
	anim->numFrames     = numSubTextures;

	anim->frameList = memory_pushBytes(arena, numSubTextures * sizeof(char *));
	for (i32 i = 0; i < numSubTextures; i++)
	{
		anim->frameList[i] = subTextureNames[i];
	}

}

Animation *asset_getAnim(AssetManager *const assetManager,
                         const char *const key)
{
	HashTableEntry *entry = getEntryFromHash(&assetManager->anims, key);

	Animation *result = NULL;
	if (entry) result = CAST(Animation *)entry->data;

	return result;
}

/*
 *********************************
 * XML Operations
 *********************************
 */
enum XmlTokenType
{
	xmltokentype_unknown,
	xmltokentype_openArrow,
	xmltokentype_closeArrow,
	xmltokentype_name,
	xmltokentype_value,
	xmltokentype_equals,
	xmltokentype_quotes,
	xmltokentype_backslash,
	xmltokentype_count,
};

typedef struct XmlToken
{
	// TODO(doyle): Dynamic size string in tokens maybe.
	enum XmlTokenType type;
	char string[128];
	i32 len;
} XmlToken;

INTERNAL XmlToken *const tokeniseXmlBuffer(MemoryArena_ *const arena,
                                           const char *const buffer,
                                           const i32 bufferSize,
                                           int *const numTokens)
{
	XmlToken *xmlTokens = memory_pushBytes(arena, 8192 * sizeof(XmlToken));
	i32 tokenIndex      = 0;
	for (i32 i = 0; i < bufferSize; i++)
	{
		char c = (CAST(char *) buffer)[i];
		switch (c)
		{
		case '<':
		case '>':
		case '=':
		case '/':
		{

			enum XmlTokenType type = xmltokentype_unknown;
			if (c == '<')
			{
				type = xmltokentype_openArrow;
			}
			else if (c == '>')
			{
				type = xmltokentype_closeArrow;
			}
			else if (c == '=')
			{
				type = xmltokentype_equals;
			}
			else
			{
				type = xmltokentype_backslash;
			}

			xmlTokens[tokenIndex].type = type;
			xmlTokens[tokenIndex].len  = 1;
			tokenIndex++;
			break;
		}

		case '"':
		{
			xmlTokens[tokenIndex].type = xmltokentype_value;
			for (i32 j = i + 1; j < bufferSize; j++)
			{
				char c = (CAST(char *) buffer)[j];

				if (c == '"')
				{
					break;
				}
				else
				{
					xmlTokens[tokenIndex].string[xmlTokens[tokenIndex].len++] =
					    c;
#ifdef DENGINE_DEBUG
					ASSERT(xmlTokens[tokenIndex].len <
					       ARRAY_COUNT(xmlTokens[tokenIndex].string));
#endif
				}
			}

			// NOTE(doyle): +1 to skip the closing quotes
			i += (xmlTokens[tokenIndex].len + 1);
			tokenIndex++;
			break;
		}

		default:
		{
			if ((c >= 'a' && c <= 'z') || c >= 'A' && c <= 'Z')
			{
				xmlTokens[tokenIndex].type = xmltokentype_name;
				for (i32 j = i; j < bufferSize; j++)
				{
					char c = (CAST(char *) buffer)[j];

					if (c == ' ' || c == '=' || c == '>' || c == '<' ||
					    c == '\\')
					{
						break;
					}
					else
					{
						xmlTokens[tokenIndex]
						    .string[xmlTokens[tokenIndex].len++] = c;
#ifdef DENGINE_DEBUG
						ASSERT(xmlTokens[tokenIndex].len <
						       ARRAY_COUNT(xmlTokens[tokenIndex].string));
#endif
					}
				}
				i += xmlTokens[tokenIndex].len;
				tokenIndex++;
			}
			break;
		}
		}
	}

	// TODO(doyle): Dynamic token allocation
	*numTokens = 8192;
	return xmlTokens;
}

INTERNAL XmlNode *const buildXmlTree(MemoryArena_ *const arena,
                                     XmlToken *const xmlTokens,
                                     const i32 numTokens)
{
	XmlNode *root = MEMORY_PUSH_STRUCT(arena, XmlNode);
	if (!root) return NULL;

	XmlNode *node = root;
	node->parent  = node;

	// NOTE(doyle): Used for when closing a node with many children. We
	// automatically assign the next child after each child close within
	// a group. Hence on the last child, we open another node but the next
	// token indicates the group is closing- we need to set the last child's
	// next reference to NULL
	XmlNode *prevNode = NULL;
	for (i32 i = 0; i < numTokens; i++)
	{
		XmlToken *token = &xmlTokens[i];

		switch (token->type)
		{

		case xmltokentype_openArrow:
		{
			/* Open arrows indicate closing parent node or new node name */
			XmlToken *nextToken = &xmlTokens[++i];
			if (nextToken->type == xmltokentype_backslash)
			{
				nextToken = &xmlTokens[++i];
				if (common_strcmp(nextToken->string, node->parent->name) == 0)
				{
					node->parent->isClosed = TRUE;

					if (prevNode)
					{
						prevNode->next = NULL;
					}

					XmlNode *parent = node->parent;
					// TODO(doyle): Mem free
					//PLATFORM_MEM_FREE(arena, node, sizeof(XmlNode));
					node            = node->parent;
				}
				else
				{
#ifdef DENGINE_DEBUG
					DEBUG_LOG(
					    "Closing xml node name does not match parent name");
#endif
				}
			}
			else if (nextToken->type == xmltokentype_name)
			{
				node->name = nextToken->string;
			}
			else
			{
#ifdef DENGINE_DEBUG
				DEBUG_LOG("Unexpected token type after open arrow");
#endif
			}

			token = nextToken;
			break;
		}

		case xmltokentype_name:
		{
			// TODO(doyle): Store latest attribute pointer so we aren't always
			// chasing the linked list each iteration. Do the same for children
			// node

			/* Xml Attributes are a linked list, get first free entry */
			XmlAttribute *attribute = &node->attribute;
			if (attribute->init)
			{
				while (attribute->next)
					attribute = attribute->next;

				attribute->next = MEMORY_PUSH_STRUCT(arena, XmlAttribute);
				attribute       = attribute->next;
			}

			/* Just plain text is a node attribute name */
			attribute->name = token->string;

			/* Followed by the value */
			token            = &xmlTokens[++i];
			attribute->value = token->string;
			attribute->init  = TRUE;
			break;
		}

		case xmltokentype_closeArrow:
		{
			XmlToken prevToken = xmlTokens[i - 1];
			prevNode = node;

			/* Closed node means we can return to parent */
			if (prevToken.type == xmltokentype_backslash)
			{
				node->isClosed = TRUE;
				node = node->parent;
			}
			
			if (!node->isClosed)
			{
				/* Unclosed node means next fields will be children of node */

				/* If the first child is free allocate, otherwise we have to
				 * iterate through the child's next node(s) */
				if (!node->child)
				{
					// TODO(doyle): Mem alloc error checking
					node->child         = MEMORY_PUSH_STRUCT(arena, XmlNode);
					node->child->parent = node;
					node                = node->child;
				}
				else
				{
					XmlNode *nodeToCheck = node->child;
					while (nodeToCheck->next)
						nodeToCheck = nodeToCheck->next;

					nodeToCheck->next = MEMORY_PUSH_STRUCT(arena, XmlNode);
					nodeToCheck->next->parent = node;
					node                      = nodeToCheck->next;
				}
			}
			break;
		}

		default:
		{
			break;
		}

		}
	}

	return root;
}

INTERNAL void parseXmlTreeToGame(AssetManager *assetManager, MemoryArena_ *arena,
                                 XmlNode *root)
{
	XmlNode *node = root;
	while (node)
	{
		/*
		 *********************************
		 * Branch on node names
		 *********************************
		 */
		if (common_strcmp(node->name, "TextureAtlas") == 0)
		{
			XmlNode *atlasXmlNode = node;
			TexAtlas *atlas  = NULL;
			if (common_strcmp(node->attribute.name, "imagePath") == 0)
			{

				/*
				 **********************************************
				 * Create a texture atlas with imageName as key
				 **********************************************
				 */
				char *imageName = atlasXmlNode->attribute.value;
				i32 numSubTex   = 1024;
				atlas           = asset_getFreeTexAtlasSlot(assetManager, arena,
				                                  imageName, numSubTex);

				if (!atlas)
				{
					DEBUG_LOG(
					    "parseXmlTreeToGame() failed: Could not get free atlas "
					    "entry");
					return;
				}

				/*
				 *************************************************
				 * Load a texture to hash, with imageName as key
				 *************************************************
				 */
				char *dataDir    = "data/textures/WorldTraveller/";
				i32 dataDirLen   = common_strlen(dataDir);
				i32 imageNameLen = common_strlen(imageName);
				i32 totalPathLen = (dataDirLen + imageNameLen) + 1;

				char *imagePath =
				    memory_pushBytes(arena, totalPathLen * sizeof(char));
				common_strncat(imagePath, dataDir, dataDirLen);
				common_strncat(imagePath, imageName, imageNameLen);

				Texture *tex = asset_loadTextureImage(assetManager, arena,
				                                      imagePath, imageName);

				if (!tex)
				{
					DEBUG_LOG("parseXmlTreeToGame() failed: Could not load image");
					// TODO(doyle): Mem free
					//PLATFORM_MEM_FREE(arena, imagePath,
					//                  totalPathLen * sizeof(char));
					return;
				}

				// TODO(doyle): Mem free
				//PLATFORM_MEM_FREE(arena, imagePath,
				//                  totalPathLen * sizeof(char));
				atlas->tex = tex;

				/*
				 *************************************************
				 * Iterate over XML attributes
				 *************************************************
				 */
				XmlNode *atlasChildNode = atlasXmlNode->child;
				while (atlasChildNode)
				{
					if (common_strcmp(atlasChildNode->name, "SubTexture") == 0)
					{
						XmlAttribute *subTexAttrib = &atlasChildNode->attribute;

						char *key   = NULL;
						SubTexture subTex = {0};
						while (subTexAttrib)
						{

							// TODO(doyle): Work around for now in xml reading,
							// reading the last node closing node not being
							// merged to the parent
							if (!subTexAttrib->name) continue;

							if (common_strcmp(subTexAttrib->name, "name") == 0)
							{
								char *value = subTexAttrib->value;
								key         = value;
							}
							else if (common_strcmp(subTexAttrib->name, "x") ==
							         0)
							{
								char *value  = subTexAttrib->value;
								i32 valueLen = common_strlen(value);
								i32 intValue = common_atoi(value, valueLen);

								subTex.rect.min.x = CAST(f32) intValue;
							}
							else if (common_strcmp(subTexAttrib->name, "y") ==
							         0)
							{
								char *value  = subTexAttrib->value;
								i32 valueLen = common_strlen(value);

								i32 intValue = common_atoi(value, valueLen);
								subTex.rect.min.y = CAST(f32) intValue;
							}
							else if (common_strcmp(subTexAttrib->name,
							                       "width") == 0)
							{
								char *value  = subTexAttrib->value;
								i32 valueLen = common_strlen(value);
								i32 intValue = common_atoi(value, valueLen);

								subTex.rect.max.w = CAST(f32) intValue;
							}
							else if (common_strcmp(subTexAttrib->name,
							                       "height") == 0)
							{
								char *value  = subTexAttrib->value;
								i32 valueLen = common_strlen(value);
								i32 intValue = common_atoi(value, valueLen);

								subTex.rect.max.h = CAST(f32) intValue;
							}
							else if (common_strcmp(subTexAttrib->name,
							                       "hand_offset_x") == 0)
							{
								char *value  = subTexAttrib->value;
								i32 valueLen = common_strlen(value);
								i32 intValue = common_atoi(value, valueLen);

								subTex.offset.x = CAST(f32) intValue;
							}
							else if (common_strcmp(subTexAttrib->name,
							                       "hand_offset_y") == 0)
							{
								char *value  = subTexAttrib->value;
								i32 valueLen = common_strlen(value);
								i32 intValue = common_atoi(value, valueLen);

								subTex.offset.y = CAST(f32) intValue;
							}
							else
							{
#ifdef DENGINE_DEBUG
								DEBUG_LOG(
								    "Unsupported xml attribute in SubTexture");
#endif
							}
							subTexAttrib = subTexAttrib->next;
						}

						// TODO(doyle): XML specifies 0,0 top left, we
						// prefer 0,0 bottom right, so offset by size since 0,0
						// is top left and size creates a bounding box below it
						subTex.rect.min.y = 1024 - subTex.rect.min.y;
						subTex.rect.min.y -= subTex.rect.max.h;
						subTex.offset.y = subTex.rect.max.h - subTex.offset.y;

#ifdef DENGINE_DEBUG
						ASSERT(key);
#endif
						SubTexture *subTexInHash =
						    getFreeAtlasSubTexSlot(atlas, arena, key);
						*subTexInHash = subTex;
					}
					else
					{
#ifdef DENGINE_DEBUG
						DEBUG_LOG("Unsupported xml node name not parsed");
#endif
					}

					atlasChildNode = atlasChildNode->next;
				}
			}
			else
			{
#ifdef DENGINE_DEBUG
				DEBUG_LOG("Unsupported xml node");
#endif
			}
		}
		else
		{
#ifdef DENGINE_DEBUG
			DEBUG_LOG("Unsupported xml node name not parsed");
#endif
		}
		node = node->next;
	}
}

INTERNAL void recursiveFreeXmlTree(MemoryArena_ *const arena,
                                   XmlNode *node)
{
	if (!node)
	{
		return;
	} else
	{
		// NOTE(doyle): First attribute is statically allocated, only if there's
		// more attributes do we dynamically allocate
		XmlAttribute *attrib = node->attribute.next;

		while (attrib)
		{
			XmlAttribute *next = attrib->next;

			attrib->name  = NULL;
			attrib->value = NULL;
			// TODO(doyle): Mem free
			// PLATFORM_MEM_FREE(arena, attrib, sizeof(XmlAttribute));
			attrib = next;
		}

		recursiveFreeXmlTree(arena, node->child);
		recursiveFreeXmlTree(arena, node->next);

		node->name     = NULL;
		node->isClosed = FALSE;
		// TODO(doyle): Mem free
		// PLATFORM_MEM_FREE(arena, node, sizeof(XmlNode));
		node = NULL;
	}
}

INTERNAL void freeXmlData(MemoryArena_ *const arena, XmlToken *tokens,
                          const i32 numTokens, XmlNode *tree)
{
	if (tree) recursiveFreeXmlTree(arena, tree);
	// TODO(doyle): Mem free
	// if (tokens) PLATFORM_MEM_FREE(arena, tokens, numTokens * sizeof(XmlToken));
}

/*
 *********************************
 * Everything else
 *********************************
 */
const i32 asset_loadXmlFile(AssetManager *const assetManager,
                            MemoryArena_ *const arena,
                            const PlatformFileRead *const fileRead)
{
	i32 result = 0;
	/* Tokenise buffer */
	i32 numTokens       = 0;
	XmlToken *xmlTokens =
	    tokeniseXmlBuffer(arena, fileRead->buffer, fileRead->size, &numTokens);

	/* Build XML tree from tokens */
	XmlNode *xmlTree = buildXmlTree(arena, xmlTokens, numTokens);

	if (xmlTree)
	{
		/* Parse XML tree to game structures */
		parseXmlTreeToGame(assetManager, arena, xmlTree);
	}
	else
	{
		result = -1;
	}

	/* Free data */
	freeXmlData(arena, xmlTokens, numTokens, xmlTree);

	return result;
}

AudioVorbis *const asset_getVorbis(AssetManager *const assetManager,
                                   const char *const key)
{

	HashTableEntry *entry = getEntryFromHash(&assetManager->audio, key);

	AudioVorbis *result = NULL;
	if (entry) result = CAST(AudioVorbis *)entry->data;

	return result;
}

const i32 asset_loadVorbis(AssetManager *assetManager, MemoryArena_ *arena,
                           const char *const path, const char *const key)
{
	HashTableEntry *entry = getFreeHashSlot(&assetManager->audio, arena, key);
	if (!entry) return -1;

	// TODO(doyle): Remember to free vorbis file if we remove from memory
	PlatformFileRead fileRead = {0};
	platform_readFileToBuffer(arena, path, &fileRead);

	entry->data = MEMORY_PUSH_STRUCT(arena, AudioVorbis);

	i32 error;
	AudioVorbis *audio = CAST(AudioVorbis *) entry->data;
	audio->file =
	    stb_vorbis_open_memory(fileRead.buffer, fileRead.size, &error, NULL);

	if (!audio->file)
	{
		printf("stb_vorbis_open_memory() failed: Error code %d\n", error);
		// TODO(doyle): Mem free
		// platform_closeFileRead(arena, &fileRead);
		stb_vorbis_close(audio->file);
		return 0;
	}

	audio->name            = entry->key;
	audio->info            = stb_vorbis_get_info(audio->file);
	audio->lengthInSamples = stb_vorbis_stream_length_in_samples(audio->file);
	audio->lengthInSeconds = stb_vorbis_stream_length_in_seconds(audio->file);
	audio->data            = CAST(u8 *) fileRead.buffer;
	audio->size            = fileRead.size;

	return 0;
}

INTERNAL GLuint createShaderFromPath(MemoryArena_ *arena, const char *const path,
                                     GLuint shadertype)
{
	PlatformFileRead file = {0};

	// TODO(doyle): Revise platform reads
	i32 status = platform_readFileToBuffer(arena, path, &file);
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

	platform_closeFileRead(arena, &file);

	return result;
}

INTERNAL u32 shaderLoadProgram(const GLuint vertexShader,
                               const GLuint fragmentShader)
{
	u32 result = glCreateProgram();
	glAttachShader(result, vertexShader);
	glAttachShader(result, fragmentShader);
	glLinkProgram(result);
	GL_CHECK_ERROR();

	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
	GL_CHECK_ERROR();

	GLint success;
	GLchar infoLog[512];
	glGetProgramiv(result, GL_LINK_STATUS, &success);
	GL_CHECK_ERROR();
	if (result == 0)
	{
		glGetProgramInfoLog(result, 512, NULL, infoLog);
		printf("glLinkProgram failed: %s\n", infoLog);
		ASSERT(TRUE);
	}
	GL_CHECK_ERROR();

	return result;
}

u32 asset_getShader(AssetManager *assetManager, const enum ShaderList type)
{
	if (type < shaderlist_count) return assetManager->shaders[type];

#ifdef DENGINE_DEBUG
	ASSERT(INVALID_CODE_PATH);
#endif
	return -1;
}

const i32 asset_loadShaderFiles(AssetManager *assetManager, MemoryArena_ *arena,
                                const char *const vertexPath,
                                const char *const fragmentPath,
                                const enum ShaderList type)
{
	GLuint vertexShader = createShaderFromPath(arena, vertexPath, GL_VERTEX_SHADER);
	GLuint fragmentShader =
	    createShaderFromPath(arena, fragmentPath, GL_FRAGMENT_SHADER);

	u32 shaderId = shaderLoadProgram(vertexShader, fragmentShader);
	if (shaderId == 0) return -1;

	assetManager->shaders[type] = shaderId;
	return 0;
}

INTERNAL FontPack *getMatchingFontPack(AssetManager *assetManager,
                                       const char *const name)
{
	FontPack *result = NULL;
	for (i32 i = 0; i < ARRAY_COUNT(assetManager->fontPack); i++)
	{
		FontPack *checkFontPack = &assetManager->fontPack[i];
		if (common_strcmp(checkFontPack->name, name) == 0)
		{
			result = checkFontPack;
			break;
		}
	}

	return result;
}

Font *asset_getFontCreateSizeOnDemand(AssetManager *assetManager,
                                      MemoryArena_ *persistentArena,
                                      MemoryArena_ *transientArena, char *name,
                                      i32 size)
{

	Font *result = asset_getFont(assetManager, name, size);

	if (result == NULL)
	{
		FontPack *pack = getMatchingFontPack(assetManager, name);
		if (pack != NULL)
		{
			for(i32 i = 0; i < pack->fontIndex; i++)
			{
				Font *checkFont = &pack->font[i];
				if (checkFont->size == size)
				{
					result = checkFont;
					break;
				}
			}

			if (result == NULL)
			{
				asset_loadTTFont(assetManager, persistentArena, transientArena,
				                 pack->filePath, name, size);

				result = asset_getFont(assetManager, name, size);
			}
		}
		else
		{
			// TODO(doyle): Logging
		}
	}

	return result;
}

Font *asset_getFont(AssetManager *assetManager, char *name, i32 size)
{
	Font *result = NULL;
	FontPack *pack = getMatchingFontPack(assetManager, name);

	if (pack != NULL) {
		for (i32 j = 0; j < ARRAY_COUNT(pack->font); j++)
		{
			if (pack->font[j].fontHeight == size)
			{
				result = &pack->font[j];
				break;
			}
		}
	}

	return result;
}

/* Individual glyph bitmap generated from STB used for creating a font sheet */
typedef struct GlyphBitmap
{
	v2 dimensions;
	u32 *pixels;
	i32 codepoint;
} GlyphBitmap;

const i32 asset_loadTTFont(AssetManager *assetManager,
                           MemoryArena_ *persistentArena,
                           MemoryArena_ *transientArena, char *filePath,
                           char *name, i32 targetFontHeight)
{
	/*
	 ****************************************
	 * Initialise assetmanager font reference
	 ****************************************
	 */
	FontPack *fontPack = NULL;
	for (i32 i = 0; i < assetManager->fontPackIndex; i++)
	{
		FontPack *checkFontPack = &assetManager->fontPack[i];
		if (common_strcmp(checkFontPack->name, name) == 0)
		{
			fontPack = checkFontPack;
			break;
		}
	}

	if (fontPack == NULL) {
		fontPack = &assetManager->fontPack[assetManager->fontPackIndex++];
		fontPack->name = name;
		fontPack->filePath = filePath;
		ASSERT(assetManager->fontPackIndex <
		       ARRAY_COUNT(assetManager->fontPack));
	}

	Font *font = NULL;
	for (i32 j = 0; j < fontPack->fontIndex; j++)
	{
		Font *checkFont = &fontPack->font[j];
		if (checkFont->fontHeight == targetFontHeight)
		{
			font = &fontPack->font[j];
			break;
		}
	}

	if (font == NULL)
	{
		font = &fontPack->font[fontPack->fontIndex++];
		ASSERT(fontPack->fontIndex < ARRAY_COUNT(fontPack->font));
	}
	else
	{
		return 0;
	}

	TempMemory tempRegion = memory_begin_temporary_region(transientArena);

	PlatformFileRead fontFileRead = {0};
	i32 result =
	    platform_readFileToBuffer(transientArena, filePath, &fontFileRead);
	if (result) return result;

	stbtt_fontinfo fontInfo = {0};
	stbtt_InitFont(&fontInfo, fontFileRead.buffer,
	               stbtt_GetFontOffsetForIndex(fontFileRead.buffer, 0));

	font->codepointRange = V2i(32, 127);
	v2 codepointRange = font->codepointRange;
	const i32 numGlyphs = CAST(i32)(codepointRange.y - codepointRange.x);

	GlyphBitmap *glyphBitmaps =
	    memory_pushBytes(transientArena, numGlyphs * sizeof(GlyphBitmap));
	v2 largestGlyphDimension = V2(0, 0);

	font->fontHeight = targetFontHeight;
	f32 scaleY = stbtt_ScaleForPixelHeight(&fontInfo, (f32)targetFontHeight);

	i32 ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

	ascent  = CAST(i32)(ascent * scaleY);
	descent = CAST(i32)(descent * scaleY);
	lineGap = CAST(i32)(lineGap * scaleY);

	font->metrics = CAST(FontMetrics){ascent, descent, lineGap};

	font->charMetrics =
	    memory_pushBytes(persistentArena, numGlyphs * sizeof(CharMetrics));

	/*
	 ************************************************************
	 * Use STB_TrueType to generate a series of bitmap characters
	 ************************************************************
	 */
	i32 glyphIndex = 0;
	for (i32 codepoint = CAST(i32) codepointRange.x;
	     codepoint < CAST(i32) codepointRange.y; codepoint++)
	{
		// NOTE(doyle): ScaleX if not specified, is then calculated based on the
		// ScaleY component
		i32 width, height, xOffset, yOffset;
		u8 *const monoBitmap =
		    stbtt_GetCodepointBitmap(&fontInfo, 0, scaleY, codepoint, &width,
		                             &height, &xOffset, &yOffset);

		u8 *source       = monoBitmap;
		u32 *colorBitmap =
		    memory_pushBytes(transientArena, width * height * sizeof(u32));
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

		if (height > CAST(f32)largestGlyphDimension.h)
			largestGlyphDimension.h = CAST(f32)height;
		if (width > CAST(f32)largestGlyphDimension.w)
			largestGlyphDimension.w = CAST(f32)width;

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
	i32 glyphsPerRow = (MAX_TEXTURE_SIZE / CAST(i32)font->maxSize.w);

#ifdef DENGINE_DEBUG
	i32 glyphsPerCol = MAX_TEXTURE_SIZE / CAST(i32)font->maxSize.h;
	if ((glyphsPerRow * glyphsPerCol) <= numGlyphs)
	{
		printf(
		    "asset_loadTTFont() warning: The target font height creates a "
		    "glyph sheet that exceeds the available space!");

		ASSERT(INVALID_CODE_PATH);
	}
#endif

	i32 bitmapSize = SQUARED(TARGET_TEXTURE_SIZE) * TARGET_BYTES_PER_PIXEL;
	u32 *fontBitmap =
	    memory_pushBytes(transientArena, bitmapSize * sizeof(u32));
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
	char charToEncode = CAST(char)codepointRange.x;

	i32 numSubTex = numGlyphs;
	TexAtlas *fontAtlas = asset_getFreeTexAtlasSlot(
	    assetManager, persistentArena, "font", numSubTex);

	/*
	 *********************************************************
	 * Load individual glyph bitmap data into one font bitmap
	 *********************************************************
	 */
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
				v2 origin =
				    V2(CAST(f32)(glyphIndex * font->maxSize.w), CAST(f32) row);

				// NOTE(doyle): Since charToEncode starts from 0 and we record
				// all ascii characters, charToEncode represents the character
				// 1:1
				const char key[2] = {charToEncode, 0};
				SubTexture *subTex =
				    getFreeAtlasSubTexSlot(fontAtlas, persistentArena, key);
				subTex->rect = CAST(Rect){origin, font->maxSize};
				charToEncode++;
			}

			/* Copy over exactly one row of pixels */
			i32 numPixelsToPad = CAST(i32)font->maxSize.w;
			if (verticalPixelsBlitted < activeGlyph.dimensions.h)
			{
				const i32 srcPitch =
				    CAST(i32) activeGlyph.dimensions.w * verticalPixelsBlitted;
				const u32 *src = activeGlyph.pixels + srcPitch;

				const i32 numPixelsToCopy = CAST(i32)activeGlyph.dimensions.w;

				for (i32 count = 0; count < numPixelsToCopy; count++)
					*destRow++ = *src++;

				/*
				 * NOTE(doyle): If the glyph is smaller than largest glyph
				 * available size, don't advance src pointer any further
				 * (NULL/mixes up rows), instead just advance the final bitmap
				 * pointer by the remaining distance
				 */
				numPixelsToPad =
				    CAST(i32)(font->maxSize.w - activeGlyph.dimensions.w);
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

	/*
	 *******************************************
	 * Generate and store font bitmap to assets
	 *******************************************
	 */
	Texture *tex = asset_getFreeTexSlot(assetManager, persistentArena, "font");
	*tex         = texture_gen(MAX_TEXTURE_SIZE, MAX_TEXTURE_SIZE, 4,
	                   CAST(u8 *) fontBitmap);

#ifdef WT_RENDER_FONT_FILE
	/* save out a 4 channel image */
	stbi_write_png("out.png", MAX_TEXTURE_SIZE, MAX_TEXTURE_SIZE, 4, fontBitmap,
	               MAX_TEXTURE_SIZE * 4);
#endif

	// TODO(doyle): Mem free
	// PLATFORM_MEM_FREE(arena, fontBitmap, bitmapSize);

	fontAtlas->tex = tex;
	font->atlas   = fontAtlas;

	// NOTE(doyle): Formula derived from STB Font
	font->verticalSpacing =
	    font->metrics.ascent - font->metrics.descent + font->metrics.lineGap;

	for (i32 i = 0; i < numGlyphs; i++)
	{
		i32 glyphBitmapSizeInBytes = CAST(i32) glyphBitmaps[i].dimensions.w *
		                             CAST(i32) glyphBitmaps[i].dimensions.h *
		                             sizeof(u32);
	}

	memory_end_temporary_region(tempRegion);
	return 0;
}

const v2 asset_stringDimInPixels(const Font *const font,
                                 const char *const string)
{
	v2 stringDim = V2(0, 0);
	for (i32 i = 0; i < common_strlen(string); i++)
	{
		i32 codepoint = string[i];

#ifdef DENGINE_DEBUG
		ASSERT(codepoint >= font->codepointRange.x &&
		       codepoint <= font->codepointRange.y)
#endif

		i32 relativeIndex = CAST(i32)(codepoint - font->codepointRange.x);

		v2 charDim  = font->charMetrics[relativeIndex].trueSize;
		stringDim.x += charDim.x;
		stringDim.y = (charDim.y > stringDim.y) ? charDim.y : stringDim.y;
	}

	return stringDim;
}

void asset_unitTest(MemoryArena_ *arena)
{
	PlatformFileRead xmlFileRead = {0};
	i32 result = platform_readFileToBuffer(
	    arena, "data/textures/WorldTraveller/ClaudeSprite.xml", &xmlFileRead);
	if (result)
	{
		DEBUG_LOG(
		    "unitTest() error: Could not load XML file for memory free test");
	}
	else
	{
		/* Tokenise buffer */
		i32 numTokens       = 0;
		XmlToken *xmlTokens = tokeniseXmlBuffer(arena, xmlFileRead.buffer,
		                                        xmlFileRead.size, &numTokens);
		/* Build XML tree from tokens */
		XmlNode *xmlTree = buildXmlTree(arena, xmlTokens, numTokens);
		freeXmlData(arena, xmlTokens, numTokens, xmlTree);
	}
}
