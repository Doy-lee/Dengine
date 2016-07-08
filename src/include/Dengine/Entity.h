#ifndef DENGINE_ENTITY_H
#define DENGINE_ENTITY_H

#include "Dengine/Texture.h"
#include "Dengine/Math.h"

enum Direction
{
	direction_north,
	direction_west,
	direction_south,
	direction_east,
	direction_num,
	direction_null,
};

typedef struct EntityAnim
{
	v4 *rect;
	i32 numRects;
	i32 currRectIndex;

	f32 duration;
	f32 currDuration;
} EntityAnim;

typedef struct Entity
{
	v2 pos;  // Position
	v2 dPos; // Velocity
	v2 size;
	enum Direction direction;
	Texture *tex;
	b32 collides;

	// TODO(doyle): String based access
	EntityAnim anim[16];
	i32 freeAnimIndex;
	i32 currAnimIndex;
} Entity;

INTERNAL inline v4 getEntityScreenRect(Entity entity)
{
	v4 result = getRect(entity.pos, entity.size);
	return result;
}

#endif
