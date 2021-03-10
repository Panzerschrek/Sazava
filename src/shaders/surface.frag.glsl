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
layout(location=1) in flat float f_surface_index;

layout(set= 0, binding= 0, std430) buffer readonly csg_data_block
{
	float surfaces_description[];
};

layout(set= 0, binding= 1, std430) buffer readonly csg_expressions_block
{
	int expressions_description[];
};

layout(location=0) out vec4 out_color;

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
	// Find itersection between ray from camera and surface, solving quadratic equation relative to "distance" variable.
	// This variable is not real distance, since input direction vector is not normalized.
	int offset= int(f_surface_index) * 16;
	float xx= surfaces_description[offset+0];
	float yy= surfaces_description[offset+1];
	float zz= surfaces_description[offset+2];
	float xy= surfaces_description[offset+3];
	float xz= surfaces_description[offset+4];
	float yz= surfaces_description[offset+5];
	float x = surfaces_description[offset+6];
	float y = surfaces_description[offset+7];
	float z = surfaces_description[offset+8];
	float k = surfaces_description[offset+9];

	vec3 n= f_dir;
	vec3 v= cam_pos.xyz;
	vec3 xx_yy_zz= vec3( xx, yy, zz );
	vec3 xy_xz_yz= vec3( xy, xz, yz );
	vec3 x_y_z= vec3( x, y, z );
	float a= dot( xx_yy_zz, n * n ) + dot( xy_xz_yz, n.xxy * n.yzz );
	float b= 2.0 * dot( xx_yy_zz, n * v ) + dot( xy_xz_yz, v.xxy * n.yzz + v.yzz * n.xxy ) + dot( x_y_z, n );
	float c= dot( xx_yy_zz, v * v ) + dot( xy_xz_yz, v.xxy * v.yzz ) + dot( x_y_z, v ) + k;

	float dist0, dist1;
	if( a == 0.0 )
	{
		if( b == 0.0 )
		{
			discard;
		}
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

	{
		int expressions_stack[8]; // TODO - replace with "bool"
		expressions_stack[0]= 1;
		int stack_size= 0;

		int o= 1, end= expressions_description[0];
		for( int i= 0; i < 128 && o < end; ++i ) // Limit number of iterations
		{
			int operation= expressions_description[o];
			++o;
			if( operation == 100 )
			{
				int l= expressions_stack[stack_size - 2];
				int r= expressions_stack[stack_size - 1];
				expressions_stack[ stack_size - 2 ]= l & r;
				--stack_size;
			}
			else if( operation == 101 )
			{
				int l= expressions_stack[stack_size - 2];
				int r= expressions_stack[stack_size - 1];
				expressions_stack[ stack_size - 2 ]= l | r;
				--stack_size;
			}
			else if( operation == 102 )
			{
				int l= expressions_stack[stack_size - 2];
				int r= expressions_stack[stack_size - 1];
				expressions_stack[ stack_size - 2 ]= l & ~r;
				--stack_size;
			}
			else if( operation == 200 )
			{
				int surf= expressions_description[o] * 16;
				++o;
				float xx= surfaces_description[surf+0];
				float yy= surfaces_description[surf+1];
				float zz= surfaces_description[surf+2];
				float xy= surfaces_description[surf+3];
				float xz= surfaces_description[surf+4];
				float yz= surfaces_description[surf+5];
				float x = surfaces_description[surf+6];
				float y = surfaces_description[surf+7];
				float z = surfaces_description[surf+8];
				float k = surfaces_description[surf+9];

				float val=
					xx * intersection_pos.x * intersection_pos.x +
					yy * intersection_pos.y * intersection_pos.y +
					zz * intersection_pos.z * intersection_pos.z +
					xy * intersection_pos.x * intersection_pos.y +
					xz * intersection_pos.x * intersection_pos.z +
					yz * intersection_pos.y * intersection_pos.z +
					x * intersection_pos.x +
					y * intersection_pos.y +
					z * intersection_pos.z +
					k;

				if( val >= 0.01 ) // TODO - remove this epsilon, avoid self-intersection by excluding surface from its expression
					expressions_stack[stack_size]= 0;
				else
					expressions_stack[stack_size]= 1;
				++stack_size;
			}
			else
				break;

			if( stack_size == 0 )
				break;
		}

		if( expressions_stack[0] == 0 )
			discard;
	}

	vec3 normal=
		2.0 * xx_yy_zz * intersection_pos +
		vec3( xy, xy, xz ) * intersection_pos.yxx +
		vec3( xz, yz, yz ) * intersection_pos.zzy +
		x_y_z;

	normal= normalize(normal);
	float sun_light_dot= max( dot( normal, dir_to_sun_normalized.xyz ), 0.0 );

	float smooth_size= dist * 0.02 / max( 0.1, -dot( normal, n ) );
	vec3 tex_value= TextureFetch3d( intersection_pos, smooth_size );
	vec3 color= tex_value * ( sun_light_dot * sun_color.rgb + ambient_light_color.rgb );

	out_color= vec4( color, 0.5 );

	const float z_far= 256.0;
	// Store squared length, converted to range [0; z_far * z_far]
	gl_FragDepth= dot( vec_to_intersection_pos, vec_to_intersection_pos ) * (1.0 / (z_far * z_far));
}
