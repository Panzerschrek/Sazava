#version 450

layout(location= 0) out noperspective vec2 f_tex_coord;

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

	gl_Position= vec4(pos[gl_VertexIndex], 0.0, 1.0);
	f_tex_coord= pos[gl_VertexIndex] * 0.5 + vec2(0.5, 0.5);
}
