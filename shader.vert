#version 330 core

uniform mat4 mv;
uniform mat4 mvp;

in vec3 pos;
in vec3 normal;

out vec3 clr;

void main() {
    clr = normalize(vec3(transpose(inverse(mv)) * vec4(normal, 0)));
    gl_Position = mvp * vec4(pos, 1);
}
