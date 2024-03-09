#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../engine/renderEngine.h"
#include "../geometries/geometry.h"
#include <memory>
#include <algorithm>
#include <vector>


class Camera3D {
private:
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 right;
    const float fov;
    float aspectRatio;
    float pitch;
    const float maxPitch = 88;
    const float minPitch = -88;
    float yaw;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    std::shared_ptr<renderer::RenderEngine> engine;

public:
    std::vector<unsigned int> shaderProgramIds;

    Camera3D(std::shared_ptr<renderer::RenderEngine> engine, std::vector<unsigned int> shaderProgramIds, const glm::vec3& position, float fov, float aspectRatio)
        : engine(engine), shaderProgramIds(shaderProgramIds), position(position), fov(fov), pitch(0.0f), yaw(0.0f), aspectRatio(aspectRatio) {
        updateViewMatrix();
        updateProjectionMatrix();
    }

    glm::mat4 getViewMatrix() const {
        return viewMatrix;
    }

    glm::mat4 getProjectionMatrix() const {
        return projectionMatrix;
    }

    void move(const glm::vec3& offset) {
        position += offset;
        updateViewMatrix();
    }

    void setAspectRatio(float xDy) {
        aspectRatio = xDy;
        updateProjectionMatrix();
    }

    void setPitchAndYaw(float pitch, float yaw) {
        this->pitch = std::clamp(pitch, minPitch, maxPitch);
        this->yaw = yaw;
        updateViewMatrix();
    }

    void incrementPitchAndYaw(float dpitch, float dyaw) {
        setPitchAndYaw(pitch + dpitch, yaw + dyaw);
    }

    void rotateAroundPoint(const glm::vec3& point, float r, float dpitch, float dyaw) {
        pitch = std::clamp(pitch + dpitch, minPitch, maxPitch);
        yaw += dyaw;
        glm::vec3 offset;
        offset.x = -cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        offset.y = -sin(glm::radians(pitch));
        offset.z = -sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        position = point + offset * r;
        updateViewMatrix();
    }

    void setUniforms() {
        for (auto shaderProgramId : shaderProgramIds) {
            engine->activateGPUProgram(shaderProgramId);
            engine->setUniformVec3(shaderProgramId, "camera.position", position);
            engine->setUniformMat4(shaderProgramId, "camera.projectionMatrix", projectionMatrix);
            engine->setUniformMat4(shaderProgramId, "camera.viewMatrix", viewMatrix);
        }
    }

    glm::vec3 getFrontVec() const {
        return direction;
    }

    glm::vec3 getRightVec() const {
        return right;
    }

    glm::vec3 getPosition() const {
        return position;
    }

private:
    void updateViewMatrix() {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction = glm::normalize(front);

        right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
        glm::vec3 up = glm::normalize(glm::cross(right, front));

        viewMatrix = glm::lookAt(position, position + direction, up);
    }

    void updateProjectionMatrix() {
        projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, 0.5f, 300.0f);
    }

};

