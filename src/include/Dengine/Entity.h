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

enum EntityAnimId
{
	entityanimid_idle,
	entityanimid_walk,
	entityanimid_wave,
	entityanimid_battlePose,
	entityanimid_tackle,
	entityanimid_count,
	entityanimid_invalid,
};

typedef struct EntityAnim
{
	v4 *rect;
	i32 numRects;
	i32 currRectIndex;

	f32 duration;
	f32 currDuration;
} EntityAnim;

typedef struct EntityStats
{
	f32 maxHealth;
	f32 health;

	f32 actionRate;
	f32 actionTimer;
	f32 actionSpdMul;
} EntityStats;

typedef struct Entity
{
	v2 pos;  // Position
	v2 dPos; // Velocity
	v2 size;

	enum EntityType type;
	enum Direction direction;

	Texture *tex;
	b32 collides;

	// TODO(doyle): String based access
	EntityAnim anim[16];
	i32 currAnimId;
	u32 currAnimCyclesCompleted;

	EntityStats *stats;
} Entity;

INTERNAL inline v4 getEntityScreenRect(Entity entity)
{
	v4 result = math_getRect(entity.pos, entity.size);
	return result;
}

#endif
