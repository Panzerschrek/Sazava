#version 450

layout(push_constant) uniform uniforms_block
{
	vec4 dummy;
};

layout(location= 0) in noperspective vec2 f_pos;

layout(location= 0) out vec4 color;

void main()
{
	color= vec4( f_pos * 0.5 + vec2( 0.5, 0.5 ), 0.0, 1.0 );
}
