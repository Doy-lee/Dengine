#ifndef DENGINE_ENTITY_H
#define DENGINE_ENTITY_H

#include "Dengine/Common.h"
#include "Dengine/Math.h"
#include "Dengine/Assets.h"

typedef struct AssetManager AssetManager;
typedef struct AudioRenderer AudioRenderer;
typedef struct MemoryArena MemoryArena_;
typedef struct World World;
typedef struct EventQueue EventQueue;

typedef struct Entity Entity;

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
	entitytype_weapon,
	entitytype_projectile,
	entitytype_attackObject,
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
	entityattack_claudeAttack,
	entityattack_claudeAttackUp,
	entityattack_claudeAttackDown,
	entityattack_claudeDragonHowl,
	entityattack_claudeEnergySword,
	entityattack_claudeRipperBlast,
	entityattack_claudeAirSlash,
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

	Entity *weapon;
} EntityStats;

typedef struct EntityAnim
{
	Animation *anim;
	i32 currFrame;
	f32 currDuration;

	u32 timesCompleted;
} EntityAnim;

struct Entity
{
	i32 id;

	i32 childIds[8];
	i32 numChilds;

	v2 pos;  // Position
	v2 dPos; // Velocity
	v2 hitbox;
	v2 size;

	f32 scale;
	f32 rotation;

	b32 invisible;

	enum EntityState state;
	enum EntityType type;
	enum Direction direction;

	Texture *tex;
	b32 flipX;
	b32 flipY;

	// TODO(doyle): Two collision flags, we want certain entities to collide
	// with certain types of entities only (i.e. projectile from hero to enemy,
	// only collides with enemy). Having two flags is redundant, but! it does
	// allow for early-exit in collision check if the entity doesn't collide at
	// all
	b32 collides;
	enum EntityType collidesWith[entitytype_count];

	EntityAnim animList[16];
	i32 currAnimId;

	EntityStats *stats;

	// TODO(doyle): Audio mixing instead of multiple renderers
	AudioRenderer *audioRenderer;
	i32 numAudioRenderers;
};

SubTexture entity_getActiveSubTexture(Entity *const entity);
void entity_setActiveAnim(EventQueue *eventQueue, Entity *const entity,
                          const char *const animName);
void entity_updateAnim(EventQueue *eventQueue, Entity *const entity,
                       const f32 dt);
void entity_addAnim(AssetManager *const assetManager, Entity *const entity,
                    const char *const animName);
Entity *const entity_add(MemoryArena_ *const arena, World *const world,
                         const v2 pos, const v2 size, const f32 scale,
                         const enum EntityType type,
                         const enum Direction direction, Texture *const tex,
                         const b32 collides);
void entity_clearData(MemoryArena_ *const arena, World *const world,
                      Entity *const entity);
Entity *entity_get(World *const world, const i32 entityId);
i32 entity_getIndex(World *const world, const i32 entityId);
#endif
