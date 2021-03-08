#version 450

layout(push_constant) uniform uniforms_block
{
	mat4 mat;
	vec4 cam_pos;
};

layout(location=0) in vec3 f_dir;
layout(location = 0) out vec4 out_color;

vec3 TextureFetch3d( vec3 coord, float smooth_size )
{
	vec3 tc_mod= abs( fract( coord * 6.0 ) - vec3( 0.5, 0.5, 0.5 ) );
	vec3 tc_step= smoothstep( 0.25 - smooth_size, 0.25 + smooth_size, tc_mod );

	float bit= abs( abs( tc_step.x - tc_step.y ) - tc_step.z );
	bit= bit * 0.5 + 0.4;
	return vec3( bit, bit, bit );
}


void main()
{
	vec3 c= TextureFetch3d( cam_pos.xyz + f_dir, 0.01 );
	out_color= vec4( c, 0.5 );
}
