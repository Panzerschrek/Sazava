#version 450

layout(push_constant) uniform uniforms_block
{
	mat4 mat;
	vec4 cam_pos;
	vec4 dir_to_sun_normalized;
	vec4 sun_color;
	vec4 ambient_light_color;
};

layout(location=0) in vec3 f_dir;
layout(location=1) in flat float f_surface_description_offset;

layout(set= 0, binding= 0, std430) buffer readonly csg_data_block
{
	float surfaces_description[];
};

layout(set= 0, binding= 1, std430) buffer readonly csg_expressions_block
{
	int expressions_description[];
};

layout(location=0) out vec4 out_color;

struct SurfaceDescription
{
	vec3 xx_yy_zz;
	vec3 xy_xz_yz;
	vec3 x_y_z;
	float k;
};

SurfaceDescription FetchSurface(int index)
{
	SurfaceDescription s;

	int offset= index * 16;
	s.xx_yy_zz= vec3( surfaces_description[offset+0], surfaces_description[offset+1], surfaces_description[offset+2] );
	s.xy_xz_yz= vec3( surfaces_description[offset+3], surfaces_description[offset+4], surfaces_description[offset+5] );
	s.x_y_z   = vec3( surfaces_description[offset+6], surfaces_description[offset+7], surfaces_description[offset+8] );
	s.k= surfaces_description[offset+9];

	return s;
}

struct TextureVecs
{
	vec3 u;
	vec3 v;
};

TextureVecs FetchTextureVecs(int index)
{
	TextureVecs tv;

	int offset= index * 16;
	tv.u= vec3( surfaces_description[offset+10], surfaces_description[offset+11], surfaces_description[offset+12] );
	tv.v= vec3( surfaces_description[offset+13], surfaces_description[offset+14], surfaces_description[offset+15] );
	return tv;
}

vec3 TextureFetch3d( vec3 coord, float smooth_size )
{
	vec3 tc_mod= abs( fract( coord * 6.0 ) - vec3( 0.5, 0.5, 0.5 ) );
	vec3 tc_step= smoothstep( 0.25 - smooth_size, 0.25 + smooth_size, tc_mod );

	float bit= abs( abs( tc_step.x - tc_step.y ) - tc_step.z );
	bit= bit * 0.5 + 0.4;
	return vec3( bit, bit, bit );
}

bool IsInsideFigure(vec3 pos)
{
	bool expressions_stack[16];
	int stack_size= 0;

	const int
		op_code_mul= 0,
		op_code_add= 1,
		op_code_sub= 2,
		op_code_leaf= 3,
		op_code_one_leaf= 4;

	int
		offset= int(f_surface_description_offset) + 2,
		end_offset= expressions_description[ int(f_surface_description_offset) + 1 ];
	while( offset < end_offset )
	{
		int op= expressions_description[offset];
		++offset;
		switch( op )
		{
		case op_code_mul:
			{
				bool l= expressions_stack[stack_size - 2];
				bool r= expressions_stack[stack_size - 1];
				expressions_stack[stack_size - 2]= l && r;
				--stack_size;
			}
			break;
		case op_code_add:
			{
				bool l= expressions_stack[stack_size - 2];
				bool r= expressions_stack[stack_size - 1];
				expressions_stack[stack_size - 2]= l || r;
				--stack_size;
			}
			break;
		case op_code_sub:
			{
				bool l= expressions_stack[stack_size - 2];
				bool r= expressions_stack[stack_size - 1];
				expressions_stack[stack_size - 2]= l && !r;
				--stack_size;
			}
			break;
		case op_code_leaf:
			{
				int surface_index= expressions_description[offset];
				++offset;
				SurfaceDescription s= FetchSurface( surface_index );

				float val=
					dot( s.xx_yy_zz, pos * pos ) +
					dot( s.xy_xz_yz, pos.xxy * pos.yzz ) +
					dot( s.x_y_z, pos ) +
					s.k;

				expressions_stack[stack_size]= val < 0.0;
				++stack_size;
			}
			break;
		case op_code_one_leaf:
			{
				expressions_stack[stack_size]= true;
				++stack_size;
			}
			break;
		}

		if( stack_size == 0 )
			break;
	}

	return expressions_stack[0];
}

void main()
{
	// Find itersection between ray from camera and surface, solving quadratic equation relative to "distance" variable.
	// This variable is not real distance, since input direction vector is not normalized.
	int surface_index= expressions_description[int(f_surface_description_offset)];
	SurfaceDescription s= FetchSurface( surface_index );

	vec3 n= f_dir;
	vec3 v= cam_pos.xyz;
	float a= dot( s.xx_yy_zz, n * n ) + dot( s.xy_xz_yz, n.xxy * n.yzz );
	float b= 2.0 * dot( s.xx_yy_zz, n * v ) + dot( s.xy_xz_yz, v.xxy * n.yzz + v.yzz * n.xxy ) + dot( s.x_y_z, n );
	float c= dot( s.xx_yy_zz, v * v ) + dot( s.xy_xz_yz, v.xxy * v.yzz ) + dot( s.x_y_z, v ) + s.k;

	float dist0, dist1;
	if( a == 0.0 )
	{
		if( b == 0.0 )
			discard;

		dist0= dist1= -c / b;
	}
	else
	{
		float d= b * b - 4.0 * a * c;
		if( d < 0.0 )
			discard;

		float d_root= sqrt(d);
		float two_a= 2.0 * a;
		dist0= ( -b - d_root ) / two_a;
		dist1= ( -b + d_root ) / two_a;
	}

	float dist_min= min(dist0, dist1);
	float dist_max= max(dist0, dist1);
	if( dist_max < 0.0 )
		discard;

	float dist= dist_min;
	if( dist_min <= 0.0 )
		dist= dist_max;

	vec3 vec_to_intersection_pos= n * dist;
	vec3 intersection_pos= v + vec_to_intersection_pos;

	if( !IsInsideFigure( intersection_pos ) )
	{
		dist= dist_max;
		vec_to_intersection_pos= n * dist;
		intersection_pos= v + vec_to_intersection_pos;

		if( !IsInsideFigure( intersection_pos ) )
			discard;
	}

	vec3 normal=
		2.0 * s.xx_yy_zz * intersection_pos +
		s.xy_xz_yz.rrg * intersection_pos.yxx +
		s.xy_xz_yz.gbb * intersection_pos.zzy +
		s.x_y_z;

	normal= normalize(normal);

	float dir_normal_dot= dot( normal, n );
	if( dir_normal_dot > 0.0 )
		normal= -normal;

	float sun_light_dot= max( dot( normal, dir_to_sun_normalized.xyz ), 0.0 );

	TextureVecs texture_vecs= FetchTextureVecs( surface_index );
	vec3 tex_coord= vec3( dot( texture_vecs.u, intersection_pos ), dot(texture_vecs.v, intersection_pos ), 0.0 );

	float smooth_size= dist * 0.02 / max( 0.1, abs(dir_normal_dot) );
	vec3 tex_value= TextureFetch3d( tex_coord, smooth_size );

	vec3 color= tex_value * ( sun_light_dot * sun_color.rgb + ambient_light_color.rgb );
	out_color= vec4( color, 0.5 );

	const float z_far= 256.0;
	// Store squared length, converted to range [0; z_far * z_far]
	gl_FragDepth= dot( vec_to_intersection_pos, vec_to_intersection_pos ) * (1.0 / (z_far * z_far));
}
