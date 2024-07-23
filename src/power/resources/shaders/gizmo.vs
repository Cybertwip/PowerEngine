#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 aProjection;
uniform mat4 aView;
uniform mat4 aModel;

void main() {
    gl_Position = aProjection * aView * vec4(aPosition, 1.0);
}
