#include <WorldTraveller/WorldTraveller.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void worldTraveller_gameInit(GameState *state)
{
	/* Initialise assets */

	asset_loadTextureImage("data/textures/WorldTraveller/elisa-spritesheet1.png",
	                 "hero");
	glCheckError();

	state->state       = state_active;
	Renderer *renderer = &state->renderer;
	asset_loadShaderFiles("data/shaders/sprite.vert.glsl",
	                      "data/shaders/sprite.frag.glsl", "sprite");
	glCheckError();

	renderer->shader = asset_getShader("sprite");
	shader_use(renderer->shader);
	glm::mat4 projection = glm::ortho(0.0f, CAST(GLfloat) state->width, 0.0f,
	                                  CAST(GLfloat) state->height, 0.0f, 1.0f);
	shader_uniformSetMat4fv(renderer->shader, "projection", projection);
	glCheckError();

	/* Init renderer */
	// NOTE(doyle): Draws a series of triangles (three-sided polygons) using
	// vertices v0, v1, v2, then v2, v1, v3 (note the order)
	glm::vec4 vertices[] = {
	    //  x     y       s     t
	    {0.0f, 1.0f, 0.0f, 1.0f}, // Top left
	    {0.0f, 0.0f, 0.0f, 0.0f}, // Bottom left
	    {1.0f, 1.0f, 1.0f, 1.0f}, // Top right
	    {1.0f, 0.0f, 1.0f, 0.0f}, // Bottom right
	};

	GLuint VBO;
	/* Create buffers */
	glGenVertexArrays(1, &renderer->quadVAO);
	glGenBuffers(1, &VBO);
	glCheckError();

	/* Bind buffers */
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindVertexArray(renderer->quadVAO);

	/* Configure VBO */
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glCheckError();

	/* Configure VAO */
	const GLuint numVertexElements = 4;
	const GLuint vertexSize        = sizeof(glm::vec4);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, numVertexElements, GL_FLOAT, GL_FALSE, vertexSize,
	                      (GLvoid *)0);
	glCheckError();

	/* Unbind */
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glCheckError();
	/* Init hero */

	Entity *hero = &state->hero;
	hero->tex    = asset_getTexture("hero");
	hero->size   = glm::vec2(100, 100);

	glm::vec2 screenCentre =
	    glm::vec2(state->width / 2.0f, state->height / 2.0f);
	glm::vec2 heroOffsetToCentre =
	    glm::vec2(-((i32)hero->size.x / 2), -((i32)hero->size.y / 2));

	glm::vec2 heroCentered = screenCentre + heroOffsetToCentre;
	hero->pos              = heroCentered;

	srand(CAST(u32)(glfwGetTime()));
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

	glm::vec2 ddPos = glm::vec2(0, 0);

	if (state->keys[GLFW_KEY_SPACE])
	{
	}

	if (state->keys[GLFW_KEY_RIGHT])
	{
		ddPos.x = 1.0f;
	}
	if (state->keys[GLFW_KEY_LEFT])
	{
		ddPos.x = -1.0f;
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
		ddPos *= 0.70710678118f;
	}

	f32 heroSpeed = CAST(f32)(22.0f * METERS_TO_PIXEL); // m/s^2
	if (state->keys[GLFW_KEY_LEFT_SHIFT])
	{
		heroSpeed = CAST(f32)(22.0f * 5.0f * METERS_TO_PIXEL);
	}
	ddPos *= heroSpeed;

	// TODO(doyle): Counteracting force on player's acceleration is arbitrary
	Entity *hero = &state->hero;
	ddPos += -(5.5f * hero->dPos);

	glm::vec2 newHeroP =
	    (0.5f * ddPos) * squared(dt) + (hero->dPos * dt) + hero->pos;

	hero->dPos = (ddPos * dt) + hero->dPos;
	hero->pos  = newHeroP;
}

void worldTraveller_gameUpdateAndRender(GameState *state, const f32 dt)
{
	/* Update */
	parseInput(state, dt);
	glCheckError();

	/* Render */
	renderer_entity(&state->renderer, &state->hero);
	// TODO(doyle): Clean up lines
	// Renderer::~Renderer() { glDeleteVertexArrays(1, &this->quadVAO); }
}

