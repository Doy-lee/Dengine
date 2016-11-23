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

enum EntityType
{
	entitytype_invalid,
	entitytype_ship,
	entitytype_asteroid,
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
	v2 dP;

	v2 hitbox;
	v2 size;

	// NOTE(doyle): Offset from origin point to the entity's considered "center"
	// point, all operations work from this point, i.e. rotation, movement,
	// collision detection
	// If this is a polygon, the offset should be from the 1st vertex point
	// specified
	v2 offset;

	enum RenderMode renderMode;
	i32 numVertexPoints;
	v2 *vertexPoints;

	f32 scale;
	Degrees rotation;

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

SubTexture entity_getActiveSubTexture(Entity *const entity);
void entity_setActiveAnim(Entity *const entity, const char *const animName);
void entity_updateAnim(Entity *const entity, const f32 dt);
void entity_addAnim(AssetManager *const assetManager, Entity *const entity,
                    const char *const animName);

v2 *entity_createVertexList(MemoryArena_ *transientArena, Entity *entity);
#endif
