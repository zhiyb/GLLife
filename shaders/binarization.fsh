#version 430 core

layout (location = 0) out vec4 fragColour;

in vec2 texCoord;
uniform sampler2D tex;

float luminance(in vec3 colour)
{
	// https://en.wikipedia.org/wiki/Grayscale#Luma_coding_in_video_systems
	return 0.2126 * colour.r + 0.7152 * colour.g + 0.0722 * colour.b;
}

void main()
{
	bool y = luminance(texture(tex, texCoord).rgb) > 0.5;
	fragColour = vec4(y, y, y, 1.0);
}
