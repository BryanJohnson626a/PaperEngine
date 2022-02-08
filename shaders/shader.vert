#version 450

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model[500];
    mat3 tex_offset[500];
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 vertex_tex_coord;

layout(location = 1) out vec2 frag_tex_coord;

void main()
{
    gl_Position = ubo.projection * ubo.view * ubo.model[gl_InstanceIndex] * vec4(vertex_position, 1);
    vec3 t = ubo.tex_offset[gl_InstanceIndex] * vec3(vertex_tex_coord, 1);
    frag_tex_coord = t.xy;
}