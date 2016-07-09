#ifndef DENGINE_SHADER_H
#define DENGINE_SHADER_H

#include "Dengine/OpenGL.h"
#include "Dengine/Math.h"

typedef struct Shader
{
	GLuint id;
} Shader;

const i32 shader_loadProgram(Shader *const shader,
                             const GLuint vertexShader,
                             const GLuint fragmentShader);

void shader_uniformSet1i(Shader *const shader, const GLchar *name,
                         const GLuint data);
void shader_uniformSetMat4fv(Shader *const shader, const GLchar *name,
                             mat4 data);
void shader_uniformSetVec4f(Shader *const shader, const GLchar *name,
                            v4 data);

void shader_use(const Shader *const shader);

#endif
