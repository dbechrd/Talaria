#version 330 core

layout(location = 1) in vec2 attr_uv;
layout(location = 2) in vec3 attr_position;

//uniform mat4 u_proj;
//uniform mat4 u_view;
//uniform mat4 u_model;
uniform int u_face;

out vs_out {
	vec3 uvw;
} vertex;

void main()
{
    //vec4 pos = u_model * vec4(attr_position, 1.0);
    //gl_Position = u_proj * u_view * pos;
	gl_Position = vec4(attr_position, 1.0);

	vec2 uv_cube = attr_uv.xy * 2.0 - 1.0;

	switch (u_face) {
	case 0:
		vertex.uvw = vec3(1.0, uv_cube.y, uv_cube.x);
		break;
	case 1:
		vertex.uvw = vec3(-1.0, uv_cube.y, -uv_cube.x);
		break;
	case 2:
		vertex.uvw = vec3(uv_cube.x, 1.0, uv_cube.y);
		break;
	case 3:
		vertex.uvw = vec3(uv_cube.x, -1.0, -uv_cube.y);
		break;
	case 4:
		vertex.uvw = vec3(-uv_cube.x, uv_cube.y, 1.0);
		break;
	case 5:
		vertex.uvw = vec3(uv_cube.x, uv_cube.y, -1.0);
		break;
	};
}
