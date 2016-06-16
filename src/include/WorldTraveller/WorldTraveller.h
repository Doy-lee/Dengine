#ifndef WORLDTRAVELLER_GAME_H
#define WORLDTRAVELLER_GAME_H

#include <Dengine/OpenGL.h>
#include <Dengine/Common.h>
#include <Dengine/Renderer.h>
#include <Dengine/Shader.h>
#include <Dengine/AssetManager.h>
#include <Dengine/Entity.h>

namespace WorldTraveller
{
GLOBAL_VAR const i32 NUM_KEYS = 1024;
GLOBAL_VAR const i32 METERS_TO_PIXEL = 100;

enum Cardinal
{
	cardinal_north = 0,
	cardinal_west  = 1,
	cardinal_south = 2,
	cardinal_east  = 3,
	cardinal_num,
	cardinal_null

};

enum State
{
	state_active,
	state_menu,
	state_win
};

class Game
{
public:
	State state;
	GLboolean keys[NUM_KEYS];
	i32 width, height;

	Game(i32 width, i32 height);
	~Game();

	void init();

	void update(const f32 dt);
	void render();
private:
	Dengine::Shader *shader;
	Dengine::Renderer *renderer;
	Dengine::Entity hero;
};
}

#endif
