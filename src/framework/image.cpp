#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "GL/glew.h"
#include "../extra/picopng.h"
#include "image.h"
#include "utils.h"
#include "camera.h"
#include "mesh.h"


Image::Image() {
	width = 0; height = 0;
	pixels = NULL;
}

Image::Image(unsigned int width, unsigned int height)
{
	this->width = width;
	this->height = height;
	pixels = new Color[width*height];
	memset(pixels, 0, width * height * sizeof(Color));
}

// Copy constructor
Image::Image(const Image& c)
{
	pixels = NULL;
	width = c.width;
	height = c.height;
	bytes_per_pixel = c.bytes_per_pixel;
	if(c.pixels)
	{
		pixels = new Color[width*height];
		memcpy(pixels, c.pixels, width*height*bytes_per_pixel);
	}
}

// Assign operator
Image& Image::operator = (const Image& c)
{
	if(pixels) delete pixels;
	pixels = NULL;

	width = c.width;
	height = c.height;
	bytes_per_pixel = c.bytes_per_pixel;

	if(c.pixels)
	{
		pixels = new Color[width*height*bytes_per_pixel];
		memcpy(pixels, c.pixels, width*height*bytes_per_pixel);
	}
	return *this;
}

Image::~Image()
{
	if(pixels) 
		delete pixels;
}

void Image::Render()
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glDrawPixels(width, height, bytes_per_pixel == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

// Change image size (the old one will remain in the top-left corner)
void Image::Resize(unsigned int width, unsigned int height)
{
	Color* new_pixels = new Color[width*height];
	unsigned int min_width = this->width > width ? width : this->width;
	unsigned int min_height = this->height > height ? height : this->height;

	for(unsigned int x = 0; x < min_width; ++x)
		for(unsigned int y = 0; y < min_height; ++y)
			new_pixels[ y * width + x ] = GetPixel(x,y);

	delete pixels;
	this->width = width;
	this->height = height;
	pixels = new_pixels;
}

// Change image size and scale the content
void Image::Scale(unsigned int width, unsigned int height)
{
	Color* new_pixels = new Color[width*height];

	for(unsigned int x = 0; x < width; ++x)
		for(unsigned int y = 0; y < height; ++y)
			new_pixels[ y * width + x ] = GetPixel((unsigned int)(this->width * (x / (float)width)), (unsigned int)(this->height * (y / (float)height)) );

	delete pixels;
	this->width = width;
	this->height = height;
	pixels = new_pixels;
}

Image Image::GetArea(unsigned int start_x, unsigned int start_y, unsigned int width, unsigned int height)
{
	Image result(width, height);
	for(unsigned int x = 0; x < width; ++x)
		for(unsigned int y = 0; y < height; ++y)
		{
			if( (x + start_x) < this->width && (y + start_y) < this->height) 
				result.SetPixelUnsafe( x, y, GetPixel(x + start_x,y + start_y) );
		}
	return result;
}

void Image::FlipY()
{
	int row_size = bytes_per_pixel * width;
	Uint8* temp_row = new Uint8[row_size];
#pragma omp simd
	for (int y = 0; y < height * 0.5; y += 1)
	{
		Uint8* pos = (Uint8*)pixels + y * row_size;
		memcpy(temp_row, pos, row_size);
		Uint8* pos2 = (Uint8*)pixels + (height - y - 1) * row_size;
		memcpy(pos, pos2, row_size);
		memcpy(pos2, temp_row, row_size);
	}
	delete[] temp_row;
}

bool Image::LoadPNG(const char* filename, bool flip_y)
{
	std::string sfullPath = absResPath(filename);
	std::ifstream file(sfullPath, std::ios::in | std::ios::binary | std::ios::ate);

	// Get filesize
	std::streamsize size = 0;
	if (file.seekg(0, std::ios::end).good()) size = file.tellg();
	if (file.seekg(0, std::ios::beg).good()) size -= file.tellg();

	if (!size)
		return false;

	std::vector<unsigned char> buffer;

	// Read contents of the file into the vector
	if (size > 0)
	{
		buffer.resize((size_t)size);
		file.read((char*)(&buffer[0]), size);
	}
	else
		buffer.clear();

	std::vector<unsigned char> out_image;

	if (decodePNG(out_image, width, height, buffer.empty() ? 0 : &buffer[0], (unsigned long)buffer.size(), true) != 0)
		return false;

	size_t bufferSize = out_image.size();
	unsigned int originalBytesPerPixel = (unsigned int)bufferSize / (width * height);
	
	// Force 3 channels
	bytes_per_pixel = 3;

	if (originalBytesPerPixel == 3) {
		pixels = new Color[bufferSize];
		memcpy(pixels, &out_image[0], bufferSize);
	}
	else if (originalBytesPerPixel == 4) {

		unsigned int newBufferSize = width * height * bytes_per_pixel;
		pixels = new Color[newBufferSize];

		unsigned int k = 0;
		for (unsigned int i = 0; i < bufferSize; i += originalBytesPerPixel) {
			pixels[k] = Color(out_image[i], out_image[i + 1], out_image[i + 2]);
			k++;
		}
	}

	// Flip pixels in Y
	if (flip_y)
		FlipY();

	return true;
}

// Loads an image from a TGA file
bool Image::LoadTGA(const char* filename, bool flip_y)
{
	unsigned char TGAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	unsigned char TGAcompare[12];
	unsigned char header[6];
	unsigned int imageSize;
	unsigned int bytesPerPixel;

    std::string sfullPath = absResPath( filename );

	FILE * file = fopen( sfullPath.c_str(), "rb");
   	if ( file == NULL || fread(TGAcompare, 1, sizeof(TGAcompare), file) != sizeof(TGAcompare) ||
		memcmp(TGAheader, TGAcompare, sizeof(TGAheader)) != 0 ||
		fread(header, 1, sizeof(header), file) != sizeof(header))
	{
		std::cerr << "File not found: " << sfullPath.c_str() << std::endl;
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

	// Save info in image
	if(pixels)
		delete pixels;

	width = tgainfo->width;
	height = tgainfo->height;
	pixels = new Color[width*height];

	// Convert to float all pixels
	for (unsigned int y = 0; y < height; ++y) {
		for (unsigned int x = 0; x < width; ++x) {
			unsigned int pos = y * width * bytesPerPixel + x * bytesPerPixel;
			// Make sure we don't access out of memory
			if( (pos < imageSize) && (pos + 1 < imageSize) && (pos + 2 < imageSize))
				SetPixelUnsafe(x, height - y - 1, Color(tgainfo->data[pos + 2], tgainfo->data[pos + 1], tgainfo->data[pos]));
		}
	}

	// Flip pixels in Y
	if (flip_y)
		FlipY();

	delete tgainfo->data;
	delete tgainfo;

	return true;
}

// Saves the image to a TGA file
bool Image::SaveTGA(const char* filename)
{
	unsigned char TGAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	std::string fullPath = absResPath(filename);
	FILE *file = fopen(fullPath.c_str(), "wb");
	if ( file == NULL )
	{
		perror("Failed to open file: ");
		return false;
	}

	unsigned short header_short[3];
	header_short[0] = width;
	header_short[1] = height;
	unsigned char* header = (unsigned char*)header_short;
	header[4] = 24;
	header[5] = 0;

	fwrite(TGAheader, 1, sizeof(TGAheader), file);
	fwrite(header, 1, 6, file);

	// Convert pixels to unsigned char
	unsigned char* bytes = new unsigned char[width*height*3];
	for(unsigned int y = 0; y < height; ++y)
		for(unsigned int x = 0; x < width; ++x)
		{
			Color c = pixels[y*width+x];
			unsigned int pos = (y*width+x)*3;
			bytes[pos+2] = c.r;
			bytes[pos+1] = c.g;
			bytes[pos] = c.b;
		}

	fwrite(bytes, 1, width*height*3, file);
	fclose(file);

	return true;
}

void Image::DrawRect(int x, int y, int w, int h, const Color& borderColor, int borderWidth, bool fillShapes, const Color& fillColor) {
	// Draw the filled rectangle if requested
	if (fillShapes) {
		for (int i = 0; i < h; ++i) {
			for (int j = 0; j < w; ++j) {
				int px = x + j;
				int py = y + i;
				if (px >= 0 && px < (int)width && py >= 0 && py < (int)height) {
					SetPixelUnsafe(px, py, fillColor);
				}
			}
		}
	}

	// Draw the rectangle border with the specified width
	for (int bw = 0; bw < borderWidth; ++bw) {
		// Top and Bottom borders
		for (int i = 0; i < w; ++i) {
			int topPx = x + i;
			int topPy = y + bw;
			int bottomPx = x + i;
			int bottomPy = y + h - 1 - bw;

			if (topPx >= 0 && topPx < (int)width && topPy >= 0 && topPy < (int)height) {
				SetPixelUnsafe(topPx, topPy, borderColor); // Top border
			}
			if (bottomPx >= 0 && bottomPx < (int)width && bottomPy >= 0 && bottomPy < (int)height) {
				SetPixelUnsafe(bottomPx, bottomPy, borderColor); // Bottom border
			}
		}

		// Left and Right borders
		for (int i = 0; i < h; ++i) {
			int leftPx = x + bw;
			int leftPy = y + i;
			int rightPx = x + w - 1 - bw;
			int rightPy = y + i;

			if (leftPx >= 0 && leftPx < (int)width && leftPy >= 0 && leftPy < (int)height) {
				SetPixelUnsafe(leftPx, leftPy, borderColor); // Left border
			}
			if (rightPx >= 0 && rightPx < (int)width && rightPy >= 0 && rightPy < (int)height) {
				SetPixelUnsafe(rightPx, rightPy, borderColor); // Right border
			}
		}
	}
}




void Image::ScanLineDDA(int x0, int y0, int x1, int y1, int& xIntersect, int y) {
	if (y1 == y0) return; // Prevent division by zero

	float slope = float(x1 - x0) / float(y1 - y0);  // Calculate the slope

	// Calculate the intersection point on the scanline at y
	xIntersect = x0 + int(slope * (y - y0));
}

void Image::DrawTriangle(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Color& borderColor, bool isFilled, const Color& fillColor) {
	// Create non-const local copies of the vertices
	Vector2 v0 = p0;
	Vector2 v1 = p1;
	Vector2 v2 = p2;

	// Draw the triangle's border
	DrawLineDDA(v0.x, v0.y, v1.x, v1.y, borderColor);
	DrawLineDDA(v1.x, v1.y, v2.x, v2.y, borderColor);
	DrawLineDDA(v2.x, v2.y, v0.x, v0.y, borderColor);

	// If the triangle should be filled
	if (isFilled) {
		// Sort vertices by y-coordinate (ascending order)
		Vector2 sorted[3] = { v0, v1, v2 };
		std::sort(sorted, sorted + 3, [](const Vector2& a, const Vector2& b) { return a.y < b.y; });

		// Extract sorted vertices
		v0 = sorted[0];
		v1 = sorted[1];
		v2 = sorted[2];

		for (int y = v0.y; y <= v2.y; ++y) {
			// For the current scanline, calculate intersections with edges
			int x1, x2;
			ScanLineDDA(v0.x, v0.y, v1.x, v1.y, x1, y); // v0 to v1
			ScanLineDDA(v1.x, v1.y, v2.x, v2.y, x2, y); // v1 to v2
			ScanLineDDA(v2.x, v2.y, v0.x, v0.y, x1, y); // v2 to v0

			// Fill pixels between x1 and x2 for the current y
			for (int x = x1; x <= x2; ++x) {
				SetPixel(x, y, fillColor);
			}
		}
	}
}



void Image::DrawLineDDA(int x0, int y0, int x1, int y1, const Color& c) {
	int dx = x1 - x0;
	int dy = y1 - y0;

	int steps = std::max(abs(dx), abs(dy));

	// Avoid division by zero
	if (steps == 0) {
		SetPixel(x0, y0, c);
		return;
	}

	float x_inc = dx / (float)steps;
	float y_inc = dy / (float)steps;

	float x = x0;
	float y = y0;

	for (int i = 0; i <= steps; i++) {
		if (x >= 0 && x < width && y >= 0 && y < height) {
			SetPixel(round(x), round(y), c);
		}
		x += x_inc;
		y += y_inc;
	}
}


void Image::DrawCircle(int x, int y, int r, const Color& borderColor, int borderWidth, bool isFilled, const Color& fillColor)
{
	// Midpoint Circle Drawing Algorithm to draw the perimeter
	int cx = x, cy = y; // Circle center
	int radius = r;
	int d = 1 - radius;  // Midpoint decision variable
	int x0 = 0, y0 = radius;

	// Function to plot points
	auto plotCirclePoints = [&](int cx, int cy, int x, int y, const Color& color) {
		SetPixel(cx + x, cy + y, color); // Octant 1
		SetPixel(cx - x, cy + y, color); // Octant 2
		SetPixel(cx + x, cy - y, color); // Octant 3
		SetPixel(cx - x, cy - y, color); // Octant 4
		SetPixel(cx + y, cy + x, color); // Octant 5
		SetPixel(cx - y, cy + x, color); // Octant 6
		SetPixel(cx + y, cy - x, color); // Octant 7
		SetPixel(cx - y, cy - x, color); // Octant 8
		};

	// Draw the border (perimeter) of the circle
	plotCirclePoints(cx, cy, x0, y0, borderColor);
	while (x0 < y0)
	{
		if (d < 0)
		{
			d += 2 * x0 + 3;
		}
		else
		{
			d += 2 * (x0 - y0) + 5;
			--y0;
		}
		++x0;
		plotCirclePoints(cx, cy, x0, y0, borderColor);
	}

	// Fill the circle if the isFilled flag is true
	if (isFilled)
	{
		for (int i = -radius; i <= radius; ++i) {
			int height = static_cast<int>(sqrt(radius * radius - i * i)); // Calculate the y-distance for each x (i)
			for (int j = cx - height; j <= cx + height; ++j) {
				SetPixel(j, cy + i, fillColor); // Fill horizontal lines inside the circle
			}
		}
	}

	// Handle borderWidth by drawing concentric circles
	for (int width = 1; width < borderWidth; ++width) {
		int newRadius = radius - width;
		if (newRadius <= 0) break; // Avoid negative radius
		int d = 1 - newRadius;  // Reset decision variable
		int x0 = 0, y0 = newRadius;

		// Draw the concentric circle
		plotCirclePoints(cx, cy, x0, y0, borderColor);
		while (x0 < y0)
		{
			if (d < 0)
			{
				d += 2 * x0 + 3;
			}
			else
			{
				d += 2 * (x0 - y0) + 5;
				--y0;
			}
			++x0;
			plotCirclePoints(cx, cy, x0, y0, borderColor);
		}
	}
}


//3.2

void Image::DrawImage(const Image& image, int x, int y)
{
	for (int i = 0; i < image.height; ++i)
	{
		for (int j = 0; j < image.width; ++j)
		{
			int dst_x = x + j;
			int dst_y = y + i;

			if (dst_x >= 0 && dst_x < this->width && dst_y >= 0 && dst_y < this->height)
			{
				this->SetPixel(dst_x, dst_y, image.GetPixel(j, i));
			}
		}
	}
}





#ifndef IGNORE_LAMBDAS

// You can apply and algorithm for two images and store the result in the first one
// ForEachPixel( img, img2, [](Color a, Color b) { return a + b; } );
template <typename F>
void ForEachPixel(Image& img, const Image& img2, F f) {
	for(unsigned int pos = 0; pos < img.width * img.height; ++pos)
		img.pixels[pos] = f( img.pixels[pos], img2.pixels[pos] );
}

#endif

FloatImage::FloatImage(unsigned int width, unsigned int height)
{
	this->width = width;
	this->height = height;
	pixels = new float[width * height];
	memset(pixels, 0, width * height * sizeof(float));
}

// Copy constructor
FloatImage::FloatImage(const FloatImage& c) {
	pixels = NULL;

	width = c.width;
	height = c.height;
	if (c.pixels)
	{
		pixels = new float[width * height];
		memcpy(pixels, c.pixels, width * height * sizeof(float));
	}
}

// Assign operator
FloatImage& FloatImage::operator = (const FloatImage& c)
{
	if (pixels) delete pixels;
	pixels = NULL;

	width = c.width;
	height = c.height;
	if (c.pixels)
	{
		pixels = new float[width * height * sizeof(float)];
		memcpy(pixels, c.pixels, width * height * sizeof(float));
	}
	return *this;
}

FloatImage::~FloatImage()
{
	if (pixels)
		delete pixels;
}

// Change image size (the old one will remain in the top-left corner)
void FloatImage::Resize(unsigned int width, unsigned int height)
{
	float* new_pixels = new float[width * height];
	unsigned int min_width = this->width > width ? width : this->width;
	unsigned int min_height = this->height > height ? height : this->height;

	for (unsigned int x = 0; x < min_width; ++x)
		for (unsigned int y = 0; y < min_height; ++y)
			new_pixels[y * width + x] = GetPixel(x, y);

	delete pixels;
	this->width = width;
	this->height = height;
	pixels = new_pixels;
}

void Image::Fade(float factor) {
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			Color& pixel = GetPixel(x, y); // Get a reference to the pixel
			pixel.r = static_cast<int>(pixel.r * factor);
			pixel.g = static_cast<int>(pixel.g * factor);
			pixel.b = static_cast<int>(pixel.b * factor);
		}
	}
}
