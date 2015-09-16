#version 400 core
// https://en.wikipedia.org/wiki/Conway's_Game_of_Life

layout(location = 0) out vec4 fragColour;

in vec2 texCoord;
uniform sampler2D tex;

int neighbours()
{
	vec4 clr;
	// The offset value must be a constant expression
#if 0
	ivec2 offsets1[4] = ivec2[4](ivec2(-1, -1), ivec2(0, -1), ivec2(1, -1), ivec2(-1 ,0));
	ivec2 offsets2[4] = ivec2[4](ivec2(-1, 1), ivec2(0, 1), ivec2(1, 1), ivec2(1 ,0));
	clr = textureGatherOffsets(tex, texCoord, offsets1, 1);		// Green channel
	//clr += textureGatherOffsets(tex, texCoord, offsets1, 2);	// Blue channel
	clr += textureGatherOffsets(tex, texCoord, offsets2, 1);	// Green channel
	//clr += textureGatherOffsets(tex, texCoord, offsets2, 2);	// Blue channel
	return int(clr.x + clr.y + clr.z + clr.w);
#else
	clr = textureLodOffset(tex, texCoord, 0, ivec2(0, -1));
	clr += textureLodOffset(tex, texCoord, 0, ivec2(0, 1));
	clr += textureLodOffset(tex, texCoord, 0, ivec2(-1, -1));
	clr += textureLodOffset(tex, texCoord, 0, ivec2(-1, 0));
	clr += textureLodOffset(tex, texCoord, 0, ivec2(-1, 1));
	clr += textureLodOffset(tex, texCoord, 0, ivec2(1, -1));
	clr += textureLodOffset(tex, texCoord, 0, ivec2(1, 0));
	clr += textureLodOffset(tex, texCoord, 0, ivec2(1, 1));
	return int(clr.g);
#endif
}

void main()
{
	fragColour = textureLod(tex, texCoord, 0);
	int nei = neighbours();
	if (fragColour.g == 0.) {	// Died
		if (nei == 3)			// Reproduction
			fragColour.rgb = vec3(0., 1., 0.);
		else if (fragColour.r > 1. / 255.)
			fragColour.r = fragColour.r - 1. / 255.;
	} else {
		if (nei == 2 || nei == 3) {	// Lives
			if (fragColour.b < 1. - 1. / 255.)
				fragColour.b = fragColour.b + 1. / 255.;
		} else				// Under-population, overcrowding
			fragColour.rgb = vec3(1., 0., 0.);
	}
}
