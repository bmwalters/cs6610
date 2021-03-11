#version 330 core

uniform mat4 vp;

in vec3 pos;

out vec3 dir;

void main() {
    dir = pos;
    gl_Position = vp * vec4(pos, 1);
}
