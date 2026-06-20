#version 410 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

uniform mat4 vp;
uniform mat4 m;

out vec3 interpolated_pos;

void main()
{
    vec4 world_pos = m *vec4(pos, 1.0);

    gl_Position = vp * world_pos;

    interpolated_pos = vec3(world_pos);
}
