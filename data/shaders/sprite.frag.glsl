#version 330 core

in vec2 texCoord;
out vec4 color;

uniform sampler2D tex;
uniform vec4 spriteColor;

void main()
{
	color = spriteColor * texture(tex, texCoord);
}
