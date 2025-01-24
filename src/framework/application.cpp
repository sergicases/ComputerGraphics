#include "application.h"
#include "mesh.h"
#include "shader.h"
#include "utils.h" 
#include <cstdlib> // For rand()
#include <ctime>  
#include <cstdlib>



Application::Application(const char* caption, int width, int height)
{
	this->window = createWindow(caption, width, height);

	int w, h;
	SDL_GetWindowSize(window, &w, &h);

	this->mouse_state = 0;
	this->time = 0.f;
	this->window_width = w;
	this->window_height = h;
	this->keystate = SDL_GetKeyboardState(nullptr);

	this->framebuffer.Resize(w, h);
	this->canvas.Resize(w, h);  // Initialize canvas
	this->canvas.Fill(Color::BLACK);  // Start with black background

	this->borderWidth = 1;

	InitParticles(); // Initialize particles
	

}

Application::~Application()
{
}

void Application::Init(void)
{
	std::cout << "Initiating app..." << std::endl;
	LoadToolbar();
	if (particleMode) {
		UpdateParticles(dt);
	}
	
	
}

// Render one frame
void Application::Render(void) {
	// Clear the framebuffer


	// Copy the persistent canvas to the framebuffer
	framebuffer = canvas;

	// Draw all stored lines
	for (const auto& line : drawnLines) {
		framebuffer.DrawLineDDA(
			line.start.x, line.start.y,
			line.end.x, line.end.y,
			line.color
		);
	}


	// Preview current drawing (line, rectangle, circle, or triangle)
	if (isDrawingLine) {
		if (currentMode == DRAW_LINE) {
			framebuffer.DrawLineDDA(
				lineStart.x, lineStart.y,
				mouse_position.x, mouse_position.y,
				Color::WHITE
			);
		}
		else if (currentMode == DRAW_RECT) {
			int x = std::min(lineStart.x, mouse_position.x);
			int y = std::min(lineStart.y, mouse_position.y);
			int w = abs(mouse_position.x - lineStart.x);
			int h = abs(mouse_position.y - lineStart.y);

			// Pass fillShapes to determine fill behavior
			framebuffer.DrawRect(x, y, w, h, Color::WHITE, borderWidth, fillShapes, Color::GRAY);
		}

		else if (currentMode == DRAW_CIRCLE) {
			int radius = static_cast<int>(sqrt(pow(mouse_position.x - lineStart.x, 2) + pow(mouse_position.y - lineStart.y, 2)));

			framebuffer.DrawCircle(lineStart.x, lineStart.y, radius, Color::WHITE, borderWidth, fillShapes, Color::GRAY);
		}
		else if (currentMode == DRAW_TRIANGLE) {
			// Preview the triangle with the vertices set so far
			if (triangleClickCount > 0) {
				for (int i = 0; i < triangleClickCount - 1; i++) {
					framebuffer.DrawLineDDA(
						triangleVertices[i].x, triangleVertices[i].y,
						triangleVertices[i + 1].x, triangleVertices[i + 1].y,
						Color::WHITE
					);
				}

				// Preview the current edge being formed
				framebuffer.DrawLineDDA(
					triangleVertices[triangleClickCount - 1].x,
					triangleVertices[triangleClickCount - 1].y,
					mouse_position.x,
					mouse_position.y,
					Color::WHITE
				);
			}
		}
	}

	// Render particles
	if (particleMode) {
		UpdateParticles(dt / 1000.0f); // Convert milliseconds to seconds
		RenderParticles(&framebuffer); // Properly draw particles onto the framebuffer
	}

	RenderToolbar();

	// Final render to the screen
	framebuffer.Render();
}




// Called after render
void Application::Update(float seconds_elapsed)
{

}

//keyboard press event 
void Application::OnKeyPressed(SDL_KeyboardEvent event) {
	switch (event.keysym.sym) {
	case SDLK_1: // Draw Lines
		currentMode = DRAW_LINE;
		std::cout << "Line drawing mode activated!" << std::endl;
		break;

	case SDLK_2: // Draw Rectangles
		currentMode = DRAW_RECT;
		std::cout << "Rectangle drawing mode activated!" << std::endl;
		break;

	case SDLK_3: // Draw Circles
		currentMode = DRAW_CIRCLE;
		std::cout << "Circle drawing mode activated!" << std::endl;
		break;

	case SDLK_4: // Draw Triangles
		currentMode = DRAW_TRIANGLE;
		triangleClickCount = 0; // Reset triangle click count
		std::cout << "Triangle drawing mode activated!" << std::endl;
		break;

	case SDLK_5: // Paint
		currentMode = FREE_DRAW;
		std::cout << "Free-drawing mode activated!" << std::endl;
		break;

	case SDLK_6: // Animation
		particleMode = !particleMode; // Toggle particle mode
		if (particleMode) {
			std::cout << "Particle animation activated!" << std::endl;
		}
		else {
			std::cout << "Particle animation deactivated!" << std::endl;
		}
		break;

	case SDLK_f:
		fillShapes = !fillShapes;
		std::cout << "Fill shapes mode " << (fillShapes ? "activated" : "deactivated") << "!" << std::endl;
		break;

	case SDLK_PLUS: // Increase Border Width
	case SDLK_EQUALS: // Some keyboards map "+" as "="
		borderWidth++;
		if (borderWidth > 20) { // Max border width
			borderWidth = 20;
		}
		std::cout << "Border width increased to: " << borderWidth << std::endl;
		break;

	case SDLK_MINUS: // Decrease Border Width
		borderWidth--;
		if (borderWidth < 1) { // Min border width
			borderWidth = 1;
		}
		std::cout << "Border width decreased to: " << borderWidth << std::endl;
		break;

	default:
		break;
	}
}





void Application::OnMouseButtonDown(SDL_MouseButtonEvent event) {
	if (event.button == SDL_BUTTON_LEFT) {
		Vector2 mousePos(event.x, window_height - event.y); // Invert Y-axis

		// Check if a button is clicked
		for (size_t i = 0; i < buttonPositions.size(); ++i) {
			Vector2 pos = buttonPositions[i];
			Image& button = toolbarButtons[i];

			if (mousePos.x >= pos.x && mousePos.x < pos.x + button.width &&
				mousePos.y >= pos.y && mousePos.y < pos.y + button.height) {
				std::cout << "Button " << i << " clicked!" << std::endl;
				if (i == 0) { // Clear button
					canvas.Fill(Color::BLACK);  // Reset the canvas to black
					framebuffer = canvas;      // Update the framebuffer
					framebuffer.Render();      // Render the cleared framebuffer
					std::cout << "Canvas cleared!" << std::endl;
				}
				else if (i == 1) { // Load button
					Image imageToLoad;
					if (imageToLoad.LoadPNG("../res/images/fruits.png")) { // Load the image
						canvas.DrawImage(imageToLoad, 0, 0); // Draw the image at the top-left corner
						framebuffer = canvas;               // Update the framebuffer
						framebuffer.Render();               // Render the updated framebuffer
						std::cout << "Image loaded onto the canvas!" << std::endl;
					}
					else {
						std::cerr << "Failed to load image: ../res/images/fruits.png" << std::endl;
					}
				}
				else if (i == 2) { // Save button
					if (canvas.SaveTGA("images/output.tga")) { // Save the canvas to output.tga
						std::cout << "Canvas saved as output.tga!" << std::endl;
					}
					else {
						std::cerr << "Failed to save canvas!" << std::endl;
					}
				}
				else if (i == 3) { // Eraser button
					currentMode = ERASE;
					std::cout << "Eraser mode activated!" << std::endl;
				}
				// Handle tool buttons (line, rectangle, etc.)
				
				else if (i == 4) {
					currentMode = DRAW_LINE;
					std::cout << "Line drawing mode activated!" << std::endl;
				}
				else if (i == 5) {
					currentMode = DRAW_RECT;
					std::cout << "Rectangle drawing mode activated!" << std::endl;
				}
				else if (i == 6) {
					currentMode = DRAW_CIRCLE;
					std::cout << "Circle drawing mode activated!" << std::endl;
				}
				else if (i == 7) {
					currentMode = DRAW_TRIANGLE;
					triangleClickCount = 0; // Reset triangle click count
					std::cout << "Triangle drawing mode activated!" << std::endl;
				}
				if (i == 8) { // Free-drawing button
					currentMode = FREE_DRAW;
					std::cout << "Free-drawing mode activated!" << std::endl;
				}
				// Handle color buttons
				else if (i >= 9 && i <= 15) {
					switch (i) {
					case 9: selectedColor = Color::BLACK; break;
					case 10: selectedColor = Color::WHITE; break;
					case 11: selectedColor = Color(255, 192, 203); break; // Pink
					case 12: selectedColor = Color::YELLOW; break;
					case 13: selectedColor = Color::RED; break;
					case 14: selectedColor = Color::BLUE; break;
					case 15: selectedColor = Color::CYAN; break;
					}
					std::cout << "Color changed to: ("
						<< (int)selectedColor.r << ", "
						<< (int)selectedColor.g << ", "
						<< (int)selectedColor.b << ")" << std::endl;
				}
				return; // Exit once a button is handled
			}
		}

		// Handle drawing start (line, rectangle, circle, or triangle)
		if (currentMode == ERASE || currentMode != NONE) {
			isDrawingLine = true;
			lineStart = mousePos;
			std::cout << "Start position: (" << lineStart.x << ", " << lineStart.y << ")" << std::endl;
		}
		if (currentMode == FREE_DRAW) {
			isDrawingLine = true; // Track that free-drawing is active
			lineStart = mousePos;
		}
		if (currentMode == DRAW_TRIANGLE) {
			if (triangleClickCount < 3) {
				triangleVertices[triangleClickCount] = mousePos;
				std::cout << "Triangle vertex " << triangleClickCount + 1 << " set at: ("
					<< mousePos.x << ", " << mousePos.y << ")" << std::endl;
				triangleClickCount++;

				// If all three vertices are set, draw the triangle
				if (triangleClickCount == 3) {
					canvas.DrawTriangle(
						triangleVertices[0],
						triangleVertices[1],
						triangleVertices[2],
						Color::WHITE,
						fillShapes, // Fill the triangle
						selectedColor
					);
					framebuffer = canvas;
					framebuffer.Render();

					std::cout << "Triangle drawn with vertices: ("
						<< triangleVertices[0].x << ", " << triangleVertices[0].y << "), ("
						<< triangleVertices[1].x << ", " << triangleVertices[1].y << "), ("
						<< triangleVertices[2].x << ", " << triangleVertices[2].y << ")"
						<< std::endl;

					triangleClickCount = 0; // Reset for next triangle
				}
			}
		}
	}
}


void Application::OnMouseButtonUp(SDL_MouseButtonEvent event) {
	if (event.button == SDL_BUTTON_LEFT) {
		Vector2 mousePos(event.x, window_height - event.y); // Invert Y-axis

		if (currentMode == DRAW_LINE && isDrawingLine) {
			Vector2 lineEnd(event.x, window_height - event.y);

			canvas.DrawLineDDA(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, selectedColor);
			framebuffer = canvas;
			framebuffer.Render();

			std::cout << "Line drawn from (" << lineStart.x << ", " << lineStart.y << ") to ("
				<< lineEnd.x << ", " << lineEnd.y << ") with color: ("
				<< (int)selectedColor.r << ", "
				<< (int)selectedColor.g << ", "
				<< (int)selectedColor.b << ")" << std::endl;

			isDrawingLine = false;
		}


		if (currentMode == DRAW_RECT && isDrawingLine) {
			Vector2 rectEnd(event.x, window_height - event.y);

			int x = std::min(lineStart.x, rectEnd.x);
			int y = std::min(lineStart.y, rectEnd.y);
			int w = abs(rectEnd.x - lineStart.x);
			int h = abs(rectEnd.y - lineStart.y);

			// Pass the dynamic `fillShapes` variable instead of hardcoding `true`
			canvas.DrawRect(x, y, w, h, selectedColor, borderWidth, fillShapes, selectedColor);
			framebuffer = canvas;
			framebuffer.Render();

			std::cout << "Rectangle drawn at (" << x << ", " << y << ") with width: " << w
				<< " and height: " << h << " and fill: " << (fillShapes ? "ON" : "OFF")
				<< " and color: (" << (int)selectedColor.r << ", " << (int)selectedColor.g
				<< ", " << (int)selectedColor.b << ")" << std::endl;

			isDrawingLine = false;
		}



		if (currentMode == DRAW_CIRCLE && isDrawingLine) {
			Vector2 circleEnd(event.x, window_height - event.y);

			int radius = static_cast<int>(sqrt(pow(circleEnd.x - lineStart.x, 2) + pow(circleEnd.y - lineStart.y, 2)));

			canvas.DrawCircle(lineStart.x, lineStart.y, radius, selectedColor, borderWidth, fillShapes, selectedColor);
			framebuffer = canvas;
			framebuffer.Render();

			std::cout << "Circle drawn at center (" << lineStart.x << ", " << lineStart.y
				<< ") with radius: " << radius << " and color: ("
				<< (int)selectedColor.r << ", "
				<< (int)selectedColor.g << ", "
				<< (int)selectedColor.b << ")" << std::endl;

			isDrawingLine = false;
		}

		if (event.button == SDL_BUTTON_LEFT) {
			if (currentMode == ERASE) {
				isDrawingLine = false;
				std::cout << "Eraser operation finished." << std::endl;
			}
		}

		if (event.button == SDL_BUTTON_LEFT && currentMode == FREE_DRAW) {
			isDrawingLine = false; // Stop free-drawing
			std::cout << "Free-drawing stopped." << std::endl;
		}
		

	}
}




// Modified OnMouseMove
void Application::OnMouseMove(SDL_MouseButtonEvent event) {
	if (currentMode == ERASE && isDrawingLine) {
		Vector2 mousePos(event.x, window_height - event.y); // Invert Y-axis

		// Cast mousePos.x and mousePos.y to integers
		int mouseX = static_cast<int>(mousePos.x);
		int mouseY = static_cast<int>(mousePos.y);

		// Define eraser radius
		int radius = eraserSize / 2;
		int radiusSquared = radius * radius;

		// Define the bounding box for the circular eraser
		int startX = std::max(0, mouseX - radius);
		int startY = std::max(0, mouseY - radius);
		int endX = std::min(static_cast<int>(canvas.width) - 1, mouseX + radius);
		int endY = std::min(static_cast<int>(canvas.height) - 1, mouseY + radius);

		// Erase pixels within the circular area
		for (int y = startY; y <= endY; ++y) {
			for (int x = startX; x <= endX; ++x) {
				// Check if the point (x, y) is within the circle
				int dx = x - mouseX;
				int dy = y - mouseY;
				if (dx * dx + dy * dy <= radiusSquared) {
					canvas.SetPixel(x, y, Color::BLACK); // Set pixel to black
				}
			}
		}

		// Update the framebuffer and render
		framebuffer = canvas;
		framebuffer.Render();
	}
	if (currentMode == FREE_DRAW && isDrawingLine) {
		Vector2 mousePos(event.x, window_height - event.y); // Invert Y-axis

		// Draw a line segment from the last position to the current position
		canvas.DrawLineDDA(lineStart.x, lineStart.y, mousePos.x, mousePos.y, selectedColor);

		// Update the framebuffer and render
		framebuffer = canvas;
		framebuffer.Render();

		// Update the last position
		lineStart = mousePos;
	}
}



void Application::OnWheel(SDL_MouseWheelEvent event)
{
	float dy = event.preciseY;

	// ...
}

void Application::OnFileChanged(const char* filename)
{ 
	Shader::ReloadSingleShader(filename);
}

//3.2
void Application::LoadToolbar()
{
	std::vector<std::string> buttonPaths = {
		"../res/images/clear.png",     // Clear button
		"../res/images/load.png",      // Load button
		"../res/images/save.png",      // Save button
		"../res/images/eraser.png" ,    // Eraser button
		
		"../res/images/line.png",       // Line button
		"../res/images/rectangle.png", // Rectangle button
		"../res/images/circle.png",    // Circle button
		"../res/images/triangle.png",  // Triangle button
		"../res/images/cursorv3.png",
		"../res/images/black.png",     // Color button
		"../res/images/white.png",     // Color button
		"../res/images/pink.png",     // Color button
		"../res/images/yellow.png",     // Color button
		"../res/images/red.png",     // Color button
		"../res/images/blue.png",     // Color button
		"../res/images/cyan.png",     // Color button

	};

	int buttonX = 10;   // Initial X position for the toolbar
	int buttonY = 10;   // Initial Y position for the toolbar
	int buttonSpacing = 10; // Spacing between buttons

	for (const auto& path : buttonPaths) {
		Image buttonImage;

		// Attempt to load the PNG
		if (!buttonImage.LoadPNG(path.c_str(), false)) {
			std::cerr << "Failed to load image: " << path << std::endl;
			continue; // Skip this button if loading fails
		}

		toolbarButtons.push_back(buttonImage);

		// Position each button horizontally
		buttonPositions.push_back({ static_cast<float>(buttonX), static_cast<float>(buttonY) });

		buttonX += buttonImage.width + buttonSpacing;
	}
}



void Application::RenderToolbar()
{
	// Draw the toolbar buttons
	for (size_t i = 0; i < toolbarButtons.size(); ++i)
	{
		framebuffer.DrawImage(toolbarButtons[i], buttonPositions[i].x, buttonPositions[i].y);
	}

	// Draw the separating bar below the toolbar
	int barY = buttonPositions[0].y + toolbarButtons[0].height + 10; // Position below the buttons
	int barHeight = 5; // Height of the bar
	Color barColor = Color(200, 200, 200); // Light gray color

	// Create an Image representing the bar
	Image barImage(framebuffer.width, barHeight);
	barImage.Fill(barColor); // Fill the bar image with the desired color

	// Draw the bar image onto the framebuffer
	framebuffer.DrawImage(barImage, 0, barY);
}


//COlorful snowing-like animation but upwards
void Application::InitParticles() {
	srand(static_cast<unsigned int>(std::time(nullptr))); // Seed RNG

	for (int i = 0; i < MAX_PARTICLES; ++i) {
		// Start at a random position 
		particles[i].position = Vector2(rand() % framebuffer.width, rand() % framebuffer.height);

		
		float speed = static_cast<float>(rand() % 200) ; 
		particles[i].velocity = Vector2(0.0f, speed);
		particles[i].color = Color(rand() % 256, rand() % 256, rand() % 256);
		particles[i].acceleration = 1.0f;
		particles[i].ttl = static_cast<float>(rand() % 5000) / 1000.0f + 5.0f;
		particles[i].inactive = false;

		// Random size (2-6)
		particles[i].size = static_cast<float>(rand() % 5 + 2);
	}
}

void Application::UpdateParticles(float dt) {
	for (int i = 0; i < MAX_PARTICLES; ++i) {
		if (!particles[i].inactive) {
			// Update position
			particles[i].position = particles[i].position + particles[i].velocity * dt;

			// Decrease TTL
			particles[i].ttl -= dt;

			// Mark as inactive if TTL expires
			if (particles[i].ttl <= 0) {
				particles[i].inactive = true;
			}
			// Mark as inactive if the particle leaves the screen bounds (y >= screen height)
			if (particles[i].position.y >= framebuffer.height) {
				particles[i].inactive = true;
			}
		}
		// Respawn inactive particles
		if (particles[i].inactive) {
			// Respawn at a random position
			particles[i].position = Vector2(rand() % framebuffer.width,rand()% framebuffer.height); 

			// Randomize downward velocity
			float speed = static_cast<float>(rand() % 200) / 100.0f + 1.5f; 
			particles[i].velocity = Vector2(0.0f, speed);

			// Reset properties
			particles[i].ttl = static_cast<float>(rand() % 5000) / 1000.0f + 5.0f;
			particles[i].inactive = false;

			// Randomize color
			particles[i].color = Color(rand() % 256, rand() % 256, rand() % 256);
		}
	}
}


void Application::RenderParticles(Image* framebuffer) {
	for (int i = 0; i < MAX_PARTICLES; ++i) {
		if (!particles[i].inactive) {
			// Draw filled circles based on particle size
			framebuffer->DrawCircle(
				static_cast<int>(particles[i].position.x),
				static_cast<int>(particles[i].position.y),
				static_cast<int>(particles[i].size),
				particles[i].color,   // Border color
				0,                    // No border width
				true,                 // Filled circle
				particles[i].color    // Fill color
			);
		}
	}
}











