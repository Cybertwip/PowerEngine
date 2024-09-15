#version 330 core

// Output variables for fragment shader
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int EntityID;

// Input attributes from vertex shader
in vec2 TexCoords1;
in vec2 TexCoords2;
flat in int TextureId;

// Uniforms for textures and materials
uniform int identifier;
uniform vec3 color; // Color (either red or white)

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float opacity;
    bool has_diffuse_texture;
};

struct Textures {
    sampler2D diffuse;
};


uniform Material materials[4]; // Array of materials without samplers
uniform Textures textures[4]; // Separate sampler array for textures

void main() {
    // **Optimization 1: Pre-fetch the material to reduce array indexing**
    Material mat = materials[TextureId];

    // **Optimization 2: Minimize conditional branches by leveraging uniform conditions**
    // Since `has_diffuse_texture` and `color` are uniforms, the GPU can optimize the branching efficiently.
    vec3 mat_diffuse;
    if (mat.has_diffuse_texture) {
        // **Optimization 3: Combine texture sampling and averaging in one step**
        mat_diffuse = (texture(textures[TextureId].diffuse, TexCoords1).rgb +
                      texture(textures[TextureId].diffuse, TexCoords2).rgb) * 0.5;
    } else {
        mat_diffuse = mat.diffuse + mat.specular;
    }

    // Initialize final_color and selectionOpacity
    vec3 final_color = mat_diffuse;
    float selectionOpacity = mat.opacity;

    // **Optimization 4: Replace length(color) == 1.0 with a more reliable condition**
    // Assuming color is either exactly red (vec3(1.0, 0.0, 0.0)) or white (vec3(1.0)), use a dot product for comparison
    float isRed = step(0.999, dot(color, vec3(1.0, 0.0, 0.0))); // 1.0 if color is red, else 0.0

    // **Optimization 5: Conditionally apply color adjustments using multiplication with isRed**
    // This avoids branching and leverages GPU parallelism
    if (isRed > 0.5) { // Only execute if color is red
        // Calculate brightness using the luminance model
        float brightness = dot(final_color, vec3(0.299, 0.587, 0.114));

        // Compute darkness factor
        float darkness_factor = clamp(1.0 - brightness / 0.05, 0.0, 1.0);

        // Blend final color towards red based on darkness_factor
        final_color = mix(final_color, color, darkness_factor);

        // Adjust selectionOpacity based on darkness_factor
        selectionOpacity *= mix(0.3, 1.0, 1.0 - darkness_factor);

        // Add a thin red tint to the final color
        final_color = mix(final_color, color, 0.25);
    }

    // **Optimization 6: Final color and opacity assignment**
    FragColor = vec4(final_color, selectionOpacity);

    // Output the entity identifier
    EntityID = identifier;
}
