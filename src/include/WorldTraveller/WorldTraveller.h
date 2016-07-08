#ifndef WORLDTRAVELLER_GAME_H
#define WORLDTRAVELLER_GAME_H

#include "Dengine/Common.h"
#include "Dengine/Entity.h"
#include "Dengine/Renderer.h"

#define NUM_KEYS 1024
#define METERS_TO_PIXEL 64

enum State
{
	state_active,
	state_menu,
	state_win,
};

typedef struct World
{
	Entity *entities;
	i32 maxEntities;

	enum TexList texType;

	v2 cameraPos; // In pixels
	v4 bounds; // In pixels

	i32 heroIndex;
	i32 freeEntityIndex;

} World;

typedef struct GameState
{
	enum State state;
	b32 keys[NUM_KEYS];

	Renderer renderer;

	World world[4];
	i32 currWorldIndex;
	i32 tileSize;

	AssetManager assetManager;
} GameState;


void worldTraveller_gameInit(GameState *state, v2i windowSize);
void worldTraveller_gameUpdateAndRender(GameState *state, const f32 dt);
#endif
