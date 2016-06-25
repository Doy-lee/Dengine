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
	state_win,
};

typedef struct Tile
{
	v2 pos;
} Tile;

typedef struct World
{
	Tile tiles[2048];
	enum TexList texType;
} World;

typedef struct GameState
{
	enum State state;
	b32 keys[NUM_KEYS];
	i32 width, height;

	Renderer renderer;
	i32 heroIndex;

	World world[4];
	i32 currWorldIndex;
	i32 tileSize;

	// TODO(doyle): Make size of list dynamic
	Entity entityList[256];
	i32 freeEntityIndex;
} GameState;

void worldTraveller_gameInit(GameState *state);
void worldTraveller_gameUpdateAndRender(GameState *state, const f32 dt);
#endif
