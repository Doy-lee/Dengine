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
	asset_loadShaderFiles(assetManager, arena, "data/shaders/sprite.vert.glsl",
	                      "data/shaders/sprite.frag.glsl",
	                      shaderlist_sprite);

	i32 result =
	    asset_loadTTFont(assetManager, arena, "C:/Windows/Fonts/Arialbd.ttf");

	if (result) {
		ASSERT(TRUE);
	}
}

void initRenderer(GameState *state, v2 windowSize) {
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
	glGenVertexArrays(ARRAY_COUNT(renderer->vao), renderer->vao);
	glGenBuffers(ARRAY_COUNT(renderer->vbo), renderer->vbo);
	GL_CHECK_ERROR();

	{
		// Bind buffers and configure vao, vao automatically intercepts
		// glBindCalls and associates the state with that buffer for us
		glBindVertexArray(renderer->vao[rendermode_quad]);
		glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo[rendermode_quad]);

		glEnableVertexAttribArray(0);
		u32 numVertexElements = 4;
		u32 stride            = sizeof(Vertex);

		glVertexAttribPointer(0, numVertexElements, GL_FLOAT,
		                      GL_FALSE, stride, (GLvoid *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	{
		glBindVertexArray(renderer->vao[rendermode_triangle]);
		glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo[rendermode_triangle]);

		glEnableVertexAttribArray(0);
		u32 numVertexElements = 3;
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
	if (!state->init) {

		memory_arenaInit(&state->persistentArena, memory->persistent,
		                 memory->persistentSize);
		memory_arenaInit(&state->transientArena, memory->transient,
		                 memory->transientSize);

		initAssetManager(state);
		initRenderer(state, windowSize);

		state->pixelsPerMeter = 70.0f;

		{ // Init ship entity
			Entity *ship    = &state->entityList[state->entityIndex++];
			ship->id        = 0;
			ship->pos       = V2(100.0f, 100.0f);
			ship->hitbox    = V2(100.0f, 100.0f);
			ship->size      = V2(100.0f, 100.0f);
			ship->scale     = 1;
			ship->type      = entitytype_ship;
			ship->direction = direction_null;
			ship->tex       = NULL;
			ship->collides  = FALSE;
		}

		state->camera.pos = V2(0, 0);
		state->camera.size = state->renderer.size;


		state->init = TRUE;
	}
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

		if (entity->type == entitytype_ship) {


			v2 acceleration = {0};
			if (getKeyStatus(&state->input.keys[keycode_up], readkeytype_repeat,
			                 0.0f, dt))
			{
				acceleration.y = 1.0f;
			}

			if (getKeyStatus(&state->input.keys[keycode_down],
			                 readkeytype_repeat, 0.0f, dt))
			{
				acceleration.y = -1.0f;
			}

			if (getKeyStatus(&state->input.keys[keycode_left],
			                 readkeytype_repeat, 0.0f, dt))
			{
				acceleration.x = -1.0f;
			}

			if (getKeyStatus(&state->input.keys[keycode_right],
			                 readkeytype_repeat, 0.0f, dt))
			{
				acceleration.x = 1.0f;
			}

			if (acceleration.x != 0.0f && acceleration.y != 0.0f)
			{
				// NOTE(doyle): Cheese it and pre-compute the vector for
				// diagonal using pythagoras theorem on a unit triangle 1^2
				// + 1^2 = c^2
				acceleration = v2_scale(acceleration, 0.70710678118f);
			}

			/*
			    Assuming acceleration A over t time, then integrate twice to get

			    newVelocity = a*t + oldVelocity
			    newPos = (a*t^2)/2 + oldVelocity*t + oldPos
			*/

			acceleration = v2_scale(acceleration, state->pixelsPerMeter * 25);

			v2 oldVelocity = entity->velocity;
			v2 resistance = v2_scale(oldVelocity, 4.0f);
			acceleration = v2_sub(acceleration, resistance);

			entity->velocity = v2_add(v2_scale(acceleration, dt), oldVelocity);

			v2 halfAcceleration = v2_scale(acceleration, 0.5f);
			v2 halfAccelerationDtSquared =
			    v2_scale(halfAcceleration, (SQUARED(dt)));
			v2 oldVelocityDt = v2_scale(oldVelocity, dt);
			v2 oldPos = entity->pos;
			entity->pos      = v2_add(
			    v2_add(halfAccelerationDtSquared, oldVelocityDt), oldPos);
		}

		RenderFlags flags = renderflag_wireframe;
		renderer_entity(&state->renderer, state->camera, entity, V2(0, 0),
		                0, V4(0.4f, 0.8f, 1.0f, 1.0f), flags);
	}

	TrianglePoints triangle = {0};
	triangle.points[0] = V2(100, 200);
	triangle.points[2] = V2(100, 300);
	triangle.points[1] = V2(200, 100);

	LOCAL_PERSIST Radians rotation = 0.0f;
	rotation += DEGREES_TO_RADIANS(((60.0f) * dt));

	RenderFlags flags = renderflag_wireframe;
	renderer_triangle(&state->renderer, state->camera, triangle, V2(0, 0),
	                  rotation, NULL, V4(1, 1, 1, 1), flags);

	renderer_renderGroups(&state->renderer);
}
