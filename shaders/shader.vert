#version 450

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 vertex_tex_coord;

layout(location = 1) out vec2 frag_tex_coord;

void main()
{
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(vertex_position, 1);
    frag_tex_coord = vertex_tex_coord;
}