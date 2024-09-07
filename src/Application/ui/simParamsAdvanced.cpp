#include "simParamsAdvanced.h"
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>


using namespace genericfsim::manager;
using namespace genericfsim::simulator;

void showSimParamsAdvanced(genericfsim::manager::SimulationConfig& config, int screenWidth, SimulationManager& simulationManager)
{
	ImGui::Begin("Advanced simulation parameters");
	ImGui::Separator();
	ImGui::RadioButton("PIC", (int*)&config.simulatorConfig.transferType, (int)Simulator::P2G2PType::PIC);
	ImGui::SameLine();
	ImGui::RadioButton("FLIP", (int*)&config.simulatorConfig.transferType, (int)Simulator::P2G2PType::FLIP);
	ImGui::SameLine();
	ImGui::RadioButton("APIC", (int*)&config.simulatorConfig.transferType, (int)Simulator::P2G2PType::APIC);
	ImGui::SameLine();
	ImGui::Checkbox("Stop particles", &config.simulatorConfig.stopParticles);
	ImGui::SameLine();
	ImGui::Checkbox("Top is solid", &config.isTopOfContainerSolid);
	ImGui::RadioButton("Bridson solver", (int*)&config.gridSolverType, static_cast<int>(SimulationConfig::GridSolverType::BRIDSON));
	ImGui::SameLine();
	ImGui::RadioButton("Basic solver", (int*)&config.gridSolverType, static_cast<int>(SimulationConfig::GridSolverType::BASIC));
	if (config.gridSolverType == SimulationConfig::GridSolverType::BRIDSON)
	{
		ImGui::SetNextItemWidth(screenWidth * 0.18f);
		ImGui::SliderFloat("density", &config.fluidDensity, 0.1f, 30.0f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(screenWidth * 0.18f);
		ImGui::SliderFloat("solver tolerance", &config.residualTolerance, 1e-8f, 1e-4f, "%e");
	}

	ImGui::Checkbox("Gravity", &config.simulatorConfig.gravityEnabled);
	ImGui::SameLine(0, 30);
	ImGui::Checkbox("Push apart", &config.simulatorConfig.pushApartEnabled);
	ImGui::SameLine(0, 30);
	ImGui::Checkbox("Pressure", &config.pressureEnabled);

	ImGui::Separator();
	ImGui::SetNextItemWidth(screenWidth * 0.40f);
	ImGui::SliderFloat("Grid resolution", &config.gridResolution, 0.6, 10);
	ImGui::Text("Gridsize x: %d  y: %d  z: %d", simulationManager.getGridSize().x, simulationManager.getGridSize().y, simulationManager.getGridSize().z);
	ImGui::SetNextItemWidth(screenWidth * 0.40f);
	ImGui::SliderFloat("Particle radius", &config.particleRadius, 0.04f, 1.0f);
	ImGui::SetNextItemWidth(screenWidth * 0.40f);
	ImGui::SliderInt("N", &config.incompressibilityIterationCount, 1, 600);
	ImGui::SetNextItemWidth(screenWidth * 0.40f);
	ImGui::SliderFloat("Average P", &config.averagePressure, 0.01f, 20.0f);
	ImGui::SetNextItemWidth(screenWidth * 0.40f);
	ImGui::SliderFloat("Pressure k", &config.pressureK, 0.5f, 10.0f);
	ImGui::SetNextItemWidth(screenWidth * 0.40f);
	ImGui::SliderFloat("G", &config.simulatorConfig.gravity, -0.01f, -1000.0f);
	ImGui::SetNextItemWidth(screenWidth * 0.40f);
	ImGui::SliderFloat("Flip", &config.simulatorConfig.flipRatio, 0.0f, 1.0f);

	ImGui::End();
}
