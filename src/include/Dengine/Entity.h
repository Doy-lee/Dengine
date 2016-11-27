#ifndef ENTITY_H
#define ENTITY_H

#include "Dengine/Assets.h"
#include "Dengine/Common.h"
#include "Dengine/Renderer.h"

typedef struct AudioRenderer AudioRenderer;
typedef struct MemoryArena_ MemoryArena;

enum Direction
{
	direction_north,
	direction_northwest,
	direction_west,
	direction_southwest,
	direction_south,
	direction_southeast,
	direction_east,
	direction_northeast,
	direction_count,
	direction_null,
	direction_num,
};

INTERNAL inline enum Direction invertDirection(enum Direction direction)
{
	enum Direction result = (direction + 4) % direction_count;

	return result;
}

INTERNAL inline enum Direction rotateDirectionWest90(enum Direction direction)
{
	enum Direction result = (direction + 2) % direction_count;

	return result;
}

INTERNAL inline enum Direction rotateDirectionEast90(enum Direction direction)
{
	enum Direction result = (direction + 2) % direction_count;

	return result;
}


#define NULL_ENTITY_ID 0
enum EntityType
{
	entitytype_invalid,
	entitytype_null,
	entitytype_ship,

	// NOTE(doyle): Asteroids must be grouped since we use >= and <= logic to
	// combine asteroid logic together
	entitytype_asteroid_small,
	entitytype_asteroid_medium,
	entitytype_asteroid_large,

	entitytype_particle,

	entitytype_bullet,
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
	u32 id;

	v2 pos;
	v2 dP;
	v2 particleInitDp;

	v2 hitbox;
	v2 size;

	/*
	   NOTE(doyle): Offset is the vector to shift the polygons "origin" to the
	   world origin. In our case this should strictly be negative as we have
	   a requirement that all vertex points must be strictly positive.
   */
	v2 offset;

	enum RenderMode renderMode;
	i32 numVertexPoints;
	v2 *vertexPoints;

	f32 scale;
	Degrees rotation;

	enum EntityType type;
	enum Direction direction;

	v4 color;
	Texture *tex;
	b32 flipX;
	b32 flipY;

	EntityAnim animList[16];
	i32 animListIndex;
} Entity;

SubTexture entity_subTexGetCurr(Entity *const entity);
void entity_animSet(Entity *const entity, const char *const animName);
void entity_animUpdate(Entity *const entity, const f32 dt);
void entity_animAdd(AssetManager *const assetManager, Entity *const entity,
                    const char *const animName);

v2 *entity_generateUpdatedVertexList(MemoryArena_ *transientArena,
                                     Entity *entity);
#endif
