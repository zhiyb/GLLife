#version 430 core

layout (location = 0) in ivec2 vertex;
uniform mat4 projection;

out vec2 texCoord;

void main()
{
	gl_Position = projection * vec4(vertex, 0., 1.);
	texCoord = (vertex + ivec2(1, 1)) / ivec2(2, 2);
}
