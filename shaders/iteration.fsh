#version 430 core
// https://en.wikipedia.org/wiki/Conway's_Game_of_Life

layout (location = 0) out vec4 fragColour;

in vec2 texCoord;
uniform sampler2D tex;

int neighbours()
{
	int cnt = 0;
	// The offset value must be a constant expression
	cnt += textureOffset(tex, texCoord, ivec2(-1, -1)).g != 0 ? 1 : 0;
	cnt += textureOffset(tex, texCoord, ivec2(-1, 0)).g != 0 ? 1 : 0;
	cnt += textureOffset(tex, texCoord, ivec2(-1, 1)).g != 0 ? 1 : 0;
	cnt += textureOffset(tex, texCoord, ivec2(0, -1)).g != 0 ? 1 : 0;
	cnt += textureOffset(tex, texCoord, ivec2(0, 1)).g != 0 ? 1 : 0;
	cnt += textureOffset(tex, texCoord, ivec2(1, -1)).g != 0 ? 1 : 0;
	cnt += textureOffset(tex, texCoord, ivec2(1, 0)).g != 0 ? 1 : 0;
	cnt += textureOffset(tex, texCoord, ivec2(1, 1)).g != 0 ? 1 : 0;
	return cnt;
}

void main()
{
	fragColour = texture(tex, texCoord);
	int nei = neighbours();
	if (fragColour.g == 0.) {		// Died
		if (nei == 3)			// Reproduction
			fragColour.rgb = vec3(0., 1., 0.);
		else
			fragColour.r = max(fragColour.r - 1. / 255., 0.);
	} else {
		if (nei == 2 || nei == 3)	// Lives
			fragColour.b = min(fragColour.b + 1. / 255., 1.);
		else				// Under-population, overcrowding
			fragColour.rgb = vec3(1., 0., 0.);
	}
}
