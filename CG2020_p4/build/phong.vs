uniform mat4 model;
uniform mat4 viewprojection;


varying vec3 v_wPos;
varying vec3 v_wNormal;


void main()
{	
	vec3 wPos = (model * vec4( gl_Vertex.xyz, 1.0)).xyz;
	vec3 wNormal = (model * vec4( gl_Normal.xyz, 0.0)).xyz;

	v_wPos = wPos;
	v_wNormal = wNormal;

	gl_Position = viewprojection * vec4(wPos,1.0);
}