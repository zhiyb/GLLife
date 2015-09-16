#version 330 core

in vec2 vertex;
in ivec2 texVertex;

uniform ivec2 vpSize;	// View port size
uniform ivec2 texSize;	// Texture size
uniform int zoom;	// Zomming
uniform vec2 move;	// Movement / offset

out vec2 texCoord;

void main()
{
	gl_Position = vec4((vertex - move * 2. / texSize) * pow(2, zoom) * (vec2(texSize) / vec2(vpSize)), 0., 1.);
	texCoord = texVertex;
}
