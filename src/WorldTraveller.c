#include <Dengine/AssetManager.h>
#include <Dengine/Math.h>
#include <WorldTraveller/WorldTraveller.h>

//TODO(doyle): This is temporary! Maybe abstract into our platform layer, or
//choose to load assets outside of WorldTraveller!
#include <stdlib.h>

void updateBufferObject(GLuint vbo, v4 texNDC)
{
	// TODO(doyle): We assume that vbo and vao are assigned
	// NOTE(doyle): Draws a series of triangles (three-sided polygons) using
	// vertices v0, v1, v2, then v2, v1, v3 (note the order)
	v4 vertices[] = {
		//  x     y       s     t
		{0.0f, 1.0f, texNDC.x, texNDC.y}, // Top left
		{0.0f, 0.0f, texNDC.x, texNDC.w}, // Bottom left
		{1.0f, 1.0f, texNDC.z, texNDC.y}, // Top right
	    {1.0f, 0.0f, texNDC.z, texNDC.w}, // Bottom right
	};

	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// TODO(doyle): glBufferSubData
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void worldTraveller_gameInit(GameState *state)
{
	/* Initialise assets */
	asset_loadTextureImage(
	    "data/textures/WorldTraveller/TerraSprite1024.png", texlist_hero);
	asset_loadShaderFiles("data/shaders/sprite.vert.glsl",
	                      "data/shaders/sprite.frag.glsl", shaderlist_sprite);
	glCheckError();

	state->state = state_active;

	/* Init hero */
	SpriteAnim heroAnim = {NULL, 1, 0, 1.0f, 1.0f};
	// TODO(doyle): Get rid of
	heroAnim.rect = (v4 *)calloc(1, sizeof(v4));
	heroAnim.rect[0] = V4(746.0f, 1018.0f, 804.0f, 920.0f);

	Entity heroEnt = {V2(0.0f, 0.0f),
	                  V2(0.0f, 0.0f),
	                  V2(58.0f, 98.0f),
	                  direction_east,
	                  asset_getTexture(texlist_hero),
	                  heroAnim};

	state->heroIndex = state->freeEntityIndex;
	state->entityList[state->freeEntityIndex++] = heroEnt;
	Entity *hero = &state->entityList[state->heroIndex];

	Texture *heroSheet = hero->tex;
	v2 sheetSize = V2(CAST(f32)heroSheet->width, CAST(f32)heroSheet->height);
	if (sheetSize.x != sheetSize.y)
	{
		printf(
		    "worldTraveller_gameInit() warning: Sprite sheet is not square: "
		    "%dx%dpx\n",
		    CAST(i32) sheetSize.w, CAST(i32) sheetSize.h);
	}

	/* Create a NPC */
	SpriteAnim npcAnim = {NULL, 2, 0, 0.3f, 0.3f};
	// TODO(doyle): Get rid of
	npcAnim.rect = (v4 *)calloc(2, sizeof(v4));
	npcAnim.rect[0] = V4(944.0f, 918.0f, 1010.0f, 816.0f);
	npcAnim.rect[1] = V4(944.0f, 812.0f, 1010.0f, 710.0f);

	Entity npcEnt = {V2(300.0f, 300.0f), V2(0.0f, 0.0f), hero->size,
	                 direction_null, hero->tex, npcAnim};
	state->entityList[state->freeEntityIndex++] = npcEnt;

	/* Init renderer */
	Renderer *renderer = &state->renderer;
	renderer->shader = asset_getShader(shaderlist_sprite);
	shader_use(renderer->shader);

	const mat4 projection = mat4_ortho(0.0f, CAST(f32) state->width, 0.0f,
	                                   CAST(f32) state->height, 0.0f, 1.0f);
	shader_uniformSetMat4fv(renderer->shader, "projection", projection);
	glCheckError();

	/* Create buffers */
	glGenVertexArrays(1, &renderer->vao);
	glGenBuffers(1, &renderer->vbo);
	glCheckError();

	/* Bind buffers */
	glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
	glBindVertexArray(renderer->vao);

	/* Configure VAO */
	const GLuint numVertexElements = 4;
	const GLuint vertexSize        = sizeof(v4);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, numVertexElements, GL_FLOAT, GL_FALSE, vertexSize,
	                      (GLvoid *)0);
	glCheckError();

	/* Unbind */
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glCheckError();
}

INTERNAL void parseInput(GameState *state, const f32 dt)
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

	Entity *hero = &state->entityList[state->heroIndex];
	v2 ddPos = V2(0, 0);

	if (state->keys[GLFW_KEY_SPACE])
	{
	}

	if (state->keys[GLFW_KEY_RIGHT])
	{
		ddPos.x = 1.0f;
		hero->direction = direction_east;
	}
	if (state->keys[GLFW_KEY_LEFT])
	{
		ddPos.x = -1.0f;
		hero->direction = direction_west;
	}

	if (state->keys[GLFW_KEY_UP])
	{
		ddPos.y = 1.0f;
	}
	if (state->keys[GLFW_KEY_DOWN])
	{
		ddPos.y = -1.0f;
	}

	if (ddPos.x != 0.0f && ddPos.y != 0.0f)
	{
		// NOTE(doyle): Cheese it and pre-compute the vector for diagonal using
		// pythagoras theorem on a unit triangle
		// 1^2 + 1^2 = c^2
		ddPos = v2_scale(ddPos, 0.70710678118f);
	}

	f32 heroSpeed = CAST(f32)(22.0f * METERS_TO_PIXEL); // m/s^2
	if (state->keys[GLFW_KEY_LEFT_SHIFT])
	{
		heroSpeed = CAST(f32)(22.0f * 5.0f * METERS_TO_PIXEL);
	}
	ddPos = v2_scale(ddPos, heroSpeed);

	// TODO(doyle): Counteracting force on player's acceleration is arbitrary
    ddPos = v2_sub(ddPos, v2_scale(hero->dPos, 5.5f));

	/*
	   NOTE(doyle): Calculate new position from acceleration with old velocity
	   new Position     = (a/2) * (t^2) + (v*t) + p,
	   acceleration     = (a/2) * (t^2)
	   old velocity     =                 (v*t)
	 */
	v2 ddPosNew = v2_scale(v2_scale(ddPos, 0.5f), squared(dt));
	v2 dPos     = v2_scale(hero->dPos, dt);
	v2 newHeroP = v2_add(v2_add(ddPosNew, dPos), hero->pos);

	// f'(t) = curr velocity = a*t + v, where v is old velocity
    hero->dPos = v2_add(hero->dPos, v2_scale(ddPos, dt));
	hero->pos  = newHeroP;
}

void worldTraveller_gameUpdateAndRender(GameState *state, const f32 dt)
{
	/* Update */
	parseInput(state, dt);
	glCheckError();

	// NOTE(doyle): Factor to normalise sprite sheet rect coords to -1, 1
	Entity *hero  = &state->entityList[state->heroIndex];
	f32 ndcFactor = 1.0f / CAST(f32) hero->tex->width;

	ASSERT(state->freeEntityIndex < ARRAY_COUNT(state->entityList));
	for (i32 i = 0; i < state->freeEntityIndex; i++)
	{
		Entity *entity   = &state->entityList[i];
		SpriteAnim *anim = &entity->anim;

		v4 currFrameRect = anim->rect[anim->currRectIndex];
		anim->currDuration -= dt;
		if (anim->currDuration <= 0.0f)
		{
			anim->currRectIndex++;
			anim->currRectIndex = anim->currRectIndex % anim->numRects;
			currFrameRect         = anim->rect[anim->currRectIndex];
			anim->currDuration  = anim->duration;
		}

		v4 texNDC = v4_scale(currFrameRect, ndcFactor);
		if (entity->direction == direction_east)
		{
			// NOTE(doyle): Flip the x coordinates to flip the tex
			v4 tmpNDC = texNDC;
			texNDC.x  = tmpNDC.z;
			texNDC.z  = tmpNDC.x;
		}

		updateBufferObject(state->renderer.vbo, texNDC);
		renderer_entity(&state->renderer, entity, 0.0f, V3(0, 0, 0));
	}

	// TODO(doyle): Clean up lines
	// Renderer::~Renderer() { glDeleteVertexArrays(1, &this->quadVAO); }
}
