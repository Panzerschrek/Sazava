#version 450

layout(push_constant) uniform uniforms_block
{
	mat4 mat;
};

void main()
{
	vec3 pos= vec3(
			-0.5 + float((gl_VertexIndex >> 0) & 1),
			-0.5 + float((gl_VertexIndex >> 1) & 1),
			-0.5 + float((gl_VertexIndex >> 2) & 1) );

	gl_Position= mat * vec4(pos, 1.0);
}
