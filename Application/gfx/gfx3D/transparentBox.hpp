#pragma once

#include <memory>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <engineUtils/object3D.hpp>
#include <engine/renderEngine.h>
#include <engineUtils/camera3D.hpp>
#include <geometries/quad.h>


class TransparentBox {
private:
	std::vector<std::unique_ptr<Object3D>> boxSides;
	const Camera3D& camera;
	std::shared_ptr<RenderEngine> engine;

public:
	TransparentBox(const Camera3D& camera, std::shared_ptr<RenderEngine> engine, const glm::vec4& color, unsigned int gpuProgram) : camera(camera), engine(engine) {
		std::shared_ptr<Quad> side = std::make_shared<Quad>(1, std::vector<glm::vec3>{ glm::vec3(0, 0, 0) }, std::vector<glm::vec4>{ color }, engine, 1, 1);
		for (int p = 0; p < 6; p++) {
			boxSides.push_back(std::make_unique<Object3D>(side, gpuProgram, glm::mat4(1.0f), glm::vec3(1.2, 1.2, 1.2), 10));
		}
	}

	void draw(const glm::vec3& center, const glm::vec3& size) {
		const glm::vec3 camPos = camera.getPosition();

		//make the box faces sorted by distance from camera
		std::map<float, int> indexes;

		glm::vec3 posX1 = glm::vec3(0, center.y, center.z);
		boxSides[0]->M = glm::translate(glm::mat4(1.0), posX1) * glm::rotate(glm::mat4(1.0), PI * 0.5f, glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(size.z, size.y, 1));
		indexes.insert(std::make_pair(glm::dot(camPos - posX1, camPos - posX1), 0));
		glm::vec3 posX2 = glm::vec3(size.x, center.y, center.z);
		boxSides[1]->M = glm::translate(glm::mat4(1.0), posX2) * glm::rotate(glm::mat4(1.0), PI * 0.5f, glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(size.z, size.y, 1));
		indexes.insert(std::make_pair(glm::dot(camPos - posX2, camPos - posX2), 1));
		glm::vec3 posY1 = glm::vec3(center.x, 0, center.z);
		boxSides[2]->M = glm::translate(glm::mat4(1.0), posY1) * glm::rotate(glm::mat4(1.0), PI * 0.5f, glm::vec3(1, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(size.x, size.z, 1));
		indexes.insert(std::make_pair(glm::dot(camPos - posY1, camPos - posY1), 2));
		glm::vec3 posY2 = glm::vec3(center.x, size.y, center.z);
		boxSides[3]->M = glm::translate(glm::mat4(1.0), posY2) * glm::rotate(glm::mat4(1.0), PI * 0.5f, glm::vec3(1, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(size.x, size.z, 1));
		indexes.insert(std::make_pair(glm::dot(camPos - posY2, camPos - posY2), 3));
		glm::vec3 posZ1 = glm::vec3(center.x, center.y, 0);
		boxSides[4]->M = glm::translate(glm::mat4(1.0), posZ1) * glm::scale(glm::mat4(1.0), glm::vec3(size.x, size.y, 1));
		indexes.insert(std::make_pair(glm::dot(camPos - posZ1, camPos - posZ1), 4));
		glm::vec3 posZ2 = glm::vec3(center.x, center.y, size.z);
		boxSides[5]->M = glm::translate(glm::mat4(1.0), posZ2) * glm::scale(glm::mat4(1.0), glm::vec3(size.x, size.y, 1));
		indexes.insert(std::make_pair(glm::dot(camPos - posZ2, camPos - posZ2), 5));

		//draw the box faces from farthest to closest
		for (auto it = indexes.rbegin(); it != indexes.rend(); it++) {
			boxSides[it->second]->draw();
		}
	}
};

