#version 330 core

uniform samplerCube env;

in vec3 dir;

out vec4 color;

void main() {
    color = texture(env, dir);
}
