#include "controlElements.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "spdlog/spdlog.h"
#include "controlRegistryPrivate.h"
#include "controlCollection.h"

using namespace controls;

struct _ApplicationControl
{
	CheckBox is2D{ "app.is_2d", "2D" };
	CheckBox is2DSim{ "app.is_2d_sim", "2D Sim" };
};

struct _SimulationControl
{
	CheckBox runSimulation{ "sim.run", "Run" };
	CheckBox autoDt{ "sim.auto_dt", "Auto dt", true };
	Button reset{ "sim.restart", "Restart sim" };
	Button step{ "sim.step", "Step sim" };
	SliderFloat dt{ "sim.dt", "dt", 0.001f, 0.1f, 0.01 };
	SliderInt particleCount{ "sim.particle_count", "Particle Count", 1, 100000, 20000 };
	CheckBox showAdvanced{ "sim.show_advanced", "Show advanced params" };
};

struct _SimulationAdvancedControl
{
	RadioButton transferType{ "sim.transfer_type", {"PIC", "FLIP", "APIC"} };
	CheckBox stopParticles{ "sim.stop_particles", "Stop particles" };
	CheckBox topIsSolid{ "sim.top_is_solid", "Top is solid" };
	RadioButton gridSolver{ "sim.grid_solver", {"Bridson", "Basic"} };
	SliderFloat density{ "sim.solver_density", "Density", 0.1f, 30.0f, 1.0f };
	SliderFloat solverTolerance{ "sim.solver_tolerance", "Solver tolerance", 1e-8f, 1e-4f, 1e-6f, "%e" };
	CheckBox gravity{ "sim.gravity", "Gravity", true };
	CheckBox pushApart{ "sim.push_apart", "Push apart" };
	CheckBox pressure{ "sim.pressure", "Pressure", true };
	SliderFloat gridResolution{ "sim.grid_resolution", "Grid resolution", 0.6f, 10.0f, 1.508f };
	SliderFloat particleRadius{ "sim.particle_radius", "Particle radius", 0.04f, 1.0f, 0.182f };
	SliderInt incompressibilityIterations{ "sim.incompressibility_iterations", "N", 1, 600, 80 };
	SliderFloat averagePressure{ "sim.average_pressure", "Average P", 0.01f, 20.0f, 5.43f };
	SliderFloat pressureK{ "sim.pressure_k", "Pressure k", 0.5f, 10.0f, 1.23f };
	SliderFloat gravityValue{ "sim.gravity_value", "G", -0.01f, -1000.0f, -250.0f };
	SliderFloat flip{ "sim.flip", "Flip", 0.0f, 1.0f, 0.85f };
};

struct _SimulationObstacleControl
{
	CheckBox showObstacles{ "obs.show_obstacles", "Show obstacles", true };
	Button addSphere{ "obs.add_sphere", "Add sphere" };
	Button addRectangle{ "obs.add_rectangle", "Add rectangle" };
	/*Button addParticleSource{ "obs.add_particle_source", "Add particle source" };
	Button addParticleSink{ "obs.add_particle_sink", "Add particle sink" };*/
	Button addMesh{ "obs.add_mesh", "Add mesh" };
	Button remove{ "obs.remove", "Remove" };
	TextInput meshFile{ "obs.mesh_file", "File path", "meshes/" };
	ColorPicker color{ "obs.color", "Color", glm::vec3(0.9f, 0.6f, 0.2f) };
	SliderFloat obstacleRadius{ "obs.obstacle_radius", "Obstacle radius", 0.5f, 20.0f, 6.0f };
	SliderFloat obstacleX{ "obs.obstacle_x", "Obstacle X", 1.0f, 40.0f, 6.0f };
	SliderFloat obstacleY{ "obs.obstacle_y", "Obstacle Y", 1.0f, 40.0f, 6.0f };
	SliderFloat obstacleZ{ "obs.obstacle_z", "Obstacle Z", 1.0f, 40.0f, 6.0f };
	SliderFloat meshScale{ "obs.mesh_scale", "Mesh scale", 0.5f, 20.0f, 6.0f };
	/*SliderFloat spawnRate{ "obs.spawn_rate", "Spawn rate", 1.0f, 5000.0f, 1600.0f };
	SliderFloat spawnSpeed{ "obs.spawn_speed", "Spawn speed", 0.1f, 10.0f, 4.0f };
	CheckBox enableSpawn{ "obs.enable_spawn", "Enable spawn" };
	CheckBox enableDespawn{ "obs.enable_despawn", "Enable despawn" };*/
};

struct _RenderingSettings
{
	CheckBox showBox{ "render.show_box", "Show box" };
	CheckBox showGrid{ "render.show_grid", "Show grid" };
	RadioButton fluidRenderMode{ "render.fluid_render_mode", { "None", "Particles", "Surface" }, 2 };
};

struct _ParticleRendeingSettings
{
	CheckBox particleSpeedColorEnabled{ "render.particle_speed_color_en", "Particle speed color", true };
	SliderFloat maxParticleSpeed{ "render.max_particle_speed", "Max particle speed", 1.0f, 200.0f, 36.0f };
	ColorPicker particleColor{ "render.particle_color", "Particle color", glm::vec3(0.0f, 0.4f, 0.95f) };
	ColorPicker particleSpeedColor{ "render.particle_speed_color", "Particle speed color", glm::vec3(0.4f, 0.93f, 0.88f) };
	SliderFloat speedColorExp{ "render.particle_speed_color_exp", "Speed color exponent", 0.0f, 10.0f, 1.0f };
};

struct _SurfaceRenderingSettings
{
	ColorPicker surfaceColor{ "render.surface_color", "Surface color", glm::vec3(0.0f, 0.4f, 0.95f) };
	CheckBox bilateralFilterEnabled{ "render.bilateral_filter_en", "Bilateral filter", true };
	SliderFloat smoothingSize{ "render.smoothing_size", "Smoothing kernel size", 0.01f, 6.0f, 1.7f };
	SliderFloat blurScale{ "render.blur_scale", "Blur scale", 0.01f, 0.4f, 0.06f };
	SliderFloat blurDepthFalloff{ "render.blur_depth_falloff", "Blur depth falloff", 100.0f, 10000.0f, 1900.0f };
	CheckBox sprayEnabled{ "render.spray_en", "Spray", true };
	SliderFloat sprayDensityThreshold{ "render.spray_density_threshold", "Spray density threshold", 0.0f, 10.0f, 1.2f };
	CheckBox fluidTransparencyEnabled{ "render.fluid_transparency_en", "Transparent fluid", true };
	SliderFloat fluidTransparencyBlurSize{ "render.fluid_transparency_blur_size", "Fluid thickness blur size", 0.01f, 6.0f, 2.4f };
	SliderFloat fluidTransparency{ "render.fluid_transparency", "Fluid transparency", 0.0f, 3.0f, 1.334f };
	CheckBox fluidSurfaceNoiseEnabled{ "render.fluid_surface_noise_en", "Fluid surface noise", true };
	SliderFloat fluidSurfaceNoiseScale{ "render.fluid_surface_noise_scale", "Noise scale", 0.01f, 5.0f, 0.956f };
	SliderFloat fluidSurfaceNoiseStrength{ "render.fluid_surface_noise_strength", "Noise strength", 0.0f, 1.0f, 0.123f };
	SliderFloat fluidSurfaceNoiseSpeed{ "render.fluid_surface_noise_speed", "Noise speed", 0.0f, 1.0f, 0.317f };
};

namespace controls
{

class _ControlCollectionPrivate
{
private:
	_ControlRegistryPrivate& registry;
	_ApplicationControl appControl;
	_SimulationControl simControl;
	_SimulationAdvancedControl simAdvControl;
	_SimulationObstacleControl simObsControl;
	_RenderingSettings renderControl;
	_ParticleRendeingSettings particleRenderControl;
	_SurfaceRenderingSettings surfaceRenderControl;

private:
	void drawApplicationControls();
	void drawSimulationControls();
	void drawSimulationAdvancedControls();
	void drawSimulationObstacleControls();
	void drawRenderingControls();

public:
	_ControlCollectionPrivate()
		: registry(_ControlRegistryPrivate::getInstance())
	{ }

	void draw();
};

}

void _ControlCollectionPrivate::drawApplicationControls()
{
	ImGui::Begin("General controls");
	appControl.is2D.draw();
	ImGui::SameLine();
	appControl.is2DSim.draw();
	ImGui::SameLine();
	ImGui::Text("FPS: %.1lf", registry.getOrDefault("telemetry.fps", 0.0f));
	ImGui::End();
}

void _ControlCollectionPrivate::drawSimulationControls()
{
	ImGui::Begin("Simulation controls");
	simControl.runSimulation.draw();
	ImGui::SameLine();
	simControl.reset.draw();
	ImGui::SameLine();
	simControl.autoDt.draw();
	ImGui::SameLine();
	simControl.step.draw();
	if (!registry.get<bool>("sim.auto_dt"))
		simControl.dt.draw();
	simControl.particleCount.draw();
	simControl.showAdvanced.draw();
	ImGui::End();
}

void _ControlCollectionPrivate::drawSimulationAdvancedControls()
{
	if (registry.get<bool>("sim.show_advanced"))
	{
		ImGui::Begin("Advanced simulation controls");
		simAdvControl.gravity.draw();
		ImGui::SameLine();
		simAdvControl.stopParticles.draw();
		ImGui::SameLine();
		simAdvControl.topIsSolid.draw();
		ImGui::SameLine();
		simAdvControl.pushApart.draw();

		simAdvControl.gridResolution.draw();
		simAdvControl.particleRadius.draw();
		simAdvControl.gravityValue.draw();

		simAdvControl.transferType.draw();
		simAdvControl.flip.draw();

		simAdvControl.gridSolver.draw();

		simAdvControl.density.draw();
		simAdvControl.pressure.draw();
		simAdvControl.solverTolerance.draw();
		simAdvControl.incompressibilityIterations.draw();
		simAdvControl.averagePressure.draw();
		simAdvControl.pressureK.draw();
		ImGui::End();
	}
}

void _ControlCollectionPrivate::drawSimulationObstacleControls()
{
	ImGui::Begin("Obstacle controls");
	simObsControl.showObstacles.draw();
	ImGui::SameLine();
	simObsControl.addSphere.draw();
	ImGui::SameLine();
	simObsControl.addRectangle.draw();
	/*ImGui::SameLine();
	simObsControl.addParticleSource.draw();
	ImGui::SameLine();
	simObsControl.addParticleSink.draw();*/
	ImGui::SameLine();
	simObsControl.remove.draw();

	simObsControl.addMesh.draw();
	ImGui::SameLine();
	simObsControl.meshFile.draw();
	ImGui::SameLine();
	simObsControl.color.draw();

	simObsControl.obstacleRadius.draw();
	simObsControl.obstacleX.draw();
	simObsControl.obstacleY.draw();
	simObsControl.obstacleZ.draw();
	simObsControl.meshScale.draw();

	/*simObsControl.enableDespawn.draw();
	ImGui::SameLine();
	simObsControl.enableSpawn.draw();
	simObsControl.spawnRate.draw();
	simObsControl.spawnSpeed.draw();*/

	ImGui::End();
}

void _ControlCollectionPrivate::drawRenderingControls()
{
	ImGui::Begin("Rendering controls");
	renderControl.showBox.draw();
	ImGui::SameLine();
	renderControl.fluidRenderMode.draw();
	ImGui::SameLine();
	renderControl.showGrid.draw();

	if (registry.get<int>("render.fluid_render_mode") == 1)
	{
		particleRenderControl.particleSpeedColorEnabled.draw();
		particleRenderControl.speedColorExp.draw();
		particleRenderControl.maxParticleSpeed.draw();
		particleRenderControl.particleColor.draw();
		particleRenderControl.particleSpeedColor.draw();
	}
	else if (registry.get<int>("render.fluid_render_mode") == 2)
	{
		surfaceRenderControl.bilateralFilterEnabled.draw();
		ImGui::SameLine();
		surfaceRenderControl.sprayEnabled.draw();
		ImGui::SameLine();
		surfaceRenderControl.fluidTransparencyEnabled.draw();
		ImGui::SameLine();
		surfaceRenderControl.fluidSurfaceNoiseEnabled.draw();
		ImGui::SameLine();
		surfaceRenderControl.surfaceColor.draw();

		surfaceRenderControl.smoothingSize.draw();
		surfaceRenderControl.blurScale.draw();
		surfaceRenderControl.blurDepthFalloff.draw();
		surfaceRenderControl.sprayDensityThreshold.draw();
		surfaceRenderControl.fluidTransparencyBlurSize.draw();
		surfaceRenderControl.fluidTransparency.draw();
		surfaceRenderControl.fluidSurfaceNoiseScale.draw();
		surfaceRenderControl.fluidSurfaceNoiseStrength.draw();
		surfaceRenderControl.fluidSurfaceNoiseSpeed.draw();
	}
	ImGui::End();
}

void _ControlCollectionPrivate::draw()
{
	drawApplicationControls();
	drawSimulationControls();
	if (registry.get<bool>("sim.show_advanced"))
		drawSimulationAdvancedControls();
	drawSimulationObstacleControls();
	drawRenderingControls();
}

controls::_ControlCollection::_ControlCollection() : data(new _ControlCollectionPrivate())
{ }

controls::_ControlCollection::~_ControlCollection()
{ }

void controls::_ControlCollection::draw()
{
	data->draw();
}
