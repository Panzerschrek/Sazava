#version 450

layout(push_constant) uniform uniforms_block
{
	vec4 blur_vector;
};

layout(binding= 0) uniform sampler2D tex;

layout(location= 0) in noperspective vec2 f_tex_coord;

layout(location= 0) out vec4 color;

void main()
{
	vec2 tex_size= vec2(textureSize(tex, 0));
	int pixels_along_vector= int(dot(blur_vector.xy, tex_size));

	vec4 r= vec4(0.0, 0.0, 0.0, 0.0);
	vec2 blur_vector_scaled= blur_vector.xy / float(pixels_along_vector);

	const float c_exp_factor= -6.0;
	float attenuation_factor= c_exp_factor / float(pixels_along_vector * pixels_along_vector);
	float scale_factor_sub= exp2(c_exp_factor);

	float scale_factor_accumulated= 0.0;
	for(int i= -pixels_along_vector; i <= pixels_along_vector; ++i)
	{
		vec4 tex_value= texture(tex, f_tex_coord + float(i) * blur_vector_scaled);
		float scale_factor= exp2(attenuation_factor * float(i*i)) - scale_factor_sub;
		r+= scale_factor * tex_value;
		scale_factor_accumulated+= scale_factor;
	}
	color= r / scale_factor_accumulated;
}
