#ifndef DENGINE_ENTITY_H
#define DENGINE_ENTITY_H

#include "Dengine/Common.h"
#include "Dengine/Math.h"

typedef struct AssetManager AssetManager;
typedef struct AudioRenderer AudioRenderer;
typedef struct MemoryArena MemoryArena;
typedef struct Texture Texture;
typedef struct Animation Animation;
typedef struct World World;

enum Direction
{
	direction_north,
	direction_west,
	direction_south,
	direction_east,
	direction_num,
	direction_null,
};

enum EntityType
{
	entitytype_null,
	entitytype_hero,
	entitytype_npc,
	entitytype_mob,
	entitytype_tile,
	entitytype_soundscape,
	entitytype_count,
};

enum EntityState
{
	entitystate_idle,
	entitystate_battle,
	entitystate_attack,
	entitystate_dead,
	entitystate_count,
	entitystate_invalid,
};

enum EntityAttack
{
	entityattack_tackle,
	entityattack_count,
	entityattack_invalid,
};

typedef struct EntityStats
{
	f32 maxHealth;
	f32 health;

	f32 actionRate;
	f32 actionTimer;
	f32 actionSpdMul;

	f32 busyDuration;
	i32 entityIdToAttack;
	enum EntityAttack queuedAttack;
} EntityStats;

typedef struct EntityAnim
{
	Animation *anim;
	i32 currFrame;
	f32 currDuration;
} EntityAnim;

typedef struct Entity
{
	i32 id;

	v2 pos;  // Position
	v2 dPos; // Velocity
	v2 hitboxSize;
	v2 renderSize;

	enum EntityState state;
	enum EntityType type;
	enum Direction direction;

	Texture *tex;
	b32 collides;

	EntityAnim animList[16];
	i32 currAnimId;

	EntityStats *stats;
	AudioRenderer *audioRenderer;
	i32 numAudioRenderers;
} Entity;

void entity_setActiveAnim(Entity *const entity, const char *const animName);
void entity_updateAnim(Entity *const entity, const f32 dt);
void entity_addAnim(AssetManager *const assetManager, Entity *const entity,
                    const char *const animName);
Entity *const entity_add(MemoryArena *const arena, World *const world,
                         const v2 pos, const v2 size,
                         const enum EntityType type,
                         const enum Direction direction, Texture *const tex,
                         const b32 collides);
void entity_clearData(MemoryArena *const arena, World *const world,
                      Entity *const entity);
i32 entity_getIndex(World *const world, const i32 entityId);
#endif
