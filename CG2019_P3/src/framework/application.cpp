#include "application.h"
#include "utils.h"
#include "image.h"
#include "mesh.h"

Mesh* mesh = NULL;
Camera* camera = NULL;
Image* texture = NULL;

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

	framebuffer.resize(w, h);
}

//Here we have already GL working, so we can create meshes and textures
//Here we have already GL working, so we can create meshes and textures
void Application::init(void)
{
	std::cout << "initiating app..." << std::endl;
	
	//here we create a global camera and set a position and projection properties
	camera = new Camera();
	camera->lookAt(Vector3(0,10,20),Vector3(0,10,0),Vector3(0,1,0)); //define eye,center,up
	camera->perspective(60, window_width / (float)window_height, 0.1, 10000); //define fov,aspect,near,far

	//load a mesh
	mesh = new Mesh();
	if( !mesh->loadOBJ("lee.obj") )
		std::cout << "FILE Lee.obj NOT FOUND" << std::endl;

	//load the texture
	texture = new Image();
	texture->loadTGA("color.tga");
}

//render one frame
void Application::render(Image& framebuffer)
{
	framebuffer.fill(Color(40, 45, 60 )); //clear

	//for every point of the mesh (to draw triangles take three points each time and connect the points between them (1,2,3,   4,5,6,   ... )
	for (int i = 0; i < mesh->vertices.size(); ++i)
	{
		Vector3 vertex = mesh->vertices[i]; //extract vertex from mesh
		Vector2 texcoord = mesh->uvs[i]; //texture coordinate of the vertex (they are normalized, from 0,0 to 1,1)

		//project every point in the mesh to normalized coordinates using the viewprojection_matrix inside camera
		//Vector3 normalized_point = camera->projectVector( vertex );

		//convert from normalized (-1 to +1) to framebuffer coordinates (0,W)
		//...

		//paint point in framebuffer (using setPixel or drawTriangle)
		//...
	}
}

//called after render
void Application::update(double seconds_elapsed)
{
	if (keystate[SDL_SCANCODE_SPACE])
	{
		//...
	}

	//example to move eye
	if (keystate[SDL_SCANCODE_LEFT])
		camera->eye.x -= 5 * seconds_elapsed;
	if (keystate[SDL_SCANCODE_RIGHT])
		camera->eye.x += 5 * seconds_elapsed;

	//if we modify the camera fields, then update matrices
	camera->updateViewMatrix();
	camera->updateProjectionMatrix();
}

//keyboard press event 
void Application::onKeyDown( SDL_KeyboardEvent event )
{
	switch(event.keysym.sym)
	{
		case SDLK_ESCAPE: exit(0); break; //ESC key, kill the app
	}
}

//keyboard released event 
void Application::onKeyUp(SDL_KeyboardEvent event)
{
	switch (event.keysym.sym)
	{
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
