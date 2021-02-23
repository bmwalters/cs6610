#version 330 core

uniform float ambient_intensity;

uniform vec3 light_pos_view;
uniform float light_intensity;

uniform vec3 Ka;  // material ambient color
uniform vec3 Kd;  // material diffuse color
uniform vec3 Ks;  // material specular color
uniform float Ns; // material phong alpha

uniform sampler2D map_Ka;
uniform sampler2D map_Kd;
uniform sampler2D map_Ks;

in vec3 pos_view;
in vec3 normal_view;
in vec2 TexCoord;

out vec4 color;

void main() {
    vec3 ambient_color = (vec4(Ka, 1) * texture(map_Ka, TexCoord)).rgb;
    vec3 diffuse_color = (vec4(Kd, 1) * texture(map_Kd, TexCoord)).rgb;
    vec3 specular_color = (vec4(Ks, 1) * texture(map_Ks, TexCoord)).rgb;

    vec3 view_direction = normalize(-pos_view);
    vec3 light_direction = normalize(light_pos_view - pos_view);

    vec3 normal_view = normalize(normal_view);
    vec3 diffuse = max(0, dot(normal_view, light_direction)) * diffuse_color;

    vec3 half_vector = normalize(light_direction + view_direction);
    vec3 specular = pow(max(0, dot(normal_view, half_vector)), Ns) * specular_color;

    color = vec4(light_intensity * (diffuse + specular) + ambient_intensity * ambient_color, 1);
}
