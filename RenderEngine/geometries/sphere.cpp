#include "sphere.h"
#include <glm/glm.hpp>


Sphere::Sphere(int instanceCount, std::vector<glm::vec3> position, std::vector<glm::vec4> color, std::shared_ptr<RenderEngine> engine, float radius, int vertexNum)
    : radius(radius), vertexNum(vertexNum), Geometry(instanceCount, position, color, engine) {
    initGeometry();
}

InputGeometryArray Sphere::generateGeometry() const {
    InputGeometryArray input;
    input.hasIndexes = true;
    input.renderAs = GL_TRIANGLES;

    input.vertexes.reserve(vertexNum * vertexNum);
    input.vertexes.push_back({ {0,-radius,0}, {0,-1,0}, {0.0,0.0} });
    
    int circleNum = vertexNum / 2 + 1;
    for (int p = 1; p < circleNum; p++) {
        float fp = float(p) / circleNum;
        float pitch = (1.0f - fp) * PI;
        for (int q = 0; q < vertexNum; q++) {
            float fq = float(q) / vertexNum;
            float yaw = fq * 2 * PI;
            glm::vec3 normal(std::cosf(yaw) * std::sinf(pitch), std::cosf(pitch), std::sinf(yaw) * std::sinf(pitch));
            input.vertexes.push_back({
                normal * radius,
                normal,
                {fq, fp}
                });
        }
    }
    input.vertexes.push_back({ {0,radius,0}, {0,1,0}, {0.0,0.0} });

    int num = input.vertexes.size() - 1;

    for (int p = 0; p < circleNum; p++) {
        for (int q = 1; q <= vertexNum; q++) {
            input.indexes.push_back(std::min(p * vertexNum + q, num));
            input.indexes.push_back(std::max((p - 1) * vertexNum + q, 0));
            int next = q % vertexNum + 1;
            input.indexes.push_back(std::max((p - 1) * vertexNum + next, 0));
            input.indexes.push_back(std::max((p - 1) * vertexNum + next, 0));
            input.indexes.push_back(std::min(p * vertexNum + q, num));
            input.indexes.push_back(std::min(p * vertexNum + next, num));
        }
    }

    return input;
}
