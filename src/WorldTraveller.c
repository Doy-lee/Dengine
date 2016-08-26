#include "Dengine/WorldTraveller.h"
#include "Dengine/Audio.h"
#include "Dengine/Debug.h"
#include "Dengine/Entity.h"
#include "Dengine/Platform.h"
#include "Dengine/UserInterface.h"

enum State
{
	state_active,
	state_menu,
	state_win,
};

enum EventType
{
	eventtype_null = 0,
	eventtype_start_attack,
	eventtype_end_attack,
	eventtype_start_anim,
	eventtype_end_anim,
	eventtype_entity_died,
	eventtype_count,
};

typedef struct Event
{
	enum EventType type;
	void *data;
} Event;

typedef struct EventQueue
{
	Event queue[1024];
	i32 numEvents;
} EventQueue;

INTERNAL Entity *getHeroEntity(World *world)
{
	Entity *result = &world->entities[entity_getIndex(world, world->heroId)];
	return result;
}

INTERNAL void addGenericMob(MemoryArena *arena, AssetManager *assetManager,
                          World *world, v2 pos)
{
#ifdef DENGINE_DEBUG
	DEBUG_LOG("Mob entity spawned");
#endif

	Entity *hero = &world->entities[entity_getIndex(world, world->heroId)];

	v2 size              = V2(58.0f, 98.0f);
	enum EntityType type = entitytype_mob;
	enum Direction dir   = direction_west;
	Texture *tex         = asset_getTex(assetManager, "ClaudeSprite.png");
	b32 collides         = TRUE;
	Entity *mob = entity_add(arena, world, pos, size, type, dir, tex, collides);

	mob->audioRenderer = PLATFORM_MEM_ALLOC(arena, 1, AudioRenderer);
	mob->audioRenderer->sourceIndex = AUDIO_SOURCE_UNASSIGNED;

	/* Populate mob animation references */
	entity_addAnim(assetManager, mob, "claudeIdle");
	entity_addAnim(assetManager, mob, "claudeRun");
	entity_addAnim(assetManager, mob, "claudeBattleIdle");
	entity_addAnim(assetManager, mob, "claudeAttack");
	entity_setActiveAnim(mob, "claudeIdle");
}

INTERNAL void rendererInit(GameState *state, v2 windowSize)
{
	AssetManager *assetManager = &state->assetManager;
	Renderer *renderer         = &state->renderer;
	renderer->size             = windowSize;
	// NOTE(doyle): Value to map a screen coordinate to NDC coordinate
	renderer->vertexNdcFactor =
	    V2(1.0f / renderer->size.w, 1.0f / renderer->size.h);
	renderer->shader = asset_getShader(assetManager, shaderlist_sprite);
	shader_use(renderer->shader);

	const mat4 projection =
	    mat4_ortho(0.0f, renderer->size.w, 0.0f, renderer->size.h, 0.0f, 1.0f);
	shader_uniformSetMat4fv(renderer->shader, "projection", projection);
	GL_CHECK_ERROR();

	/* Create buffers */
	glGenVertexArrays(1, &renderer->vao);
	glGenBuffers(1, &renderer->vbo);
	GL_CHECK_ERROR();

	/* Bind buffers */
	glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
	glBindVertexArray(renderer->vao);

	/* Configure VAO */
	const GLuint numVertexElements = 4;
	const GLuint vertexSize        = sizeof(v4);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, numVertexElements, GL_FLOAT, GL_FALSE, vertexSize,
	                      (GLvoid *)0);
	GL_CHECK_ERROR();

	/* Unbind */
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	GL_CHECK_ERROR();

#ifdef DENGINE_DEBUG
	DEBUG_LOG("Renderer initialised");
#endif
}

typedef struct XmlAttribute
{
	b32 init;
	char *name;
	char *value;

	struct XmlAttribute *next;
} XmlAttribute;

typedef struct XmlNode
{
	char *name;
	XmlAttribute attribute;

	// NOTE(doyle): Track if node has more children
	b32 isClosed;

	// NOTE(doyle): Direct child/parent
	struct XmlNode *parent;
	struct XmlNode *child;

	// NOTE(doyle): Else all other nodes
	struct XmlNode *next;
} XmlNode;

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

#define DEBUG_RECURSIVE_PRINT_XML_TREE(sig) debug_recursivePrintXmlTree(sig, 1)
INTERNAL void debug_recursivePrintXmlTree(XmlNode *root, i32 levelsDeep)
{
	if (!root)
	{
		return;
	}
	else
	{
		for (i32 i = 0; i < levelsDeep; i++)
		{
			printf("-");
		}

		printf("%s ", root->name);

		XmlAttribute *attribute = &root->attribute;
		printf("| %s = %s", attribute->name, attribute->value);

		while (attribute->next)
		{
			attribute = attribute->next;
			printf("| %s = %04s", attribute->name, attribute->value);
		}
		printf("\n");

		debug_recursivePrintXmlTree(root->child, levelsDeep+1);
		debug_recursivePrintXmlTree(root->next, levelsDeep);
	}
}

INTERNAL XmlToken *tokeniseXmlBuffer(MemoryArena *arena, char *buffer,
                                     i32 bufferSize, int *numTokens)
{
	/*
	 **********************
	 * Tokenise XML Buffer
	 **********************
	 */
	XmlToken *xmlTokens = PLATFORM_MEM_ALLOC(arena, 8192, XmlToken);
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

INTERNAL XmlNode *buildXmlTree(MemoryArena *arena, XmlToken *xmlTokens,
                               i32 numTokens)
{
	XmlNode *root = PLATFORM_MEM_ALLOC(arena, 1, XmlNode);
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
					PLATFORM_MEM_FREE(arena, node, sizeof(XmlNode));

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

				attribute->next = PLATFORM_MEM_ALLOC(arena, 1, XmlAttribute);
				attribute = attribute->next;
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
					node->child         = PLATFORM_MEM_ALLOC(arena, 1, XmlNode);
					node->child->parent = node;
					node                = node->child;
				}
				else
				{
					XmlNode *nodeToCheck = node->child;
					while (nodeToCheck->next)
						nodeToCheck = nodeToCheck->next;

					nodeToCheck->next = PLATFORM_MEM_ALLOC(arena, 1, XmlNode);
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

INTERNAL void parseXmlTreeToGame(AssetManager *assetManager, MemoryArena *arena,
                                 XmlNode *root)
{
	XmlNode *node = root;
	while (node)
	{
		if (common_strcmp(node->name, "TextureAtlas") == 0)
		{
			XmlNode *atlasXmlNode = node;
			TexAtlas *atlasEntry  = NULL;
			if (common_strcmp(node->attribute.name, "imagePath") == 0)
			{
				char *imageName = atlasXmlNode->attribute.value;
				atlasEntry = asset_makeTexAtlas(assetManager, arena, imageName);

				char *dataDir       = "data/textures/WorldTraveller/";
				char imagePath[512] = {0};
				common_strncat(imagePath, dataDir, common_strlen(dataDir));
				common_strncat(imagePath, imageName, common_strlen(imageName));

				asset_loadTextureImage(assetManager, arena, imagePath,
				                       imageName);

				atlasEntry->key = PLATFORM_MEM_ALLOC(
				    arena, common_strlen(imageName) + 1, char);
				common_strncpy(atlasEntry->key, imageName,
				               common_strlen(imageName));

				atlasEntry->tex = asset_getTex(assetManager, imageName);

				XmlNode *atlasChildNode = atlasXmlNode->child;
				while (atlasChildNode)
				{
					if (common_strcmp(atlasChildNode->name, "SubTexture") == 0)
					{
						XmlAttribute *subTextureAttrib =
						    &atlasChildNode->attribute;

						AtlasSubTexture newSubTexEntry = {0};
						while (subTextureAttrib)
						{

							// TODO(doyle): Work around for now in xml reading,
							// reading the last node closing node not being
							// merged to the parent
							if (!subTextureAttrib->name) continue;

							if (common_strcmp(subTextureAttrib->name, "name") ==
							    0)
							{
								char *value        = subTextureAttrib->value;
								newSubTexEntry.key = value;
							}
							else if (common_strcmp(subTextureAttrib->name,
							                       "x") == 0)
							{
								char *value  = subTextureAttrib->value;
								i32 valueLen = common_strlen(value);
								i32 intValue = common_atoi(value, valueLen);

								newSubTexEntry.rect.pos.x = CAST(f32) intValue;
							}
							else if (common_strcmp(subTextureAttrib->name,
							                       "y") == 0)
							{
								char *value  = subTextureAttrib->value;
								i32 valueLen = common_strlen(value);

								i32 intValue = common_atoi(value, valueLen);
								newSubTexEntry.rect.pos.y = CAST(f32) intValue;
							}
							else if (common_strcmp(subTextureAttrib->name,
							                       "width") == 0)
							{
								char *value  = subTextureAttrib->value;
								i32 valueLen = common_strlen(value);
								i32 intValue = common_atoi(value, valueLen);

								newSubTexEntry.rect.size.w = CAST(f32) intValue;
							}
							else if (common_strcmp(subTextureAttrib->name,
							                       "height") == 0)
							{
								char *value  = subTextureAttrib->value;
								i32 valueLen = common_strlen(value);
								i32 intValue = common_atoi(value, valueLen);

								newSubTexEntry.rect.size.h = CAST(f32) intValue;
							}
							else
							{
#ifdef DENGINE_DEBUG
								DEBUG_LOG(
								    "Unsupported xml attribute in SubTexture");
#endif
							}
							subTextureAttrib = subTextureAttrib->next;
						}

						// TODO(doyle): XML specifies 0,0 top left, we
						// prefer 0,0 bottom right, so offset by size since 0,0
						// is top left and size creates a bounding box below it
						newSubTexEntry.rect.pos.y =
						    1024 - newSubTexEntry.rect.pos.y;
						newSubTexEntry.rect.pos.y -= newSubTexEntry.rect.size.h;

#ifdef DENGINE_DEBUG
						ASSERT(newSubTexEntry.key)
#endif

						u32 subTexHashIndex = common_murmurHash2(
						    newSubTexEntry.key,
						    common_strlen(newSubTexEntry.key), 0xDEADBEEF);
						subTexHashIndex =
						    subTexHashIndex % ARRAY_COUNT(atlasEntry->subTex);

						// NOTE(doyle): Hash collision
						AtlasSubTexture *subTexEntry =
						    &atlasEntry->subTex[subTexHashIndex];
						if (subTexEntry->key)
						{
#ifdef DENGINE_DEBUG

							// NOTE(doyle): Two textures have the same access
							// name
							ASSERT(common_strcmp(subTexEntry->key,
							                     newSubTexEntry.key) != 0);
#endif
							while (subTexEntry->next)
								subTexEntry = subTexEntry->next;

							subTexEntry->next =
							    PLATFORM_MEM_ALLOC(arena, 1, AtlasSubTexture);
							subTexEntry = subTexEntry->next;
						}

						*subTexEntry = newSubTexEntry;
						i32 keyLen   = common_strlen(newSubTexEntry.key);

						subTexEntry->key =
						    PLATFORM_MEM_ALLOC(arena, keyLen + 1, char);
						common_strncpy(subTexEntry->key, newSubTexEntry.key,
						               keyLen);
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

INTERNAL void recursiveFreeXmlTree(MemoryArena *arena, XmlNode *node)
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
			PLATFORM_MEM_FREE(arena, attrib, sizeof(XmlAttribute));
			attrib = next;
		}

		recursiveFreeXmlTree(arena, node->child);
		recursiveFreeXmlTree(arena, node->next);

		node->name     = NULL;
		node->isClosed = FALSE;
		PLATFORM_MEM_FREE(arena, node, sizeof(XmlNode));
	}
}

INTERNAL void freeXmlData(MemoryArena *arena, XmlToken *tokens, i32 numTokens,
                          XmlNode *tree)
{
	if (tree) recursiveFreeXmlTree(arena, tree);
	if (tokens) PLATFORM_MEM_FREE(arena, tokens, numTokens * sizeof(XmlToken));
}

INTERNAL void assetInit(GameState *state)
{
	AssetManager *assetManager = &state->assetManager;
	MemoryArena *arena = &state->arena;

	/* Create empty 1x1 4bpp black texture */
	u32 bitmap       = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | (0xFF << 0);
	Texture *tex = asset_getAndAllocFreeTexSlot(assetManager, arena, "nullTex");
	*tex         = texture_gen(1, 1, 4, CAST(u8 *)(&bitmap));

	PlatformFileRead terrainXml = {0};
	i32 result = platform_readFileToBuffer(
	    arena, "data/textures/WorldTraveller/terrain.xml", &terrainXml);
	
	if (result)
	{
		DEBUG_LOG("Failed to read sprite sheet xml");
	}
	else
	{
		/* Tokenise buffer */
		i32 numTokens    = 0;
		XmlToken *xmlTokens = tokeniseXmlBuffer(arena, terrainXml.buffer,
		                                        terrainXml.size, &numTokens);

		/* Build XML tree from tokens */
		XmlNode *xmlTree = buildXmlTree(arena, xmlTokens, numTokens);

		/* Parse XML tree to game structures */
		parseXmlTreeToGame(assetManager, arena, xmlTree);

		/* Free data */
		freeXmlData(arena, xmlTokens, numTokens, xmlTree);
		platform_closeFileRead(arena, &terrainXml);

		TexAtlas *terrainAtlas = asset_getTexAtlas(assetManager, "terrain.png");

		i32 numSubTextures = 1;
		f32 duration = 1.0f;
		char *grassTerrain[1] = {"grass.png"};
		asset_addAnimation(assetManager, arena, "terrainGrass", terrainAtlas,
		                   grassTerrain, numSubTextures, duration);
	}

	PlatformFileRead claudeXml = {0};
	result = platform_readFileToBuffer(
	    arena, "data/textures/WorldTraveller/ClaudeSprite.xml", &claudeXml);

	if (result)
	{
		DEBUG_LOG("Failed to read sprite sheet xml");
	}
	else
	{
		/* Tokenise buffer */
		i32 numTokens    = 0;
		XmlToken *xmlTokens = tokeniseXmlBuffer(arena, claudeXml.buffer,
		                                        claudeXml.size, &numTokens);

		/* Build XML tree from tokens */
		XmlNode *xmlTree = buildXmlTree(arena, xmlTokens, numTokens);

		/* Parse XML tree to game structures */
		parseXmlTreeToGame(assetManager, arena, xmlTree);

		/* Free data */
		freeXmlData(arena, xmlTokens, numTokens, xmlTree);
		platform_closeFileRead(arena, &claudeXml);

		TexAtlas *claudeAtlas =
		    asset_getTexAtlas(assetManager, "ClaudeSprite.png");

		char *claudeIdle[1] = {"ClaudeSprite_Walk_Left_01"};
		f32 duration        = 1.0f;
		i32 numRects        = ARRAY_COUNT(claudeIdle);
		asset_addAnimation(assetManager, arena, "claudeIdle", claudeAtlas,
		                   claudeIdle, numRects, duration);

		// Run animation
		char *claudeRun[6] = {
		    "ClaudeSprite_Run_Left_01", "ClaudeSprite_Run_Left_02",
		    "ClaudeSprite_Run_Left_03", "ClaudeSprite_Run_Left_04",
		    "ClaudeSprite_Run_Left_05", "ClaudeSprite_Run_Left_06"};
		duration = 0.1f;
		numRects = ARRAY_COUNT(claudeRun);
		asset_addAnimation(assetManager, arena, "claudeRun", claudeAtlas,
		                   claudeRun, numRects, duration);

		// Battle Idle animation
		char *claudeBattleIdle[3] = {"ClaudeSprite_BattleIdle_Left_01",
		                             "ClaudeSprite_BattleIdle_Left_02",
		                             "ClaudeSprite_BattleIdle_Left_03"};
		numRects = ARRAY_COUNT(claudeBattleIdle);
		duration = 0.2f;
		asset_addAnimation(assetManager, arena, "claudeBattleIdle", claudeAtlas,
		                   claudeBattleIdle, numRects, duration);

		// Attack Left animation
		char *claudeAttack[6] = {
		    "ClaudeSprite_Attack_Left_01", "ClaudeSprite_Attack_Left_02",
		    "ClaudeSprite_Attack_Left_03", "ClaudeSprite_Attack_Left_04",
		    "ClaudeSprite_Attack_Left_05", "ClaudeSprite_Attack_Left_06"};
		numRects = ARRAY_COUNT(claudeAttack);
		duration = 0.1f;
		asset_addAnimation(assetManager, arena, "claudeAttack", claudeAtlas,
		                   claudeAttack, numRects, duration);

		//  Victory animation
		char *claudeVictory[8] = {
		    "ClaudeSprite_Battle_Victory_01", "ClaudeSprite_Battle_Victory_02",
		    "ClaudeSprite_Battle_Victory_03", "ClaudeSprite_Battle_Victory_04",
		    "ClaudeSprite_Battle_Victory_05", "ClaudeSprite_Battle_Victory_06",
		    "ClaudeSprite_Battle_Victory_07", "ClaudeSprite_Battle_Victory_08"};
		numRects = ARRAY_COUNT(claudeVictory);
		duration = 0.1f;
		asset_addAnimation(assetManager, arena, "claudeVictory", claudeAtlas,
		                   claudeVictory, numRects, duration);

#ifdef DENGINE_DEBUG
		DEBUG_LOG("Animations created");
#endif
	}

	/* Load shaders */
	asset_loadShaderFiles(assetManager, arena, "data/shaders/sprite.vert.glsl",
	                      "data/shaders/sprite.frag.glsl",
	                      shaderlist_sprite);

	result =
	    asset_loadTTFont(assetManager, arena, "C:/Windows/Fonts/Arialbd.ttf");

#ifdef DENGINE_DEBUG
	if (result) DEBUG_LOG("Font loading failed");
	GL_CHECK_ERROR();
	DEBUG_LOG("Assets loaded");
#endif

	/* Load sound */

	char *audioPath =
	    "data/audio/Motoi Sakuraba - Stab the sword of justice.ogg";
	asset_loadVorbis(assetManager, arena, audioPath, audiolist_battle);
	audioPath = "data/audio/Motoi Sakuraba - Field of Exper.ogg";
	asset_loadVorbis(assetManager, arena, audioPath, audiolist_overworld);
	audioPath = "data/audio/nuindependent_hit22.ogg";
	asset_loadVorbis(assetManager, arena, audioPath, audiolist_tackle);

#ifdef DENGINE_DEBUG
	DEBUG_LOG("Sound assets initialised");
#endif
}

INTERNAL void entityInit(GameState *state, v2 windowSize)
{
	AssetManager *assetManager = &state->assetManager;
	MemoryArena *arena = &state->arena;

	/* Init world */
	const i32 targetWorldWidth  = 100 * METERS_TO_PIXEL;
	const i32 targetWorldHeight = 10 * METERS_TO_PIXEL;
#if 0
	v2 worldDimensionInTiles    = V2i(targetWorldWidth / state->tileSize,
	                               targetWorldHeight / state->tileSize);
#else
	v2 worldDimensionInTiles =
	    V2i(CAST(i32)(windowSize.w / state->tileSize) * 2,
	        CAST(i32) windowSize.h / state->tileSize);
#endif

	for (i32 i = 0; i < ARRAY_COUNT(state->world); i++)
	{
		World *const world = &state->world[i];
		world->maxEntities = 16384;
		world->entities = PLATFORM_MEM_ALLOC(arena, world->maxEntities, Entity);
		world->entityIdInBattle =
		    PLATFORM_MEM_ALLOC(arena, world->maxEntities, i32);
		world->numEntitiesInBattle = 0;
		world->bounds =
		    math_getRect(V2(0, 0), v2_scale(worldDimensionInTiles,
		                                    CAST(f32) state->tileSize));
		world->uniqueIdAccumulator = 0;

#if 1
		TexAtlas *const atlas = asset_getTexAtlas(assetManager, "terrain.png");

		for (i32 y = 0; y < 1; y++)
		{
			for (i32 x = 0; x < worldDimensionInTiles.x; x++)
			{
#ifdef DENGINE_DEBUG
				ASSERT(worldDimensionInTiles.x * worldDimensionInTiles.y <
				       world->maxEntities);
#endif
				v2 pos = V2(CAST(f32) x * state->tileSize,
				            CAST(f32) y * state->tileSize);
				v2 size =
				    V2(CAST(f32) state->tileSize, CAST(f32) state->tileSize);
				enum EntityType type = entitytype_tile;
				enum Direction dir = direction_null;
				Texture *tex = asset_getTex(assetManager, "terrain.png");
				b32 collides = FALSE;
				Entity *tile = entity_add(arena, world, pos, size, type, dir,
				                          tex, collides);

				entity_addAnim(assetManager, tile, "terrainGrass");
				entity_setActiveAnim(tile, "terrainGrass");
			}
		}
#endif
	}

	World *const world = &state->world[state->currWorldIndex];
	world->cameraPos = V2(0.0f, 0.0f);

	/* Add world soundscape */
	Renderer *renderer   = &state->renderer;
	v2 size              = V2(10.0f, 10.0f);
	v2 pos               = V2(0, 0);
	enum EntityType type = entitytype_soundscape;
	enum Direction dir   = direction_null;
	Texture *tex         = NULL;
	b32 collides         = FALSE;
	Entity *soundscape =
	    entity_add(arena, world, pos, size, type, dir, tex, collides);

	world->soundscape = soundscape;
	soundscape->audioRenderer = PLATFORM_MEM_ALLOC(arena, 1, AudioRenderer);
	soundscape->audioRenderer->sourceIndex = AUDIO_SOURCE_UNASSIGNED;

	/* Init hero entity */
	size            = V2(58.0f, 98.0f);
	pos             = V2(size.x, CAST(f32) state->tileSize);
	type            = entitytype_hero;
	dir             = direction_east;
	tex             = asset_getTex(assetManager, "ClaudeSprite.png");
	collides        = TRUE;
	Entity *hero =
	    entity_add(arena, world, pos, size, type, dir, tex, collides);

	hero->audioRenderer = PLATFORM_MEM_ALLOC(arena, 1, AudioRenderer);
	hero->audioRenderer->sourceIndex = AUDIO_SOURCE_UNASSIGNED;
	world->heroId                    = hero->id;
	world->cameraFollowingId         = hero->id;

	/* Populate hero animation references */
	entity_addAnim(assetManager, hero, "claudeIdle");
	entity_addAnim(assetManager, hero, "claudeRun");
	entity_addAnim(assetManager, hero, "claudeBattleIdle");
	entity_addAnim(assetManager, hero, "claudeAttack");
	entity_setActiveAnim(hero, "claudeIdle");

	/* Create a NPC */
	pos         = V2(hero->pos.x * 3, CAST(f32) state->tileSize);
	size        = hero->hitboxSize;
	type        = entitytype_npc;
	dir         = direction_null;
	tex         = hero->tex;
	collides    = FALSE;
	Entity *npc = entity_add(arena, world, pos, size, type, dir, tex, collides);

	/* Populate npc animation references */
	entity_addAnim(assetManager, npc, "claudeVictory");
	entity_setActiveAnim(npc, "claudeVictory");

	/* Create a Mob */
	pos = V2(renderer->size.w - (renderer->size.w / 3.0f),
	         CAST(f32) state->tileSize);
	addGenericMob(arena, assetManager, world, pos);

#ifdef DENGINE_DEBUG
	DEBUG_LOG("World populated");
#endif
}

INTERNAL v2 getPosRelativeToRect(Rect rect, v2 offset,
                                 enum RectBaseline baseline)
{
#ifdef DENGINE_DEBUG
	ASSERT(baseline < rectbaseline_count);
#endif
	v2 result = {0};

	v2 posToOffsetFrom = rect.pos;
	switch (baseline)
	{
		case rectbaseline_top:
			posToOffsetFrom.y += (rect.size.h);
			posToOffsetFrom.x += (rect.size.w * 0.5f);
		    break;
		case rectbaseline_topLeft:
			posToOffsetFrom.y += (rect.size.h);
			break;
		case rectbaseline_topRight:
			posToOffsetFrom.y += (rect.size.h);
			posToOffsetFrom.x += (rect.size.w);
			break;
		case rectbaseline_bottom:
			posToOffsetFrom.x += (rect.size.w * 0.5f);
			break;
		case rectbaseline_bottomRight:
			posToOffsetFrom.x += (rect.size.w);
			break;
		case rectbaseline_left:
			posToOffsetFrom.y += (rect.size.h * 0.5f);
			break;
		case rectbaseline_right:
			posToOffsetFrom.x += (rect.size.w);
			posToOffsetFrom.y += (rect.size.h * 0.5f);
			break;

		case rectbaseline_bottomLeft:
		    break;
		default:
#ifdef DENGINE_DEBUG
		    DEBUG_LOG(
		        "getPosRelativeToRect() warning: baseline enum not recognised");
#endif
			break;
	}

	result = v2_add(posToOffsetFrom, offset);
	return result;
}

INTERNAL void unitTest(MemoryArena *arena)
{
	ASSERT(common_atoi("-2", common_strlen("-2")) == -2);
	ASSERT(common_atoi("100", common_strlen("100")) == 100);
	ASSERT(common_atoi("1", common_strlen("1")) == 1);
	ASSERT(common_atoi("954 32", common_strlen("954 32")) == 954);

	ASSERT(common_atoi("", 0) == -1);
	ASSERT(common_atoi(" 32", common_strlen(" 32")) == -1);
	ASSERT(common_atoi("+32", common_strlen("+32")) == 32);
	ASSERT(common_atoi("+ 32", common_strlen("+ 32")) == 0);


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
		i32 memBefore = arena->bytesAllocated;
		i32 numTokens    = 0;
		XmlToken *xmlTokens = tokeniseXmlBuffer(arena, xmlFileRead.buffer,
		                                        xmlFileRead.size, &numTokens);
		/* Build XML tree from tokens */
		XmlNode *xmlTree = buildXmlTree(arena, xmlTokens, numTokens);
		freeXmlData(arena, xmlTokens, numTokens, xmlTree);
		i32 memAfter = arena->bytesAllocated;

		ASSERT(memBefore == memAfter);
	}
}

// TODO(doyle): Remove and implement own random generator!
#include <time.h>
#include <stdlib.h>
void worldTraveller_gameInit(GameState *state, v2 windowSize)
{
#ifdef DENGINE_DEBUG
	unitTest(&state->arena);
#endif

	i32 result = audio_init(&state->audioManager);
	if (result)
	{
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
	}

	state->state              = state_active;
	state->currWorldIndex     = 0;
	state->tileSize           = 70;

	state->uiState.uniqueId   = 1;
	state->uiState.keyEntered = keycode_null;
	state->uiState.keyMod     = keycode_null;
	state->uiState.keyChar    = keycode_null;

	common_strncpy(state->uiState.statWindow.title, "Stat Menu",
	               common_strlen("Stat Menu"));
	state->uiState.statWindow.id                  = 99;
	state->uiState.statWindow.rect.pos            = V2(300, 400);
	state->uiState.statWindow.rect.size           = V2(300, 400);
	state->uiState.statWindow.prevFrameWindowHeld = FALSE;
	state->uiState.statWindow.windowHeld          = FALSE;


	WindowState *debugWindow = &state->uiState.debugWindow;
	common_strncpy(debugWindow->title, "Debug Menu",
	               common_strlen("Debug Menu"));
	debugWindow->id                  = 98;
	debugWindow->numChildUiItems     = 0;
	debugWindow->rect.size           = V2(400, 200);
	debugWindow->rect.pos = V2(windowSize.w - debugWindow->rect.size.w - 10,
	                           windowSize.h - debugWindow->rect.size.h - 10);
	debugWindow->prevFrameWindowHeld = FALSE;
	debugWindow->windowHeld          = FALSE;

	UiItem *audioBtn =
	    (debugWindow->childUiItems) + debugWindow->numChildUiItems++;
	common_strncpy(audioBtn->label, "Toggle Music",
	               common_strlen("Toggle Music"));
	audioBtn->id        = userInterface_generateId(&state->uiState);
	audioBtn->rect.size = V2(100, 50);
	audioBtn->rect.pos = getPosRelativeToRect(debugWindow->rect, V2(10, -65.0f),
	                                          rectbaseline_topLeft);
	audioBtn->type = uitype_button;

	UiItem *debugBtn =
	    (debugWindow->childUiItems) + debugWindow->numChildUiItems++;
	common_strncpy(debugBtn->label, "Toggle Debug",
	               common_strlen("Toggle Debug"));
	debugBtn->id        = userInterface_generateId(&state->uiState);
	debugBtn->rect.size = V2(100, 50);
	debugBtn->rect.pos  = getPosRelativeToRect(audioBtn->rect, V2(25, 0),
	                                          rectbaseline_bottomRight);
	debugBtn->type = uitype_button;

	UiItem *scrollbar =
	    (debugWindow->childUiItems) + debugWindow->numChildUiItems++;
	scrollbar->id     = userInterface_generateId(&state->uiState);

	scrollbar->rect.size = V2(16, debugWindow->rect.size.h);
	scrollbar->rect.pos =
	    getPosRelativeToRect(debugWindow->rect, V2(-scrollbar->rect.size.w, 0),
	                         rectbaseline_bottomRight);
	scrollbar->value    = 0;
	scrollbar->maxValue = 160;
	scrollbar->type = uitype_scrollbar;

	UiItem *textField =
	    (debugWindow->childUiItems) + debugWindow->numChildUiItems++;
	textField->id = userInterface_generateId(&state->uiState);
	textField->rect.size = V2(200, 20);
	textField->rect.pos  = getPosRelativeToRect(
	    audioBtn->rect, V2(0, -textField->rect.size.h - 10),
	    rectbaseline_bottomLeft);

	common_strncpy(textField->string, "Hello world",
	               common_strlen("Hello world"));
	textField->type = uitype_textField;

	state->config.playWorldAudio   = FALSE;
	state->config.showDebugDisplay = TRUE;

	assetInit(state);
	rendererInit(state, windowSize);
	entityInit(state, windowSize);

	srand(CAST(u32)(time(NULL)));
}

INTERNAL inline v4 getEntityScreenRect(Entity entity)
{
	v4 result = math_getRect(entity.pos, entity.hitboxSize);
	return result;
}

enum ReadKeyType
{
	readkeytype_oneShot,
	readkeytype_delayedRepeat,
	readkeytype_repeat,
	readkeytype_count,
};

#define KEY_DELAY_NONE 0.0f

INTERNAL b32 getKeyStatus(KeyState *key, enum ReadKeyType readType,
                          f32 delayInterval, f32 dt)
{

	if (!key->endedDown) return FALSE;

	switch(readType)
	{
	case readkeytype_oneShot:
	{
		if (key->newHalfTransitionCount > key->oldHalfTransitionCount)
			return TRUE;
		break;
	}
	case readkeytype_repeat:
	case readkeytype_delayedRepeat:
	{
		if (key->newHalfTransitionCount > key->oldHalfTransitionCount)
		{
			if (readType == readkeytype_delayedRepeat)
			{
				// TODO(doyle): Let user set arbitrary delay after initial input
				key->delayInterval = 2 * delayInterval;
			}
			else
			{
				key->delayInterval = delayInterval;
			}
			return TRUE;
		}
		else if (key->delayInterval <= 0.0f)
		{
			key->delayInterval = delayInterval;
			return TRUE;
		}
		else
		{
			key->delayInterval -= dt;
		}
		break;
	}
	default:
#ifdef DENGINE_DEBUG
		DEBUG_LOG("getKeyStatus() error: Invalid ReadKeyType enum");
		ASSERT(INVALID_CODE_PATH);
#endif
		break;
	}

	return FALSE;
}

INTERNAL Rect createWorldBoundedCamera(World *world, v2 size)
{
	Rect camera = {world->cameraPos, size};
	// NOTE(doyle): Lock camera if it passes the bounds of the world
	if (camera.pos.x <= world->bounds.x)
		camera.pos.x = world->bounds.x;

	// TODO(doyle): Allow Y panning when we need it
	f32 cameraTopBoundInPixels = camera.pos.y + camera.size.h;
	if (cameraTopBoundInPixels >= world->bounds.w)
		camera.pos.y = (world->bounds.w - camera.size.h);

	f32 cameraRightBoundInPixels = camera.pos.x + camera.size.w;
	if (cameraRightBoundInPixels >= world->bounds.z)
		camera.pos.x = (world->bounds.z - camera.size.w);

	if (camera.pos.y <= world->bounds.y) camera.pos.y = world->bounds.y;
	return camera;
}

#define ENTITY_IN_BATTLE TRUE
#define ENTITY_NOT_IN_BATTLE FALSE
#define ENTITY_NULL_ID -1
INTERNAL i32 findBestEntityToAttack(World *world, Entity attacker)
{
#ifdef DENGINE_DEBUG
	ASSERT(world);
	ASSERT(attacker.type == entitytype_hero || attacker.type == entitytype_mob);
#endif
	i32 result = 0;

	// TODO(doyle): If attacker is mob- retrieve hero entity id directly, change
	// when we have party members!
	if (attacker.type == entitytype_mob)
	{
		Entity hero = world->entities[entity_getIndex(world, world->heroId)];

		if (hero.state == entitystate_dead) result = ENTITY_NULL_ID;
		else result = hero.id;

		return result;
	}

	/* Attacker is hero */
	Entity hero = attacker;
	for (i32 i = world->maxEntities; i >= 0; i--)
	{
		Entity targetEntity = world->entities[i];
		if (hero.id == targetEntity.id) continue;
		if (world->entityIdInBattle[targetEntity.id] == ENTITY_IN_BATTLE)
		{
			result = targetEntity.id;
			return result;
		}
	}
	
	// NOTE(doyle): Not all "battling" entities have been enumerated yet in the
	// update loop, guard against when using function
	return ENTITY_NULL_ID;
}

INTERNAL inline void updateWorldBattleEntities(World *world, Entity *entity,
                                               b32 isInBattle)
{
#ifdef DENGINE_DEBUG
	ASSERT(isInBattle == ENTITY_IN_BATTLE ||
	       isInBattle == ENTITY_NOT_IN_BATTLE);
	ASSERT(world && entity);
#endif
	world->entityIdInBattle[entity->id] = isInBattle;

	if (isInBattle)
	{
		world->numEntitiesInBattle++;
	}
	else
	{
		world->numEntitiesInBattle--;
	}

#ifdef DENGINE_DEBUG
	ASSERT(world->numEntitiesInBattle >= 0);
#endif
}

// TODO(doyle): Function too vague
INTERNAL inline void resetEntityState(World *world, Entity *entity)
{
	updateWorldBattleEntities(world, entity, ENTITY_NOT_IN_BATTLE);
	entity_setActiveAnim(entity, "claudeIdle");
	entity->stats->busyDuration     = 0;
	entity->stats->actionTimer      = entity->stats->actionRate;
	entity->stats->queuedAttack     = entityattack_invalid;
	entity->stats->entityIdToAttack = ENTITY_NULL_ID;
}

INTERNAL registerEvent(EventQueue *eventQueue, enum EventType type, void *data)
{
#ifdef DENGINE_DEBUG
	ASSERT(eventQueue && type < eventtype_count);
	ASSERT(eventQueue->numEvents+1 < ARRAY_COUNT(eventQueue->queue));
#endif
	i32 currIndex = eventQueue->numEvents++;
	eventQueue->queue[currIndex].type = type;
	eventQueue->queue[currIndex].data = data;
}

INTERNAL void entityStateSwitch(EventQueue *eventQueue, World *world,
                                Entity *entity, enum EntityState newState)
{
#ifdef DENGINE_DEBUG
	ASSERT(world && entity)
	ASSERT(entity->type == entitytype_mob || entity->type == entitytype_hero)
#endif
	if (entity->state == newState) return;

	switch(entity->state)
	{
	case entitystate_idle:
		switch (newState)
		{
		case entitystate_battle:
			updateWorldBattleEntities(world, entity, ENTITY_IN_BATTLE);
			entity->stats->entityIdToAttack =
			    findBestEntityToAttack(world, *entity);
			break;

		// TODO(doyle): Corner case- if move out of range and entity has
		// switched to idle mode, we reach the attacker entity and they continue
		// attacking it since there's no check before attack if entity is idle
		// or not (i.e. has moved out of frame last frame).
		case entitystate_dead:
			registerEvent(eventQueue, eventtype_entity_died, CAST(void *)entity);
			entity_setActiveAnim(entity, "claudeIdle");
			entity->stats->busyDuration     = 0;
			entity->stats->actionTimer      = entity->stats->actionRate;
			entity->stats->queuedAttack     = entityattack_invalid;
			entity->stats->entityIdToAttack = ENTITY_NULL_ID;
			break;

		case entitystate_attack:
		default:
#ifdef DENGINE_DEBUG
			ASSERT(INVALID_CODE_PATH);
#endif
			break;
		}
		break;
	case entitystate_battle:
		switch (newState)
		{
		case entitystate_attack:
		{
			break;
		}
		case entitystate_idle:
			resetEntityState(world, entity);
			break;
		case entitystate_dead:
			registerEvent(eventQueue, eventtype_entity_died, CAST(void *)entity);
			resetEntityState(world, entity);
			break;
		default:
#ifdef DENGINE_DEBUG
			ASSERT(INVALID_CODE_PATH);
#endif
			break;
		}
		break;
	case entitystate_attack:
		switch (newState)
		{
		case entitystate_battle:
			entity_setActiveAnim(entity, "claudeBattleIdle");
			entity->stats->actionTimer  = entity->stats->actionRate;
			entity->stats->busyDuration = 0;
			break;
		// NOTE(doyle): Entity has been forced out of an attack (out of range)
		case entitystate_idle:
			resetEntityState(world, entity);
			break;
		case entitystate_dead:
			registerEvent(eventQueue, eventtype_entity_died, CAST(void *)entity);
			resetEntityState(world, entity);
			break;
		default:
#ifdef DENGINE_DEBUG
			ASSERT(INVALID_CODE_PATH);
#endif
			break;
		}
		break;
	case entitystate_dead:
		switch (newState)
		{
		case entitystate_idle:
		case entitystate_battle:
		case entitystate_attack:
		default:
#ifdef DENGINE_DEBUG
			ASSERT(INVALID_CODE_PATH);
#endif
			break;
		}
		break;
	default:
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
		break;
	}

	entity->state = newState;
}

typedef struct AttackSpec
{
	Entity *attacker;
	Entity *defender;
	i32 damage;
} AttackSpec;

INTERNAL void beginAttack(EventQueue *eventQueue, World *world,
                          Entity *attacker)
{
#ifdef DENGINE_DEBUG
	ASSERT(attacker->stats->entityIdToAttack != ENTITY_NULL_ID);
	ASSERT(attacker->state == entitystate_battle);
#endif

	entityStateSwitch(eventQueue, world, attacker, entitystate_attack);
	switch (attacker->stats->queuedAttack)
	{
	case entityattack_tackle:
		entity_setActiveAnim(attacker, "claudeAttack");
		EntityAnim attackAnim = attacker->animList[attacker->currAnimId];
		f32 busyDuration      = attackAnim.anim->frameDuration *
		                   CAST(f32) attackAnim.anim->numFrames;
		attacker->stats->busyDuration = busyDuration;

		if (attacker->direction == direction_east)
			attacker->dPos.x += (1.0f * METERS_TO_PIXEL);
		else
			attacker->dPos.x -= (1.0f * METERS_TO_PIXEL);
		break;
	default:
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
		break;
	}
}

// TODO(doyle): MemArena here is temporary until we incorporate AttackSpec to
// battle in general!
INTERNAL void endAttack(MemoryArena *arena, EventQueue *eventQueue,
                        World *world, Entity *attacker)
{
#ifdef DENGINE_DEBUG
	ASSERT(attacker->stats->entityIdToAttack != ENTITY_NULL_ID);
#endif

	entityStateSwitch(eventQueue, world, attacker, entitystate_battle);
	switch (attacker->stats->queuedAttack)
	{
	case entityattack_tackle:
		// TODO(doyle): Move animation offsets out and into animation type
		if (attacker->direction == direction_east)
			attacker->dPos.x -= (1.0f * METERS_TO_PIXEL);
		else
			attacker->dPos.x += (1.0f * METERS_TO_PIXEL);

		break;
	default:
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
		break;
	}

	/* Compute attack damage */
	Entity *defender = NULL;

	// TODO(doyle): Implement faster lookup for entity id in entity table
	b32 noMoreValidTargets = FALSE;
	do
	{
		/* Get target entity to attack */
		i32 entityIdToAttack = attacker->stats->entityIdToAttack;
		i32 entityIndex      = entity_getIndex(world, entityIdToAttack);

		if (entityIndex != -1)
		{
			defender = &world->entities[entityIndex];
#ifdef DENGINE_DEBUG
			ASSERT(defender->type == entitytype_mob ||
			       defender->type == entitytype_hero);
#endif
		}
		else
		{
			i32 entityIdToAttack = findBestEntityToAttack(world, *attacker);
			if (entityIdToAttack == ENTITY_NULL_ID)
			{
				noMoreValidTargets = TRUE;
			}
			else
			{
				attacker->stats->entityIdToAttack =
				    findBestEntityToAttack(world, *attacker);
			}
		}
	} while (!defender && noMoreValidTargets == TRUE);

	if (!noMoreValidTargets)
	{
		i32 damage = 50;

		AttackSpec *attackSpec = PLATFORM_MEM_ALLOC(arena, 1, AttackSpec);
		attackSpec->attacker   = attacker;
		attackSpec->defender   = defender;
		attackSpec->damage     = damage;

		registerEvent(eventQueue, eventtype_end_attack, attackSpec);
		// TODO(doyle): Use attacker stats in battle equations
		if (attacker->type == entitytype_hero)
		{
			defender->stats->health -= damage;
		}

		if (defender->stats->health <= 0)
		{
			entityStateSwitch(eventQueue, world, defender, entitystate_dead);
			entityStateSwitch(eventQueue, world, attacker, entitystate_idle);
		}
	}
}

INTERNAL void sortWorldEntityList(World *world)
{
	b32 listHasChanged       = TRUE;
	i32 numUnsortedEntities  = world->freeEntityIndex;
	while (listHasChanged)
	{
		listHasChanged = FALSE;
		for (i32 i = 0; i < numUnsortedEntities-1; i++)
		{
			// TODO(doyle): Better sort algorithm
			Entity *entityA = &world->entities[i];
			Entity *entityB = &world->entities[i+1];
			if (entityA->type == entitytype_null ||
			    (entityB->type != entitytype_null && entityA->id > entityB->id))
			{
				Entity tempEntity         = world->entities[i];
				world->entities[i]        = world->entities[i + 1];
				world->entities[i + 1]    = tempEntity;
				listHasChanged = TRUE;
			}
		}
		numUnsortedEntities--;
	}
}

typedef struct DamageDisplay
{
	char damageStr[12];
	v2 pos;
	f32 lifetime;
} DamageDisplay;

typedef struct BattleState
{
	DamageDisplay damageDisplay[128];
} BattleState;

GLOBAL_VAR BattleState battleState = {0};

void worldTraveller_gameUpdateAndRender(GameState *state, f32 dt)
{
	// TODO(doyle): use proper dt limiting techniques
	if (dt >= 1.0f) dt = 1.0f;
	GL_CHECK_ERROR();

	AssetManager *assetManager = &state->assetManager;
	Renderer *renderer         = &state->renderer;
	World *const world         = &state->world[state->currWorldIndex];
	Font *font                 = &assetManager->font;
	MemoryArena *arena         = &state->arena;

	/*
	 **********************
	 * Parse Input Keys
	 **********************
	 */
	{
		/*
		 **********************
		 * Parse Alphabet Keys
		 **********************
		 */
		KeyState *keys = state->input.keys;
		for (enum KeyCode code = 0; code < keycode_count; code++)
		{
			b32 parseKey = FALSE;

			KeyState *key = &keys[code];
			if (key->newHalfTransitionCount > key->oldHalfTransitionCount)
			{
				i32 numTransitions =
				    key->newHalfTransitionCount - key->oldHalfTransitionCount;

				if (!IS_EVEN(numTransitions))
					key->endedDown = (key->endedDown) ? FALSE : TRUE;
			}

			if (code >= keycode_a && code <= keycode_z) parseKey = TRUE;
			if (code >= keycode_A && code <= keycode_Z) parseKey = TRUE;
			if (code >= keycode_0 && code <= keycode_9) parseKey = TRUE;

			if (!parseKey) continue;

			if (getKeyStatus(key, readkeytype_repeat, 0.15f, dt))
			{
				// TODO(doyle): Multi press within frame overrides ui parsing
				state->uiState.keyChar = code;
			}
		}

		/*
		 **************************
		 * Parse Game Control Keys
		 **************************
		 */
		if (getKeyStatus(&keys[keycode_enter], readkeytype_oneShot,
		                 KEY_DELAY_NONE, dt))
		{
			state->uiState.keyEntered = keycode_enter;
		}

		if (getKeyStatus(&keys[keycode_tab], readkeytype_delayedRepeat, 0.25f,
		                 dt))
		{
			state->uiState.keyEntered = keycode_tab;
		}

		if (getKeyStatus(&keys[keycode_space], readkeytype_delayedRepeat, 0.25f,
		                 dt))
		{
			state->uiState.keyChar = keycode_space;
		}

		if (getKeyStatus(&keys[keycode_backspace], readkeytype_delayedRepeat,
		                 0.1f, dt))
		{
			state->uiState.keyEntered = keycode_backspace;
		}

		if (getKeyStatus(&keys[keycode_left_square_bracket],
		                 readkeytype_delayedRepeat, 0.25f, dt))
		{

			Renderer *renderer = &state->renderer;
			f32 yPos           = CAST(f32)(rand() % CAST(i32) renderer->size.h);
			f32 xModifier      = 5.0f - CAST(f32)(rand() % 3);

			v2 pos =
			    V2(renderer->size.w - (renderer->size.w / xModifier), yPos);
			addGenericMob(&state->arena, &state->assetManager, world, pos);
		}
	}

	/*
	 ******************************
	 * Update entities and render
	 ******************************
	 */
	EventQueue eventQueue = {0};
	Rect camera           = createWorldBoundedCamera(world, renderer->size);
	AudioManager *audioManager = &state->audioManager;

	ASSERT(world->freeEntityIndex < world->maxEntities);
	for (i32 i = 0; i < world->freeEntityIndex; i++)
	{
		Entity *const entity = &world->entities[i];

		/* Reposition camera if entity is targeted id */
		if (world->cameraFollowingId == entity->id)
		{
			// NOTE(doyle): Set the camera such that the hero is centered
			v2 offsetFromEntityToCameraOrigin =
			    V2((entity->pos.x - (0.5f * state->renderer.size.w)), (0.0f));

			// NOTE(doyle): Account for the hero's origin being the bottom left
			offsetFromEntityToCameraOrigin.x += (entity->hitboxSize.x * 0.5f);
			world->cameraPos = offsetFromEntityToCameraOrigin;
		}

		if (entity->state == entitystate_dead) continue;

		if (entity->type == entitytype_soundscape)
		{
			AudioRenderer *audioRenderer = entity->audioRenderer;
			if (!state->config.playWorldAudio)
			{
				// TODO(doyle): Use is playing flag, not just streaming flag
				if (audioRenderer->state == audiostate_playing)
				{
					audio_pauseVorbis(&state->audioManager, audioRenderer);
				}
			}
			else
			{
				if (world->numEntitiesInBattle > 0)
				{
					AudioVorbis *battleTheme =
					    asset_getVorbis(assetManager, audiolist_battle);
					if (audioRenderer->audio)
					{
						if (audioRenderer->audio->type != audiolist_battle)
						{
							audio_streamPlayVorbis(arena, &state->audioManager,
							                       audioRenderer, battleTheme,
							                       AUDIO_REPEAT_INFINITE);
						}
					}
					else
					{
						audio_streamPlayVorbis(arena, &state->audioManager,
						                       audioRenderer, battleTheme,
						                       AUDIO_REPEAT_INFINITE);
					}
				}
				else
				{
					AudioVorbis *overworldTheme =
					    asset_getVorbis(assetManager, audiolist_overworld);
					if (audioRenderer->audio)
					{
						if (audioRenderer->audio->type != audiolist_overworld)
						{
							audio_streamPlayVorbis(
							    arena, &state->audioManager, audioRenderer,
							    overworldTheme, AUDIO_REPEAT_INFINITE);
						}
					}
					else
					{
						audio_streamPlayVorbis(arena, &state->audioManager,
						                       audioRenderer, overworldTheme,
						                       AUDIO_REPEAT_INFINITE);
					}
				}

				if (audioRenderer->state == audiostate_paused)
				{
					audio_resumeVorbis(&state->audioManager, audioRenderer);
				}
			}
		}

		/*
		 *****************************************************
		 * Calculate Mob In Battle Range
		 *****************************************************
		 */
		if (entity->type == entitytype_mob)
		{
			// TODO(doyle): Currently calculated in pixels, how about meaningful
			// game units?
			f32 battleThreshold = 500.0f;
			Entity *hero        = getHeroEntity(world);
			f32 distance        = v2_magnitude(hero->pos, entity->pos);

			enum EntityState newState = entitystate_invalid;
			if (distance <= battleThreshold)
			{
				// NOTE(doyle): If in range but in battle, no state change
				if (entity->state == entitystate_battle ||
				    entity->state == entitystate_attack)
				{
					newState = entity->state;
				}
				else
				{
					newState = entitystate_battle;
				}
			}
			else
			{
				newState = entitystate_idle;
			}

			entityStateSwitch(&eventQueue, world, entity, newState);
		}

		/*
		 **************************
		 * Calculate Hero Movement
		 **************************
		 */
		if (entity->type == entitytype_hero)
		{
			/*
			   Equations of Motion
			   f(t)  = position     m
			   f'(t) = velocity     m/s
			   f"(t) = acceleration m/s^2

			   The user supplies an acceleration, a, and by integrating
			   f"(t) = a,                   where a is a constant, acceleration
			   f'(t) = a*t + v,             where v is a constant, old velocity
			   f (t) = (a/2)*t^2 + v*t + p, where p is a constant, old position
			 */

			Entity *hero   = entity;
			KeyState *keys = state->input.keys;

			v2 ddPos = V2(0, 0);
			if (hero->stats->busyDuration <= 0)
			{
				if (getKeyStatus(&keys[keycode_right], readkeytype_repeat,
				                 KEY_DELAY_NONE, dt))
				{
					ddPos.x         = 1.0f;
					hero->direction = direction_east;
				}

				if (getKeyStatus(&keys[keycode_left], readkeytype_repeat,
				                 KEY_DELAY_NONE, dt))
				{
					ddPos.x         = -1.0f;
					hero->direction = direction_west;
				}

				if (getKeyStatus(&keys[keycode_up], readkeytype_repeat,
				                 KEY_DELAY_NONE, dt))
				{
					ddPos.y = 1.0f;
				}

				if (getKeyStatus(&keys[keycode_down], readkeytype_repeat,
				                 KEY_DELAY_NONE, dt))
				{
					ddPos.y = -1.0f;
				}

				if (ddPos.x != 0.0f && ddPos.y != 0.0f)
				{
					// NOTE(doyle): Cheese it and pre-compute the vector for
					// diagonal using pythagoras theorem on a unit triangle 1^2
					// + 1^2 = c^2
					ddPos = v2_scale(ddPos, 0.70710678118f);
				}
			}

			/*
			 **************************
			 * Calculate Hero Speed
			 **************************
			 */
			// NOTE(doyle): Clipping threshold for snapping velocity to 0
			f32 epsilon    = 15.0f;
			v2 epsilonDpos = v2_sub(V2(epsilon, epsilon),
			                        V2(ABS(hero->dPos.x), ABS(hero->dPos.y)));

			char *currAnimName = hero->animList[hero->currAnimId].anim->key;
			if (epsilonDpos.x >= 0.0f && epsilonDpos.y >= 0.0f)
			{
				hero->dPos = V2(0.0f, 0.0f);
				if (common_strcmp(currAnimName, "claudeRun") == 0)
				{
					entity_setActiveAnim(hero, "claudeIdle");
				}
			}
			else if (common_strcmp(currAnimName, "claudeIdle") == 0)
			{
				entity_setActiveAnim(hero, "claudeRun");
			}

			f32 heroSpeed = 6.2f * METERS_TO_PIXEL;
			if (getKeyStatus(&state->input.keys[keycode_leftShift],
			                 readkeytype_repeat, KEY_DELAY_NONE, dt))
			{
				// TODO: Context sensitive command separation
				state->uiState.keyMod = keycode_leftShift;
				heroSpeed = CAST(f32)(22.0f * 10.0f * METERS_TO_PIXEL);
			}
			else
			{
				state->uiState.keyMod = keycode_null;
			}

			ddPos = v2_scale(ddPos, heroSpeed);
			// TODO(doyle): Counteracting force on player's acceleration is
			// arbitrary
			ddPos = v2_sub(ddPos, v2_scale(hero->dPos, 5.5f));

			/*
			   NOTE(doyle): Calculate new position from acceleration with old
			   velocity
			   new Position     = (a/2) * (t^2) + (v*t) + p,
			   acceleration     = (a/2) * (t^2)
			   old velocity     =                 (v*t)
			 */
			v2 ddPosNew = v2_scale(v2_scale(ddPos, 0.5f), SQUARED(dt));
			v2 dPos     = v2_scale(hero->dPos, dt);
			v2 newHeroP = v2_add(v2_add(ddPosNew, dPos), hero->pos);

			/*
			 **************************
			 * Collision Detection
			 **************************
			 */
			// TODO(doyle): Only check collision for entities within small
			// bounding box of the hero
			b32 heroCollided = FALSE;
			if (hero->collides == TRUE)
			{
				for (i32 i = 0; i < world->maxEntities; i++)
				{
					Entity collider = world->entities[i];
					if (collider.state == entitystate_dead) continue;
					if (collider.id == world->heroId) continue;

					if (collider.collides)
					{
						Rect heroRect = {newHeroP, hero->hitboxSize};

						v2 heroTopLeftP = getPosRelativeToRect(
						    heroRect, V2(0, 0), rectbaseline_topLeft);
						v2 heroTopRightP = getPosRelativeToRect(
						    heroRect, V2(0, 0), rectbaseline_topRight);
						v2 heroBottomLeftP = getPosRelativeToRect(
						    heroRect, V2(0, 0), rectbaseline_bottomLeft);
						v2 heroBottomRightP = getPosRelativeToRect(
						    heroRect, V2(0, 0), rectbaseline_bottomRight);

						Rect colliderRect = {collider.pos, collider.hitboxSize};

						if (math_pointInRect(colliderRect, heroTopLeftP) ||
						    math_pointInRect(colliderRect, heroTopRightP) ||
						    math_pointInRect(colliderRect, heroBottomLeftP) ||
						    math_pointInRect(colliderRect, heroBottomRightP))
						{
							heroCollided = TRUE;
							break;
						}
					}
				}
			}

			if (heroCollided)
			{
				hero->dPos = V2(0.0f, 0.0f);
			}
			else
			{
				// f'(t) = curr velocity = a*t + v, where v is old velocity
				hero->dPos = v2_add(hero->dPos, v2_scale(ddPos, dt));
				hero->pos  = newHeroP;
			}
		}

		/*
		 **************************************************
		 * Conduct Battle For Mobs In Range
		 **************************************************
		 */
		if (entity->type == entitytype_mob || entity->type == entitytype_hero)
		{
			EntityStats *stats = entity->stats;
			if (entity->state == entitystate_battle)
			{
				if (stats->actionTimer > 0)
					stats->actionTimer -= dt * stats->actionSpdMul;

				if (stats->actionTimer <= 0)
				{
					stats->actionTimer = 0;
					if (stats->queuedAttack == entityattack_invalid)
						stats->queuedAttack = entityattack_tackle;

					/* Launch up attack animation */
					beginAttack(&eventQueue, world, entity);
				}
			}
			else if (entity->state == entitystate_attack)
			{
				// TODO(doyle): Untested if the attacker and the defender same
				Entity *attacker = entity;
				stats->busyDuration -= dt;
				if (stats->busyDuration <= 0)
				{
					/* Apply attack damage */
					endAttack(&state->arena, &eventQueue, world, entity);
				}
			}
		}

		/*
		 ****************
		 * Update Entity
		 ****************
		 */
		if (entity->audioRenderer)
		{
			audio_updateAndPlay(&state->arena, &state->audioManager,
			                    entity->audioRenderer);
		}

		if (entity->tex)
		{
			entity_updateAnim(entity, dt);
			/* Calculate region to render */
			renderer_entity(renderer, camera, entity, V2(0, 0), 0,
			                V4(1, 1, 1, 1));
		}
	}

	/*
	 *****************************************
	 * Process Events From Entity Update Loop
	 *****************************************
	 */
	i32 numDeadEntities = 0;
	for (i32 i = 0; i < eventQueue.numEvents; i++)
	{
		Event event = eventQueue.queue[i];
		switch(event.type)
		{
		case eventtype_end_attack:
		{
			if (!event.data) continue;

			AttackSpec *attackSpec = (CAST(AttackSpec *) event.data);
			Entity *attacker       = attackSpec->attacker;
			Entity *defender       = attackSpec->defender;

			audio_playVorbis(arena, audioManager, attacker->audioRenderer,
			                 asset_getVorbis(assetManager, audiolist_tackle),
			                 1);

			/* Get first free string position and store the damage str data */
			for (i32 i = 0; i < ARRAY_COUNT(battleState.damageDisplay); i++)
			{
				if (battleState.damageDisplay[i].lifetime <= 0.0f)
				{
					common_memset(
					    battleState.damageDisplay[i].damageStr, 0,
					    ARRAY_COUNT(battleState.damageDisplay[i].damageStr));

					battleState.damageDisplay[i].lifetime = 1.0f;
					battleState.damageDisplay[i].pos      = defender->pos;
					common_itoa(
					    attackSpec->damage,
					    battleState.damageDisplay[i].damageStr,
					    ARRAY_COUNT(battleState.damageDisplay[i].damageStr));

					break;
				}
			}

			PLATFORM_MEM_FREE(&state->arena, attackSpec, sizeof(AttackSpec));
			break;
		}
		// NOTE(doyle): We delete dead entities at the end of the update
		// loop incase a later indexed entity deletes an earlier indexed
		// entity, the entity will exist for an additional frame
		case eventtype_entity_died:
		{
#ifdef DEGINE_DEBUG
			DEBUG_LOG("Entity died: being deleted");
#endif
			if (!event.data) continue;

			Entity *entity = (CAST(Entity *) event.data);
			audio_stopVorbis(&state->arena, audioManager,
			                 entity->audioRenderer);
			entity_clearData(&state->arena, world, entity);
			numDeadEntities++;
			break;
		}
		default:
		{
			break;
		}
		}
	}


	/*
	 ****************************
	 * Update World From Results
	 ****************************
	 */
	// TODO(doyle): Dead hero not accounted for here
	Entity *hero = getHeroEntity(world);
	if (world->numEntitiesInBattle > 0)
	{
		// NOTE(doyle): If battle entities is 1 then only the hero left
		if (hero->state == entitystate_battle &&
		    world->numEntitiesInBattle == 1)
			entityStateSwitch(&eventQueue, world, hero, entitystate_idle);
		else if (hero->state != entitystate_attack)
		{
			entityStateSwitch(&eventQueue, world, hero, entitystate_battle);
		}
	}
	else
	{
		if (hero->state == entitystate_battle)
		{
			hero->state                       = entitystate_idle;
			world->entityIdInBattle[hero->id] = FALSE;
			entity_setActiveAnim(hero, "claudeIdle");
		}
		hero->stats->entityIdToAttack = -1;
		hero->stats->actionTimer      = hero->stats->actionRate;
		hero->stats->busyDuration     = 0;
	}

	sortWorldEntityList(world);
	world->freeEntityIndex -= numDeadEntities;

	/*
	 ********************
	 * UI Rendering Code
	 ********************
	 */
	/* Render all damage strings */
	for (i32 i = 0; i < ARRAY_COUNT(battleState.damageDisplay); i++)
	{
		if (battleState.damageDisplay[i].lifetime > 0.0f)
		{
			battleState.damageDisplay[i].lifetime -= dt;

			char *damageString = battleState.damageDisplay[i].damageStr;
			v2 damagePos       = battleState.damageDisplay[i].pos;
			f32 lifetime       = battleState.damageDisplay[i].lifetime;

			// TODO(doyle): Incorporate UI into entity list or it's own list
			// with a rendering lifetime value
			renderer_string(renderer, &state->arena, camera, font,
			                damageString, damagePos, V2(0, 0), 0,
			                V4(1, 1, 1, lifetime));
		}
	}

	// INIT IMGUI
	state->uiState.hotItem = 0;

	/* Draw ui */
	if (state->uiState.keyChar == keycode_i)
	{
		state->config.showStatMenu =
		    (state->config.showStatMenu == TRUE) ? FALSE : TRUE;
	}

	if (state->config.showStatMenu)
	{
		WindowState *statWindow = &state->uiState.statWindow;
		userInterface_window(&state->uiState, &state->arena, assetManager,
		                     renderer, font, state->input, statWindow);
	}

	/* Draw debug window */
	WindowState *debugWindow = &state->uiState.debugWindow;
	i32 activeId =
	    userInterface_window(&state->uiState, &state->arena, assetManager,
	                         renderer, font, state->input, debugWindow);

	// TODO(doyle): Name lookup to user interface id
	if (activeId == 1)
	{
		state->config.playWorldAudio =
		    (state->config.playWorldAudio == TRUE) ? FALSE : TRUE;
	}
	else if (activeId == 2)
	{
		state->config.showDebugDisplay =
		    (state->config.showDebugDisplay == TRUE) ? FALSE : TRUE;
	}

	// RESET IMGUI
	if (!state->input.keys[keycode_mouseLeft].endedDown)
		state->uiState.activeItem = 0;
	else if (state->uiState.activeItem == 0)
		state->uiState.activeItem = -1;

	if (state->uiState.keyEntered == keycode_tab) state->uiState.kbdItem = 0;
	state->uiState.keyEntered = keycode_null;
	state->uiState.keyChar    = keycode_null;

	/* Draw hero avatar */
	TexAtlas *heroAtlas =
	    asset_getTexAtlas(assetManager, "ClaudeSprite.png");
	Rect heroAvatarRect =
	    asset_getAtlasSubTexRect(heroAtlas, "ClaudeSprite_Avatar_01");
	v2 heroAvatarP =
	    V2(10.0f, (renderer->size.h * 0.5f) - (0.5f * heroAvatarRect.size.h));

	v4 heroAvatarTexRect      = {0};
	heroAvatarTexRect.vec2[0] = heroAvatarRect.pos;
	heroAvatarTexRect.vec2[1] = v2_add(heroAvatarRect.pos, heroAvatarRect.size);

	RenderTex heroRenderTex = {hero->tex, heroAvatarTexRect};
	renderer_staticRect(renderer, heroAvatarP, heroAvatarRect.size, V2(0, 0), 0,
	                    heroRenderTex, V4(1, 1, 1, 1));

	char heroAvatarStr[20];
	snprintf(heroAvatarStr, ARRAY_COUNT(heroAvatarStr), "HP: %3.0f/%3.0f",
	         hero->stats->health, hero->stats->maxHealth);
	f32 strLenInPixels =
	    CAST(f32)(font->maxSize.w * common_strlen(heroAvatarStr));
	v2 strPos =
	    V2(heroAvatarP.x, heroAvatarP.y - (0.5f * heroAvatarRect.size.h));
	renderer_staticString(&state->renderer, &state->arena, font, heroAvatarStr,
	                      strPos, V2(0, 0), 0, V4(0, 0, 1, 1));

	renderer_staticString(&state->renderer, &state->arena, font, heroAvatarStr,
	                      strPos, V2(0, 0), 0, V4(0, 0, 1, 1));

	for (i32 i = 0; i < world->maxEntities; i++)
	{
		Entity *entity = &world->entities[i];
		if (entity->id == hero->id)
			continue;

		if (entity->state == entitystate_attack ||
		    entity->state == entitystate_battle)
		{
			v2 difference    = v2_sub(entity->pos, hero->pos);
			f32 angle        = math_atan2f(difference.y, difference.x);
			f32 angleDegrees = RADIANS_TO_DEGREES(angle);

			v2 heroCenter = v2_add(hero->pos, v2_scale(hero->hitboxSize, 0.5f));
			RenderTex renderTex = renderer_createNullRenderTex(assetManager);
			f32 distance        = v2_magnitude(hero->pos, entity->pos);
			renderer_rect(&state->renderer, camera, heroCenter,
			              V2(distance, 2.0f), V2(0, 0), angle, renderTex,
			              V4(1, 0, 0, 1.0f));
		}
	}

	for (i32 i = 0; i < keycode_count; i++)
	{
		state->input.keys[i].oldHalfTransitionCount =
		    state->input.keys[i].newHalfTransitionCount;
	}

    /*
	 ********************
	 * DEBUG CODE
	 ********************
	 */
#ifdef DENGINE_DEBUG
	for (i32 i = 0; i < world->freeEntityIndex-1; i++)
	{
		ASSERT(world->entities[i].type != entitytype_null);
		ASSERT(world->entities[i].id < world->entities[i+1].id);
	}
#endif

#ifdef DENGINE_DEBUG
	if (state->config.showDebugDisplay)
	{
		debug_drawUi(state, dt);
	}
#endif
}
