#version 450

layout(binding = 1) uniform sampler2D texture_sampler;

layout(location = 1) in vec2 frag_tex_coord;

layout(location = 0) out vec4 out_color;

void main()
{
    vec4 final_color = texture(texture_sampler, frag_tex_coord);
    if (final_color.a < 1)
        discard;
    out_color = final_color;
}