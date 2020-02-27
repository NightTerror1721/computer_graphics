//global variables from the CPU
uniform mat4 model;
uniform mat4 viewprojection;

//vars to pass to the pixel shader
varying vec3 v_wPos;
varying vec3 v_wNormal;
varying vec3 v_color; //creo una variable pel color 

//here create uniforms for all the data we need here
uniform vec3 ambiental_intensity;
uniform vec3 ambiental_color;
uniform vec3 diffuse_intensity;
uniform vec3 difuse_color;
uniform vec3 specular_intensity;
uniform vec3 specular_color;
uniform vec3 light_position;
uniform vec3 eye_position;



void main()
{	
	//convert local coordinate to world coordinates
	vec3 wPos = (model * vec4( gl_Vertex.xyz, 1.0)).xyz;
	//convert local normal to world coordinates
	vec3 wNormal = (model * vec4( gl_Normal.xyz, 0.0)).xyz;

	//pass them to the pixel shader interpolated
	v_wPos = wPos;
	v_wNormal = wNormal;

	//in GOURAUD compute the color here and pass it to the pixel shader
	

	//project the vertex by the model view projection 
	gl_Position = viewprojection * vec4(wPos,1.0); //output of the vertex shader
}