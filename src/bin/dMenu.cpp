#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "imgui_internal.h"
#include "imgui.h"

#include "dMenu.h"
#include "menus/Trainer.h"
#include "menus/AIM.h"
#include "menus/Settings.h"
#include "menus/ModSettings.h"

#include "Renderer.h"
void DMenu::draw() 
{
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
	if (Settings::lockWindowPos) {
		windowFlags |= ImGuiWindowFlags_NoMove;
	}
	if (Settings::lockWindowSize) {
		windowFlags |= ImGuiWindowFlags_NoResize;
	}
	ImGui::Begin("dMenu", NULL, windowFlags);
	
	if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
		// Render each tab as a button
		if (ImGui::TabItemButton("Trainer", ImGuiTabItemFlags_Trailing)) {
			currentTab = Trainer;
		}
		if (ImGui::TabItemButton("AIM", ImGuiTabItemFlags_Trailing)) {
			currentTab = AIM;
		}
		if (ImGui::TabItemButton("Settings", ImGuiTabItemFlags_Trailing)) {
			currentTab = Settings;
		}
		if (ImGui::TabItemButton("Mod Settings", ImGuiTabItemFlags_Trailing)) {
			currentTab = ModSettings;
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
	case ModSettings:
		ModSettings::show();
		break;
	case Settings:
		Settings::show();
		break;

	}

	ImGui::End();
}

ImVec2 DMenu::relativeSize(float a_width, float a_height)
{
	ImVec2 parentSize = ImGui::GetMainViewport()->Size;
	return ImVec2(a_width * parentSize.x, a_height * parentSize.y);
}


void DMenu::init(float a_screenWidth, float a_screenHeight)
{
	INFO("Initializing DMenu");
	AIM::init();
	Trainer::init();
	ModSettings::init();
	Settings::init();

	ImVec2 mainWindowSize = { float(a_screenWidth * Settings::relative_window_size_h), float(a_screenHeight * Settings::relative_window_size_v) };
	ImGui::SetNextWindowSize(mainWindowSize, ImGuiCond_FirstUseEver);
	ImVec2 mainWindowPos = { Settings::windowPos_x, Settings::windowPos_y };
	ImGui::SetNextWindowSize(mainWindowPos, ImGuiCond_FirstUseEver);

	INFO("DMenu initialized");
	

	initialized = true;
}

