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

enum GameWorldStateFlags
{
	gameworldstateflags_init          = (1 << 0),
	gameworldstateflags_level_started = (1 << 1),
	gameworldstateflags_player_lost   = (1 << 2),
	gameworldstateflags_create_player = (1 << 3),
};

typedef struct GameWorldState
{
	enum GameWorldStateFlags flags;

	MemoryArena_ entityArena;

	v2 *entityVertexListCache[entitytype_count];
	Entity *entityList;
	i32 entityListSize;
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
	f32 starOpacity;
	f32 starMinOpacity;
	b32 starFadeAway;
	i32 numStarP;

	// TODO(doyle): Audio mixing instead of multiple renderers
	AudioRenderer *audioRenderer;
	i32 numAudioRenderers;

	f32 pixelsPerMeter;
	Rect camera;
	v2 size;

	// TODO(doyle): Ensure we change this if it gets too big
	b32 collisionTable[entitytype_count][entitytype_count];

	i32 score;
	i32 scoreMultiplier;
	f32 scoreMultiplierBarTimer;
	f32 scoreMultiplierBarThresholdInS;

	f32 timeSinceLastShot;

} GameWorldState;

typedef struct StartMenuState
{
	b32 init;

	f32 startPromptBlinkTimer;
	b32 startPromptShow;

	char **resStrings;
	i32 numResStrings;
	i32 resStringDisplayIndex;

	b32 newResolutionRequest;

	b32 optionsShow;
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

#define ASTEROID_GET_STATE_DATA(state, type) (type *)asteroid_getStateData_(state, appstate_##type)
void *asteroid_getStateData_(GameState *state, enum AppState appState);

void asteroid_gameUpdateAndRender(GameState *state, Memory *memory,
                                  v2 windowSize, f32 dt);

#endif
