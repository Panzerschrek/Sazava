#version 450

// This uniforms block must be same in both vertex/fragment shaders!
layout(push_constant) uniform uniforms_block
{
	mat4 view_matrix;
	vec4 cam_pos;
	vec4 dir_to_sun_normalized;
	vec4 sun_color;
	vec4 ambient_light_color;
};

layout(location= 0) out noperspective vec3 f_dir;

void main()
{
	vec2 pos[3]=
			vec2[3](
				vec2(-3.0, -1.0),
				vec2(+3.0, -1.0),
				vec2( 0.0, +2.0)
			);

	gl_Position= vec4(pos[gl_VertexIndex], 0.0, 1.0);
	f_dir= ( view_matrix * vec4( pos[gl_VertexIndex], 1.0, 1.0 ) ).xyz;
}
