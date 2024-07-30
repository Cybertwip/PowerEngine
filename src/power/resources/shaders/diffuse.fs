#version 330 core

// Output variables for fragment shader
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int EntityID;
layout(location = 2) out int UniqueID;

// Input attributes from vertex shader
in vec2 TexCoords1;
in vec2 TexCoords2;

// Uniforms
uniform sampler2D texture_diffuse1; // Diffuse texture

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float opacity;
    bool has_diffuse_texture;
};

uniform Material material;
uniform vec3 color; // Add this uniform for setting color

void main() {
    vec3 mat_diffuse = material.diffuse;
    if (material.has_diffuse_texture) {
        mat_diffuse = (texture(texture_diffuse1, TexCoords1).rgb + texture(texture_diffuse1, TexCoords2).rgb) / 2;
    } else {
        mat_diffuse = material.diffuse + material.specular;
    }

    // Combine the material diffuse color with the uniform color
    vec3 final_color = mat_diffuse * color;

    // Set the fragment color to the combined color
    FragColor = vec4(final_color, material.opacity);
}
