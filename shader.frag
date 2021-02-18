#version 330 core

uniform float ambient_intensity;

uniform vec3 light_pos_view;
uniform float light_intensity;

uniform vec3 mat_diffuse;
uniform vec3 mat_specular;
uniform float mat_phongalpha;

in vec3 pos_view;
in vec3 normal_view;

out vec4 color;

void main() {
    vec3 view_direction = normalize(-pos_view);
    vec3 light_direction = normalize(light_pos_view - pos_view);

    vec3 normal_view = normalize(normal_view);
    vec3 diffuse = max(0, dot(normal_view, light_direction)) * mat_diffuse;

    vec3 half_vector = normalize(light_direction + view_direction);
    vec3 specular = mat_specular * pow(max(0, dot(normal_view, half_vector)), mat_phongalpha);

    color = vec4(light_intensity * (diffuse + specular) + ambient_intensity * mat_diffuse, 1);
}
