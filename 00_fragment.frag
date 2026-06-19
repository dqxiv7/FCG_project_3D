#version 410 core

in vec3 interpolated_pos;

uniform vec3 camera_pos;

struct Light {
    vec3 direct_pos;
    vec3 direct_val;
    vec3 ambient_val;
};
uniform Light light;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
uniform Material material;


out vec3 interpolated_color;
void main()
{
    vec3 dx = dFdx(interpolated_pos);
    vec3 dy = dFdy(interpolated_pos);

    vec3 interpolated_normal = normalize (cross (dx, dy));

    // Ambient
    vec3 ambient = material.ambient * light.ambient_val;

    // Diffuse
    vec3 light_dir = normalize (light.direct_pos - interpolated_pos);
    float diff = max (dot (interpolated_normal, light_dir), 0.0);
    vec3 diffuse = material.diffuse * diff * light.direct_val;

    // Specular
    vec3 view_dir = normalize (camera_pos - interpolated_pos);
    vec3 reflect_dir = reflect (-light_dir, interpolated_normal);
    float spec = pow (max (dot (view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular = material.specular * spec * light.direct_val;

    // Combine all components
    interpolated_color = ambient + diffuse + specular;
}