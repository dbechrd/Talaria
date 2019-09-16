#version 330 core

uniform vec4 u_color;

out vec4 final_color;

void main()
{
    final_color = u_color;
}
