#include "basicGeometries.h"
#include <vector>
#include <cmath>

using namespace renderer;

Circle::Circle(int vertexNum) : Geometry(GL_TRIANGLE_FAN, vertexNum)
{
    std::vector<BasicVertex> vertexes;
    vertexes.reserve(vertexNum);
    vertexes.push_back({ {0,0,0,1}, {0,0,1,0}, {0.5,0.5} });
    const float angleIncrement = 2 * PI / (vertexNum - 2);

    for (int p = 0; p < vertexNum - 1; p++)
    {
        const float angle = angleIncrement * p;
        vertexes.push_back({
            {std::cosf(angle), std::sinf(angle), 0, 1},
            {0, 0, 1, 0},
            {(std::cosf(angle) + 1) / 2, (std::sinf(angle) + 1) / 2}
            });
    }
    
    std::vector<ArrayAttribute> attributes = {
		ArrayAttribute{0, 4, GL_FLOAT, 0},
        ArrayAttribute{1, 4, GL_FLOAT, sizeof(float) * 4},
        ArrayAttribute{2, 2, GL_FLOAT, sizeof(float) * 8}
	};

    createVbo(vertexes, attributes);
}

Square::Square() : Geometry(GL_TRIANGLES)
{
	std::vector<BasicVertex> vertexes;
	std::vector<unsigned int> indexes;

	vertexes.push_back({
		{-0.5, -0.5, 0, 1},
		{0, 0, 1, 0},
		{0, 0}
		});
	vertexes.push_back({
		{-0.5, 0.5, 0, 1},
		{0, 0, 1, 0},
		{0, 1}
		});
	vertexes.push_back({
		{0.5, -0.5, 0, 1},
		{0, 0, 1, 0},
		{1, 0}
		});
	vertexes.push_back({
		{0.5, 0.5, 0, 1},
		{0, 0, 1, 0},
		{1, 1}
		});

	indexes.push_back(0);
	indexes.push_back(1);
	indexes.push_back(2);
	indexes.push_back(1);
	indexes.push_back(2);
	indexes.push_back(3);

	std::vector<ArrayAttribute> attributes = {
		ArrayAttribute{0, 4, GL_FLOAT, 0},
		ArrayAttribute{1, 4, GL_FLOAT, sizeof(float) * 4},
		ArrayAttribute{2, 2, GL_FLOAT, sizeof(float) * 8}
	};

	createVbo(vertexes, attributes);
	createIndexBuffer(indexes);
}

FlipSquare::FlipSquare() : Geometry(GL_TRIANGLES)
{
	std::vector<BasicVertex> vertexes;
	std::vector<unsigned int> indexes;

	vertexes.push_back({
		{-0.5, -0.5, 0, 1},
		{0, 0, 1, 0},
		{0, 1}
		});
	vertexes.push_back({
		{-0.5, 0.5, 0, 1},
		{0, 0, 1, 0},
		{0, 0}
		});
	vertexes.push_back({
		{0.5, -0.5, 0, 1},
		{0, 0, 1, 0},
		{1, 1}
		});
	vertexes.push_back({
		{0.5, 0.5, 0, 1},
		{0, 0, 1, 0},
		{1, 0}
		});

	indexes.push_back(0);
	indexes.push_back(1);
	indexes.push_back(2);
	indexes.push_back(1);
	indexes.push_back(2);
	indexes.push_back(3);

	std::vector<ArrayAttribute> attributes = {
		ArrayAttribute{0, 4, GL_FLOAT, 0},
		ArrayAttribute{1, 4, GL_FLOAT, sizeof(float) * 4},
		ArrayAttribute{2, 2, GL_FLOAT, sizeof(float) * 8}
	};

	createVbo(vertexes, attributes);
	createIndexBuffer(indexes);
}

Cube::Cube() : Geometry(GL_TRIANGLES)
{
	std::vector<BasicVertex> vertexes;

	vertexes.push_back({
		{-0.5,-0.5,-0.5,1},
		{0,-1,0,0},
		{0,0}
		});
	vertexes.push_back({
		{-0.5,-0.5,0.5,1},
		{0,-1,0,0},
		{0,1}
		});
	vertexes.push_back({
		{0.5,-0.5,0.5,1},
		{0,-1,0,0},
		{1,1}
		});
	vertexes.push_back({
		{0.5,-0.5,0.5,1},
		{0,-1,0,0},
		{1,1}
		});
	vertexes.push_back({
		{0.5,-0.5,-0.5,1},
		{0,-1,0,0},
		{1,0}
		});
	vertexes.push_back({
		{-0.5,-0.5,-0.5,1},
		{0,-1,0,0},
		{0,0}
		});

	vertexes.push_back({
		{-0.5,0.5,-0.5,1},
		{0,1,0,0},
		{0,0}
		});
	vertexes.push_back({
		{-0.5,0.5,0.5,1},
		{0,1,0,0},
		{0,1}
		});
	vertexes.push_back({
		{0.5,0.5,0.5,1},
		{0,1,0,0},
		{1,1}
		});
	vertexes.push_back({
		{0.5,0.5,0.5,1},
		{0,1,0,0},
		{1,1}
		});
	vertexes.push_back({
		{0.5,0.5,-0.5,1},
		{0,1,0,0},
		{1,0}
		});
	vertexes.push_back({
		{-0.5,0.5,-0.5,1},
		{0,1,0,0},
		{0,0}
		});

	vertexes.push_back({
		{-0.5,-0.5,-0.5,1},
		{0,0,-1,0},
		{1,0}
		});
	vertexes.push_back({
		{0.5,-0.5,-0.5,1},
		{0,0,-1,0},
		{0,0}
		});
	vertexes.push_back({
		{0.5,0.5,-0.5,1},
		{0,0,-1,0},
		{0,1}
		});
	vertexes.push_back({
		{0.5,0.5,-0.5,1},
		{0,0,-1,0},
		{0,1}
		});
	vertexes.push_back({
		{-0.5,0.5,-0.5,1},
		{0,0,-1,0},
		{1,1}
		});
	vertexes.push_back({
		{-0.5,-0.5,-0.5,1},
		{0,0,-1,0},
		{1,0}
		});

	vertexes.push_back({
		{-0.5,-0.5,0.5,1},
		{0,0,1,0},
		{0,0}
		});
	vertexes.push_back({
		{0.5,-0.5,0.5,1},
		{0,0,1,0},
		{1,0}
		});
	vertexes.push_back({
		{0.5,0.5,0.5,1},
		{0,0,1,0},
		{1,1}
		});
	vertexes.push_back({
		{0.5,0.5,0.5,1},
		{0,0,1,0},
		{1,1}
		});
	vertexes.push_back({
		{-0.5,0.5,0.5,1},
		{0,0,1,0},
		{0,1}
		});
	vertexes.push_back({
		{-0.5,-0.5,0.5,1},
		{0,0,1,0},
		{0,0}
		});

	vertexes.push_back({
		{-0.5,-0.5,-0.5,1},
		{-1,0,0,0},
		{0,0}
		});
	vertexes.push_back({
		{-0.5,-0.5,0.5,1},
		{-1,0,0,0},
		{0,1}
		});
	vertexes.push_back({
		{-0.5,0.5,0.5,1},
		{-1,0,0,0},
		{1,1}
		});
	vertexes.push_back({
		{-0.5,0.5,0.5,1},
		{-1,0,0,0},
		{1,1}
		});
	vertexes.push_back({
		{-0.5,0.5,-0.5,1},
		{-1,0,0,0},
		{1,0}
		});
	vertexes.push_back({
		{-0.5,-0.5,-0.5,1},
		{-1,0,0,0},
		{0,0}
		});

	vertexes.push_back({
		{0.5,-0.5,-0.5,1},
		{1,0,0,0},
		{1,0}
		});
	vertexes.push_back({
		{0.5,-0.5,0.5,1},
		{1,0,0,0},
		{1,1}
		});
	vertexes.push_back({
		{0.5,0.5,0.5,1},
		{1,0,0,0},
		{0,1}
		});
	vertexes.push_back({
		{0.5,0.5,0.5,1},
		{1,0,0,0},
		{0,1}
		});
	vertexes.push_back({
		{0.5,0.5,-0.5,1},
		{1,0,0,0},
		{0,0}
		});
	vertexes.push_back({
		{0.5,-0.5,-0.5,1},
		{1,0,0,0},
		{1,0}
		});

	std::vector<ArrayAttribute> attributes = {
		ArrayAttribute{0, 4, GL_FLOAT, 0},
		ArrayAttribute{1, 4, GL_FLOAT, sizeof(float) * 4},
		ArrayAttribute{2, 2, GL_FLOAT, sizeof(float) * 8}
	};

	createVbo(vertexes, attributes);
	setVertexNum(vertexes.size());
}

Sphere::Sphere(int vertexNum) : Geometry(GL_TRIANGLES)
{
	std::vector<BasicVertex> vertexes;
	std::vector<unsigned int> indexes;

	vertexes.push_back({ {0,-1,0,1}, {0,-1,0,0}, {0.0,0.0} });

	int circleNum = vertexNum / 2 + 1;
	for (int p = 1; p < circleNum; p++)
	{
		float fp = float(p) / circleNum;
		float pitch = (1.0f - fp) * PI;
		for (int q = 0; q < vertexNum; q++)
		{
			float fq = float(q) / vertexNum;
			float yaw = fq * 2 * PI;
			glm::vec4 normal(std::cosf(yaw) * std::sinf(pitch), std::cosf(pitch), std::sinf(yaw) * std::sinf(pitch), 0);
			vertexes.push_back({
				glm::vec4(0, 0, 0, 1) + normal,
				normal,
				{fq, fp}
				});
		}
	}
	vertexes.push_back({ {0,1,0,1}, {0,1,0,0}, {0.0,0.0} });

	int num = vertexes.size() - 1;

	for (int p = 0; p < circleNum; p++)
	{
		for (int q = 1; q <= vertexNum; q++)
		{
			indexes.push_back(std::min(p * vertexNum + q, num));
			indexes.push_back(std::max((p - 1) * vertexNum + q, 0));
			int next = q % vertexNum + 1;
			indexes.push_back(std::max((p - 1) * vertexNum + next, 0));
			indexes.push_back(std::max((p - 1) * vertexNum + next, 0));
			indexes.push_back(std::min(p * vertexNum + q, num));
			indexes.push_back(std::min(p * vertexNum + next, num));
		}
	}

	std::vector<ArrayAttribute> attributes = {
		ArrayAttribute{0, 4, GL_FLOAT, 0},
		ArrayAttribute{1, 4, GL_FLOAT, sizeof(float) * 4},
		ArrayAttribute{2, 2, GL_FLOAT, sizeof(float) * 8}
	};

	createVbo(vertexes, attributes);
	createIndexBuffer(indexes);
}

// Creates a rectangle with the bottom in the origo and pointing upwards, at the top there is a 4 sided pyramid pointing upwards.
renderer::Arrow4::Arrow4(float width, float baseHeight, float tipHeight) : Geometry(GL_TRIANGLES)
{
	std::vector<BasicVertex> vertexes;
	width *= 0.5f;

	vertexes.push_back({
		{-width,0.0,-width,1},
		{0,-1,0,0},
		{0,0}
		});
	vertexes.push_back({
		{-width,0.0,width,1},
		{0,-1,0,0},
		{0,1}
		});
	vertexes.push_back({
		{width,0.0,width,1},
		{0,-1,0,0},
		{1,1}
		});
	vertexes.push_back({
		{width,0.0,width,1},
		{0,-1,0,0},
		{1,1}
		});
	vertexes.push_back({
		{width,0.0,-width,1},
		{0,-1,0,0},
		{1,0}
		});
	vertexes.push_back({
		{-width,0.0,-width,1},
		{0,-1,0,0},
		{0,0}
		});

	vertexes.push_back({
		{-width,0.0,-width,1},
		{0,0,-1,0},
		{1,0}
		});
	vertexes.push_back({
		{width,0.0,-width,1},
		{0,0,-1,0},
		{0,0}
		});
	vertexes.push_back({
		{width,baseHeight,-width,1},
		{0,0,-1,0},
		{0,1}
		});
	vertexes.push_back({
		{width,baseHeight,-width,1},
		{0,0,-1,0},
		{0,1}
		});
	vertexes.push_back({
		{-width,baseHeight,-width,1},
		{0,0,-1,0},
		{1,1}
		});
	vertexes.push_back({
		{-width,0.0,-width,1},
		{0,0,-1,0},
		{1,0}
		});

	vertexes.push_back({
		{-width,0.0,width,1},
		{0,0,1,0},
		{0,0}
		});
	vertexes.push_back({
		{width,0.0,width,1},
		{0,0,1,0},
		{1,0}
		});
	vertexes.push_back({
		{width,baseHeight,width,1},
		{0,0,1,0},
		{1,1}
		});
	vertexes.push_back({
		{width,baseHeight,width,1},
		{0,0,1,0},
		{1,1}
		});
	vertexes.push_back({
		{-width,baseHeight,width,1},
		{0,0,1,0},
		{0,1}
		});
	vertexes.push_back({
		{-width,0.0,width,1},
		{0,0,1,0},
		{0,0}
		});

	vertexes.push_back({
		{-width,0.0,-width,1},
		{-1,0,0,0},
		{0,0}
		});
	vertexes.push_back({
		{-width,0.0,width,1},
		{-1,0,0,0},
		{0,1}
		});
	vertexes.push_back({
		{-width,baseHeight,width,1},
		{-1,0,0,0},
		{1,1}
		});
	vertexes.push_back({
		{-width,baseHeight,width,1},
		{-1,0,0,0},
		{1,1}
		});
	vertexes.push_back({
		{-width,baseHeight,-width,1},
		{-1,0,0,0},
		{1,0}
		});
	vertexes.push_back({
		{-width,0.0,-width,1},
		{-1,0,0,0},
		{0,0}
		});

	vertexes.push_back({
		{width,0.0,-width,1},
		{1,0,0,0},
		{1,0}
		});
	vertexes.push_back({
		{width,0.0,width,1},
		{1,0,0,0},
		{1,1}
		});
	vertexes.push_back({
		{width,baseHeight,width,1},
		{1,0,0,0},
		{0,1}
		});
	vertexes.push_back({
		{width,baseHeight,width,1},
		{1,0,0,0},
		{0,1}
		});
	vertexes.push_back({
		{width,baseHeight,-width,1},
		{1,0,0,0},
		{0,0}
		});
	vertexes.push_back({
		{width,0.0,-width,1},
		{1,0,0,0},
		{1,0}
		});

	// Tip
	const float tipWidth = width * 2.0f;
	vertexes.push_back({
		{-tipWidth,baseHeight,-tipWidth,1},
		{0,-1,0,0},
		{0,0}
		});
	vertexes.push_back({
		{-tipWidth,baseHeight,tipWidth,1},
		{0,-1,0,0},
		{0,1}
		});
	vertexes.push_back({
		{tipWidth,baseHeight,tipWidth,1},
		{0,-1,0,0},
		{1,1}
		});
	vertexes.push_back({
		{tipWidth,baseHeight,tipWidth,1},
		{0,-1,0,0},
		{1,1}
		});
	vertexes.push_back({
		{tipWidth,baseHeight,-tipWidth,1},
		{0,-1,0,0},
		{1,0}
		});
	vertexes.push_back({
		{-tipWidth,baseHeight,-tipWidth,1},
		{0,-1,0,0},
		{0,0}
		});

	const float tipNormalAngle = PI * 0.5f - std::atanf(tipHeight / tipWidth);
	const glm::vec2 tipNormal = glm::vec2(std::cosf(tipNormalAngle), std::sinf(tipNormalAngle));

	vertexes.push_back({
		{0,baseHeight + tipHeight,0,1},
		{tipNormal.x,tipNormal.y,0,0},
		{0.5f,0.5f}
		});
	vertexes.push_back({
		{tipWidth,baseHeight,-tipWidth,1},
		{tipNormal.x,tipNormal.y,0,0},
		{1,0}
		});
	vertexes.push_back({
		{tipWidth,baseHeight,tipWidth,1},
		{tipNormal.x,tipNormal.y,0,0},
		{1,1}
		});

	vertexes.push_back({
		{0,baseHeight + tipHeight,0,1},
		{-tipNormal.x,tipNormal.y,0,0},
		{0.5f,0.5f}
		});
	vertexes.push_back({
		{-tipWidth,baseHeight,-tipWidth,1},
		{-tipNormal.x,tipNormal.y,0,0},
		{0,0}
		});
	vertexes.push_back({
		{-tipWidth,baseHeight,tipWidth,1},
		{-tipNormal.x,tipNormal.y,0,0},
		{0,1}
		});

	vertexes.push_back({
		{0,baseHeight + tipHeight,0,1},
		{0,tipNormal.y,tipNormal.x,0},
		{0.5f,0.5f}
		});
	vertexes.push_back({
		{-tipWidth,baseHeight,tipWidth,1},
		{0,tipNormal.y,tipNormal.x,0},
		{0,1}
		});
	vertexes.push_back({
		{tipWidth,baseHeight,tipWidth,1},
		{0,tipNormal.y,tipNormal.x,0},
		{1,1}
		});

	vertexes.push_back({
		{0,baseHeight + tipHeight,0,1},
		{0,tipNormal.y,-tipNormal.x,0},
		{0.5f,0.5f}
		});
	vertexes.push_back({
		{-tipWidth,baseHeight,-tipWidth,1},
		{0,tipNormal.y,-tipNormal.x,0},
		{0,0}
		});
	vertexes.push_back({
		{tipWidth,baseHeight,-tipWidth,1},
		{0,tipNormal.y,-tipNormal.x,0},
		{1,0}
		});

	std::vector<ArrayAttribute> attributes = {
		ArrayAttribute{0, 4, GL_FLOAT, 0},
		ArrayAttribute{1, 4, GL_FLOAT, sizeof(float) * 4},
		ArrayAttribute{2, 2, GL_FLOAT, sizeof(float) * 8}
	};

	createVbo(vertexes, attributes);
	setVertexNum(vertexes.size());

}
