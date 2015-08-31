// GLSL Pixel shader definition


// GLSL Texture sampler (used to access single textels from a texture)
uniform sampler2D currentTexture;

// Load in values defined in the material:
uniform float adjustTextureAspectRatio;	// Higher value = smaller X coords = wider texture on X value
uniform float adjustTextureScale;   	// Higher value = smaller coords = wider texture
uniform vec2 adjustTextureOffset;   	// Texture recenter

void main(void)
{
	// These are UV coordinates of the UV circle center, which corresponds to
	// the vertex of the sphere in the middle (right in front of the user),
	// in respect to which scaling should take place.
	// This value is useful to make calibration more user-friendly.
	// 
	// In this specific case, sphere mesh has been modeled so that UV coordinates
	// in this vertex are (0,0). This formula has been found experimentally,
	// since Ogre probably interprets coords in its own way.
	adjustTextureOffset.x = -adjustTextureOffset.x;
	vec2 uvScaleCenter = vec2(adjustTextureOffset.x,1-(-adjustTextureOffset.y));

	vec2 AlteredTexCoord;
	// apply offset
	AlteredTexCoord = gl_TexCoord[0].xy + adjustTextureOffset;
	// apply centered ratio (only to x) and scale
	AlteredTexCoord.x = ((AlteredTexCoord.x - uvScaleCenter.x) * 1/adjustTextureAspectRatio * 1/adjustTextureScale) + uvScaleCenter.x;
	AlteredTexCoord.y = ((AlteredTexCoord.y - uvScaleCenter.y) * 1/adjustTextureScale) + uvScaleCenter.y;
    // apply color using altered uv map
    gl_FragColor = texture2D
    (
        currentTexture,
        AlteredTexCoord.xy
    );
}