#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <algorithm>
#include <vector>

#include "../engine/shaderProgram.h"
#include "../engine/uniforms.hpp"


namespace renderer
{

class Camera3D : public UniformGatherer, public UniformGathererGlobal
{
public:
    Camera3D(const glm::vec3& position, float fov, float aspectRatio)
        : UniformGatherer("camera.", true, this->position, viewMatrix, projectionMatrix),
        fov(fov), pitch(0.0f), yaw(0.0f), aspectRatio(aspectRatio)
    {
        this->position = glm::vec4(position, 1);
        updateViewMatrix();
        updateProjectionMatrix();
    }

    glm::mat4 getViewMatrix() const {
        return *viewMatrix;
    }

    glm::mat4 getProjectionMatrix() const {
        return *projectionMatrix;
    }

    void move(const glm::vec3& offset) {
        *position += glm::vec4(offset, 0);
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
        position = glm::vec4(point + offset * r, 1);
        updateViewMatrix();
    }

    glm::vec3 getFrontVec() const {
        return direction;
    }

    glm::vec3 getRightVec() const {
        return right;
    }

    glm::vec3 getPosition() const {
        return *position;
    }

    glm::vec3 getMouseRayDir(const glm::vec2& mousePos) const
    {
        glm::vec3 nearPoint = glm::unProject(glm::vec3(mousePos, 0), getViewMatrix(), getProjectionMatrix(), glm::vec4(0, 0, 1, 1));
        glm::vec3 farPoint = glm::unProject(glm::vec3(mousePos, 1), getViewMatrix(), getProjectionMatrix(), glm::vec4(0, 0, 1, 1));
        return glm::normalize(farPoint - nearPoint);
    }

protected:
    void setUniformsGlobal(const ShaderProgram& program) const override
    {
        setUniforms(program);
    }

private:
    u_var(position, glm::vec4);
    glm::vec3 direction;
    glm::vec3 right;
    const float fov;
    float aspectRatio;
    float pitch;
    const float maxPitch = 88;
    const float minPitch = -88;
    float yaw;
    u_var(viewMatrix, glm::mat4);
    u_var(projectionMatrix, glm::mat4);

    void updateViewMatrix() {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction = glm::normalize(front);

        right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
        glm::vec3 up = glm::normalize(glm::cross(right, front));

        viewMatrix = glm::lookAt(glm::vec3(*position), glm::vec3(*position) + direction, up);
        setUniformsForAllPrograms();
    }

    void updateProjectionMatrix() {
        projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, 0.5f, 300.0f);
        setUniformsForAllPrograms();
    }

};



} // namespace renderer

