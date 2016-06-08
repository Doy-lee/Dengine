#include <Breakout\Game.h>

namespace Breakout
{
Game::Game(i32 width, i32 height) {}
Game::~Game() {}

void Game::init(Dengine::AssetManager *assetManager)
{
	/* Initialise assets */
	i32 result = 0;
	result = assetManager->loadShaderFiles("data/shaders/sprite.vert.glsl",
	                                      "data/shaders/sprite.frag.glsl",
	                                      "sprite");
	if (result)
	{
		// TODO(doyle): Do something
	}

	result = assetManager->loadTextureImage("data/textures/container.jpg",
	                                       "container");
	if (result)
	{
		
	}

	result = assetManager->loadTextureImage("data/textures/wall.jpg",
	                                       "wall");
	if (result)
	{
		
	}

	result = assetManager->loadTextureImage("data/textures/awesomeface.png",
	                                       "awesomeface");
	if (result)
	{
		
	}

	result = assetManager->loadTextureImage("data/textures/plain_terrain.png",
	                                       "plain_terrain");

	this->shader = assetManager->getShader("sprite");

	/* Init player */
	const Dengine::Texture *tex = assetManager->getTexture("plain_terrain");
	this->player = Dengine::Sprite();
	this->player.loadSprite(tex, glm::vec2(0, 0));

	/* Init game state */
	this->state = GAME_ACTIVE;

}

void Game::processInput(const f32 dt) {}
void Game::update(const f32 dt) {}
void Game::render()
{
	shader->use();
	this->player.render(shader);
}
}
