#ifndef ASTEROID_H
#define ASTEROID_H

#include "Dengine/AssetManager.h"
#include "Dengine/Audio.h"
#include "Dengine/Common.h"
#include "Dengine/Entity.h"
#include "Dengine/MemoryArena.h"
#include "Dengine/Platform.h"
#include "Dengine/Renderer.h"

typedef struct World
{
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

	// TODO(doyle): Audio mixing instead of multiple renderers
	AudioRenderer *audioRenderer;
	i32 numAudioRenderers;

	f32 pixelsPerMeter;
	v2 worldSize;
	Rect camera;

	// TODO(doyle): Ensure we change this if it gets too big
	b32 collisionTable[entitytype_count][entitytype_count];
} World;

typedef struct GameState {
	b32 init;

	MemoryArena_ transientArena;
	MemoryArena_ persistentArena;

	AudioManager audioManager;
	AssetManager assetManager;
	InputBuffer input;
	Renderer renderer;
	World world;

} GameState;

void asteroid_gameUpdateAndRender(GameState *state, Memory *memory,
                                  v2 windowSize, f32 dt);

#endif
