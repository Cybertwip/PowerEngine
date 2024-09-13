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
uniform vec3 color; // Add this uniform for setting color

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
    // For each material, check if it has a diffuse texture
    vec3 mat_diffuse = materials[TextureId].diffuse;
    if (materials[TextureId].has_diffuse_texture) {
        mat_diffuse = (texture(materials[TextureId].texture_diffuse, TexCoords1).rgb + texture(materials[TextureId].texture_diffuse, TexCoords2).rgb) / 2;
    } else {
        mat_diffuse = materials[TextureId].diffuse + materials[TextureId].specular;
    }

    // Compute the final fragment color by averaging the material contributions
    vec3 final_color = mat_diffuse;

    // Set the fragment color
    FragColor = vec4(final_color * color, materials[TextureId].opacity); 

    EntityID = identifier;
}
