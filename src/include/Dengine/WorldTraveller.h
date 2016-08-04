#ifndef WORLDTRAVELLER_GAME_H
#define WORLDTRAVELLER_GAME_H

#include "Dengine/AssetManager.h"
#include "Dengine/Audio.h"
#include "Dengine/Common.h"
#include "Dengine/Entity.h"
#include "Dengine/Math.h"
#include "Dengine/MemoryArena.h"
#include "Dengine/Platform.h"
#include "Dengine/Renderer.h"

#define NUM_KEYS 1024
#define METERS_TO_PIXEL 240

typedef struct UiState
{
	v2 *mouseP;
	b32 *mouseIsDown;
	i32 hotItem;
	i32 activeItem;
} UiState;

typedef struct World
{
	Entity *entities;
	i32 maxEntities;
	b32 *entityIdInBattle;
	i32 numEntitiesInBattle;

	enum TexList texType;

	v2 cameraPos; // In pixels
	v4 bounds; // In pixels

	i32 heroId;
	i32 freeEntityIndex;
	u32 uniqueIdAccumulator;

	Entity *soundscape;
} World;

typedef struct GameState
{
	enum State state;
	KeyInput input;
	v2 mouse;

	Renderer renderer;

	World world[4];
	i32 currWorldIndex;
	i32 tileSize;

	AssetManager assetManager;
	AudioManager audioManager;
	MemoryArena arena;
} GameState;

void worldTraveller_gameInit(GameState *state, v2 windowSize);
void worldTraveller_gameUpdateAndRender(GameState *state, f32 dt);
#endif
