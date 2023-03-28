#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "imgui_internal.h"
#include "imgui.h"

#include "dMenu.h"
#include "menus/Trainer.h"
#include "menus/AIM.h"

#include "Renderer.h"
void DMenu::draw() 
{
	ImGui::SetNextWindowSize(mainWindowSize, ImGuiCond_FirstUseEver);
	
	ImGui::Begin("dMenu");
	
	if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
		// Render each tab as a button
		if (ImGui::TabItemButton("Trainer", currentTab == Trainer)) {
			currentTab = Trainer;
		}
		if (ImGui::TabItemButton("AIM", currentTab == AIM)) {
			currentTab = AIM;
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
	mainWindowSize = { float(a_screenWidth * 0.5), float(a_screenHeight * 0.7) };
	AIM::init();
	INFO("DMenu initialized");
	
	initialized = true;
}

