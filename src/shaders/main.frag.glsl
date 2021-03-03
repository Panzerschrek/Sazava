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

layout(location= 0) in noperspective vec3 f_dir;

layout(set= 0, binding= 0, std430) buffer readonly csg_data_block
{
	float csg_data[];
};

layout(location= 0) out vec4 color;

const float pi= 3.1415926535;
const float inv_pi= 1.0 / pi;
const float z_near= 0.02;
const float shadow_offset= 0.01;
const float almost_infinity= 1.0e16;

struct RangePoint
{
	float dist;
	vec3 tc; // including texture index
	vec3 normal;
	// Store here sqaured scale of texture coordinates
	float tc_scale;
};

struct Range
{
	RangePoint min;
	RangePoint max;
};

Range MultiplyRanges( Range range0, Range range1 )
{
	Range res;

	if( range0.min.dist > range1.min.dist )
		res.min= range0.min;
	else
		res.min= range1.min;

	if( range0.max.dist < range1.max.dist )
		res.max= range0.max;
	else
		res.max= range1.max;

	return res;
}

Range AddRanges( Range range0, Range range1 )
{
	if( range0.min.dist >= range0.max.dist )
		return range1;
	if( range1.min.dist >= range1.max.dist )
		return range0;

	Range res;

	if( range0.min.dist < range1.min.dist )
		res.min= range0.min;
	else
		res.min= range1.min;

	if( range0.max.dist > range1.max.dist )
		res.max= range0.max;
	else
		res.max= range1.max;

	return res;
}

Range SubtractRanges( Range range0, Range range1 )
{
	if( range0.min.dist >= range0.max.dist || range1.min.dist >= range1.max.dist )
		return range0; // Invalid ranges.

	if( range0.max.dist <= range1.min.dist || range0.min.dist >= range1.max.dist )
		return range0; // Ther is no intersection between ranges - just return first range.

	if( range1.min.dist > range0.min.dist && range1.max.dist < range0.max.dist )
		return range0; // Second range is inside first - just return first range.

	Range res;
	if( range0.min.dist >= range1.min.dist && range0.max.dist <= range1.max.dist )
	{
		// Range is completely zeroed.
		res.min.dist= range0.min.dist;
		res.max.dist= range0.min.dist;
	}
	else if( range0.min.dist >= range1.min.dist )
	{
		res.min= range1.max;
		res.max= range0.max;
		res.min.normal= -res.min.normal;
	}
	else
	{
		res.min= range0.min;
		res.max= range1.min;
		res.max.normal= -res.max.normal;
	}

	return res;
}

// Input plane "normal" may be not normalized. Scale normal and binormal together to scale texture coordinates.
Range GetPlaneIntersection(
	vec3 plane_point, vec3 plane_normal, vec3 plane_binormal,
	vec3 beam_point, vec3 beam_dir_normalized )
{
	Range range;

	float signed_distance_to_plane= dot( plane_normal, plane_point - beam_point );
	float dirs_dot = dot( plane_normal, beam_dir_normalized );
	if( dirs_dot == 0.0 )
	{
		range.min.dist= -almost_infinity;
		range.max.dist= +almost_infinity;
		return range;
	}

	float dist= signed_distance_to_plane / dirs_dot;

	// Project texture, using plane point as texture basis center and normal and binormal as basis vectors.
	// Calculate tanged using normal and binormal.
	// This approach allows texture mapping with non-orthogonal basis, but does not allow mirrored texture mapping and mapping with non-orthogonal basis.
	vec3 intersection_pos= beam_point + dist * beam_dir_normalized;
	vec3 intersection_pos_relative= intersection_pos - plane_point;
	vec3 plane_tangent= cross( plane_normal, plane_binormal );

	vec3 tc= vec3( dot( intersection_pos_relative, plane_binormal ), dot( intersection_pos_relative, plane_tangent ), 0.0 );

	range.min.tc= tc;
	range.max.tc= tc;
	range.min.normal= plane_normal;
	range.max.normal= plane_normal;

	// TODO - set proper values for scale.
	range.min.tc_scale= 1.0;
	range.max.tc_scale= 1.0;

	if( dirs_dot > 0.0 )
	{
		range.min.dist= z_near;
		range.max.dist= dist;
	}
	else
	{
		range.min.dist= max( z_near, dist );
		range.max.dist= +almost_infinity;
	}
	return range;
}

Range GetSphereIntersection( vec3 start, vec3 dir_normalized, vec3 center, float square_radius )
{
	// Find itersection between ray and sphere, solving quadratic equation relative do "distance" variable.
	vec3 v0= start - center;

	//float a= dot( dir_normalized, dir_normalized ); // "a" is always 1.0 since direction is normalized.
	float b= 2.0 * dot( dir_normalized, v0 );
	float c= dot( v0, v0 ) - square_radius;

	float d= b * b - 4.0 * c;
	if( d < 0.0 )
	{
		// No quadratic equation roots - has no intersection.
		Range res;
		res.min.dist= +almost_infinity;
		res.max.dist= -almost_infinity;
		return res;
	}

	float d_root= sqrt(d);
	float dist_min= 0.5 * ( -b - d_root );
	float dist_max= 0.5 * ( -b + d_root );

	vec3 pos_min= v0 + dir_normalized * dist_min;
	vec3 pos_max= v0 + dir_normalized * dist_max;

	// TODO - provide basis vectors for texture mapping.
	Range res;
	res.min.dist= dist_min;
	res.max.dist= dist_max;
	res.min.tc= vec3( 3.0 * inv_pi * vec2( atan( pos_min.y, pos_min.x ), atan( length(pos_min.xy), pos_min.z ) ), 0.0 );
	res.max.tc= vec3( 3.0 * inv_pi * vec2( atan( pos_max.y, pos_max.x ), atan( length(pos_max.xy), pos_max.z ) ), 0.0 );
	res.min.normal= pos_min;
	res.max.normal= pos_max;
	res.min.tc_scale= square_radius;
	res.max.tc_scale= square_radius;

	res.min.dist= max( z_near, res.min.dist );

	return res;
}

Range GetCylinderIntersection( vec3 start, vec3 dir_normalized, vec3 center, vec3 normal, vec3 binormal, float square_radius )
{
	// Find itersection between ray and cylinder, solving quadratic equation relative do "distance" variable.
	// Before doing this, convert input vectors into cylinder space, using basis vectors.
	vec3 tangent= cross( normal, binormal );

	vec3 v0= start - center;
	v0= vec3( dot( v0, tangent ), dot( v0, binormal ), dot( v0, normal ) );
	vec3 dir= vec3( dot( dir_normalized, tangent ), dot( dir_normalized, binormal ), dot( dir_normalized, normal ) );

	float a= dot( dir.xy, dir.xy );
	float b= 2.0 * dot( dir.xy, v0.xy );
	float c= dot( v0.xy, v0.xy ) - square_radius;

	float dist_min, dist_max;
	if( a == 0.0 )
	{
		if( b == 0.0 )
		{
			// No linear equation roots - has no intersection.
			Range res;
			res.min.dist= +almost_infinity;
			res.max.dist= -almost_infinity;
			return res;
		}
		dist_min= dist_max= -c / b;
	}
	else
	{
		float d= b * b - 4.0 * a * c;
		if( d < 0.0 )
		{
			// No quadratic equation roots - has no intersection.
			Range res;
			res.min.dist= +almost_infinity;
			res.max.dist= -almost_infinity;
			return res;
		}

		float d_root= sqrt(d);
		float two_a= 2.0 * a;
		dist_min= ( -b - d_root ) / two_a;
		dist_max= ( -b + d_root ) / two_a;
	}

	vec3 pos_min= v0 + dir * dist_min;
	vec3 pos_max= v0 + dir * dist_max;

	Range res;
	res.min.dist= dist_min;
	res.max.dist= dist_max;
	res.min.tc= vec3( 3.0 * inv_pi * atan( pos_min.y, pos_min.x ), pos_min.z, 0.0 );
	res.max.tc= vec3( 3.0 * inv_pi * atan( pos_max.y, pos_max.x ), pos_max.z, 0.0 );
	res.min.normal= tangent * pos_min.x + binormal * pos_min.y;
	res.max.normal= tangent * pos_max.x + binormal * pos_max.y;
	res.min.tc_scale= 1.0;
	res.max.tc_scale= 1.0;

	res.min.dist= max( z_near, res.min.dist );

	return res;
}

Range GetConeIntersection( vec3 start, vec3 dir_normalized, vec3 center, vec3 normal, vec3 binormal, float square_tangent )
{
	// Find itersection between ray and cone, solving quadratic equation relative do "distance" variable.
	// Before doing this, convert input vectors into cone space, using basis vectors.
	vec3 tangent= cross( normal, binormal );

	vec3 v0= start - center;
	v0= vec3( dot( v0, tangent ), dot( v0, binormal ), dot( v0, normal ) );
	vec3 dir= vec3( dot( dir_normalized, tangent ), dot( dir_normalized, binormal ), dot( dir_normalized, normal ) );

	float a= dot( dir.xy, dir.xy ) - square_tangent * ( dir.z * dir.z );
	float b= 2.0 * ( dot( dir.xy, v0.xy ) - square_tangent * ( dir.z * v0.z ) );
	float c= dot( v0.xy, v0.xy ) - square_tangent * ( v0.z * v0.z );

	float dist_min, dist_max;
	if( a == 0.0 )
	{
		if( b == 0.0 )
		{
			// No linear equation roots - has no intersection.
			Range res;
			res.min.dist= +almost_infinity;
			res.max.dist= -almost_infinity;
			return res;
		}
		dist_min= dist_max= -c / b;
	}
	else
	{
		float d= b * b - 4.0 * a * c;
		if( d < 0.0 )
		{
			// No quadratic equation roots - has no intersection.
			Range res;
			res.min.dist= +almost_infinity;
			res.max.dist= -almost_infinity;
			return res;
		}

		float d_root= sqrt(d);
		float two_a= 2.0 * a;
		dist_min= ( -b - d_root ) / two_a;
		dist_max= ( -b + d_root ) / two_a;
	}

	vec3 pos_min= v0 + dir * dist_min;
	vec3 pos_max= v0 + dir * dist_max;
	vec3 normal_min= vec3( pos_min.xy, -square_tangent * pos_min.z );
	vec3 normal_max= vec3( pos_max.xy, -square_tangent * pos_max.z );

	Range res;
	res.min.dist= dist_min;
	res.max.dist= dist_max;
	res.min.tc= vec3( 3.0 * inv_pi * atan( pos_min.y, pos_min.x ), pos_min.z, 0.0 );
	res.max.tc= vec3( 3.0 * inv_pi * atan( pos_max.y, pos_max.x ), pos_max.z, 0.0 );
	res.min.normal= tangent * normal_min.x + binormal * normal_min.y + normal * normal_min.z;
	res.max.normal= tangent * normal_max.x + binormal * normal_max.y + normal * normal_max.z;
	res.min.tc_scale= 1.0;
	res.max.tc_scale= 1.0;

	// Disable lower part of cone, because we need to produce convex body.
	if( a > 0.0 )
	{
		if( pos_min.z < 0.0 )
		{
			res.min.dist= +almost_infinity;
			res.max.dist= -almost_infinity;
		}
	}
	else if( c > 0.0 )
	{
		// Outside cone.
		if( pos_min.z < 0.0 )
		{
			res.min.dist= +almost_infinity;
			res.max.dist= -almost_infinity;
		}
		else
			res.max.dist= +almost_infinity;
	}
	else
	{
		// Inside cone.
		if( v0.z < 0.0 )
		{
			// Inside lower cone.
			if( b > 0.0 )
				res.max.dist= +almost_infinity;
		}
		else
		{
			// Inside upper cone.
			res.min.dist= z_near;

			if( b < 0.0 )
				res.max.dist= +almost_infinity;
		}
	}

	res.min.dist= max( z_near, res.min.dist );

	return res;
}

Range GetParaboloidIntersection( vec3 start, vec3 dir_normalized, vec3 center, vec3 normal, vec3 binormal, float k )
{
	// Find itersection between ray and paraboloid, solving quadratic equation relative do "distance" variable.
	// Before doing this, convert input vectors into paraboloid space, using basis vectors.
	vec3 tangent= cross( normal, binormal );

	vec3 v0= start - center;
	v0= vec3( dot( v0, tangent ), dot( v0, binormal ), dot( v0, normal ) );
	vec3 dir= vec3( dot( dir_normalized, tangent ), dot( dir_normalized, binormal ), dot( dir_normalized, normal ) );

	float a= dot( dir.xy, dir.xy );
	float b= 2.0 * dot( dir.xy, v0.xy ) - k * dir.z;
	float c= dot( v0.xy, v0.xy ) - k * v0.z;

	float dist_min, dist_max;
	if( a == 0.0 )
	{
		if( b == 0.0 )
		{
			// No linear equation roots - has no intersection.
			Range res;
			res.min.dist= +almost_infinity;
			res.max.dist= -almost_infinity;
			return res;
		}
		dist_min= dist_max= -c / b;
	}
	else
	{
		float d= b * b - 4.0 * a * c;
		if( d < 0.0 )
		{
			// No quadratic equation roots - has no intersection.
			Range res;
			res.min.dist= +almost_infinity;
			res.max.dist= -almost_infinity;
			return res;
		}

		float d_root= sqrt(d);
		float two_a= 2.0 * a;
		dist_min= ( -b - d_root ) / two_a;
		dist_max= ( -b + d_root ) / two_a;
	}

	vec3 pos_min= v0 + dir * dist_min;
	vec3 pos_max= v0 + dir * dist_max;
	vec3 normal_min= vec3( pos_min.xy, -0.5 * k );
	vec3 normal_max= vec3( pos_max.xy, -0.5 * k );

	Range res;
	res.min.dist= dist_min;
	res.max.dist= dist_max;
	res.min.tc= vec3( 3.0 * inv_pi * atan( pos_min.y, pos_min.x ), pos_min.z, 0.0 );
	res.max.tc= vec3( 3.0 * inv_pi * atan( pos_max.y, pos_max.x ), pos_max.z, 0.0 );
	res.min.normal= tangent * normal_min.x + binormal * normal_min.y + normal * normal_min.z;
	res.max.normal= tangent * normal_max.x + binormal * normal_max.y + normal * normal_max.z;
	res.min.tc_scale= 1.0;
	res.max.tc_scale= 1.0;

	res.min.dist= max( z_near, res.min.dist );

	return res;
}

Range GetSceneIntersection( vec3 start_pos, vec3 dir_normalized )
{
	const int ranges_stack_size_max= 8;
	Range ranges_stack[ranges_stack_size_max];
	int ranges_stack_size= 0;

	int offset_end= int(csg_data[0]);
	int offset= 1;
	while(offset < offset_end)
	{
		int element_type= int(csg_data[offset]);
		++offset;

		if( element_type == 0 )
			break;
		else if( element_type == 1 )
		{
			if( ranges_stack_size < 2 )
				break;

			Range range0= ranges_stack[ ranges_stack_size - 1 ];
			Range range1= ranges_stack[ ranges_stack_size - 2 ];
			Range result_range= MultiplyRanges( range0, range1 );

			ranges_stack[ ranges_stack_size - 2 ]= result_range;
			--ranges_stack_size;
		}
		else if( element_type == 2 )
		{
			if( ranges_stack_size < 2 )
				break;

			Range range0= ranges_stack[ ranges_stack_size - 1 ];
			Range range1= ranges_stack[ ranges_stack_size - 2 ];
			Range result_range= AddRanges( range0, range1 );

			ranges_stack[ ranges_stack_size - 2 ]= result_range;
			--ranges_stack_size;
		}
		else if( element_type == 3 )
		{
			if( ranges_stack_size < 2 )
				break;

			Range range0= ranges_stack[ ranges_stack_size - 2 ];
			Range range1= ranges_stack[ ranges_stack_size - 1 ];
			Range result_range= SubtractRanges( range0, range1 );

			ranges_stack[ ranges_stack_size - 2 ]= result_range;
			--ranges_stack_size;
		}
		else if( element_type == 101 )
		{
			if( ranges_stack_size >= ranges_stack_size_max )
				break;

			vec3 center= vec3( csg_data[offset+0], csg_data[offset+1], csg_data[offset+2] );
			float square_radius= csg_data[offset+3];
			offset+= 4;

			ranges_stack[ranges_stack_size]=
				GetSphereIntersection( start_pos, dir_normalized, center, square_radius );
			++ranges_stack_size;
		}
		else if( element_type == 102 )
		{
			if( ranges_stack_size >= ranges_stack_size_max )
				break;

			vec3 point   = vec3( csg_data[offset+0], csg_data[offset+1], csg_data[offset+2] );
			vec3 normal  = vec3( csg_data[offset+3], csg_data[offset+4], csg_data[offset+5] );
			vec3 binormal= vec3( csg_data[offset+6], csg_data[offset+7], csg_data[offset+8] );
			offset+= 9;

			ranges_stack[ranges_stack_size]=
				GetPlaneIntersection( point, normal, binormal, start_pos, dir_normalized );
			++ranges_stack_size;
		}
		else if( element_type == 103 )
		{
			if( ranges_stack_size >= ranges_stack_size_max )
				break;

			vec3 center  = vec3( csg_data[offset+0], csg_data[offset+1], csg_data[offset+2] );
			vec3 normal  = vec3( csg_data[offset+3], csg_data[offset+4], csg_data[offset+5] );
			vec3 binormal= vec3( csg_data[offset+6], csg_data[offset+7], csg_data[offset+8] );
			float square_radius= csg_data[offset+9];
			offset+= 10;

			ranges_stack[ranges_stack_size]=
				GetCylinderIntersection( start_pos, dir_normalized, center, normal, binormal, square_radius );
			++ranges_stack_size;
		}
		else if( element_type == 104 )
		{
			if( ranges_stack_size >= ranges_stack_size_max )
				break;

			vec3 center  = vec3( csg_data[offset+0], csg_data[offset+1], csg_data[offset+2] );
			vec3 normal  = vec3( csg_data[offset+3], csg_data[offset+4], csg_data[offset+5] );
			vec3 binormal= vec3( csg_data[offset+6], csg_data[offset+7], csg_data[offset+8] );
			float square_tangent= csg_data[offset+9];
			offset+= 10;

			ranges_stack[ranges_stack_size]=
				GetConeIntersection( start_pos, dir_normalized, center, normal, binormal, square_tangent );
			++ranges_stack_size;
		}
		else if( element_type == 105 )
		{
			if( ranges_stack_size >= ranges_stack_size_max )
				break;

			vec3 center  = vec3( csg_data[offset+0], csg_data[offset+1], csg_data[offset+2] );
			vec3 normal  = vec3( csg_data[offset+3], csg_data[offset+4], csg_data[offset+5] );
			vec3 binormal= vec3( csg_data[offset+6], csg_data[offset+7], csg_data[offset+8] );
			float factor= csg_data[offset+9];
			offset+= 10;

			ranges_stack[ranges_stack_size]=
				GetParaboloidIntersection( start_pos, dir_normalized, center, normal, binormal, factor );
			++ranges_stack_size;
		}
		else
			break;
	}

	if( ranges_stack_size == 0 )
	{
		Range res;
		res.min.dist= +almost_infinity;
		res.max.dist= -almost_infinity;
		return res;
	}
	return ranges_stack[0];
}

vec3 TextureFetch( vec3 tc, float smooth_size )
{
	tc.xy*= 12.0;

	const vec2 texture_patterns[10]= vec2[10]
	(
		vec2( 1.0, 1.0 ), vec2( 1.0, 2.0 ), vec2( 1.0, 3.0 ), vec2( 1.0, 4.0 ), vec2( 1.0, 5.0 ),
		vec2( 2.0, 3.0 ), vec2( 2.0, 5.0 ), vec2( 3.0, 4.0 ), vec2( 3.0, 5.0 ), vec2( 4.0, 5.0 )
	);

	vec2 pattern_u= texture_patterns[ int(mod( tc.z, 10.0 ) ) ];
	vec2 pattern_v= texture_patterns[ int(mod( tc.z / 10.0, 10.0 ) ) ];

	vec2 pattern_size= vec2( pattern_u.x + pattern_u.y, pattern_v.x + pattern_v.y );
	vec2 tc_mod= abs( mod( tc.xy, pattern_size ) - pattern_size * 0.5 );

	vec2 pattern_border= vec2( pattern_u.x, pattern_v.x ) * 0.5;
	vec2 pattern_delta= vec2( smooth_size, smooth_size );
	vec2 tc_step= smoothstep( pattern_border - pattern_delta, pattern_border + pattern_delta, tc_mod );

	float bit= abs( tc_step.x - tc_step.y ) * 0.5 + 0.4;
	return vec3( bit, bit, bit );
}

void main()
{
	vec3 dir_normalized= normalize(f_dir);
	Range range= GetSceneIntersection( cam_pos.xyz, dir_normalized );

	if( range.max.dist <= range.min.dist )
	{
		float sun_dir_dot= dot( dir_normalized, dir_to_sun_normalized.xyz );
		float sun_factor= smoothstep( 0.998, 1.0, sun_dir_dot );
		float sun_ligh_scale= 2.0;
		float ambient_light_scale= 1.5;
		vec3 result_color= sun_ligh_scale * sun_factor * sun_color.rgb + ambient_light_scale * ambient_light_color.rgb;
		color= vec4( result_color, 0.0 );
	}
	else
	{
		vec3 normal= normalize( range.min.normal );
		float sun_light_dot= max( dot( normal, dir_to_sun_normalized.xyz ), 0.0 );

		float shadow_factor;
		{
			vec3 intersection_pos= cam_pos.xyz + range.min.dist * dir_normalized + shadow_offset * dir_to_sun_normalized.xyz;
			Range shadow_range= GetSceneIntersection( intersection_pos, dir_to_sun_normalized.xyz );
			if( shadow_range.max.dist <= shadow_range.min.dist )
				shadow_factor= 1.0;
			else
				shadow_factor= 0.0;
		}

		// Calculate approximation of smooth size for texture fetch.
		// TODO - count also viewport size and field of view.
		float smooth_size= range.min.dist * inversesqrt(range.min.tc_scale) * 0.1 / max( 0.1, -dot( normal, dir_normalized ) );
		vec3 tex_value= TextureFetch( range.min.tc, smooth_size );

		vec3 result_light= tex_value * ( sun_light_dot * shadow_factor * sun_color.rgb + ambient_light_color.rgb );
		color = vec4( result_light, 1.0);
	}
}
