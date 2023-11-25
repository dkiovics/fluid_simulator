#include "circle.h"


Circle::Circle(int instanceCount, std::vector<glm::vec3> position, std::vector<glm::vec4> color, std::shared_ptr<RenderEngine> engine, float radius, int vertexNum) 
    : radius(radius), vertexNum(vertexNum), Geometry(instanceCount, position, color, engine) { 
    initGeometry();
}

InputGeometryArray Circle::generateGeometry() const {
    InputGeometryArray input;
    input.hasIndexes = false;
    input.renderAs = GL_TRIANGLE_FAN;

    input.vertexes.reserve(vertexNum);
    input.vertexes.push_back({ {0,0,0}, {0,0,1}, {0.5,0.5} });
    const float angleIncrement = 2 * PI / (vertexNum-2);
    for (int p = 0; p < vertexNum - 1; p++) {
        const float angle = angleIncrement * p;
        input.vertexes.push_back({
            {std::cosf(angle) * radius, std::sinf(angle) * radius, 0},
            {0, 0, 1},
            {(std::cosf(angle) + 1) / 2, (std::sinf(angle) + 1) / 2}
            });
    }
    
    return input;
}
