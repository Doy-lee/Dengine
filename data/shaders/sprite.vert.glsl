#version 330 core
layout (location = 0) in vec4 data; // (vec2)pos, (vec2)texCoord

out vec2 texCoord;

uniform mat4 model;
uniform mat4 projection;

void main()
{
	gl_Position = projection * model * vec4(data.xy, 0.0f, 1.0f);
	texCoord = data.zw;
}
