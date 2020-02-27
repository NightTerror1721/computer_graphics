//global variables from the CPU
uniform mat4 model; //matriu de les coordenades locals
uniform mat4 viewprojection; //matriu (matrix_view x matrix_projection) (més info P3)

//vars to pass to the pixel shader
varying vec3 v_wPos; //Posició ja projectada
varying vec3 v_wNormal; //Normal ja projectada (ja ha passat de coordenades locals a coordenades del mon, s'ha extret ja del model)

//here create uniforms for all the data we need here

void main()
{	
	//convert local coordinate to world coordinates
	vec3 wPos = (model * vec4( gl_Vertex.xyz, 1.0)).xyz; //:)
	//convert local normal to world coordinates
	vec3 wNormal = (model * vec4( gl_Normal.xyz, 0.0)).xyz; //:)

	//pass them to the pixel shader interpolated
	v_wPos = wPos; //envia la info al pixel shader
	v_wNormal = wNormal; // lo mismo pero amb la normal :)

	//in GOURAUD compute the color here and pass it to the pixel shader
	//...

	//project the vertex by the model view projection 
	gl_Position = viewprojection * vec4(wPos,1.0); //output of the vertex shader //fa la projecció (del món al clip space)
}