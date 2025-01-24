/*  
	+ This class encapsulates the application, is in charge of creating the data, getting the user input, process the update and render.
*/

#pragma once

#include "main/includes.h"
#include "framework.h"
#include "image.h"
#include <ctime> // For time()


class Application
{
public:

	// Window
	struct DrawnLine {
		Vector2 start;
		Vector2 end;
		Color color;
	};

	// Add these as class members in Application class
	std::vector<DrawnLine> drawnLines;  // Store all drawn lines
	Image canvas;


	SDL_Window* window = nullptr;
	int window_width;
	int window_height;

	float time;

	// Input
	const Uint8* keystate;
	int mouse_state; // Tells which buttons are pressed
	Vector2 mouse_position; // Last mouse position
	Vector2 mouse_delta; // Mouse movement in the last frame

	void OnKeyPressed(SDL_KeyboardEvent event);
	void OnMouseButtonDown(SDL_MouseButtonEvent event);
	void OnMouseButtonUp(SDL_MouseButtonEvent event);
	void OnMouseMove(SDL_MouseButtonEvent event);
	void OnWheel(SDL_MouseWheelEvent event);
	void OnFileChanged(const char* filename);

	// CPU Global framebuffer
	Image framebuffer;

	// Constructor and main methods
	Application(const char* caption, int width, int height);
	~Application();

	void Init(void);
	void Render(void);
	void Update(float dt);

	// Other methods to control the app
	void SetWindowSize(int width, int height) {
		glViewport(0, 0, width, height);
		this->window_width = width;
		this->window_height = height;
		this->framebuffer.Resize(width, height);
	}

	Vector2 GetWindowSize()
	{
		int w, h;
		SDL_GetWindowSize(window, &w, &h);
		return Vector2(float(w), float(h));
	}

	int borderWidth;

	//3.2
	std::vector<Image> toolbarButtons; // Holds button images
	std::vector<Vector2> buttonPositions; // Positions of buttons
	Vector2 selectedButton; // Tracks the selected button (optional for feedback)

	void LoadToolbar(); // Load button images
	void RenderToolbar(); // Render toolbar buttons

	enum DrawingMode {
		NONE,
		DRAW_LINE,
		DRAW_RECT,
		DRAW_CIRCLE,
		DRAW_TRIANGLE,
		ERASE,
		FREE_DRAW // Add free-drawing mode
	};



	DrawingMode currentMode = NONE; // Track the current drawing mode
	Vector2 lineStart;             // Store the starting point of the line
	bool isDrawingLine = false;    // Track if the user is in the middle of drawing a line


	struct DrawnRect {
		int x, y, width, height;
		Color color;
	};

	int triangleClickCount = 0; // Track the number of clicks for triangle vertices
	Vector2 triangleVertices[3]; // Store the three vertices of the triangle
	
	Color selectedColor = Color::WHITE; // Default color is white

	int eraserSize = 10; // Size of the eraser (10x10 square)

	static const int MAX_PARTICLES = 5000; // Maximum number of particles

	struct Particle {
		Vector2 position;
		Vector2 velocity;
		Color color;
		float acceleration;
		float ttl;
		bool inactive;
		float size; // New: size of the particle
	};


	Particle particles[MAX_PARTICLES];  // Array to store all particles
	bool particleMode = false;          // Toggle for particle animation

	void InitParticles(); // Declare the method
	void Application::UpdateParticles(float dt);
	void Application::RenderParticles(Image* framebuffer);

	float dt = 16.0f; // Assuming ~60 FPS (16 milliseconds per frame)


	
	bool fillShapes = false;
	
	




	



};