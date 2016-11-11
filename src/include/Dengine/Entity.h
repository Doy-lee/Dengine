#ifndef ENTITY_H
#define ENTITY_H

#include "Dengine/Assets.h"
#include "Dengine/Common.h"

typedef struct AudioRenderer AudioRenderer;

enum Direction
{
	direction_north,
	direction_west,
	direction_south,
	direction_east,
	direction_null,
	direction_num,
};

enum EntityType
{
	entitytype_invalid,
	entitytype_ship,
	entitytype_count,
};

typedef struct EntityAnim
{
	Animation *anim;
	i32 currFrame;
	f32 currDuration;

	u32 timesCompleted;
} EntityAnim;

typedef struct Entity
{
	i32 id;

	i32 childIds[8];
	i32 numChilds;

	v2 pos;
	v2 velocity;
	v2 hitbox;
	v2 size;

	f32 scale;
	f32 rotation;

	b32 invisible;

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

	EntityAnim animList[16];
	i32 animListIndex;

	// TODO(doyle): Audio mixing instead of multiple renderers
	AudioRenderer *audioRenderer;
	i32 numAudioRenderers;
} Entity;

#endif
