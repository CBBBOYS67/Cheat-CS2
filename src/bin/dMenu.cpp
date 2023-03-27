#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "imgui_internal.h"
#include "imgui.h"

#include "dMenu.h"
void DMenu::draw() {
	float screenSizeX = ImGui::GetIO().DisplaySize.x, screenSizeY = ImGui::GetIO().DisplaySize.y;
	ImGui::SetNextWindowSize(ImVec2(screenSizeX * 0.25, screenSizeY * 0.25), ImGuiCond_FirstUseEver);
	
	ImGui::Begin("dMenu");
	
	if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_FittingPolicyDefault_)) {
		// Render each tab as a button
		if (ImGui::TabItemButton("Trainer", currentTab == Trainer)) {
			currentTab = Trainer;
		}
		if (ImGui::TabItemButton("About", currentTab == About)) {
			currentTab = About;
		}
		if (ImGui::TabItemButton("Contact", currentTab == Contact)) {
			currentTab = Contact;
		}
		if (ImGui::TabItemButton("Help", currentTab == Help)) {
			currentTab = Help;
		}
		ImGui::EndTabBar();
	}
	
	// Render the content based on the currently selected tab
	switch (currentTab) {
	case Trainer:
		menuTrainer();
		break;
	case About:
		ImGui::Text("About us: we are a company that specializes in creating amazing software!");
		break;
	case Contact:
		ImGui::Text("Contact us at contact@company.com");
		break;
	case Help:
		ImGui::Text("Need help? Visit our support page at www.company.com/support");
		break;
	}

	ImGui::End();
}


void DMenu::menuTrainer() {
	ImGui::PushID("World");
	if (ImGui::CollapsingHeader("World")) {
		// Render the individual settings here...
		ImGui::SliderFloat("Time", Trainer::getTimeOfDay(), 0.0f, 24.f);
		//ImGui::Checkbox("Enable Notifications", &enableNotifications);
		// ...
	}
	ImGui::PopID();
}

float* DMenu::Trainer::getTimeOfDay()
{
	return &(RE::Calendar::GetSingleton()->gameHour->value);
}


void DMenu::Trainer::setTimeOfDay(float a_in) {
	RE::Calendar::GetSingleton()->gameHour->value = a_in;
}
