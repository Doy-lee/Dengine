#ifndef DENGINE_SHADER_H
#define DENGINE_SHADER_H

#include "Dengine/Math.h"
#include "Dengine/OpenGL.h"

void shader_uniformSet1i(u32 shaderId, const GLchar *name,
                         const GLuint data);
void shader_uniformSetMat4fv(u32 shaderId, const GLchar *name,
                             mat4 data);
void shader_uniformSetVec4f(u32 shaderId, const GLchar *name,
                            v4 data);

void shader_use(u32 shaderId);

#endif
