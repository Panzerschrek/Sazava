#version 450

layout(push_constant) uniform uniforms_block
{
	mat4 mat;
};

layout(location=0) in vec3 pos;

void main()
{
	//gl_Position= mat * vec4(pos, 1.0);
	gl_Position= vec4( pos, 1.0 );
}
