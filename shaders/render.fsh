#version 330 core

layout(location = 0) out vec4 fragColour;

in vec2 texCoord;
uniform sampler2D tex;

void main()
{
	fragColour = texelFetch(tex, ivec2(texCoord), 0);
}
