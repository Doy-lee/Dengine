#include <WorldTraveller/WorldTraveller.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>

namespace WorldTraveller
{

// TODO(doyle): Entity list for each world
// TODO(doyle): Jumping mechanics
// TODO(doyle): Collision


Game::Game(i32 width, i32 height)
{
	this->width  = width;
	this->height = height;

	for (i32 i        = 0; i < NUM_KEYS; i++)
		this->keys[i] = FALSE;
}

Game::~Game() { delete this->renderer; }

void Game::init()
{
	/* Initialise assets */
	std::string texFolder = "data/textures/WorldTraveller/";
	Dengine::AssetManager::loadShaderFiles("data/shaders/sprite.vert.glsl",
	                                       "data/shaders/sprite.frag.glsl", "sprite");

	Dengine::AssetManager::loadTextureImage(texFolder + "hero.png", "hero");
	Dengine::AssetManager::loadTextureImage(texFolder + "wall.png", "wall");
	Dengine::AssetManager::loadTextureImage(texFolder + "hitMarker.png", "hitMarker");

	this->shader = Dengine::AssetManager::getShader("sprite");
	this->shader->use();

	glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(this->width), 0.0f,
	                                  static_cast<GLfloat>(this->height), 0.0f, 1.0f);
	this->shader->uniformSetMat4fv("projection", projection);

	GLuint projectionLoc = glGetUniformLocation(this->shader->id, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	/* Init game state */
	this->state    = state_active;
	this->renderer = new Dengine::Renderer(this->shader);

	/* Init hero */
	this->hero = Dengine::Entity(glm::vec2(0, 0), "hero");

	glm::vec2 screenCentre       = glm::vec2(this->width / 2.0f, this->height / 2.0f);
	glm::vec2 heroOffsetToCentre = glm::vec2(-((i32)hero.size.x / 2), -((i32)hero.size.y / 2));

	glm::vec2 heroCentered = screenCentre + heroOffsetToCentre;
	hero.pos               = heroCentered;

	srand(static_cast<u32>(glfwGetTime()));
}

void Game::update(const f32 dt)
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

	if (this->keys[GLFW_KEY_SPACE])
	{
	}

	if (this->keys[GLFW_KEY_RIGHT])
	{
		ddPos.x = 1.0f;
	}
	if (this->keys[GLFW_KEY_LEFT])
	{
		ddPos.x = -1.0f;
	}

	if (this->keys[GLFW_KEY_UP])
	{
		ddPos.y = 1.0f;
	}
	if (this->keys[GLFW_KEY_DOWN])
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

	const f32 heroSpeed = static_cast<f32>((22.0f * METERS_TO_PIXEL)); // m/s^2
	ddPos *= heroSpeed;

	// TODO(doyle): Counteracting force on player's acceleration is arbitrary
	ddPos += -(5.5f * hero.dPos);

	glm::vec2 newHeroP = (0.5f * ddPos) * Dengine::Math::squared(dt) + (hero.dPos * dt) + hero.pos;

	hero.dPos = (ddPos * dt) + hero.dPos;
	hero.pos = newHeroP;
}

void Game::render()
{

	Dengine::Renderer *renderer = this->renderer;

	Dengine::Entity wall = Dengine::Entity(glm::vec2(0, 0), "wall");
	glm::vec2 maxTilesOnScreen =
	    glm::vec2((this->width / wall.size.x), (this->height / wall.size.y));

	Dengine::Entity hitMarker = Dengine::Entity(glm::vec2(100, 100), "hitMarker");

	for (i32 x = 0; x < maxTilesOnScreen.x; x++)
	{
		for (i32 y = 0; y < maxTilesOnScreen.y; y++)
		{
			if (x == 0 || x == maxTilesOnScreen.x - 1 || y == 0 || y == maxTilesOnScreen.y - 1)
			{
				wall.pos = glm::vec2(x * wall.tex->getWidth(), y * wall.tex->getHeight());

				renderer->drawEntity(&wall);
			}
		}
	}
	renderer->drawEntity(&hero);
	renderer->drawEntity(&hitMarker);
}
} // namespace Dengine
