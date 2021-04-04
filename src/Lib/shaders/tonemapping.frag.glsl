#version 450


layout(binding= 0) uniform sampler2D tex;
layout(binding= 3) uniform sampler2D blured_tex;

layout(push_constant) uniform uniforms_block
{
	layout(offset = 16) vec4 bloom_scale; // .x used
};

layout(location= 0) in vec2 f_tex_coord; // In range [-1; 1]
layout(location= 1) in flat float f_exposure;

layout(location = 0) out vec4 out_color;

vec3 tonemapping_function(vec3 x, float exposure)
{
	// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
	x*= exposure;
	float a= 2.51;
	float b= 0.03;
	float c= 2.43;
	float d= 0.59;
	float e= 0.14;
	return (x * (a * x + b))/(x * (c * x + d) + e);
}

void main()
{
	vec3 color= texture(tex, f_tex_coord).rgb;
	vec3 blured_color= texture(blured_tex, f_tex_coord).rgb;

	color= tonemapping_function(color + blured_color * bloom_scale.x, f_exposure);

	out_color= vec4(color, 1.0);
}
