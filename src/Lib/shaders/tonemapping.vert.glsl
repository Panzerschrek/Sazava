#version 450

layout(push_constant) uniform uniforms_block
{
	vec4 mix_factor; // .x contains mix factor.
};

layout(binding= 1) uniform sampler2D brightness_tex;

layout(binding= 2, std430) buffer exposure_accumulate_buffer
{
	float exposure[6];
};

layout(location= 0) out noperspective vec2 f_tex_coord;
layout(location= 1) out flat float f_exposure;

void main()
{
	vec2 pos[6]=
			vec2[6](
				vec2(-1.0, -1.0),
				vec2(+1.0, -1.0),
				vec2(+1.0, +1.0),

				vec2(-1.0, -1.0),
				vec2(+1.0, +1.0),
				vec2(-1.0, +1.0)
			);

	// Calculate exposure separately for each vertex of quad and write to own position in buffer.
	// Mix current exposure with previous.
	// Use inverse values in mix function for better look.

	gl_Position= vec4(pos[gl_VertexIndex], 0.0, 1.0);
	f_tex_coord= pos[gl_VertexIndex];

	float brightness= dot(textureLod(brightness_tex, vec2(0.5, 0.5), 16).rgb, vec3(0.299, 0.587, 0.114));
	float cur_exposure= 0.6 * pow(brightness + 0.001, -0.75);

	exposure[gl_VertexIndex]= 1.0 / mix(1.0 / exposure[gl_VertexIndex], 1.0 / cur_exposure, mix_factor.x);
	f_exposure= exposure[gl_VertexIndex];
}
