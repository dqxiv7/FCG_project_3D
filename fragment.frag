#version 410 core

in vec3 interpolated_pos;
in vec3 interpolated_normal;

uniform vec3 camera_pos;

#define MAX_LIGHTS 4

struct Light {
    vec3 direct_pos;
    vec3 direct_val;
    vec3 ambient_val;
};
uniform Light lights[MAX_LIGHTS];
uniform int num_lights;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
uniform Material material;

uniform bool unlit;
uniform vec3 unlit_color;
uniform float unlit_alpha;

out vec4 fragment_color;
void main()
{
    if (unlit) {
        fragment_color = vec4(unlit_color, unlit_alpha);
        return;
    }

    vec3 normal = normalize (interpolated_normal);
    vec3 view_dir = normalize (camera_pos - interpolated_pos);

    vec3 result = vec3(0.0);

    for (int i = 0; i < num_lights; i++) {
        // Ambient
        vec3 ambient = material.ambient * lights[i].ambient_val;

        // Diffuse
        vec3 light_dir = normalize (lights[i].direct_pos - interpolated_pos);
        float diff = max (dot (normal, light_dir), 0.0);
        vec3 diffuse = material.diffuse * diff * lights[i].direct_val;

        // Specular
        vec3 reflect_dir = reflect (-light_dir, normal);
        float spec = pow (max (dot (view_dir, reflect_dir), 0.0), material.shininess);
        vec3 specular = material.specular * spec * lights[i].direct_val;

        result += ambient + diffuse + specular;
    }

    fragment_color = vec4(result, 1.0);
}
