#ifndef ASTEROID_H
#define ASTEROID_H

#include "Dengine/AssetManager.h"
#include "Dengine/Common.h"
#include "Dengine/Entity.h"
#include "Dengine/MemoryArena.h"
#include "Dengine/Platform.h"
#include "Dengine/Renderer.h"

typedef struct GameState {
	b32 init;

	Entity entityList[1024];
	i32 entityIndex;

	Rect camera;

	AssetManager assetManager;
	KeyInput input;

	MemoryArena_ transientArena;
	MemoryArena_ persistentArena;

	Renderer renderer;
} GameState;

void asteroid_gameUpdateAndRender(GameState *state, Memory *memory,
                                  v2 windowSize, f32 dt);

#endif
