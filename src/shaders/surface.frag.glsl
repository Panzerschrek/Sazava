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

	vec3 n= normalize(f_dir);
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

	vec3 intersection_pos= v + n * dist;

	vec3 normal=
		2.0 * xx_yy_zz * intersection_pos +
		vec3( xy, xy, xz ) * intersection_pos.yxx +
		vec3( xz, yz, yz ) * intersection_pos.zzy +
		x_y_z;

	normal= normalize(normal);
	float sun_light_dot= max( dot( normal, dir_to_sun_normalized.xyz ), 0.0 );

	vec3 tex_value= TextureFetch3d( intersection_pos, 0.01 );
	vec3 color= tex_value * ( sun_light_dot * sun_color.rgb + ambient_light_color.rgb );

	out_color= vec4( color, 0.5 );
	// TODO - set depth
}
