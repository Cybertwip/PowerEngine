#version 330 core

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int EntityID;

uniform vec3 u_color;
uniform int identifier;

void main() {
    FragColor = vec4(u_color, 1.0);
    EntityID = identifier;
}
