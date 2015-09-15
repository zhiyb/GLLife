#version 330 core
// https://en.wikipedia.org/wiki/Conway's_Game_of_Life

layout(location = 0) out vec4 fragColour;

in vec2 texCoord;
uniform sampler2D tex;

bool died(vec4 clr)
{
	return clr.gb == vec2(0, 0);
}

int neighbours()
{
	int cnt = 0;
	// The offset value must be a constant expression
	cnt += died(textureOffset(tex, texCoord, ivec2(-1, -1))) ? 0 : 1;
	cnt += died(textureOffset(tex, texCoord, ivec2(-1, 0))) ? 0 : 1;
	cnt += died(textureOffset(tex, texCoord, ivec2(-1, 1))) ? 0 : 1;
	cnt += died(textureOffset(tex, texCoord, ivec2(0, -1))) ? 0 : 1;
	//cnt += died(textureOffset(tex, texCoord, ivec2(0, 0))) ? 0 : 1;
	cnt += died(textureOffset(tex, texCoord, ivec2(0, 1))) ? 0 : 1;
	cnt += died(textureOffset(tex, texCoord, ivec2(1, -1))) ? 0 : 1;
	cnt += died(textureOffset(tex, texCoord, ivec2(1, 0))) ? 0 : 1;
	cnt += died(textureOffset(tex, texCoord, ivec2(1, 1))) ? 0 : 1;
	return cnt;
}

void main()
{
	fragColour = texture(tex, texCoord);
	int nei = neighbours();
	if (died(fragColour)) {
		if (nei == 3)			// Reproduction
			fragColour.rgb = vec3(0., 1., 0.);
		else
			fragColour.r = max(fragColour.r - 1. / 255., 0.);
	} else {
		if (nei == 2 || nei == 3) {	// Lives
			if (fragColour.b == 1.)
				fragColour.g = max(fragColour.g - 1. / 255., 0.);
			else
				fragColour.b = min(fragColour.b + 1. / 255., 1.);
		} else				// Under-population, overcrowding
			fragColour.rgb = vec3(1., 0., 0.);
	}
}
