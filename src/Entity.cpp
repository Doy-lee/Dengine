#include <Dengine/Entity.h>
#include <Dengine/AssetManager.h>

namespace Dengine
{

Entity::Entity()
: pos(glm::vec2(0, 0))
, dPos(glm::vec2(0, 0))
, size(glm::vec2(0, 0))
, tex(nullptr)
{
}

Entity::Entity(const glm::vec2 pos, const Texture *tex)
{
	this->pos  = pos;
	this->dPos = glm::vec2(0, 0);
	this->tex  = tex;
	this->size = glm::vec2(tex->getWidth(), tex->getHeight());
}

Entity::Entity(const glm::vec2 pos, glm::vec2 size, const Texture *tex)
{
	this->pos  = pos;
	this->dPos = glm::vec2(0, 0);
	this->tex  = tex;
	this->size = size;
}

Entity::Entity(const glm::vec2 pos, const std::string texName)
{
	Texture *tex = AssetManager::getTexture(texName);
	this->pos  = pos;
	this->dPos = glm::vec2(0, 0);
	this->tex  = tex;
	this->size = glm::vec2(tex->getWidth(), tex->getHeight());

}

Entity::~Entity()
{
}
}
