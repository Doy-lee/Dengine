#ifndef DENGINE_ENTITY_H
#define DENGINE_ENTITY_H

#include <Dengine/Texture.h>

#include <glm/glm.hpp>

struct Entity
{
	glm::vec2 pos;  // Position
	glm::vec2 dPos; // Velocity
	glm::vec2 size;
	Texture *tex;
};

#endif
