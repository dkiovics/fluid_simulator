#include "simParamsAdvanced.h"
#include <glm/glm.hpp>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"


//void showSimParamsAdvanced(genericfsim::manager::SimulationConfig& config)
//{
//	ImGui::Text("Advanced Simulation Parameters");
//	ImGui::Separator();
//	ImGui::Checkbox("Gravity Enabled", &config.simulatorConfig.gravityEnabled);
//	ImGui::SliderFloat("Gravity", &config.simulatorConfig.gravity, 0.0f, 1000.0f);
//	ImGui::Checkbox("Push Particles Apart Enabled", &config.pushParticlesApartEnabled);
//	ImGui::Checkbox("Push Apart Enabled", &config.pushApartEnabled);
//	ImGui::Checkbox("Particle Spawning Enabled", &config.particleSpawningEnabled);
//	ImGui::Checkbox("Particle Despawning Enabled", &config.particleDespawningEnabled);
//	ImGui::Checkbox("Stop Particles", &config.stopParticles);
//	ImGui::Separator();
//	ImGui::Text("Particle-Grid-Particle Transfer Type");
//	ImGui::RadioButton("PIC", (int*)&config.transferType, (int)genericfsim::simulator::Simulator::P2G2PType::PIC);
//	ImGui::SameLine();
//	ImGui::RadioButton("FLIP", (int*)&config.transferType, (int)genericfsim::simulator::Simulator::P2G2PType::FLIP);
//	ImGui::SameLine();
//	ImGui::RadioButton("APIC", (int*)&config.transferType, (int)genericfsim::simulator::Simulator::P2G2PType::APIC);
//	ImGui::SliderFloat("FLIP Ratio", &config.flipRatio, 0.0f, 1.0f);
//	ImGui::Separator();
//	ImGui::Text("Particle-Grid-Particle Transfer Type");
//	ImGui::RadioButton("PIC", (int*)&config.transferType, (int)genericfsim::simulator::Simulator::P2G2PType::PIC);
//	ImGui::SameLine();
//	ImGui::RadioButton("FLIP", (int*)&config.transferType, (int)genericfsim::simulator::Simulator::P2G2PType::FLIP);
//	ImGui::SameLine();
//	ImGui::RadioButton("APIC", (int*)&config.transferType, (int)genericfsim::simulator::Simulator::P2G2PType::APIC);
//	ImGui::SliderFloat("FLIP Ratio", &config.flipRatio, 0.0f, 1.0f);
//	ImGui::Separator();
//	ImGui::Text("Particle-Grid-Particle Transfer Type");
//	ImGui::RadioButton("PIC", (int*)&config.transferType, (int)genericfsim::simulator::Simulator::P2G2PType::)
//}
