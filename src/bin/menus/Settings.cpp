#include "imgui.h"
#include "imgui_internal.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "Settings.h"
#include "bin/Utils.h"

#define SETTINGFILE_PATH "Data\\SKSE\\Plugins\\dmenu\\dmenu.ini"

namespace ini
{

	void flush()
	{
		settingsLoader loader(SETTINGFILE_PATH);
		loader.setActiveSection("UI");
		loader.save(Settings::relative_window_size_h, "relative_window_size_h");
		loader.save(Settings::relative_window_size_v, "relative_window_size_v");
		loader.save(Settings::windowPos_x, "windowPos_x");
		loader.save(Settings::windowPos_y, "windowPos_y");
		loader.save(Settings::lockWindowSize, "lockWindowSize");
		loader.save(Settings::lockWindowPos, "lockWindowPos");

		loader.setActiveSection("Localization");
		uint32_t lan = static_cast<uint32_t>(Settings::currLanguage);
		loader.save(lan, "language");

		loader.flush();
	}

	void load()
	{
		settingsLoader loader(SETTINGFILE_PATH);
		loader.setActiveSection("UI");
		loader.load(Settings::relative_window_size_h, "relative_window_size_h");
		loader.load(Settings::relative_window_size_v, "relative_window_size_v");
		loader.load(Settings::windowPos_x, "windowPos_x");
		loader.load(Settings::windowPos_y, "windowPos_y");
		loader.load(Settings::lockWindowSize, "lockWindowSize");
		loader.load(Settings::lockWindowPos, "lockWindowPos");

		loader.setActiveSection("Localization");
		uint32_t lan;
		loader.load(lan, "language");
		Settings::currLanguage = static_cast<Translator::Language>(lan);
	}

	void init()
	{
		load();
	}
}

namespace UI
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

		auto windowPos = ImGui::GetWindowPos();
		// get windowpos
		Settings::windowPos_x = windowPos.x;
		Settings::windowPos_y = windowPos.y;

		// Display the relative sizes in real-time
		ImGui::Text("Width: %.2f%%", Settings::relative_window_size_h * 100.0f);
		ImGui::SameLine();
		ImGui::Text("Height: %.2f%%", Settings::relative_window_size_v * 100.0f);

		//ImGui::SameLine(ImGui::GetWindowWidth() - 100);
		ImGui::Checkbox("Lock Size", &Settings::lockWindowSize);
		//ImGui::PopStyleVar();

		// Display the relative positions in real-time
		ImGui::Text("X pos: %f", Settings::windowPos_x);
		ImGui::SameLine();
		ImGui::Text("Y pos: %f", Settings::windowPos_y);

		//ImGui::SameLine(ImGui::GetWindowWidth() - 100);
		ImGui::Checkbox("Lock Pos", &Settings::lockWindowPos);
		//ImGui::PopStyleVar();

		if (ImGui::SliderFloat("Font Size", &Settings::fontScale, 0.5f, 2.f)) {
			ImGui::GetIO().FontGlobalScale = Settings::fontScale;
		}
		ImGui::SameLine();
	}
}

namespace Localization
{
	std::vector<std::pair<Translator::Language, std::string>> _languages = {
		{Translator::Language::English, "English"},
		{Translator::Language::German, "German"},
		{Translator::Language::French, "French"},
		{Translator::Language::Spanish, "Spanish"},
		{Translator::Language::Italian, "Italian"},
		{Translator::Language::Russian, "Russian"},
		{Translator::Language::Polish, "Polish"},
		{Translator::Language::Portuguese, "Portuguese"},
		{Translator::Language::Japanese, "Japanese"},
		{Translator::Language::Chinese, "Chinese"},
		{Translator::Language::Korean, "Korean"},
		{Translator::Language::Turkish, "Turkish"},
		{Translator::Language::Arabic, "Arabic"},
	};
	void init()
	{
	}

	void show()
	{
		if (ImGui::BeginCombo("Language", _languages[(int)Settings::currLanguage].second.c_str())) {
			for (int i = 0; i < _languages.size(); i++) {
				bool isSelected = ((int)Settings::currLanguage == i);
				if (ImGui::Selectable(_languages[i].second.c_str(), isSelected)) {
					Settings::currLanguage = static_cast<Translator::Language>(i);
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		Utils::imgui::HoverNote("Language change takes place after game re-start.", "(!)");

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

	// Display size controls
	ImGui::Text("UI");
	UI::show();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Display Localization controls
	ImGui::Text("Localization");
	Localization::show();
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Unindent the contents of the window
	ImGui::Unindent();

	// Position the "Save" button at the bottom-right corner of the window
	ImGui::SameLine(ImGui::GetWindowWidth() - 100);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

	// Set the button background and text colors
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));


	if (ImGui::Button("Save", ImVec2(100, 30))) {
		ini::flush();
	}

	// Restore the previous style colors
	ImGui::PopStyleColor(4);

	// Use consistent padding and alignment
	ImGui::PopStyleVar(2);
}

void Settings::init()
{
	ini::init(); // load all settings first
	UI::init();
	Localization::init();
		
	INFO("Settings initialized.");
}
