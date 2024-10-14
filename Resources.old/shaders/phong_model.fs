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
flat in int boneId; // Flat interpolation for integer value
in vec4 FragPosLightSpace;    // Add this output

// Uniforms
uniform sampler2D shadowMap; // Shadow map texture
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

struct DirLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 direction;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

#define NR_DIR_LIGHTS 12
#define NR_POINT_LIGHTS 12

uniform Material material;
uniform int numDirectionLights;
uniform int numPointLights;
uniform uint selectionColor;
uniform uint uniqueIdentifier;
uniform DirLight dir_lights[NR_DIR_LIGHTS];
uniform PointLight pointLights[NR_POINT_LIGHTS];

// Function to calculate directional light contribution
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 mat_diffuse) {
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 ambient = light.ambient * mat_diffuse;
    vec3 diffuse = light.diffuse * diff * mat_diffuse;
    vec3 specular = light.specular * spec * material.specular;
    return ambient + diffuse + specular;
}

// Function to calculate point light contribution
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 ambient = light.ambient * material.ambient;
    vec3 diffuse = light.diffuse * diff * material.diffuse;
    vec3 specular = light.specular * spec * material.specular;
    return (ambient + diffuse + specular) * attenuation;
}


float ShadowCalculation(DirLight light, vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(Normal);
    vec3 lightDir = light.position - FragPos;
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    // check whether current frag pos is in shadow
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    return shadow;
}


void main() {
    vec3 mat_diffuse = material.diffuse;
    if (material.has_diffuse_texture) {
        mat_diffuse = (texture(texture_diffuse1, TexCoords1).rgb + texture(texture_diffuse1, TexCoords2).rgb) / 2;
    } else {
        mat_diffuse = material.diffuse + material.specular;
    }

    vec3 norm = normalize(Normal); // Normalize the interpolated normal
    vec3 viewDir = normalize(vec3(viewPos) - FragPos); // Calculate view direction

    // Initialize result with ambient light contribution
    vec4 result = vec4(vec3(0, 0, 0), material.opacity);

    for (int i = 0; i < numDirectionLights; ++i) {
        vec3 lightContribution = CalcDirLight(dir_lights[i], norm, dir_lights[i].position - FragPos, mat_diffuse);
        if (i == 0) { // Assume first directional light casts shadows
            float shadow = ShadowCalculation(dir_lights[i], FragPosLightSpace); // Calculate shadow
            result.xyz += lightContribution * (1.0 - shadow); // Apply shadow factor
        } else {
            result.xyz += lightContribution; // Other lights contribute fully
        }
    }

    for (int i = 0; i < numPointLights; i++) {
        result.xyz += CalcPointLight(pointLights[i], norm, (pointLights[i].position + FragPos) - FragPos, FragPos);
    }


    // Pass fragment color, entity ID, and unique ID to output
    FragColor = result;
    EntityID = int(selectionColor);
    UniqueID = int(uniqueIdentifier);
}
