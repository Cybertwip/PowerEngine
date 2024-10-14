#version 330 core

// Output variables for fragment shader
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int EntityID;
layout(location = 2) out int UniqueID;


// Input attributes from vertex shader
in vec2 TexCoords1;
in vec2 TexCoords2;
in vec3 Normal;
in vec3 FragPos;

// Uniforms
uniform sampler2D texture_diffuse1; // Diffuse texture
layout(std140) uniform Matrices {
    mat4 projection;
    mat4 view;
    vec4 viewPos; // View position
};
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    float opacity;
    bool has_diffuse_texture;
};

uniform Material material;
uniform uint selectionColor;
uniform uint uniqueIdentifier;

void main() {
    vec3 mat_diffuse = material.diffuse;
    if (material.has_diffuse_texture) {
        mat_diffuse = (texture(texture_diffuse1, TexCoords1).rgb + texture(texture_diffuse1, TexCoords2).rgb) / 2;
    } else {
        mat_diffuse = material.diffuse + material.specular;
    }

    // Pass fragment color, entity ID, and unique ID to output
    FragColor = vec4(mat_diffuse, material.opacity);
    EntityID = int(selectionColor);
    UniqueID = int(uniqueIdentifier);
}
