#pragma once

#include <memory>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <engineUtils/object.h>
#include <engine/renderEngine.h>
#include <engine/shaderProgram.h>
#include <engineUtils/camera3D.hpp>
#include <engineUtils/lights.hpp>
#include <geometries/basicGeometries.h>


class TransparentBox {
private:
	std::vector<std::unique_ptr<renderer::Object3D<renderer::Geometry>>> boxSides;
	std::shared_ptr<renderer::Camera3D> camera;

	struct Face
	{
		int index;
		glm::vec3 normal;
		glm::vec3 position;
	};

public:
	TransparentBox(const std::shared_ptr<renderer::Camera3D> camera, const glm::vec4& color, 
		std::shared_ptr<renderer::ShaderProgram> gpuProgram) : camera(camera)
	{
		std::shared_ptr<renderer::Square> side = std::make_shared<renderer::Square>();
		for (int p = 0; p < 6; p++) 
		{
			boxSides.push_back(std::make_unique<renderer::Object3D<renderer::Geometry>>(side, gpuProgram));
			boxSides[p]->specularColor = glm::vec4(1.2, 1.2, 1.2, 1);
			boxSides[p]->shininess = 10;
			boxSides[p]->diffuseColor = color;
		}
	}

	/**
	 * \brief draws a transparent box with the given center and size
	 * 
	 * \param center - the center of the box
	 * \param size - the size of the box
	 * \param drawBackwardFacing - whether to draw the faces facing away from the camera (the ones whose face the camera sees inside the box)
	 * \param drawForwardFacing - whether to draw the faces facing towards the camera (the ones whose face the camera sees outside the box)
	 */
	void draw(const glm::vec3& center, const glm::vec3& size, bool drawBackwardFacing, bool drawForwardFacing) 
	{
		const glm::vec3 camPos = camera->getPosition();

		//make the box faces sorted by distance from camera
		std::map<float, Face> indexes;

		glm::vec3 posX1 = glm::vec3(center.x - size.x * 0.5f, center.y, center.z);
		boxSides[0]->setModelMatrix(glm::translate(glm::mat4(1.0), posX1) * glm::rotate(glm::mat4(1.0), PI * 0.5f, glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(size.z, size.y, 1)));
		indexes.insert(std::make_pair(glm::dot(camPos - posX1, camPos - posX1), Face(0, glm::vec3(-1, 0, 0), posX1)));
		
		glm::vec3 posX2 = glm::vec3(center.x + size.x * 0.5f, center.y, center.z);
		boxSides[1]->setModelMatrix(glm::translate(glm::mat4(1.0), posX2) * glm::rotate(glm::mat4(1.0), PI * 0.5f, glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(size.z, size.y, 1)));
		indexes.insert(std::make_pair(glm::dot(camPos - posX2, camPos - posX2), Face(1, glm::vec3(1, 0, 0), posX2)));
		
		glm::vec3 posY1 = glm::vec3(center.x, center.y - size.y * 0.5f, center.z);
		boxSides[2]->setModelMatrix(glm::translate(glm::mat4(1.0), posY1) * glm::rotate(glm::mat4(1.0), PI * 0.5f, glm::vec3(1, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(size.x, size.z, 1)));
		indexes.insert(std::make_pair(glm::dot(camPos - posY1, camPos - posY1), Face(2, glm::vec3(0, -1, 0), posY1)));
		
		glm::vec3 posY2 = glm::vec3(center.x, center.y + size.y * 0.5f, center.z);
		boxSides[3]->setModelMatrix(glm::translate(glm::mat4(1.0), posY2) * glm::rotate(glm::mat4(1.0), PI * 0.5f, glm::vec3(1, 0, 0)) * glm::scale(glm::mat4(1.0), glm::vec3(size.x, size.z, 1)));
		indexes.insert(std::make_pair(glm::dot(camPos - posY2, camPos - posY2), Face(3, glm::vec3(0, 1, 0), posY2)));
		
		glm::vec3 posZ1 = glm::vec3(center.x, center.y, center.z - size.z * 0.5f);
		boxSides[4]->setModelMatrix(glm::translate(glm::mat4(1.0), posZ1) * glm::scale(glm::mat4(1.0), glm::vec3(size.x, size.y, 1)));
		indexes.insert(std::make_pair(glm::dot(camPos - posZ1, camPos - posZ1), Face(4, glm::vec3(0, 0, -1), posZ1)));
		
		glm::vec3 posZ2 = glm::vec3(center.x, center.y, center.z + size.z * 0.5f);
		boxSides[5]->setModelMatrix(glm::translate(glm::mat4(1.0), posZ2) * glm::scale(glm::mat4(1.0), glm::vec3(size.x, size.y, 1)));
		indexes.insert(std::make_pair(glm::dot(camPos - posZ2, camPos - posZ2), Face(5, glm::vec3(0, 0, 1), posZ2)));

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//draw the box faces from farthest to closest
		for (auto it = indexes.rbegin(); it != indexes.rend(); it++) {
			float d = glm::dot(camPos - it->second.position, it->second.normal);
			if (drawForwardFacing && d >= 0.0f)
				boxSides[it->second.index]->draw();
			if (drawBackwardFacing && d <= 0.0f)
				boxSides[it->second.index]->draw();
		}
		glDisable(GL_BLEND);
	}
};

