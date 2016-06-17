#ifndef WORLDTRAVELLER_GAME_H
#define WORLDTRAVELLER_GAME_H

#include <Dengine/AssetManager.h>
#include <Dengine/Common.h>
#include <Dengine/Entity.h>
#include <Dengine/Math.h>
#include <Dengine/OpenGL.h>
#include <Dengine/Renderer.h>
#include <Dengine/Shader.h>

#define NUM_KEYS 1024
#define METERS_TO_PIXEL 100

enum State
{
	state_active,
	state_menu,
	state_win
};

struct GameState
{
	State state;
	GLboolean keys[NUM_KEYS];
	i32 width, height;

	Renderer renderer;
	Entity hero;
};

void worldTraveller_gameInit(GameState *state);
void worldTraveller_gameUpdateAndRender(GameState *state, const f32 dt);
#endif
