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
	const f32 heroSpeed = static_cast<f32>((3.0f * METERS_TO_PIXEL) * dt);

	if (this->keys[GLFW_KEY_SPACE])
	{
#if 0
		Dengine::Entity hitMarker = Dengine::Entity(glm::vec2(0, 0), "hitMarker");
		glm::vec2 hitMarkerP =
		    glm::vec2((hero.pos.x * 1.5f) + hitMarker.tex->getWidth(), hero.pos.y * 1.5f);
		hitMarker.pos = hitMarkerP;
		renderer->drawEntity(&hitMarker);
#endif
	}

	if (this->keys[GLFW_KEY_RIGHT])
	{
		hero.pos.x += heroSpeed;
	}
	else if (this->keys[GLFW_KEY_LEFT])
	{
		hero.pos.x -= heroSpeed;
	}
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
