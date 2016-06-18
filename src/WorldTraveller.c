#include <Dengine/AssetManager.h>
#include <Dengine/Math.h>
#include <WorldTraveller/WorldTraveller.h>

void updateBufferObject(GLuint vbo, v4 texNDC)
{
	// TODO(doyle): We assume that vbo and vao are assigned
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
	glCheckError();
	asset_loadShaderFiles("data/shaders/sprite.vert.glsl",
	                      "data/shaders/sprite.frag.glsl", shaderlist_sprite);
	glCheckError();

	state->state             = state_active;
	state->heroLastDirection = direction_east;

	/* Init hero */
	Entity *hero = &state->hero;
	hero->tex    = asset_getTexture(texlist_hero);
	hero->size   = V2(58.0f, 98.0f);
	hero->pos    = V2(0.0f, 0.0f);

	Texture *heroSheet = hero->tex;
	v2 sheetSize = V2(CAST(f32)heroSheet->width, CAST(f32)heroSheet->height);
	if (sheetSize.x != sheetSize.y)
	{
		printf(
		    "worldTraveller_gameInit() warning: Sprite sheet is not square: "
		    "%dx%dpx\n",
		    CAST(i32) sheetSize.w, CAST(i32) sheetSize.h);
	}

	f32 ndcFactor = sheetSize.w;
	v2 heroStartPixel = V2(746.0f, 1018.0f);
	v2 heroSizeOnSheet = V2(58.0f, 98.0f);
	v4 heroRect = V4(heroStartPixel.x,
	                 heroStartPixel.y,
	                 heroStartPixel.x + heroSizeOnSheet.w,
	                 heroStartPixel.y - heroSizeOnSheet.h);

	v4 heroTexNDC = v4_scale(heroRect, 1.0f/ndcFactor);

	/* Init renderer */
	Renderer *renderer = &state->renderer;
	renderer->shader = asset_getShader(shaderlist_sprite);
	shader_use(renderer->shader);

	const mat4 projection = mat4_ortho(0.0f, CAST(f32) state->width, 0.0f,
	                                   CAST(f32) state->height, 0.0f, 1.0f);
	shader_uniformSetMat4fv(renderer->shader, "projection", projection);
	glCheckError();

	// NOTE(doyle): Draws a series of triangles (three-sided polygons) using
	// vertices v0, v1, v2, then v2, v1, v3 (note the order)
	v4 vertices[] = {
		//  x     y       s     t
		{0.0f, 1.0f, heroTexNDC.x, heroTexNDC.y}, // Top left
		{0.0f, 0.0f, heroTexNDC.x, heroTexNDC.w}, // Bottom left
		{1.0f, 1.0f, heroTexNDC.z, heroTexNDC.y}, // Top right
	    {1.0f, 0.0f, heroTexNDC.z, heroTexNDC.w}, // Bottom right
	};

	/* Create buffers */
	glGenVertexArrays(1, &renderer->vao);
	glGenBuffers(1, &renderer->vbo);
	glCheckError();

	/* Bind buffers */
	glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
	glBindVertexArray(renderer->vao);

	/* Configure VBO */
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glCheckError();

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

	v2 ddPos = V2(0, 0);

	if (state->keys[GLFW_KEY_SPACE])
	{
	}

	if (state->keys[GLFW_KEY_RIGHT])
	{
		ddPos.x = 1.0f;
		state->heroLastDirection = direction_east;
	}
	if (state->keys[GLFW_KEY_LEFT])
	{
		ddPos.x = -1.0f;
		state->heroLastDirection = direction_west;
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
	Entity *hero = &state->hero;
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

	Entity *hero       = &state->hero;
	Texture *heroSheet = hero->tex;
	f32 ndcFactor      = CAST(f32)heroSheet->width;

	v2 heroStartPixel = V2(746.0f, 1018.0f); // direction == east

	v2 heroSizeOnSheet = V2(58.0f, 98.0f);
	v4 heroRect = V4(heroStartPixel.x,
	                 heroStartPixel.y,
	                 heroStartPixel.x + heroSizeOnSheet.w,
	                 heroStartPixel.y - heroSizeOnSheet.h);

	v4 heroTexNDC = v4_scale(heroRect, 1.0f / ndcFactor);
	if (state->heroLastDirection == direction_east)
	{
		// NOTE(doyle): Flip the x coordinates to flip the tex
		v4 copyNDC = heroTexNDC;
		heroTexNDC.x = copyNDC.z;
		heroTexNDC.z = copyNDC.x;
	}

	updateBufferObject(state->renderer.vbo, heroTexNDC);
	renderer_entity(&state->renderer, hero, 0.0f, V3(0, 0, 0));

	Entity npcEnt = {V2(300.0f, 300.0f), V2(0.0f, 0.0f), hero->size, hero->tex};

	v4 npcRects[2] = {
		{944.0f, 918.0f, 1010.0f, 816.0f},
		{944.0f, 812.0f, 1010.0f, 710.0f},
	};

	LOCAL_PERSIST f32 frameDurationInSec = 0.0f;
	LOCAL_PERSIST i32 npcLastRect        = 0;

	frameDurationInSec -= dt;
	v4 npcCurrRect = npcRects[npcLastRect];
	if (frameDurationInSec <= 0.0f)
	{
		if (npcLastRect == 0)
		{
			npcCurrRect = npcRects[1];
			npcLastRect = 1;
		}
		else
		{
			npcCurrRect = npcRects[0];
			npcLastRect = 0;
		}
		frameDurationInSec = 0.3f;
	}

	v4 npcTexNDC = v4_scale(npcCurrRect, 1.0f / ndcFactor);
	updateBufferObject(state->renderer.vbo, npcTexNDC);
	renderer_entity(&state->renderer, &npcEnt, 0.0f, V3(0, 0, 0));

	// TODO(doyle): Clean up lines
	// Renderer::~Renderer() { glDeleteVertexArrays(1, &this->quadVAO); }
}

