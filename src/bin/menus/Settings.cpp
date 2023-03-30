#include "imgui.h"
#include "imgui_internal.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "Settings.h"

namespace Settings
{
	namespace Localization
	{
		void init()
		{

		}

		void show()
		{

		}
	}

	void show() 
	{
		// Use consistent padding and alignment
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));

		// Indent the contents of the window
		ImGui::Indent();
		ImGui::Spacing();

		// Display Localization controls
		ImGui::Text("Localization:");
		Localization::show();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();


		// Unindent the contents of the window
		ImGui::Unindent();

		// Use consistent padding and alignment
		ImGui::PopStyleVar(2);
	}

	void init()
	{
		Localization::init();
		INFO("Settings initialized.");
	}
}
