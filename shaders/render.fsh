#version 430 core

layout (location = 0) out vec4 fragColour;

in vec2 texCoord;
uniform sampler2D tex;

void main()
{
	//fragColour = vec4(texCoord, texture(tex, texCoord).b, 1.0);
	fragColour = texture(tex, texCoord);
}
