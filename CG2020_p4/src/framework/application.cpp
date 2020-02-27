#include "application.h"
#include "utils.h"
#include "includes.h"
#include "utils.h"

#include "image.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"
#include "camera.h"
#include "material.h"
#include "light.h"

#include <vector>

Camera* camera = NULL;
Mesh* mesh = NULL;
Shader* shader = NULL;

//might be useful...
//Material* material = NULL;
//Light* light = NULL;
Shader* phong_shader = NULL;
Shader* gouraud_shader = NULL;

Shader** selected_shader = &phong_shader;

std::vector<Light> lights;
std::vector<Material> materials;

Vector3 ambient_light(0.1,0.2,0.3); //here we can store the global ambient light of the scene

float angle = 0;

Application::Application(const char* caption, int width, int height)
{
	this->window = createWindow(caption, width, height);

	// initialize attributes
	// Warning: DO NOT CREATE STUFF HERE, USE THE INIT 
	// things create here cannot access opengl
	int w,h;
	SDL_GetWindowSize(window,&w,&h);

	this->window_width = w;
	this->window_height = h;
	this->keystate = SDL_GetKeyboardState(NULL);
}


Light NewLight(const Vector3& position, const Vector3& diffuse, const Vector3& specular)
{
	Light l;
	l.position = position;
	l.diffuse_color = diffuse;
	l.specular_color = specular;
	return std::move(l);
}

Material NewMaterial(const Vector3& ambient, const Vector3& diffuse, const Vector3& specular, float shininess)
{
	Material m;
	m.ambient = ambient;
	m.diffuse = diffuse;
	m.specular = specular;
	m.shininess = shininess;
	return std::move(m);
}

//Here we have already GL working, so we can create meshes and textures
void Application::init(void)
{
	std::cout << "initiating app..." << std::endl;
	
	//here we create a global camera and set a position and projection properties
	camera = new Camera();
	camera->lookAt(Vector3(0,20,20),Vector3(0,10,0),Vector3(0,1,0));
	camera->setPerspective(60,window_width / window_height,0.1,10000);

	//then we load a mesh
	mesh = new Mesh();
	if( !mesh->loadOBJ( "lee.obj" ) )
		std::cout << "FILE Lee.obj NOT FOUND " << std::endl;

	//we load one or several shaders...
	shader = Shader::Get( "simple.vs", "simple.ps" );

	//load your Gouraud and Phong shaders here and stored them in some global variables
	gouraud_shader = Shader::Get("gouraud.vs", "gouraud.ps");
	phong_shader = Shader::Get("phong.vs", "phong.ps");

	//CODE HERE:
	//create a light (or several) and and some materials

	materials = {
		NewMaterial({ 1, 0.1, 0.1 }, { 1, 0.1, 0.1 }, { 1, 0.65, 0.65 }, 16.0),
		NewMaterial({ 0.85, 0.5, 1 }, { 0.5, 0.85, 0.2 }, { 0.3, 0.7, 0.5 }, 24.0),
		NewMaterial({ 0.1, 0.5, 0.1 }, { 0.1, 1, 0.1 }, { 0.65, 0.65, 0.65 }, 8.0)
	};

	lights = {
		NewLight({ 5.5, 2.5, 20 }, { 0.7, 0.7, 0.7 }, { 1, 1, 1 }),
		NewLight({ -50, -10, 0 }, { 0, 0.7, 0 }, { 0, 1, 0 }),
		NewLight({ 50, -10, 0 }, { 0, 0, 0.7 }, { 0, 0, 1 }),
		//NewLight({ 0, 4, -50 }, { 1, 1, 1 }, { 1, 1, 1 }),
	};
}


void RenderObject(Shader& shader, const std::vector<Light>& lights, const Material& material, const Matrix44& model, Mesh& object)
{
	shader.setMatrix44("model", model); //upload the transform matrix to the shader
	shader.setVector3("ambient_material", material.ambient);
	shader.setVector3("diffuse_material", material.diffuse);
	shader.setVector3("specular_material", material.specular);
	shader.setFloat("shininess", material.shininess);

	bool firstLight = true;
	for (const Light& light : lights)
	{
		
		shader.setVector3("diffuse_light", light.diffuse_color);
		shader.setVector3("specular_light", light.specular_color);
		shader.setVector3("light_position", light.position);

		if (firstLight)
		{
			firstLight = false;
			glDisable(GL_BLEND);
			object.render(GL_TRIANGLES);
		}
		else
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			object.render(GL_TRIANGLES);
		}
	}

}

//render one frame
void Application::render(void)
{
	//update the aspect of the camera acording to the window size
	camera->aspect = window_width / window_height;
	camera->updateProjectionMatrix();
	//Get the viewprojection matrix from our camera
	Matrix44 viewprojection = camera->getViewProjectionMatrix();

	//set the clear color of the colorbuffer as the ambient light so it matches
	glClearColor(ambient_light.x, ambient_light.y, ambient_light.z, 1.0);

	// Clear the window and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear framebuffer and depth buffer 
	glEnable( GL_DEPTH_TEST ); //enable depth testing for occlusions
	glDepthFunc(GL_LEQUAL); //Z will pass if the Z is LESS or EQUAL to the Z of the pixel

	Shader* active_shader = *selected_shader;
	if (!active_shader)
		active_shader = phong_shader;

	//choose a shader and enable it
	active_shader->enable();
	active_shader->setMatrix44("viewprojection", viewprojection); //upload viewprojection info to the shader

	Matrix44 model_matrix;
	Material* material;

	for (int i = 0; i < 3; ++i)
	{
		model_matrix.setIdentity();
		model_matrix.rotate(angle, Vector3(0, 1, 0));
		switch (i)
		{
			default:
				model_matrix.translate(0, 0, 0);
				material = &materials[0];
				break;
			case 1:
				model_matrix.translate(-20, 0, -30);
				material = &materials[1];
				break;
			case 2:
				model_matrix.translate(20, 0, -40);
				material = &materials[2];
				break;
		}

		active_shader->setVector3("ambient_light", ambient_light);
		active_shader->setVector3("eye_position", camera->eye);

		RenderObject(*active_shader, lights, *material, model_matrix, *mesh);
	}

	//disable shader when we do not need it any more
	active_shader->disable();

	//swap between front buffer and back buffer
	SDL_GL_SwapWindow(this->window);
}

//called after render
void Application::update(double seconds_elapsed)
{
	if (keystate[SDL_SCANCODE_SPACE])
		angle += seconds_elapsed;

	if (keystate[SDL_SCANCODE_RIGHT])
		camera->eye = camera->eye + Vector3(1, 0, 0) * seconds_elapsed * 10.0;
	else if (keystate[SDL_SCANCODE_LEFT])
		camera->eye = camera->eye + Vector3(-1, 0, 0) * seconds_elapsed * 10.0;
	if (keystate[SDL_SCANCODE_UP])
		camera->eye = camera->eye + Vector3(0, 1, 0) * seconds_elapsed * 10.0;
	else if (keystate[SDL_SCANCODE_DOWN])
		camera->eye = camera->eye + Vector3(0, -1, 0) * seconds_elapsed * 10.0;

	if (keystate[SDL_SCANCODE_1])
		selected_shader = &gouraud_shader;
	else if (keystate[SDL_SCANCODE_2])
		selected_shader = &phong_shader;
}

//keyboard press event 
void Application::onKeyPressed( SDL_KeyboardEvent event )
{
	switch(event.keysym.sym)
	{
		case SDLK_ESCAPE: exit(0); break; //ESC key, kill the app
		case SDLK_r: 
			Shader::ReloadAll();
			break; //ESC key, kill the app
	}
}

//mouse button event
void Application::onMouseButtonDown( SDL_MouseButtonEvent event )
{
	if (event.button == SDL_BUTTON_LEFT) //left mouse pressed
	{
	}

}

void Application::onMouseButtonUp( SDL_MouseButtonEvent event )
{
	if (event.button == SDL_BUTTON_LEFT) //left mouse unpressed
	{

	}
}

//when the app starts
void Application::start()
{
	std::cout << "launching loop..." << std::endl;
	launchLoop(this);
}
