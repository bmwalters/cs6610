#version 330 core

uniform mat4 mv;
uniform mat4 mvp;
uniform mat3 mv_normals;

in vec3 pos;    // in model space
in vec3 normal; // in model space
in vec3 texcoord;

out vec3 pos_view;
out vec3 normal_view;
out vec2 TexCoord;

void main() {
    vec4 pos_view_h = mv * vec4(pos, 1);
    pos_view_h /= pos_view_h.w;
    pos_view = vec3(pos_view_h);
    normal_view = normalize(mv_normals * normal);
    gl_Position = mvp * vec4(pos, 1);
    TexCoord = texcoord.xy;
}
