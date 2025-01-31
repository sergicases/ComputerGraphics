#include "entity.h"

Entity::Entity(const char* path) {
	this->mesh = new Mesh();
	this->mesh->LoadOBJ(path);	
	this-> model_matrix.SetIdentity();
}

Entity::Entity() {
}

void Entity::Render(Image* framebuffer, Camera* camera, FloatImage* zbuffer) {
	const std::vector<Vector3>& vertx = mesh->GetVertices();
	Vector2 resolution = Vector2(framebuffer->width-1, framebuffer->height-1);

	for (int i = 0;i < vertx.size();i += 3) {
		Vector3 p1 = this->model_matrix * vertx[i];
		Vector3 p2 = this->model_matrix * vertx[i + 1];
		Vector3 p3 = this->model_matrix * vertx[i + 2];

		bool n1;
		p1 = camera->ProjectVector(p1, n1);
		p2 = camera->ProjectVector(p2, n1);
		p3 = camera->ProjectVector(p3, n1);

		p1.Clamp(-1, 1);
		p2.Clamp(-1, 1);
		p3.Clamp(-1, 1);

		Vector2 T1 = Vector2(p1.x + 1.f, p1.y + 1.f) * 0.5f * resolution;
		Vector2 T2 = Vector2(p2.x + 1.f, p2.y + 1.f) * 0.5f*resolution;
		Vector2 T3 = Vector2(p3.x + 1.f, p3.y + 1.f) * 0.5f * resolution;

		framebuffer->SetPixel(T1.x, T1.y, Color::RED);
		framebuffer->SetPixel(T2.x, T2.y, Color::RED);
		framebuffer->SetPixel(T3.x, T3.y, Color::RED);
	}

	
}
