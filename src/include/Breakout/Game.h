#ifndef BREAKOUT_GAME_H
#define BREAKOUT_GAME_H

#include <Dengine/OpenGL.h>
#include <Dengine/Common.h>
#include <Dengine/Sprite.h>
#include <Dengine/Shader.h>
#include <Dengine/AssetManager.h>

namespace Breakout
{

GLOBAL_VAR const i32 NUM_KEYS = 1024;

enum GameState
{
	GAME_ACTIVE,
	GAME_MENU,
	GAME_WIN
};

class Game
{
public:
	GameState state;
	GLboolean keys[NUM_KEYS];
	i32 width, height;

	Game(i32 width, i32 height);
	~Game();

	void init(Dengine::AssetManager *assetManager);

	void processInput(f32 dt);
	void update(f32 dt);
	void render();
private:
	Dengine::Shader *shader;
	Dengine::Sprite player;
};
}

#endif
