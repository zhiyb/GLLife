#version 430 core

uniform ivec3 colour;
out vec4 fragColor;

void main()
{
	fragColor = vec4(vec3(colour) / vec3(256., 256., 256.), 1.0);
}
