#version 330 core
layout (location = 0) in vec4 data; // (vec2)pos, (vec2)texCoord

out vec2 texCoord;

void main()
{
	gl_Position = vec4(data.xy, 0.0f, 1.0f);
	texCoord = data.zw;
}
