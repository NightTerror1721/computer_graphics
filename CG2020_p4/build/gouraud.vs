uniform mat4 model;
uniform mat4 viewprojection;

uniform vec3 ambient_material;  //Ka
uniform vec3 diffuse_material;  //Kd
uniform vec3 specular_material; //Ks

uniform vec3 ambient_light;  //Ia
uniform vec3 diffuse_light;  //Id
uniform vec3 specular_light; //Is

uniform float shininess;

uniform vec3 light_position;
uniform vec3 eye_position;


varying vec3 v_color;


void main()
{	
	vec3 wPos = (model * vec4( gl_Vertex.xyz, 1.0)).xyz;
	vec3 wNormal = (model * vec4( gl_Normal.xyz, 0.0)).xyz;

	vec3 vToLight = normalize(light_position - wPos);
	vec3 vToCam =  normalize(eye_position - wPos);
	vec3 vReflection = normalize(-reflect(vToLight, wNormal));

	vec3 ambient = ambient_material * ambient_light;
	vec3 diffuse = diffuse_material * diffuse_light * max(dot(wNormal, vToLight), 0.0);
	vec3 specular = specular_material * specular_light * pow(max(dot(vReflection, vToCam), 0.0), shininess);

	diffuse = clamp(diffuse, 0.0, 1.0);
	specular = clamp(specular, 0.0, 1.0);

	v_color = ambient + diffuse + specular;

	gl_Position = viewprojection * vec4(wPos,1.0);
}