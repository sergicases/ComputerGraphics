#include "mesh.h"
#include "image.h"
#include "framework.h"
#include "camera.h"

class Entity {

    Mesh* mesh;           // Pointer to a mesh object
    Matrix44 model_matrix;
    int rendering_mode;
public:
    

    Entity();                               // Default constructor
    Entity(const char* path, Matrix44 m); // Parameterized constructor

    void Entity::Render(Image* framebuffer, Camera* camera, FloatImage* zbuffer);
    
    



    

};

