#version 430 core

layout (location = 0) in ivec2 vertex;
uniform ivec2 vpSize;	// View port size
uniform ivec2 texSize;	// Texture size

out vec2 texCoord;

void main()
{
	gl_Position = vec4(vec2(vertex) * (vec2(texSize) / vec2(vpSize)), 0., 1.);
	texCoord = (vertex + ivec2(1, 1)) / ivec2(2, 2);
}
