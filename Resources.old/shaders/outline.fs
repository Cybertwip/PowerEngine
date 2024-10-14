#version 330 core
in vec2 TexCoords;
layout(location = 0) out vec4 color;
layout(location = 1) out int color2;
layout(location = 3) out int color3;

uniform vec2 iResolution;
uniform sampler2D screenTexture;
uniform vec3 outlineColor;
uniform float outlineWidth;
uniform float outlineThreshold;

const vec2 offsets[9] = vec2[](
    vec2(-1, -1), vec2(0, -1), vec2(1, -1),
    vec2(-1, 0), vec2(0, 0), vec2(1, 0),
    vec2(-1, 1), vec2(0, 1), vec2(1, 1)
);

const float kernel[9] = float[](
    1.0/16, 2.0/16, 1.0/16, 
    2.0/16, 4.0/16, 2.0/16, 
    1.0/16, 2.0/16, 1.0/16);

float smoothedIntensity(vec2 coord) {
    float intensity = 0.0;
    vec2 pixelSize = 1.0 / iResolution;
    for (int i = 0; i < 9; i++) {
        vec2 offset = offsets[i] * pixelSize;
        intensity += texture(screenTexture, coord + offset).r * kernel[i];
    }
    return intensity;
}

void main() {
    float sx = 0.0;
    float sy = 0.0;
    vec2 pixelSize = outlineWidth / iResolution;

    for (int i = 0; i < 9; i++) {
        float intensity = smoothedIntensity(TexCoords + offsets[i] * pixelSize);
        vec2 offset = offsets[i];
        // Sobel operator weights for X and Y direction
        float Gx[9] = float[]( -1, 0, 1, -2, 0, 2, -1, 0, 1 );
        float Gy[9] = float[]( 1, 2, 1, 0, 0, 0, -1, -2, -1 );
        sx += Gx[i] * intensity;
        sy += Gy[i] * intensity;
    }

    float edgeStrength = sqrt(sx*sx + sy*sy);

    float edge = edgeStrength > outlineThreshold ? 1.0 : 0.0;
    // Blend the edge detection result with the original texture color
    vec4 originalColor = texture(screenTexture, TexCoords);
    // Adjust the blending factor according to your needs; here, it uses 'edge' as a mask
    color = mix(originalColor, vec4(outlineColor, 1), edge);

    color2 = 0; // Assuming 'color2' is a placeholder or used for other purposes not defined here
    color3 = 0; // Assuming 'color2' is a placeholder or used for other purposes not defined here
}
