#ifndef WORLDTRAVELLER_GAME_H
#define WORLDTRAVELLER_GAME_H

#include "Dengine/AssetManager.h"
#include "Dengine/Audio.h"
#include "Dengine/Common.h"
#include "Dengine/Math.h"
#include "Dengine/MemoryArena.h"
#include "Dengine/Platform.h"
#include "Dengine/Renderer.h"
#include "Dengine/UserInterface.h"

#define NUM_KEYS 1024
#define METERS_TO_PIXEL 240

/* Forward declaration */
typedef struct Entity Entity;

typedef struct Config
{
	b32 playWorldAudio;
	b32 showStatMenu;
	b32 showDebugDisplay;
} Config;

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
	Config config;
	MemoryArena arena;
	UiState uiState;
} GameState;

void worldTraveller_gameInit(GameState *state, v2 windowSize);
void worldTraveller_gameUpdateAndRender(GameState *state, f32 dt);
#endif
