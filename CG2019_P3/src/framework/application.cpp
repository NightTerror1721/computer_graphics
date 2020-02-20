#include "application.h"
#include "utils.h"
#include "image.h"
#include "mesh.h"

Mesh* mesh = NULL;
Camera* camera = NULL;
Image* texture = NULL;

FloatImage* z_buffer = nullptr;

Mesh* cube = nullptr;

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
	camera->lookAt(Vector3(0, 10, 20), Vector3(0, 10, 0), Vector3(0, 1, 0)); //define eye,center,up
	camera->perspective(60, window_width / (float)window_height, 0.1, 10000); //define fov,aspect,near,far

	//load a mesh
	mesh = new Mesh();
	if (!mesh->loadOBJ("lee.obj"))
		std::cout << "FILE Lee.obj NOT FOUND" << std::endl;

	//load the texture
	texture = new Image();
	texture->loadTGA("color.tga");

	//Init zbuffer
	z_buffer = new FloatImage{ framebuffer.width, framebuffer.height };


	/* Drag input init */

	_rotationDragEnabled = false;
	_traslationDragEnabled = false;
	_dragMouseOrigin = {};
	_dragEyeOrigin = {};
	_dragCenterOrigin = {};
}

//this function fills the triangle by computing the bounding box of the triangle in screen space and using the barycentric interpolation
//to check which pixels are inside the triangle. It is slow for big triangles, but faster for small triangles
void fillTriangle(Image& colorbuffer, const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector2& uv0, const Vector2& uv1, const Vector2& uv2, Image* texture = NULL, FloatImage* zbuffer = NULL)
{
	//compute triangle bounding box in screen space
	Vector3 min_, max_;
	computeMinMax(p0, p1, p2, min_, max_);
	//clamp to screen area
	min_ = clamp(min_, Vector3(0, 0, -1), Vector3(colorbuffer.width - 1, colorbuffer.height - 1, 1));
	max_ = clamp(max_, Vector3(0, 0, -1), Vector3(colorbuffer.width - 1, colorbuffer.height - 1, 1));

	//this avoids strange artifacts if the triangle is too big, just ignore this line
	if ((min_.x == 0.0 && max_.x == colorbuffer.width - 1) || (min_.y == 0.0 && max_.y == colorbuffer.height - 1))
		return;

	//we must compute the barycentrinc interpolation coefficients
	//we precompute some of them outside of loop to speed up (because they are constant)
	Vector3 v0 = p1 - p0;
	Vector3 v1 = p2 - p0;
	float d00 = v0.dot(v0);
	float d01 = v0.dot(v1);
	float d11 = v1.dot(v1);
	float denom = d00 * d11 - d01 * d01;

	//loop all pixels inside bounding box
	for (int x = min_.x; x < max_.x; ++x)
	{
#pragma omp parallel for //HACK: this is to execute loop iterations in parallel in multiple cores, should go faster (search openmp in google for more info)
		for (int y = min_.y; y < max_.y; ++y)
		{
			Vector3 P(x, y, 0);
			Vector3 v2 = P - p0; //P is the x,y of the pixel

			//computing all weights of pixel P(x,y)
			float d20 = v2.dot(v0);
			float d21 = v2.dot(v1);
			float v = (d11 * d20 - d01 * d21) / denom;
			float w = (d00 * d21 - d01 * d20) / denom;
			float u = 1.0 - v - w;
			//check if pixel is inside or outside the triangle
			if (u < 0 || u > 1 || v < 0 || v > 1 || w < 0 || w > 1 || denom == 0)
				continue; //if it is outside, skip to next

			//here add your code to test occlusions based on the Z of the vertices and the pixel
			float depth = p0.z * u + p1.z * v + p2.z * w;
			float& zbuf_depth = zbuffer->getPixelRef(x, y);
			if (depth >= zbuf_depth)
				continue;
			zbuf_depth = depth;

			//here add your code to compute the color of the pixel
			unsigned int tx = static_cast<unsigned int>((uv0.x * u + uv1.x * v + uv2.x * w) * texture->width);
			unsigned int ty = static_cast<unsigned int>((uv0.y * u + uv1.y * v + uv2.y * w) * texture->height);
			const Color& tcolor = texture->getPixelRef(tx, ty);

			//draw the pixels in the colorbuffer x,y position
			colorbuffer.setPixel(x, y, tcolor);
		}
	}
}

#define isOutOfClip(_Point) ((_Point).x < -1 || (_Point).x > 1 || (_Point).y < -1 || (_Point).y > 1)

//render one frame
void Application::render(Image& framebuffer)
{
	framebuffer.fill(Color(40, 45, 60 )); //clear

	z_buffer->fill(FLT_MAX); //fill with maximum float value


	//for every point of the mesh (to draw triangles take three points each time and connect the points between them (1,2,3,   4,5,6,   ... )
	for (int i = 0; i < mesh->vertices.size(); i += 3)
	{
		Vector3 p0 = camera->projectVector(mesh->vertices[i]);
		Vector3 p1 = camera->projectVector(mesh->vertices[i + 1]);
		Vector3 p2 = camera->projectVector(mesh->vertices[i + 2]);

		if (isOutOfClip(p0) && isOutOfClip(p1) && isOutOfClip(p2))
			continue;

		//convert from normalized (-1 to +1) to framebuffer coordinates (0,W)
		p0.x = (p0.x + 1.f) * window_width / 2.f;
		p0.y = (p0.y + 1.f) * window_height / 2.f;

		p1.x = (p1.x + 1.f) * window_width / 2.f;
		p1.y = (p1.y + 1.f) * window_height / 2.f;

		p2.x = (p2.x + 1.f) * window_width / 2.f;
		p2.y = (p2.y + 1.f) * window_height / 2.f;

		Vector2 uv0 = mesh->uvs[i];
		Vector2 uv1 = mesh->uvs[i + 1];
		Vector2 uv2 = mesh->uvs[i + 2];

		fillTriangle(framebuffer, p0, p1, p2, uv0, uv1, uv2, texture, z_buffer);
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


	/* Mouse Drag update */
	if (_rotationDragEnabled)
	{
		float dx = (_dragMouseOrigin.x - mouse_position.x) / window_width / 2 * 3.f;
		float dy = (_dragMouseOrigin.y - mouse_position.y) / window_height / 2 * 3.f;

		Vector3 front = (camera->center - camera->eye).normalize();
		Vector3 side = front.cross(camera->up).normalize();
		Vector3 top = side.cross(front);

		Matrix44 rot_x, rot_y;
		rot_x.rotateLocal(dx, top);
		rot_y.rotateLocal(dy, side);

		Matrix44 rot = rot_x * rot_y;

		Vector3 c_pos = _dragCenterOrigin - _dragEyeOrigin;
		camera->center = rot.rotateVector(c_pos) + _dragEyeOrigin;
	}
	else if (_traslationDragEnabled)
	{
		float dx = (_dragMouseOrigin.x - mouse_position.x) / window_width / 2 * 20.f;
		float dy = (_dragMouseOrigin.y - mouse_position.y) / window_height / 2 * 20.f;

		Vector3 front = (camera->center - camera->eye).normalize();
		Vector3 side = front.cross(camera->up).normalize();
		Vector3 top = side.cross(front);

		Matrix44 rot = camera->view_matrix.getRotationOnly();
		Vector3 vtr = Vector3{ -dy, dx, 0 }.cross(front);

		camera->eye = _dragEyeOrigin + vtr;
		camera->center = _dragCenterOrigin + vtr;
	}


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
		_traslationDragEnabled = true;
		_dragMouseOrigin.set(static_cast<float>(event.x), window_height - static_cast<float>(event.y));
		_dragEyeOrigin = camera->eye;
		_dragCenterOrigin = camera->center;
	}
	if (event.button == SDL_BUTTON_RIGHT) //right mouse pressed
	{
		_rotationDragEnabled = true;
		_dragMouseOrigin.set(static_cast<float>(event.x), window_height - static_cast<float>(event.y));
		_dragEyeOrigin = camera->eye;
		_dragCenterOrigin = camera->center;
	}
}

void Application::onMouseButtonUp( SDL_MouseButtonEvent event )
{
	if (event.button == SDL_BUTTON_LEFT) //left mouse unpressed
	{
		_traslationDragEnabled = false;
	}
	if (event.button == SDL_BUTTON_RIGHT) //right mouse unpressed
	{
		_rotationDragEnabled = false;
	}
}

//when the app starts
void Application::start()
{
	std::cout << "launching loop..." << std::endl;
	launchLoop(this);
}
