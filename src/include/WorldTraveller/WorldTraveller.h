#ifndef WORLDTRAVELLER_GAME_H
#define WORLDTRAVELLER_GAME_H

#include <Dengine/Common.h>
#include <Dengine/Entity.h>
#include <Dengine/Renderer.h>

#define NUM_KEYS 1024
#define METERS_TO_PIXEL 100

enum State
{
	state_active,
	state_menu,
	state_win
};

enum Direction
{
	direction_north,
	direction_west,
	direction_south,
	direction_east,
	direction_num,
};

typedef struct GameState
{
	enum State state;
	b32 keys[NUM_KEYS];
	i32 width, height;

	Renderer renderer;
	Entity hero;
	enum Direction heroLastDirection;
} GameState;

void worldTraveller_gameInit(GameState *state);
void worldTraveller_gameUpdateAndRender(GameState *state, const f32 dt);
#endif
