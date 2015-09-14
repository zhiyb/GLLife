#version 430 core

uniform ivec3 colour;
layout (location = 0) out vec4 fragColor;

in vec2 texCoord;
uniform sampler2D tex;

void main()
{
	//fragColor = vec4(texCoord, texture(tex, texCoord).b, 1.0);
	fragColor = texture(tex, texCoord);
}
