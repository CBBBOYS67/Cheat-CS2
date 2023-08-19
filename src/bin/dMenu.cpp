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
	ImVec2 screenSize = ImGui::GetMainViewport()->Size;
	float screenSizeX = screenSize.x;
	float screenSizeY = screenSize.y;
	ImGui::SetNextWindowSize(ImVec2(Settings::relative_window_size_h * screenSizeX, Settings::relative_window_size_v * screenSizeY));
	
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
	if (Settings::lockWindowPos) {
		windowFlags |= ImGuiWindowFlags_NoMove;
	}
	if (Settings::lockWindowSize) {
		windowFlags |= ImGuiWindowFlags_NoResize;
	}
	ImGui::Begin("dMenu", NULL, windowFlags);
	
	if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
		if (ImGui::BeginTabItem("Trainer")) {
			currentTab = Trainer;
			Trainer::show();
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("AIM")) {
			currentTab = Tab::AIM;
			AIM::show();
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Mod Config")) {
			currentTab = Tab::ModSettings;
			ModSettings::show();
			ImGui::EndTabItem();
		}
		
		if (ImGui::BeginTabItem("Settings")) {
			currentTab = Tab::Settings;
			Settings::show();
			ImGui::EndTabItem();
		}
		
		

		ImGui::EndTabBar();
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

	ImVec2 mainWindowSize = { float(a_screenWidth * Settings::relative_window_size_h), float(a_screenHeight * Settings::relative_window_size_v) };
	ImGui::SetNextWindowSize(mainWindowSize, ImGuiCond_FirstUseEver);
	ImVec2 mainWindowPos = { Settings::windowPos_x, Settings::windowPos_y };
	ImGui::SetNextWindowSize(mainWindowPos, ImGuiCond_FirstUseEver);
	ImGui::GetIO().FontGlobalScale = Settings::fontScale;

	INFO("DMenu initialized");
	

	initialized = true;
}

