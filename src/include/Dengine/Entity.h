#ifndef DENGINE_ENTITY_H
#define DENGINE_ENTITY_H

#include <Dengine/Common.h>
#include <Dengine/Texture.h>

#include <glm/glm.hpp>

namespace Dengine
{
class Entity
{
public:
	Entity();
	Entity(const glm::vec2 pos, const Texture *tex);
	Entity(const glm::vec2 pos, const glm::vec2 size, const Texture *tex);
	Entity(const glm::vec2 pos, const std::string texName);
	~Entity();


	glm::vec2 pos;  // Position
	glm::vec2 dPos; // Velocity

	glm::vec2 size;
	const Texture *tex;
};
}
#endif
