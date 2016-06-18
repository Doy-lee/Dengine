#ifndef DENGINE_ENTITY_H
#define DENGINE_ENTITY_H

#include <Dengine/Texture.h>
#include <Dengine/Math.h>

typedef struct Entity
{
	v2 pos;  // Position
	v2 dPos; // Velocity
	v2 size;
	Texture *tex;
} Entity;

#endif
