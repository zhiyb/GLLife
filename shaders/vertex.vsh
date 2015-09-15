#version 430 core

layout (location = 0) in ivec2 vertex;

layout (location = 0) uniform ivec2 vpSize;	// View port size
layout (location = 1) uniform ivec2 texSize;	// Texture size
layout (location = 2) uniform int zoom;		// Zomming
layout (location = 3) uniform vec2 move;	// Movement / offset

out vec2 texCoord;

void main()
{
	gl_Position = vec4((vec2(vertex) - move * 2. / texSize) * pow(2, -zoom) * (vec2(texSize) / vec2(vpSize)), 0., 1.);
	texCoord = (vertex + ivec2(1, 1)) / ivec2(2, 2);
}
