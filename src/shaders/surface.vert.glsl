#version 450

layout(push_constant) uniform uniforms_block
{
	mat4 mat;
	vec4 cam_pos;
};

layout(location=0) in vec4 pos;

layout(location=0) out vec3 f_dir;
layout(location=1) out flat float f_surface_index;

void main()
{
	f_dir= pos.xyz - cam_pos.xyz;
	f_surface_index= pos.w;
	gl_Position= mat * vec4(pos.xyz, 1.0);
}
