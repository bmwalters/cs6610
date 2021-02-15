#version 330 core

in vec3 pos;

out vec4 clr;

uniform mat4 mvp;

void main() {
    if (pos.x == 0 && pos.y == 0 && pos.z == 0) {
        clr = vec4(1, 1, 1, 1);
    } else if (pos.y == 0 && pos.z == 0) {
        clr = vec4(1, 0, 0, 1);
    } else if (pos.x == 0 && pos.z == 0) {
        clr = vec4(0, 1, 0, 1);
    } else if (pos.x == 0 && pos.y == 0) {
        clr = vec4(0, 0, 1, 1);
    } else {
        clr = vec4(1, 1, 1, 1);
    }

    gl_Position = mvp * vec4(pos, 1);
}
