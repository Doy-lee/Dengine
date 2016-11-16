#include "Dengine/Asteroid.h"
#include "Dengine/Debug.h"

void initAssetManager(GameState *state)
{
	AssetManager *assetManager = &state->assetManager;
	MemoryArena_ *arena        = &state->persistentArena;

	i32 audioEntries         = 32;
	assetManager->audio.size = audioEntries;
	assetManager->audio.entries =
	    memory_pushBytes(arena, audioEntries * sizeof(HashTableEntry));

	i32 texAtlasEntries         = 8;
	assetManager->texAtlas.size = texAtlasEntries;
	assetManager->texAtlas.entries =
	    memory_pushBytes(arena, texAtlasEntries * sizeof(HashTableEntry));

	i32 texEntries              = 32;
	assetManager->textures.size = texEntries;
	assetManager->textures.entries =
	    memory_pushBytes(arena, texEntries * sizeof(HashTableEntry));

	i32 animEntries          = 1024;
	assetManager->anims.size = animEntries;
	assetManager->anims.entries =
	    memory_pushBytes(arena, animEntries * sizeof(HashTableEntry));

	/* Create empty 1x1 4bpp black texture */
	u32 bitmap   = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | (0xFF << 0);
	Texture *tex = asset_getFreeTexSlot(assetManager, arena, "nullTex");
	*tex         = texture_gen(1, 1, 4, CAST(u8 *)(&bitmap));

	/* Load shaders */
	asset_loadShaderFiles(
	    assetManager, arena, "data/shaders/default_tex.vert.glsl",
	    "data/shaders/default_tex.frag.glsl", shaderlist_default);

	asset_loadShaderFiles(
	    assetManager, arena, "data/shaders/default_no_tex.vert.glsl",
	    "data/shaders/default_no_tex.frag.glsl", shaderlist_default_no_tex);

	i32 result =
	    asset_loadTTFont(assetManager, arena, "C:/Windows/Fonts/Arialbd.ttf");
	if (result) ASSERT(TRUE);
}

void initRenderer(GameState *state, v2 windowSize) {
	AssetManager *assetManager = &state->assetManager;
	Renderer *renderer         = &state->renderer;
	renderer->size             = windowSize;

	// NOTE(doyle): Value to map a screen coordinate to NDC coordinate
	renderer->vertexNdcFactor =
	    V2(1.0f / renderer->size.w, 1.0f / renderer->size.h);

	const mat4 projection =
	    mat4_ortho(0.0f, renderer->size.w, 0.0f, renderer->size.h, 0.0f, 1.0f);
	for (i32 i = 0; i < shaderlist_count; i++)
	{
		renderer->shaderList[i] = asset_getShader(assetManager, i);
		shader_use(renderer->shaderList[i]);
		shader_uniformSetMat4fv(renderer->shaderList[i], "projection",
		                        projection);
		GL_CHECK_ERROR();
	}

	renderer->activeShaderId = renderer->shaderList[shaderlist_default];
	GL_CHECK_ERROR();

	/* Create buffers */
	glGenVertexArrays(ARRAY_COUNT(renderer->vao), renderer->vao);
	glGenBuffers(ARRAY_COUNT(renderer->vbo), renderer->vbo);
	GL_CHECK_ERROR();

	// Bind buffers and configure vao, vao automatically intercepts
	// glBindCalls and associates the state with that buffer for us
	for (enum RenderMode mode = 0; mode < rendermode_count; mode++)
	{
		glBindVertexArray(renderer->vao[mode]);
		glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo[mode]);

		glEnableVertexAttribArray(0);
		u32 numVertexElements = 4;
		u32 stride            = sizeof(Vertex);

		glVertexAttribPointer(0, numVertexElements, GL_FLOAT,
		                      GL_FALSE, stride, (GLvoid *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	/* Unbind */
	GL_CHECK_ERROR();

	// TODO(doyle): Lazy allocate render group capacity
	renderer->groupCapacity = 4096;
	for (i32 i = 0; i < ARRAY_COUNT(renderer->groups); i++)
	{
		renderer->groups[i].vertexList = memory_pushBytes(
		    &state->persistentArena, renderer->groupCapacity * sizeof(Vertex));
	}
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


void asteroid_gameUpdateAndRender(GameState *state, Memory *memory,
                                  v2 windowSize, f32 dt)
{
	if (!state->init)
	{

		memory_arenaInit(&state->persistentArena, memory->persistent,
		                 memory->persistentSize);
		initAssetManager(state);
		initRenderer(state, windowSize);

		state->pixelsPerMeter = 70.0f;

		{ // Init ship entity
			Entity *ship     = &state->entityList[state->entityIndex++];
			ship->id         = 0;
			ship->pos        = V2(0, 0);
			ship->size       = V2(25.0f, 50.0f);
			ship->hitbox     = ship->size;
			ship->offset     = v2_scale(ship->size, 0.5f);
			ship->scale      = 1;
			ship->type       = entitytype_ship;
			ship->direction  = direction_null;
			ship->renderMode = rendermode_triangle;
			ship->tex        = NULL;
			ship->collides   = FALSE;
		}

		state->camera.min = V2(0, 0);
		state->camera.max = state->renderer.size;
		state->init       = TRUE;

		state->worldSize = windowSize;

		debug_init(&state->persistentArena, windowSize,
		           state->assetManager.font);
	}

	memory_arenaInit(&state->transientArena, memory->transient,
	                 memory->transientSize);

	{
		KeyState *keys = state->input.keys;
		for (enum KeyCode code = 0; code < keycode_count; code++)
		{
			KeyState *keyState = &keys[code];

			u32 halfTransitionCount = keyState->newHalfTransitionCount -
			                          keyState->oldHalfTransitionCount;

			if (halfTransitionCount > 0)
			{
				b32 transitionCountIsOdd = ((halfTransitionCount & 1) == 1);

				if (transitionCountIsOdd)
				{
					if (keyState->endedDown) keyState->endedDown = FALSE;
					else keyState->endedDown = TRUE;
				}

				keyState->oldHalfTransitionCount =
				    keyState->newHalfTransitionCount;
			}
		}
	}

	for (i32 i = 0; i < state->entityIndex; i++)
	{
		Entity *entity = &state->entityList[i];
		ASSERT(entity->type != entitytype_invalid);

		v2 pivotPoint = {0};
		if (entity->type == entitytype_ship) {

			v2 ddP = {0};
			if (getKeyStatus(&state->input.keys[keycode_up], readkeytype_repeat,
			                 0.0f, dt))
			{
				// TODO(doyle): Renderer creates upfacing triangles by default,
				// but we need to offset rotation so that our base "0 degrees"
				// is right facing for trig to work
				Radians rotation = DEGREES_TO_RADIANS((entity->rotation + 90.0f));
				v2 direction     = V2(math_cosf(rotation), math_sinf(rotation));
				ddP = direction;
			}

			if (getKeyStatus(&state->input.keys[keycode_left],
			                 readkeytype_repeat, 0.0f, dt))
			{
				entity->rotation += (120.0f) * dt;
			}

			if (getKeyStatus(&state->input.keys[keycode_right],
			                 readkeytype_repeat, 0.0f, dt))
			{
				entity->rotation -= (120.0f) * dt;
			}

			if (ddP.x != 0.0f && ddP.y != 0.0f)
			{
				// NOTE(doyle): Cheese it and pre-compute the vector for
				// diagonal using pythagoras theorem on a unit triangle 1^2
				// + 1^2 = c^2
				ddP = v2_scale(ddP, 0.70710678118f);
			}

			/*
			    Assuming acceleration A over t time, then integrate twice to get

			    newVelocity = a*t + oldVelocity
			    newPos = (a*t^2)/2 + oldVelocity*t + oldPos

			*/

			ddP = v2_scale(ddP, state->pixelsPerMeter * 25);

			v2 oldDp = entity->dP;
			v2 resistance = v2_scale(oldDp, 2.0f);
			ddP = v2_sub(ddP, resistance);

			entity->dP = v2_add(v2_scale(ddP, dt), oldDp);

			v2 ddPHalf          = v2_scale(ddP, 0.5f);
			v2 ddPHalfDtSquared = v2_scale(ddPHalf, (SQUARED(dt)));
			v2 oldDpDt          = v2_scale(oldDp, dt);
			v2 oldPos           = entity->pos;
			entity->pos = v2_add(v2_add(ddPHalfDtSquared, oldDpDt), oldPos);

			pivotPoint = v2_scale(entity->size, 0.5f);
		}

		if (entity->pos.y >= state->worldSize.h)
		{
			entity->pos.y = 0;
		}
		else if (entity->pos.y < 0)
		{
			entity->pos.y = state->worldSize.h;
		}

		if (entity->pos.x >= state->worldSize.w)
		{
			entity->pos.x = 0;
		}
		else if (entity->pos.x < 0)
		{
			entity->pos.x = state->worldSize.w;
		}

		DEBUG_PUSH_VAR("Pos: %5.2f, %5.2f", entity->pos, "v2");
		DEBUG_PUSH_VAR("Velocity: %5.2f, %5.2f", entity->dP, "v2");
		DEBUG_PUSH_VAR("Rotation: %5.2f", entity->rotation, "f32");

		RenderFlags flags = renderflag_wireframe | renderflag_no_texture;
		renderer_entity(&state->renderer, state->camera, entity, pivotPoint, 0,
		                 V4(0.4f, 0.8f, 1.0f, 1.0f), flags);

		v2 leftAlignedP = v2_sub(entity->pos, entity->offset);
		renderer_rect(&state->renderer, state->camera, leftAlignedP,
		              entity->size, v2_scale(entity->size, 0.5f),
		              DEGREES_TO_RADIANS(entity->rotation), NULL,
		              V4(1.0f, 0.8f, 1.0f, 1.0f), flags);

		v2 rightAlignedP = v2_add(leftAlignedP, entity->size);
		renderer_rect(&state->renderer, state->camera, rightAlignedP, V2(4, 4),
		              v2_scale(pivotPoint, -1.0f),
		              DEGREES_TO_RADIANS(entity->rotation), NULL,
		              V4(0.4f, 0.8f, 1.0f, 1.0f), flags);
	}

	TrianglePoints triangle = {0};
	triangle.points[0] = V2(100, 200);
	triangle.points[1] = V2(200, 100);
	triangle.points[2] = V2(100, 300);

	LOCAL_PERSIST Radians rotation = 0.0f;
	rotation += DEGREES_TO_RADIANS(((60.0f) * dt));

	RenderFlags flags = renderflag_wireframe | renderflag_no_texture;
	renderer_triangle(&state->renderer, state->camera, triangle, V2(0, 0),
	                  rotation, NULL, V4(1, 1, 1, 1), flags);

	debug_drawUi(state, dt);
	debug_clearCounter();

	renderer_renderGroups(&state->renderer);
}
