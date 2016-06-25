#ifndef DENGINE_RENDERER_H
#define DENGINE_RENDERER_H

#include <Dengine/Math.h>
#include <Dengine/Entity.h>
#include <Dengine/Shader.h>

typedef struct Renderer
{
	Shader *shader;
	GLuint vao;
	GLuint vbo;
} Renderer;

void renderer_entity(Renderer *renderer, Entity *entity, f32 rotate,
                     v3 color);

void renderer_object(Renderer *renderer, v2 pos, v2 size, f32 rotate, v3 color,
                     Texture *tex);

#endif
