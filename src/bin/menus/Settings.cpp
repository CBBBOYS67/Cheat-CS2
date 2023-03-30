#include "imgui.h"
#include "imgui_internal.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "Settings.h"
#include "bin/Utils.h"

#define SETTINGFILE_PATH "Data\\SKSE\\Plugins\\dmenu.ini"

namespace ini
{
	void flush()
	{
		settingsLoader loader(SETTINGFILE_PATH);
		loader.setActiveSection("WindowSize");
		loader.save(Settings::relative_window_size_h, "relative_window_size_h");
		loader.save(Settings::relative_window_size_v, "relative_window_size_v");
	}

	void load()
	{
		settingsLoader loader(SETTINGFILE_PATH);
		loader.setActiveSection("WindowSize");
		loader.load(Settings::relative_window_size_h, "relative_window_size_h");
		loader.load(Settings::relative_window_size_v, "relative_window_size_v");
	}

	void init()
	{
		load(); //note: load() uses another simpleinia boj just for loading
	}
}

namespace WindowSize
{
	void init()
	{
	}

	void show()
	{
		ImVec2 parentSize = ImGui::GetMainViewport()->Size;

		// Calculate the relative sizes
		Settings::relative_window_size_h = ImGui::GetWindowWidth() / parentSize.x;
		Settings::relative_window_size_v = ImGui::GetWindowHeight() / parentSize.y;

		// Display the relative sizes in real-time
		ImGui::Text("Width: %.2f%%", Settings::relative_window_size_h * 100.0f);
		ImGui::SameLine();
		ImGui::Text("Height: %.2f%%", Settings::relative_window_size_v * 100.0f);
	}
}

namespace Localization
{
	void init()
	{
	}

	void show()
	{

	}
}

void Settings::show() 
{
	// Use consistent padding and alignment
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));

	// Indent the contents of the window
	ImGui::Indent();
	ImGui::Spacing();

	if (ImGui::Button("Save")) {
		ini::flush();
	}

	// Display size controls
	ImGui::Text("Window Size:");
	WindowSize::show();
	ImGui::Spacing();
	ImGui::Separator();
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

void Settings::init()
{
	ini::init();
	WindowSize::init();
	Localization::init();
		
	INFO("Settings initialized.");
}
