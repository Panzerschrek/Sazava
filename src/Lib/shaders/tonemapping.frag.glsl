#version 450


layout(binding= 0) uniform sampler2D tex;
layout(binding= 3) uniform sampler2D blured_tex;

layout(push_constant) uniform uniforms_block
{
	layout(offset = 16) vec4 deform_factor;
	vec4 bloom_scale; // .x used
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
	float scale_base= dot(f_tex_coord, f_tex_coord);
	vec2 tex_coord_r= f_tex_coord * ((scale_base + deform_factor.r) * (0.5 / (2.0 + deform_factor.r))) + vec2(0.5, 0.5);
	vec2 tex_coord_g= f_tex_coord * ((scale_base + deform_factor.g) * (0.5 / (2.0 + deform_factor.g))) + vec2(0.5, 0.5);
	vec2 tex_coord_b= f_tex_coord * ((scale_base + deform_factor.b) * (0.5 / (2.0 + deform_factor.b))) + vec2(0.5, 0.5);

	vec3 color=
		vec3(
			texture(tex, tex_coord_r).r,
			texture(tex, tex_coord_g).g,
			texture(tex, tex_coord_b).b
			);
	vec3 blured_color=
		vec3(
			texture(blured_tex, tex_coord_r).r,
			texture(blured_tex, tex_coord_g).g,
			texture(blured_tex, tex_coord_b).b
			);

	color= tonemapping_function(color + blured_color * bloom_scale.x, f_exposure);

	out_color= vec4(color, 1.0);
	//out_color= texture(tex, tex_coord_r);
}
