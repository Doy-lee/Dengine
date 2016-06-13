#include <Breakout\Game.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Breakout
{
Game::Game(i32 width, i32 height)
{
	this->width = width;
	this->height = height;
	glCheckError();
}

Game::~Game()
{
	delete this->renderer;
}

void Game::init()
{
	/* Initialise assets */
	Dengine::AssetManager::loadShaderFiles("data/shaders/sprite.vert.glsl",
	                                       "data/shaders/sprite.frag.glsl",
	                                       "sprite");

	Dengine::AssetManager::loadTextureImage("data/textures/container.jpg",
	                                        "container");
	Dengine::AssetManager::loadTextureImage("data/textures/wall.jpg", "wall");
	Dengine::AssetManager::loadTextureImage("data/textures/awesomeface.png",
	                                        "awesomeface");
	Dengine::AssetManager::loadTextureImage("data/textures/plain_terrain.jpg",
	                                        "plain_terrain");

	this->shader = Dengine::AssetManager::getShader("sprite");
	this->shader->use();

	glm::mat4 projection =
	    glm::ortho(0.0f, static_cast<GLfloat>(this->width), 0.0f,
	               static_cast<GLfloat>(this->height), 0.0f, 1.0f);
	this->shader->uniformSetMat4fv("projection", projection);

	GLuint projectionLoc = glGetUniformLocation(this->shader->id, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	/* Init game state */
	this->state = GAME_ACTIVE;
	this->renderer = new Dengine::Renderer(this->shader);
}

void Game::processInput(const f32 dt) {}
void Game::update(const f32 dt) {}
void Game::render()
{
	const Dengine::Texture *tex =
	    Dengine::AssetManager::getTexture("plain_terrain");
	glm::vec2 pos  = glm::vec2(0, 0);
	glm::vec2 size = glm::vec2(1280.0f, 720.0f);
	this->renderer->drawSprite(tex, pos, size);

	tex  = Dengine::AssetManager::getTexture("container");
	pos  = glm::vec2(200, 200);
	size = glm::vec2(250.0f, 250.0f);
	this->renderer->drawSprite(tex, pos, size);
}
}
