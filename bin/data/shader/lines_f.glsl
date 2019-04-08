#version 330 core

in vs_out {
    vec3 position;
    vec4 color;
} vertex;

out vec4 final_color;

void main()
{
    final_color = vec4(vertex.color.rgb, 1.0);
}
