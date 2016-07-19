#ifndef DENGINE_ENTITY_H
#define DENGINE_ENTITY_H

#include "Dengine/Common.h"
#include "Dengine/Math.h"
#include "Dengine/Texture.h"

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
	i32 queuedAttack;
} EntityStats;

typedef struct EntityAnim_
{
	Animation *anim;
	i32 currFrame;
	f32 currDuration;
} EntityAnim_;

typedef struct Entity
{
	v2 pos;  // Position
	v2 dPos; // Velocity
	v2 hitboxSize;
	v2 renderSize;

	enum EntityState state;
	enum EntityType type;
	enum Direction direction;

	Texture *tex;
	b32 collides;

	// TODO(doyle): String based access
	// TODO(doyle): Separate animation refs from curr animation state, entity
	// only ever has one active current animation. ATM every entity animation
	// has a currframe and duration .. either that or we stop resetting
	// animation on entity context switch
	EntityAnim_ anim[16];
	i32 currAnimId;
	u32 currAnimCyclesCompleted;

	EntityStats *stats;
} Entity;

#endif
