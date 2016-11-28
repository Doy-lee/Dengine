#ifndef ASTEROID_H
#define ASTEROID_H

#include "Dengine/AssetManager.h"
#include "Dengine/Audio.h"
#include "Dengine/Common.h"
#include "Dengine/Entity.h"
#include "Dengine/MemoryArena.h"
#include "Dengine/Platform.h"
#include "Dengine/Renderer.h"
#include "Dengine/Ui.h"

enum AppState
{
	appstate_StartMenuState,
	appstate_GameWorldState,
	appstate_count,
};

typedef struct GameWorldState
{
	b32 init;

	MemoryArena_ entityArena;

	v2 *entityVertexListCache[entitytype_count];
	Entity entityList[1024];
	i32 entityIndex;
	u32 entityIdCounter;

	u32 asteroidCounter;
	u32 numAsteroids;
	v2 *asteroidSmallVertexCache[3];
	v2 *asteroidMediumVertexCache[3];
	v2 *asteroidLargeVertexCache[3];

	v2 *bulletVertexCache;
	v2 *particleVertexCache;

	v2 *starPList;
	i32 numStarP;

	// TODO(doyle): Audio mixing instead of multiple renderers
	AudioRenderer *audioRenderer;
	i32 numAudioRenderers;

	f32 pixelsPerMeter;
	v2 worldSize;
	Rect camera;

	// TODO(doyle): Ensure we change this if it gets too big
	b32 collisionTable[entitytype_count][entitytype_count];
} GameWorldState;

typedef struct StartMenuState
{
	f32 startMenuGameStartBlinkTimer;
	b32 startMenuToggleShow;
} StartMenuState;

typedef struct GameState
{
	b32 init;

	enum AppState currState;
	void *appStateData[appstate_count];

	MemoryArena_ transientArena;
	MemoryArena_ persistentArena;

	AudioManager audioManager;
	AssetManager assetManager;
	InputBuffer input;
	Renderer renderer;

	UiState uiState;
} GameState;

#define ASTEROID_GET_STATE_DATA(state, type)                                   \
	(type *)asteroid_getStateData_(state, appstate_##type)
void *asteroid_getStateData_(GameState *state, enum AppState appState);

void asteroid_gameUpdateAndRender(GameState *state, Memory *memory,
                                  v2 windowSize, f32 dt);

#endif
