#version 450

layout(push_constant) uniform uniforms_block
{
	mat4 mat;
	vec4 cam_pos;
};

layout(location=0) in vec3 pos;
layout(location=0) out vec3 f_dir;

void main()
{
	f_dir= pos - cam_pos.xyz;
	gl_Position= mat * vec4(pos, 1.0);
}
