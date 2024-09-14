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
    sampler2D texture_diffuse;
    bool has_diffuse_texture;
};

uniform Material materials[4]; // Array of materials

void main() {
    vec3 mat_diffuse;

    // Check if the material has a diffuse texture
    if (materials[TextureId].has_diffuse_texture) {
        mat_diffuse = (texture(materials[TextureId].texture_diffuse, TexCoords1).rgb + texture(materials[TextureId].texture_diffuse, TexCoords2).rgb) / 2.0;
    } else {
        mat_diffuse = materials[TextureId].diffuse + materials[TextureId].specular;
    }

    // Compute the final fragment color
    vec3 final_color = mat_diffuse;
    float selectionOpacity = materials[TextureId].opacity; // Start with the material's base opacity

    // If the provided color is red, apply the darkness factor
    if (length(color) == 1.0) {
        // Calculate brightness of the color (use the luminance model)
        float brightness = dot(final_color, vec3(0.299, 0.587, 0.114)); // Standard luminance formula

        // Inverse brightness gives how close the color is to black (darkness factor)
        float darkness_factor = clamp(1.0 - brightness / 0.05, 0.0, 1.0); 

        // Blend final color towards red based on how dark it is
        final_color = mix(final_color, color, darkness_factor); 

        // Adjust selectionOpacity dynamically based on the darkness factor
        selectionOpacity = selectionOpacity * mix(0.3, 1.0, 1.0 - darkness_factor); // Blend between 0.3 and 1.0 opacity

        // Add a thin layer of the `color` (tinting the final color)
        final_color = mix(final_color, color, 0.25); // Mix in 10% of the color
    }


    // Set the fragment color with dynamically adjusted opacity
    FragColor = vec4(final_color, selectionOpacity);

    // Output the entity identifier
    EntityID = identifier;
}
