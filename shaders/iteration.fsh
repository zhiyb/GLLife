#version 330 core
// https://en.wikipedia.org/wiki/Conway's_Game_of_Life

layout(location = 0) out vec4 fragColour;

in vec2 texCoord;
uniform sampler2D tex;

int neighbours()
{
	vec4 clr;
	// The offset value must be a constant expression
	clr = textureOffset(tex, texCoord, ivec2(0, -1));
	clr += textureOffset(tex, texCoord, ivec2(0, 1));
	clr += textureOffset(tex, texCoord, ivec2(-1, -1));
	clr += textureOffset(tex, texCoord, ivec2(-1, 0));
	clr += textureOffset(tex, texCoord, ivec2(-1, 1));
	clr += textureOffset(tex, texCoord, ivec2(1, -1));
	clr += textureOffset(tex, texCoord, ivec2(1, 0));
	clr += textureOffset(tex, texCoord, ivec2(1, 1));
	return int(clr.g + clr.b);
}

void main()
{
	fragColour = texture(tex, texCoord);
	int nei = neighbours();
	if (fragColour.gb == vec2(0., 0.)) {	// Died
		if (nei == 3)			// Reproduction
			fragColour.rgb = vec3(0., 1., 0.);
		else if (fragColour.r > 1. / 255.)
			fragColour.r = fragColour.r - 1. / 255.;
	} else {
		if (nei == 2 || nei == 3) {	// Lives
			if (fragColour.g > 1. / 255.) {
				fragColour.g = fragColour.g - 1. / 255.;
				fragColour.b = fragColour.b + 1. / 255.;
			}
		} else				// Under-population, overcrowding
			fragColour.rgb = vec3(1., 0., 0.);
	}
}
