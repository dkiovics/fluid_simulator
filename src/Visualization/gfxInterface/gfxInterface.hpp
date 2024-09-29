#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <manager/simulationManager.h>
#include <param.hpp>


namespace visual
{

class GfxInterface : public ParamLineCollection {
public:
	/**
	 * \brief The start of the screen in pixels, (0,0) is the top left corner.
	 * Must be kept in sync with the actual window size.
	 */
	glm::ivec2 screenStart = glm::ivec2(0, 0);
	/**
	 * \brief The size of the screen in pixels.
	 * Must be kept in sync with the actual window size.
	 */
	glm::ivec2 screenSize = glm::ivec2(1000, 1000);

	virtual void setNewSimulationManager(std::shared_ptr<genericfsim::manager::SimulationManager> manager) = 0;

	virtual void handleTimePassed(double dt) { }

	virtual void render() = 0;

	virtual void addSphericalObstacle(glm::vec3 color, float r) { }
	virtual void addRectengularObstacle(glm::vec3 color, glm::vec3 size) { }
	virtual void removeObstacle() { }

	virtual ~GfxInterface() = default;

protected:
	ParamBool gridlinesEnabled = ParamBool("Gridlines", false);
	ParamColor gridLineColor = ParamColor("Gridline color", glm::vec3(0.4f, 0.0f, 0.0f));
	ParamColor particleColor = ParamColor("Particle color", glm::vec3(0.0, 0.4, 0.95));
	ParamColor particleSpeedColor = ParamColor("Particle speed color", glm::vec3(0.4, 0.93, 0.88));

	ParamFloat maxParticleSpeed = ParamFloat("Max particle speed", 36.0f, 1.0f, 200.0f);
	ParamBool particleSpeedColorEnabled = ParamBool("Particle speed color", true);

	GfxInterface()
	{
		addParamLine(ParamLine({ &gridlinesEnabled, &gridLineColor }));
		addParamLine(ParamLine({ &particleSpeedColorEnabled, &maxParticleSpeed }));
		addParamLine(ParamLine({ &particleColor, &particleSpeedColor }));
	}
};

} // namespace vis