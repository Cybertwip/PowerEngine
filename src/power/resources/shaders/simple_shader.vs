#version 330
uniform mat4 mvp;
in vec3 position;
out vec4 frag_color;
void main() {
    frag_color = vec4(vec3(0.2), 1.0);
    gl_Position = mvp * vec4(position, 1.0);
}
