// GLSL Pixel shader definition


// Texture sampler (used to access single textels from a texture)
uniform sampler2D currentTexture;

// Load in values defined in the material:
uniform float adjustTextureScale;   // Higher value = smaller coords = wider texture
uniform vec2 adjustTextureOffset;   // Texture recenter

void main(void)
{
    // Scale and apply offset to uv map, then sample and apply color
    gl_FragColor = texture2D
    (
        currentTexture,
        gl_TexCoord[0].xy * 1/adjustTextureScale - adjustTextureOffset
    );
}