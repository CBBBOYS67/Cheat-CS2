#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "imgui_internal.h"
#include "imgui.h"

#include "dMenu.h"
#include "menus/Trainer.h"
#include "menus/AIM.h"
#include "menus/Settings.h"

#include "Renderer.h"
void DMenu::draw() 
{
	ImGui::SetNextWindowSize(mainWindowSize, ImGuiCond_FirstUseEver);
	
	ImGui::Begin("dMenu");
	
	if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
		// Render each tab as a button
		if (ImGui::TabItemButton("Trainer", ImGuiTabItemFlags_None)) {
			currentTab = Trainer;
		}
		if (ImGui::TabItemButton("AIM", ImGuiTabItemFlags_None)) {
			currentTab = AIM;
		}
		if (ImGui::TabItemButton("Settings", ImGuiTabItemFlags_Trailing)) {
			currentTab = Settings;
		}
		ImGui::EndTabBar();
	}
	
	// Render the content based on the currently selected tab
	switch (currentTab) {
	case Trainer:
		Trainer::show();
		break;
	case AIM:
		AIM::show();
		break;
	}

	ImGui::End();
}

ImVec2 DMenu::relativeSize(float a_width, float a_height)
{
	return ImVec2(a_width * mainWindowSize.x, a_height * mainWindowSize.y);
}


void DMenu::init(float a_screenWidth, float a_screenHeight)
{
	INFO("Initializing DMenu");
	AIM::init();
	Trainer::init();
	Settings::init();

	mainWindowSize = { float(a_screenWidth * Settings::relative_window_size_h), float(a_screenHeight * Settings::relative_window_size_v) };
	INFO("DMenu initialized");
	

	initialized = true;
}

