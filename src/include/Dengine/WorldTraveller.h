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

enum EventType
{
	eventtype_null = 0,
	eventtype_start_attack,
	eventtype_end_attack,
	eventtype_start_anim,
	eventtype_end_anim,
	eventtype_entity_died,
	eventtype_count,
};

typedef struct Event
{
	enum EventType type;
	void *data;
} Event;

typedef struct EventQueue
{
	Event queue[1024];
	i32 numEvents;
} EventQueue;

enum RectBaseline
{
	rectbaseline_top,
	rectbaseline_topLeft,
	rectbaseline_topRight,
	rectbaseline_bottom,
	rectbaseline_bottomRight,
	rectbaseline_bottomLeft,
	rectbaseline_left,
	rectbaseline_right,
	rectbaseline_center,
	rectbaseline_count,

};

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

	i32 cameraFollowingId;
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
	EventQueue eventQueue;
} GameState;

void worldTraveller_gameInit(GameState *state, v2 windowSize);
void worldTraveller_gameUpdateAndRender(GameState *state, f32 dt);
void worldTraveller_registerEvent(EventQueue *eventQueue, enum EventType type,
                                  void *data);
#endif
