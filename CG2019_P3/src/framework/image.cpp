#include "image.h"


Image::Image() {
	width = 0; height = 0;
	pixels = NULL;
	raster = nullptr;
}

Image::Image(unsigned int width, unsigned int height)
{
	this->width = width;
	this->height = height;
	pixels = new Color[width*height];
	memset(pixels, 0, width * height * sizeof(Color));
	raster = nullptr;
}

//copy constructor
Image::Image(const Image& c) {
	pixels = NULL;

	width = c.width;
	height = c.height;
	if(c.pixels)
	{
		pixels = new Color[width*height];
		memcpy(pixels, c.pixels, width*height*sizeof(Color));
	}
	if (c.raster)
	{
		raster = new RasterInfo[width * height];
		std::memcpy(raster, c.raster, sizeof(RasterInfo) * height);
	}
	else raster = nullptr;
}

//assign operator
Image& Image::operator = (const Image& c)
{
	if(pixels) delete pixels;
	pixels = NULL;

	width = c.width;
	height = c.height;
	if(c.pixels)
	{
		pixels = new Color[width*height*sizeof(Color)];
		memcpy(pixels, c.pixels, width*height*sizeof(Color));
	}
	if (c.raster)
	{
		raster = new RasterInfo[width * height];
		std::memcpy(raster, c.raster, sizeof(RasterInfo) * height);
	}
	else raster = nullptr;
	return *this;
}

Image::~Image()
{
	if(pixels) 
		delete pixels;
	if (raster)
		delete raster;
}



//change image size (the old one will remain in the top-left corner)
void Image::resize(unsigned int width, unsigned int height)
{
	Color* new_pixels = new Color[width*height];
	unsigned int min_width = this->width > width ? width : this->width;
	unsigned int min_height = this->height > height ? height : this->height;

	for(unsigned int x = 0; x < min_width; ++x)
		for(unsigned int y = 0; y < min_height; ++y)
			new_pixels[ y * width + x ] = getPixel(x,y);

	delete pixels;
	this->width = width;
	this->height = height;
	pixels = new_pixels;

	if (raster)
	{
		delete raster;
		raster = nullptr;
	}
}

//change image size and scale the content
void Image::scale(unsigned int width, unsigned int height)
{
	Color* new_pixels = new Color[width*height];

	for(unsigned int x = 0; x < width; ++x)
		for(unsigned int y = 0; y < height; ++y)
			new_pixels[ y * width + x ] = getPixel((unsigned int)(this->width * (x / (float)width)), (unsigned int)(this->height * (y / (float)height)) );

	delete pixels;
	this->width = width;
	this->height = height;
	pixels = new_pixels;
}

Image Image::getArea(unsigned int start_x, unsigned int start_y, unsigned int width, unsigned int height)
{
	Image result(width, height);
	for(unsigned int x = 0; x < width; ++x)
		for(unsigned int y = 0; y < height; ++x)
		{
			if( (x + start_x) < this->width && (y + start_y) < this->height) 
				result.setPixel( x, y, getPixel(x + start_x,y + start_y) );
		}
	return result;
}

void Image::flipX()
{
	for(unsigned int x = 0; x < width * 0.5; ++x)
		for(unsigned int y = 0; y < height; ++y)
		{
			Color temp = getPixel(width - x - 1, y);
			setPixel( width - x - 1, y, getPixel(x,y));
			setPixel( x, y, temp );
		}
}

void Image::flipY()
{
	for(unsigned int x = 0; x < width; ++x)
		for(unsigned int y = 0; y < height * 0.5; ++y)
		{
			Color temp = getPixel(x, height - y - 1);
			setPixel( x, height - y - 1, getPixel(x,y) );
			setPixel( x, y, temp );
		}
}


//Loads an image from a TGA file
bool Image::loadTGA(const char* filename)
{
	unsigned char TGAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	unsigned char TGAcompare[12];
	unsigned char header[6];
	unsigned int bytesPerPixel;
	unsigned int imageSize;

	FILE * file = fopen(filename, "rb");
   	if ( file == NULL || fread(TGAcompare, 1, sizeof(TGAcompare), file) != sizeof(TGAcompare) ||
		memcmp(TGAheader, TGAcompare, sizeof(TGAheader)) != 0 ||
		fread(header, 1, sizeof(header), file) != sizeof(header))
	{
		std::cerr << "File not found: " << filename << std::endl;
		if (file == NULL)
			return NULL;
		else
		{
			fclose(file);
			return NULL;
		}
	}

	TGAInfo* tgainfo = new TGAInfo;
    
	tgainfo->width = header[1] * 256 + header[0];
	tgainfo->height = header[3] * 256 + header[2];
    
	if (tgainfo->width <= 0 || tgainfo->height <= 0 || (header[4] != 24 && header[4] != 32))
	{
		std::cerr << "TGA file seems to have errors or it is compressed, only uncompressed TGAs supported" << std::endl;
		fclose(file);
		delete tgainfo;
		return NULL;
	}
    
	tgainfo->bpp = header[4];
	bytesPerPixel = tgainfo->bpp / 8;
	imageSize = tgainfo->width * tgainfo->height * bytesPerPixel;
    
	tgainfo->data = new unsigned char[imageSize];
    
	if (tgainfo->data == NULL || fread(tgainfo->data, 1, imageSize, file) != imageSize)
	{
		if (tgainfo->data != NULL)
			delete tgainfo->data;
            
		fclose(file);
		delete tgainfo;
		return false;
	}

	fclose(file);

	//save info in image
	if(pixels)
		delete pixels;

	width = tgainfo->width;
	height = tgainfo->height;
	pixels = new Color[width*height];

	//convert to float all pixels
	for(unsigned int y = 0; y < height; ++y)
		for(unsigned int x = 0; x < width; ++x)
		{
			unsigned int pos = y * width * bytesPerPixel + x * bytesPerPixel;
			this->setPixel(x , height - y - 1, Color( tgainfo->data[pos+2], tgainfo->data[pos+1], tgainfo->data[pos]) );
		}

	delete tgainfo->data;
	delete tgainfo;

	return true;
}

// Saves the image to a TGA file
bool Image::saveTGA(const char* filename)
{
	unsigned char TGAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	FILE *file = fopen(filename, "wb");
	if ( file == NULL )
	{
		fclose(file);
		return false;
	}

	unsigned short header_short[3];
	header_short[0] = width;
	header_short[1] = height;
	unsigned char* header = (unsigned char*)header_short;
	header[4] = 24;
	header[5] = 0;

	//tgainfo->width = header[1] * 256 + header[0];
	//tgainfo->height = header[3] * 256 + header[2];

	fwrite(TGAheader, 1, sizeof(TGAheader), file);
	fwrite(header, 1, 6, file);

	//convert pixels to unsigned char
	unsigned char* bytes = new unsigned char[width*height*3];
	for(unsigned int y = 0; y < height; ++y)
		for(unsigned int x = 0; x < width; ++x)
		{
			Color c = pixels[(height-y-1)*width+x];
			unsigned int pos = (y*width+x)*3;
			bytes[pos+2] = c.r;
			bytes[pos+1] = c.g;
			bytes[pos] = c.b;
		}

	fwrite(bytes, 1, width*height*3, file);
	fclose(file);
	return true;
}

/* My stuff */
void Image::_clearRaster()
{
	if (!raster)
		raster = new RasterInfo[height];
	for (int i = 0; i < height; ++i)
	{
		raster[i].min = static_cast<unsigned int>(-1);
		raster[i].max = static_cast<unsigned int>(0);
	}
	//std::memset(raster, 0, sizeof(RasterInfo) * height);
}

void Image::_rasterTriangleLine(int x0, int y0, int x1, int y1)
{
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2, e2;

	for (;;) {
		
		if (y0 >= 0 && y0 < height && x0 > 0 && x0 < width)
		{
			if (x0 < raster[y0].min)
				raster[y0].min = x0;
			if (x0 > raster[y0].max)
				raster[y0].max = x0;
		}

		if (x0 == x1 && y0 == y1) break;
		e2 = err;
		if (e2 > -dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}
}

void Image::drawLine(int x0, int y0, int x1, int y1, const Color& color)
{
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2, e2;

	for (;;) {

		if(y0 >= 0 && y0 < height && x0 > 0 && x0 < width)
			pixels[y0 * width + x0] = color;

		if (x0 == x1 && y0 == y1) break;
		e2 = err;
		if (e2 > -dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}
}

void Image::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, const Color& color)
{
	// raster part //
	_clearRaster();

	_rasterTriangleLine(x0, y0, x1, y1);
	_rasterTriangleLine(x0, y0, x2, y2);
	_rasterTriangleLine(x1, y1, x2, y2);

	// fill part //
	for (unsigned int y = 0; y < height; ++y)
	{
		if (raster[y].min < raster[y].max)
		{
			unsigned int max = raster[y].max;
			for (unsigned int x = raster[y].min; x < max; ++x)
				pixels[y * width + x] = color;
		}
	}
}

Vector3 Image::_weights(int x, int y, const Vector2& p0, const Vector2& p1, const Vector2& p2)
{
	Vector2 v0 = p1 - p0;
	Vector2 v1 = p2 - p0;
	Vector2 v2 = Vector2{ static_cast<float>(x), static_cast<float>(y) } -p0;

	float d00 = v0.dot(v0);
	float d01 = v0.dot(v1);
	float d11 = v1.dot(v1);
	float d20 = v2.dot(v0);
	float d21 = v2.dot(v1);
	float denom = d00 * d11 - d01 * d01;
	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0 - v - w;

	return { u, v, w };
}

Color Image::_interpolatedColor(
	int x, int y,
	const Vector2& p0, const Vector2& p1, const Vector2& p2,
	const Color& c0, const Color& c1, const Color& c2
)
{
	Vector3 w = _weights(x, y, p0, p1, p2);

	//use weights to compute final color
	return c0 * w.x + c1 * w.y + c2 * w.z;
}

void Image::fillInterpolatedTriangle(int x0, int y0, int x1, int y1, int x2, int y2, const Color& c0, const Color& c1, const Color& c2)
{
	// raster part //
	_clearRaster();

	_rasterTriangleLine(x0, y0, x1, y1);
	_rasterTriangleLine(x0, y0, x2, y2);
	_rasterTriangleLine(x1, y1, x2, y2);

	// interpoalted fill part //
	const Vector2 p0{ static_cast<float>(x0), static_cast<float>(y0) };
	const Vector2 p1{ static_cast<float>(x1), static_cast<float>(y1) };
	const Vector2 p2{ static_cast<float>(x2), static_cast<float>(y2) };

	for (unsigned int y = 0; y < height; ++y)
	{
		if (raster[y].min < raster[y].max)
		{
			unsigned int max = raster[y].max;
			for (unsigned int x = raster[y].min; x < max; ++x)
				pixels[y * width + x] = _interpolatedColor(x, y, p0, p1, p2, c0, c1, c2);
		}
	}
}



void Image::fillTriangle(FloatImage* z_buffer, const Vector3& v0, const Vector3& v1, const Vector3& v2, const Color& color)
{
	// raster part //
	_clearRaster();

	_rasterTriangleLine(static_cast<int>(v0.x), static_cast<int>(v0.y), static_cast<int>(v1.x), static_cast<int>(v1.y));
	_rasterTriangleLine(static_cast<int>(v0.x), static_cast<int>(v0.y), static_cast<int>(v2.x), static_cast<int>(v2.y));
	_rasterTriangleLine(static_cast<int>(v1.x), static_cast<int>(v1.y), static_cast<int>(v2.x), static_cast<int>(v2.y));

	// fill part //
	const Vector2 p0{ v0.x, v0.y };
	const Vector2 p1{ v1.x, v1.y };
	const Vector2 p2{ v2.x, v2.y };

	for (unsigned int y = 0; y < height; ++y)
	{
		if (raster[y].min < raster[y].max)
		{
			unsigned int max = raster[y].max;
			for (unsigned int x = raster[y].min; x < max; ++x)
			{
				Vector3 w = _weights(x, y, p0, p1, p2);
				float depth = v0.z * w.x + v1.z * w.y + v2.z * w.z;
				float& z_buffer_depth = z_buffer->getPixelRef(x, y);
				if (depth < z_buffer_depth)
				{
					z_buffer_depth = depth;
					pixels[y * width + x] = color;
				}
			}
		}
	}
}

void Image::fillInterpolatedTriangle(FloatImage* z_buffer, const Vector3& v0, const Vector3& v1, const Vector3& v2, const Color& c0, const Color& c1, const Color& c2)
{
	// raster part //
	_clearRaster();

	_rasterTriangleLine(static_cast<int>(v0.x), static_cast<int>(v0.y), static_cast<int>(v1.x), static_cast<int>(v1.y));
	_rasterTriangleLine(static_cast<int>(v0.x), static_cast<int>(v0.y), static_cast<int>(v2.x), static_cast<int>(v2.y));
	_rasterTriangleLine(static_cast<int>(v1.x), static_cast<int>(v1.y), static_cast<int>(v2.x), static_cast<int>(v2.y));

	// fill part //
	const Vector2 p0{ v0.x, v0.y };
	const Vector2 p1{ v1.x, v1.y };
	const Vector2 p2{ v2.x, v2.y };

	for (unsigned int y = 0; y < height; ++y)
	{
		if (raster[y].min < raster[y].max)
		{
			unsigned int max = raster[y].max;
			for (unsigned int x = raster[y].min; x < max; ++x)
			{
				Vector3 w = _weights(x, y, p0, p1, p2);
				float depth = v0.z * w.x + v1.z * w.y + v2.z * w.z;
				float& z_buffer_depth = z_buffer->getPixelRef(x, y);
				if (depth < z_buffer_depth)
				{
					z_buffer_depth = depth;
					pixels[y * width + x] = c0 * w.x + c1 * w.y + c2 * w.z;
				}
			}
		}
	}
}


//Transform coord in clip space to screen space//
#define clipToScreen(_Coord, _Size) (((_Coord) + 1.f) * ((_Size) / 2.f))
#define vector2ClipToScreen(_V, _Width, _Height) (_V).x = clipToScreen((_V).x, (_Width)); (_V).y = clipToScreen((_V).y, (_Height))
void Image::fillTexturedTriangle(
	FloatImage* z_buffer,
	const Image* texture,
	const Vector3& v0, const Vector3& v1, const Vector3& v2,
	Vector2 t0, Vector2 t1, Vector2 t2
)
{
	// raster part //
	_clearRaster();

	_rasterTriangleLine(static_cast<int>(v0.x), static_cast<int>(v0.y), static_cast<int>(v1.x), static_cast<int>(v1.y));
	_rasterTriangleLine(static_cast<int>(v0.x), static_cast<int>(v0.y), static_cast<int>(v2.x), static_cast<int>(v2.y));
	_rasterTriangleLine(static_cast<int>(v1.x), static_cast<int>(v1.y), static_cast<int>(v2.x), static_cast<int>(v2.y));

	// fill part //
	const Vector2 p0{ v0.x, v0.y };
	const Vector2 p1{ v1.x, v1.y };
	const Vector2 p2{ v2.x, v2.y };

	vector2ClipToScreen(t0, texture->width, texture->height);
	vector2ClipToScreen(t1, texture->width, texture->height);
	vector2ClipToScreen(t2, texture->width, texture->height);

	for (unsigned int y = 0; y < height; ++y)
	{
		if (raster[y].min < raster[y].max)
		{
			unsigned int max = raster[y].max;
			for (unsigned int x = raster[y].min; x < max; ++x)
			{
				Vector3 w = _weights(x, y, p0, p1, p2);
				if(w.x > 100)
					w = _weights(x, y, p0, p1, p2);
				float depth = v0.z * w.x + v1.z * w.y + v2.z * w.z;
				float& z_buffer_depth = z_buffer->getPixelRef(x, y);
				if (depth < z_buffer_depth)
				{
					z_buffer_depth = depth;
					pixels[y * width + x] = texture->getPixel(
						static_cast<unsigned int>(t0.x * w.x + t1.x * w.y + t2.x * w.z),
						static_cast<unsigned int>(t0.y * w.x + t1.y * w.y + t2.y * w.z)
					);
				}
			}
		}
	}
}






FloatImage::FloatImage(unsigned int width, unsigned int height)
{
	this->width = width;
	this->height = height;
	pixels = new float[width*height];
	memset(pixels, 0, width * height * sizeof(float));
}

//copy constructor
FloatImage::FloatImage(const FloatImage& c) {
	pixels = NULL;

	width = c.width;
	height = c.height;
	if (c.pixels)
	{
		pixels = new float[width*height];
		memcpy(pixels, c.pixels, width*height * sizeof(float));
	}
}

//assign operator
FloatImage& FloatImage::operator = (const FloatImage& c)
{
	if (pixels) delete pixels;
	pixels = NULL;

	width = c.width;
	height = c.height;
	if (c.pixels)
	{
		pixels = new float[width*height * sizeof(float)];
		memcpy(pixels, c.pixels, width*height * sizeof(float));
	}
	return *this;
}

FloatImage::~FloatImage()
{
	if (pixels)
		delete pixels;
}


//change image size (the old one will remain in the top-left corner)
void FloatImage::resize(unsigned int width, unsigned int height)
{
	float* new_pixels = new float[width*height];
	unsigned int min_width = this->width > width ? width : this->width;
	unsigned int min_height = this->height > height ? height : this->height;

	for (unsigned int x = 0; x < min_width; ++x)
		for (unsigned int y = 0; y < min_height; ++y)
			new_pixels[y * width + x] = getPixel(x, y);

	delete pixels;
	this->width = width;
	this->height = height;
	pixels = new_pixels;
}


#ifndef IGNORE_LAMBDAS

//you can apply and algorithm for two images and store the result in the first one
//forEachPixel( img, img2, [](Color a, Color b) { return a + b; } );
template <typename F>
void forEachPixel(Image& img, const Image& img2, F f) {
	for(unsigned int pos = 0; pos < img.width * img.height; ++pos)
		img.pixels[pos] = f( img.pixels[pos], img2.pixels[pos] );
}

#endif