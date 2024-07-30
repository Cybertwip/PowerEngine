#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexcoords1;
layout (location = 3) in vec2 aTexcoords2;

uniform mat4 aProjection;
uniform mat4 aView;
uniform mat4 aModel;

out vec4 FragPosLightSpace;
flat out int boneId;

out vec2 TexCoords1;
out vec2 TexCoords2;
out vec3 Normal;
out vec3 FragPos;

void main()
{
    TexCoords1 = aTexcoords1;
    TexCoords2 = aTexcoords2;
    gl_Position = aProjection * aView * aModel * vec4(aPosition, 1.0);
    Normal = mat3(aModel) * aNormal;
    FragPos = vec3(aModel * vec4(aPosition, 1.0));
    boneId = 0;

    FragPosLightSpace = vec4(FragPos, 1.0);
}
