#include "imgui.h"
#include "imgui_internal.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "imgui_stdlib.h"


#include "SimpleIni.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "ModSettings.h"

#include "bin/Utils.h"
#include "Settings.h"

using json = nlohmann::json;

void ModSettings::show_reloadTranslationButton()
{
	if (ImGui::Button("Reload Translation")) {
		Translator::LoadTranslations(Settings::currLanguage);
	}
}
  // not used anymore, we auto save.
void ModSettings::show_saveButton()
{
	bool unsaved_changes = false;
	for (auto& mod : mods) {
		if (mod->dirty) {
			unsaved_changes = true;
			break;
		}
	}
	if (unsaved_changes) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.9f, 0.5f, 1.0f));
    }

    if (ImGui::Button("Save", ImVec2(120, 30))) {
        for (auto& mod : mods) {
            if (mod->dirty) {
                save_ini(mod);
				save_game_setting(mod);
                mod->dirty = false;
				for (auto callback : mod->callbacks) {
					callback();
				}
            }
        }
    }

    if (unsaved_changes) {
        ImGui::PopStyleColor(3);
    }
}

void ModSettings::show_saveJsonButton()
{
	bool unsaved_changes = false;
	for (auto& mod : mods) {
		if (mod->json_dirty) {
			unsaved_changes = true;
			break;
		}
	}
	if (unsaved_changes) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.9f, 0.5f, 1.0f));
	}

	if (ImGui::Button("Save Config", ImVec2(120, 30))) {
		for (auto& mod : mods) {
			if (mod->json_dirty) {
				save_mod_config(mod);
				mod->json_dirty = false;
			}
		}
	}

	if (unsaved_changes) {
		ImGui::PopStyleColor(3);
	}
}


void ModSettings::show_setting_author(setting_base* setting_ptr, mod_setting* mod)
{
	ImGui::PushID(setting_ptr);


	// Show input fields to edit the setting name and ini id
	if (ImGui::InputTextRequired("Name", &setting_ptr->name.def, ImGuiInputTextFlags_AutoSelectAll))
		mod->json_dirty = true;
	if (ImGui::InputTextRequired("Description", &setting_ptr->desc.def, ImGuiInputTextFlags_AutoSelectAll))
		mod->json_dirty = true;

	int current_type = setting_ptr->type;

	// Show fields specific to the selected setting type
	switch (current_type) {
	case kSettingType_Checkbox:
		{
			setting_checkbox* checkbox = dynamic_cast<setting_checkbox*>(setting_ptr);
			if (ImGui::Checkbox("Default", &checkbox->default_value)) {
				mod->json_dirty = true;
			}
			std::string old_control_id = checkbox->control_id;
			if (ImGui::InputText("Control ID", &checkbox->control_id)) { // on change of control id
				if (!old_control_id.empty()) {  // remove old control id
					m_controls.erase(old_control_id);
				}
				if (!checkbox->control_id.empty()) {
					m_controls[checkbox->control_id] = checkbox;  // update control id
				}
				mod->json_dirty = true;
			}
			break;
		}
	case kSettingType_Slider:
		{
			setting_slider* slider = dynamic_cast<setting_slider*>(setting_ptr);
			if (ImGui::InputFloat("Default", &slider->default_value)) 
				mod->json_dirty = true;
			if (ImGui::InputFloat("Min", &slider->min))
				mod->json_dirty = true;
			if (ImGui::InputFloat("Max", &slider->max))
				mod->json_dirty = true;
			if (ImGui::InputFloat("Step", &slider->step))
				mod->json_dirty = true;
			break;
		}
	case kSettingType_Textbox:
		{
			setting_textbox* textbox = dynamic_cast<setting_textbox*>(setting_ptr);
			if (ImGui::InputText("Default", &textbox->default_value))
				mod->json_dirty = true;
			if (ImGui::InputInt("Size", &textbox->buf_size))
				mod->json_dirty = true;
			break;
		}
	case kSettingType_Dropdown:
		{
			setting_dropdown* dropdown = dynamic_cast<setting_dropdown*>(setting_ptr);
			ImGui::Text("Dropdown options");
			ImGui::BeginChild("##dropdown_items", ImVec2(0, 100), true, ImGuiWindowFlags_AlwaysAutoResize);
			{
				int buf;
				if (ImGui::InputInt("Default", &buf, 0, 100)) {
					dropdown->default_value = buf;
					mod->json_dirty = true;
				}
				if (ImGui::Button("Add")) {
					dropdown->options.emplace_back();
					mod->json_dirty = true;
				}
				for (int i = 0; i < dropdown->options.size(); i++) {
					ImGui::PushID(i);
					if (ImGui::InputText("Option", &dropdown->options[i])) mod->json_dirty = true;
					ImGui::SameLine();
					if (ImGui::Button("-")) {
						dropdown->options.erase(dropdown->options.begin() + i);
						mod->json_dirty = true;
					}
					ImGui::PopID();
				}
			}
			ImGui::EndChild();
			break;
		}
	default:
		break;
	}

	ImGui::Text("Control");
	ImGui::BeginChild("##control_requirements", ImVec2(0, 100), true, ImGuiWindowFlags_AlwaysAutoResize);
	
	if (ImGui::Button("Add")) {
		setting_ptr->req.emplace_back();
		mod->json_dirty = true;
	}
	for (int i = 0; i < setting_ptr->req.size(); i++) {
		ImGui::PushID(i);
		if (ImGui::InputText("Option", &setting_ptr->req[i]))
			mod->json_dirty = true;
		ImGui::SameLine();
		if (ImGui::Button("-")) {
			setting_ptr->req.erase(setting_ptr->req.begin() + i);
			mod->json_dirty = true;
		}
		ImGui::PopID();
	}
	ImGui::EndChild();
	
	
	ImGui::Text("Localization");
	if (ImGui::BeginChild((std::string(setting_ptr->name.get()) + "##Localization").c_str(), ImVec2(0, 200), true, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (ImGui::InputText("Name", &setting_ptr->name.key, ImGuiInputTextFlags_AutoSelectAll))
			mod->json_dirty = true;
		if (ImGui::InputText("Description", &setting_ptr->desc.key, ImGuiInputTextFlags_AutoSelectAll))
			mod->json_dirty = true;
		ImGui::EndChild();
	}
	
	
	ImGui::Text("Serialization");
	ImGui::BeginChild((std::string(setting_ptr->name.get()) + "##serialization").c_str(), ImVec2(0, 200), true, ImGuiWindowFlags_AlwaysAutoResize);
	
	if (ImGui::InputTextRequired("ini ID", &setting_ptr->ini_id))
		mod->json_dirty = true;
	
	if (ImGui::InputTextRequired("ini Section", &setting_ptr->ini_section))
		mod->json_dirty = true;

	if (ImGui::InputText("Game Setting", &setting_ptr->gameSetting)) {
		mod->json_dirty = true;
	}
	if (setting_ptr->type == kSettingType_Slider) {
		if (!setting_ptr->gameSetting.empty()) {
			switch (setting_ptr->gameSetting[0]) {
				case 'f':
				case 'i':
				case 'u':
					break;
				default:
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "For sliders, game setting must start with f, i, or u for float, int, or uint respectively for type specification.");
					break;
			}
		}
	}
	ImGui::EndChild();

	//ImGui::EndChild();
	ImGui::PopID();
}

void ModSettings::show_setting_user(setting_base* setting_ptr, mod_setting* mod)
{
	float width = ImGui::GetContentRegionAvail().x * 0.5f;
	switch (setting_ptr->type) {
	case kSettingType_Checkbox:
		{
			setting_checkbox* checkbox = dynamic_cast<setting_checkbox*>(setting_ptr);

			if (ImGui::Checkbox(checkbox->name.get(), &checkbox->value)) {
				mod->dirty = true;
			}

			if (!checkbox->desc.get()) {
				ImGui::SameLine();
				ImGui::HoverNote(checkbox->desc.get());
			}
		}
		break;

	case kSettingType_Slider:
		{
			setting_slider* slider = dynamic_cast<setting_slider*>(setting_ptr);
			bool changed = false;

			// Set the width of the slider to a fraction of the available width
			ImGui::SetNextItemWidth(width);
			if (ImGui::SliderFloatWithSteps(slider->name.get(), &slider->value, slider->min, slider->max, slider->step)) {
				changed = true;
			}

			if (ImGui::IsItemHovered()) {
				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow))) {
					if (slider->value > slider->min) {
						slider->value -= slider->step;
						changed = true;
					}
				}
				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow))) {
					if (slider->value < slider->max) {
						slider->value += slider->step;
						changed = true;
					}
				}
			}

			if (changed) {
				mod->dirty = true;
			}

			if (!slider->name.empty()) {
				ImGui::SameLine();
				ImGui::HoverNote(slider->desc.get());
			}
		}
		break;

	case kSettingType_Textbox:
		{
			setting_textbox* textbox = dynamic_cast<setting_textbox*>(setting_ptr);

			ImGui::SetNextItemWidth(width);
			if (ImGui::InputText(textbox->name.get(), &textbox->value)) {
				mod->dirty = true;
			}

			if (!textbox->desc.empty()) {
				ImGui::SameLine();
				ImGui::HoverNote(textbox->desc.get());
			}
		}
		break;

	case kSettingType_Dropdown:
		{
			setting_dropdown* dropdown = dynamic_cast<setting_dropdown*>(setting_ptr);
			const char* name = dropdown->name.get();
			int selected = dropdown->value;

			std::vector<std::string> options = dropdown->options;
			std::vector<const char*> cstrings;
			for (auto& option : options) {
				cstrings.push_back(option.c_str());
			}
			ImGui::SetNextItemWidth(width);
			if (ImGui::BeginCombo(name, cstrings[selected])) {
				for (int i = 0; i < options.size(); i++) {
					bool is_selected = (selected == i);

					if (ImGui::Selectable(cstrings[i], is_selected)) {
						selected = i;
						dropdown->value = selected;
						mod->dirty = true;
					}

					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}

			if (!dropdown->desc.empty()) {
				ImGui::SameLine();
				ImGui::HoverNote(dropdown->desc.get());
			}
		}
		break;

	default:
		break;
	}
}

void ModSettings::show_modSetting(mod_setting* mod)
{
	// Set window padding and item spacing
	const float padding = 8.0f;
	const float spacing = 8.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

	// Button to add new group
	if (edit_mode) {
		if (ImGui::Button("Add Group")) {
			mod_setting::mod_setting_group* group = new mod_setting::mod_setting_group();
			group->name.def = "New Group";
			mod->groups.push_back(group);
			mod->json_dirty = true;
		}
	}

	// Iterate through each group in the mod
	for (auto& group : mod->groups) {
		ImGui::PushID(group);

		ImGui::Indent();
		// Set collapsing header background color to transparent
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

		if (edit_mode) {
			// up/down the group
			if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {
				if (group != mod->groups.front()) {
					auto it = std::find(mod->groups.begin(), mod->groups.end(), group);
					std::iter_swap(it, it - 1);
					mod->json_dirty = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::ArrowButton("##down", ImGuiDir_Down)) {
				if (group != mod->groups.back()) {
					auto it = std::find(mod->groups.begin(), mod->groups.end(), group);
					std::iter_swap(it, it + 1);
					mod->json_dirty = true;
				}
			}
			ImGui::SameLine();

		}

		// Show collapsing header and child container
		if (ImGui::CollapsingHeader(group->name.get())) {
			if (!group->desc.empty()) {
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text(group->desc.get());
					ImGui::EndTooltip();
				}
			}
			// Check if the mod settings are in edit mode and right mouse button is clicked
			if (edit_mode && ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				ImGui::OpenPopup("Edit Group");
				ImGui::SetNextWindowPos(ImGui::GetMousePos());
			}
			
			ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX));

			//ImGui::BeginChild((group->name + "##settings").c_str(), ImVec2(0, 200), true, ImGuiWindowFlags_AlwaysUseWindowPadding || ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Indent();
			if (edit_mode) {
				// button to add setting
				if (ImGui::Button("Add Setting")) {
					// correct popup position
					ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());
					ImGui::OpenPopup("New_setting");
				}
				if (ImGui::BeginPopup("New_setting")) {
					if (ImGui::Selectable("Checkbox")) {
						setting_checkbox* checkbox = new setting_checkbox();
						checkbox->editing = true;
						group->settings.push_back(checkbox);
						mod->json_dirty = true;
					}
					if (ImGui::Selectable("Slider")) {
						setting_slider* slider = new setting_slider();
						slider->editing = true;
						group->settings.push_back(slider);
						mod->json_dirty = true;
					}
					if (ImGui::Selectable("Textbox")) {
						setting_textbox* textbox = new setting_textbox();
						textbox->editing = true;
						group->settings.push_back(textbox);
						mod->json_dirty = true;
					}
					if (ImGui::Selectable("Dropdown")) {
						setting_dropdown* dropdown = new setting_dropdown();
						dropdown->editing = true;
						group->settings.push_back(dropdown);
						mod->json_dirty = true;
					}
					ImGui::EndPopup();
				}
			}

			// Iterate through each setting in the group
			for (auto& setting_ptr : group->settings) {
				ImGui::PushID(setting_ptr);
				bool available = true;
				for (auto& r : setting_ptr->req) {
					if (m_controls.contains(r)) {
						if (m_controls[r]->value == false) {
							available = false;
							break;
						}
					}
				}
				if (!available) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}

				// Add edit button next to the setting name
				if (edit_mode) {
					//ImGui::ToggleButton("Edit", &setting_ptr->editing);
					if (ImGui::Button("Edit")) {  // Get the size of the current window
						
						ImGui::OpenPopup("Edit Setting");
						ImVec2 windowSize = ImGui::GetWindowSize();
						// Set the size of the pop-up to be proportional to the window size
						ImVec2 popupSize(windowSize.x * 0.5f, windowSize.y * 0.5f);
						ImGui::SetNextWindowSize(popupSize);
					}
					if (ImGui::BeginPopup("Edit Setting")) {
						// Get the size of the current window
						show_setting_author(setting_ptr, mod);
						ImGui::EndPopup();
					}
					
					// up/down the mod
					ImGui::SameLine();
					if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {
						if (setting_ptr != group->settings.front()) {
							auto it = std::find(group->settings.begin(), group->settings.end(), setting_ptr);
							std::iter_swap(it, it - 1);
							mod->json_dirty = true;
						}
					}
					ImGui::SameLine();
					if (ImGui::ArrowButton("##down", ImGuiDir_Down)) {
						if (setting_ptr != group->settings.back()) {
							auto it = std::find(group->settings.begin(), group->settings.end(), setting_ptr);
							std::iter_swap(it, it + 1);
							mod->json_dirty = true;
						}
					}
					ImGui::SameLine();
				}
				
				show_setting_user(setting_ptr, mod);

				// delete setting
				if (edit_mode) {
					ImGui::SameLine(ImGui::GetContentRegionAvail().x -50);
					// red button
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
					if (ImGui::Button("Delete")) {
						auto it = std::find(group->settings.begin(), group->settings.end(), setting_ptr);
						if (it != group->settings.end()) {
							group->settings.erase(it);
						}
						if (setting_ptr->type == setting_type::kSettingType_Checkbox) {
							setting_checkbox* checkbox = (setting_checkbox*)setting_ptr;
							m_controls.erase(checkbox->control_id);
						}
						delete setting_ptr;
						mod->json_dirty = true;
					}
					ImGui::PopStyleColor();
				}

				if (!available) {
					ImGui::PopStyleVar();
					ImGui::PopItemFlag();
					ImGui::PopStyleColor();
				}
				ImGui::PopID();
			}
			ImGui::Unindent();
			//ImGui::EndChild();
		} else {
			if (edit_mode && ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				ImGui::OpenPopup("Edit Group");
				ImGui::SetNextWindowPos(ImGui::GetMousePos());
			}
		}


		// Context menu for editing group name
		if (ImGui::BeginPopup("Edit Group")) {
			if (ImGui::InputText("Name", &group->name.def))
				mod->json_dirty = true;
			if (ImGui::InputText("Name Translation Key", &group->name.key))
				mod->json_dirty = true;
			if (ImGui::InputText("Description", &group->desc.def))
				mod->json_dirty = true;
			if (ImGui::InputText("Description Translation Key", &group->desc.key))
				mod->json_dirty = true;
			// push red color for delete group button
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
			if (ImGui::Button("Delete Group")) {
				ImGui::OpenPopup("Delete Group confirmation");
				ImGui::SetNextWindowPos(ImGui::GetMousePos());
			}
			ImGui::PopStyleColor();
			if (ImGui::BeginPopupModal("Delete Group confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Are you sure you want to delete this group?");
				ImGui::Separator();

				if (ImGui::Button("Yes", ImVec2(120, 0))) {
					for (auto& setting : group->settings) {
						if (setting->type == kSettingType_Checkbox) {
							if (dynamic_cast<setting_checkbox*>(setting)->control_id != "") {
								// Remove the control from the control map
								m_controls.erase(dynamic_cast<setting_checkbox*>(setting)->control_id);
							}
						}
						delete setting;
					}
					auto it = std::find(mod->groups.begin(), mod->groups.end(), group);
					mod->groups.erase(it);
					mod->json_dirty = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("No", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::EndPopup();
		}
		// Restore original header color
		ImGui::PopStyleColor();

		ImGui::Unindent();
		ImGui::PopID();
	}

	// Restore original style vars
	ImGui::PopStyleVar(2);

}



void ModSettings::show()
{
	show_saveButton();
	if (edit_mode) {
		show_saveJsonButton();
		show_reloadTranslationButton();
	}
	// a button on the rhs of this same line
	ImGui::SameLine(ImGui::GetWindowWidth() - 100.0f);  // Move cursor to the right side of the window
	ImGui::ToggleButton("Edit Config", &edit_mode);

	
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	for (auto& mod : mods) {
		if (ImGui::CollapsingHeader(mod->name.c_str())) {
			show_modSetting(mod);
		}
	}
	ImGui::PopStyleColor();
}

static const std::string SETTINGS_DIR = "Data\\SKSE\\Plugins\\\dmenu\\customSettings";
void ModSettings::init()
{
	// Load all mods from the "Mods" directory
	namespace fs = std::filesystem;
	for (auto& file : std::filesystem::directory_iterator(SETTINGS_DIR)) {
		if (file.path().extension() == ".json") {
			load_json(file.path());
		}
	}

	// Read serialized settings from the .ini file for each mod
	for (auto& mod : mods) {
		load_ini(mod);
		insert_game_setting(mod);
		save_game_setting(mod);
	}
	INFO("Mod settings initialized");
}

void ModSettings::load_json(std::filesystem::path path)
{
	std::string mod_path = path.string();
	INFO("Load mod config from {}", mod_path);
	// Load the JSON file for this mod
	std::ifstream json_file(mod_path);
	if (!json_file.is_open()) {
		// Handle error opening file
		return;
	}

	// Parse the JSON file
	nlohmann::json mod_json;
	try {
		json_file >> mod_json;
	} catch (const nlohmann::json::exception& e) {
		// Handle error parsing JSON
		return;
	}

	// Create a mod_setting object to hold the settings for this mod
	mod_setting* mod = new mod_setting();

	// name is .json's name
	mod->name = path.stem().string();
	mod->json_path = SETTINGS_DIR + "\\" + path.filename().string();
	INFO("Mod json path: {}", mod->json_path);
	try {
		if (mod_json.contains("ini")) {
			mod->ini_path = mod_json["ini"].get<std::string>();
			INFO("mod config ini path: {}", mod->ini_path);
		}
		else {
			mod->ini_path = SETTINGS_DIR + "\\ini\\" + mod->name.data() + ".ini";
		}

		// Iterate through each setting group in the JSON
		for (auto& group_json : mod_json["Groups"]) {
			mod_setting::mod_setting_group* group = new mod_setting::mod_setting_group();
			group->name.def = group_json["group"].get<std::string>();
			if (group_json.contains("desc")) {
				group->desc.def = group_json["desc"].get<std::string>();
			}
			if (group_json.contains("translation")) {
				auto group_translation_json = group_json["translation"];
				if (group_translation_json.contains("name")) {
					group->name.key = group_json["translation"]["name"].get<std::string>();
				}
				if (group_translation_json.contains("desc")) {
					group->desc.key = group_json["translation"]["description"].get<std::string>();
				}
			}

			// Iterate through each setting in the group
			for (auto& setting_json : group_json["settings"]) {
				setting_base* s = nullptr;
				std::string type_str = setting_json["type"].get<std::string>();
				if (type_str == "checkbox") {
					setting_checkbox* scb = new setting_checkbox();
					scb->value = setting_json.contains("default") ? setting_json["default"].get<bool>() : false;
					scb->default_value = scb->value;
					s = scb;
				} else if (type_str == "slider") {
					setting_slider* ssl = new setting_slider();
					ssl->value = setting_json.contains("default") ? setting_json["default"].get<float>() : 0;
					ssl->min = setting_json["style"]["min"].get<float>();
					ssl->max = setting_json["style"]["max"].get<float>();
					ssl->step = setting_json["style"]["step"].get<float>();
					ssl->default_value = ssl->value;
					s = ssl;
				} else if (type_str == "textbox") {
					setting_textbox* stb = new setting_textbox();
					size_t buf_size = setting_json.contains("size") ? setting_json["size"].get<size_t>() : 64;
					stb->value = setting_json.contains("default") ? setting_json["default"].get<std::string>() : "";
					stb->default_value = stb->value;
					s = stb;
				} else if (type_str == "dropdown") {
					setting_dropdown* sdd = new setting_dropdown();
					sdd->value = setting_json.contains("default") ? setting_json["default"].get<int>() : 0;
					for (auto& option_json : setting_json["options"]) {
						sdd->options.push_back(option_json.get<std::string>());
					}
					sdd->default_value = sdd->value;
					s = sdd;
				} else {
					INFO("Unknown setting type: {}", type_str);
					continue;
				}

				s->name.def = setting_json["text"]["name"].get<std::string>();
				if (setting_json["text"].contains("desc")) {
					s->desc.def = setting_json["text"]["desc"].get<std::string>();
				}

				if (setting_json.contains("gameSetting")) {
					s->gameSetting = setting_json["gameSetting"].get<std::string>();
				}
				if (setting_json.contains("translation")) {
					if (setting_json["translation"].contains("name")) {
						s->name.key = setting_json["translation"]["name"].get<std::string>();
					}
					if (setting_json["translation"].contains("desc")) {
						s->desc.key = setting_json["translation"]["desc"].get<std::string>();
					}
				}

				if (setting_json.contains("control")) {
					if (setting_json["control"].contains("id")) {
						if (s->type == setting_type::kSettingType_Checkbox) {
							m_controls.insert({ setting_json["control"]["id"].get<std::string>(), dynamic_cast<setting_checkbox*>(s) });
							dynamic_cast<setting_checkbox*>(s)->control_id = setting_json["control"]["id"].get<std::string>();
						}
					}
					if (setting_json["control"].contains("need")) {
						for (auto& r : setting_json["control"]["need"]) {
							std::string id = r.get<std::string>();
							s->req.push_back(id);
						}
					}
				}

				s->ini_section = setting_json["ini"]["section"].get<std::string>();
				s->ini_id = setting_json["ini"]["id"].get<std::string>();

				group->settings.push_back(s);
			}
			mod->groups.push_back(group);
		}
		// Add the mod to the list of mods
		mods.push_back(mod);
	} catch (const nlohmann::json::exception& e) {
		// Handle error parsing JSON
		ERROR("Exception parsing {} : {}", mod_path, e.what());
		return;
	}

}

void ModSettings::save_mod_config(mod_setting* mod)
{
	nlohmann::json mod_json;
	mod_json["name"] = mod->name;
	mod_json["ini"] = mod->ini_path;

	for (auto& group : mod->groups) {
		nlohmann::json group_json;
		group_json["group"] = group->name.def;
		group_json["translation"]["name"] = group->name.key;
		group_json["translation"]["desc"] = group->desc.key;

		for (auto& setting : group->settings) {
			nlohmann::json setting_json;

			setting_json["text"]["name"] = setting->name.def;
			setting_json["text"]["desc"] = setting->desc.def;
			setting_json["translation"]["name"] = setting->name.key;
			setting_json["translation"]["desc"] = setting->desc.key;

			setting_json["ini"]["section"] = setting->ini_section;
			setting_json["ini"]["id"] = setting->ini_id;

			if (setting->gameSetting != "") {
				setting_json["gameSetting"] = setting->gameSetting;
			}

			std::string type_str;

			if (setting->type == setting_type::kSettingType_Checkbox) {
				auto cb_setting = dynamic_cast<setting_checkbox*>(setting);
				setting_json["default"] = cb_setting->default_value;
				if (cb_setting->control_id != "") {
					setting_json["control"]["id"] = cb_setting->control_id;
				}
				type_str = "checkbox";
			} else if (setting->type == setting_type::kSettingType_Slider) {
				auto slider_setting = dynamic_cast<setting_slider*>(setting);
				setting_json["default"] = slider_setting->default_value;
				setting_json["style"]["min"] = slider_setting->min;
				setting_json["style"]["max"] = slider_setting->max;
				setting_json["style"]["step"] = slider_setting->step;
				type_str = "slider";

			} else if (setting->type == setting_type::kSettingType_Textbox) {
				auto textbox_setting = dynamic_cast<setting_textbox*>(setting);
				setting_json["default"] = textbox_setting->default_value;
				setting_json["size"] = textbox_setting->buf_size;
				type_str = "textbox";

			} else if (setting->type == setting_type::kSettingType_Dropdown) {
				auto dropdown_setting = dynamic_cast<setting_dropdown*>(setting);
				setting_json["default"] = dropdown_setting->default_value;
				for (auto& option : dropdown_setting->options) {
					setting_json["options"].push_back(option);
				}
				type_str = "dropdown";

			}
			setting_json["type"] = type_str;

			if (!setting->req.empty()) {
				for (auto& r : setting->req) {
					setting_json["control"]["need"].push_back(r);
				}
			}

			group_json["settings"].push_back(setting_json);

		}
		mod_json["Groups"].push_back(group_json);
	}
	
	std::ofstream json_file(mod->json_path);
	if (!json_file.is_open()) {
		// Handle error opening file
		INFO("error: failed to open {}", mod->json_path);
		return;
	}

	try {
		json_file << mod_json;
	} catch (const nlohmann::json::exception& e) {
		// Handle error parsing JSON
		ERROR("Exception dumping {} : {}", mod->json_path, e.what());
	}

	insert_game_setting(mod);
	save_game_setting(mod);

	INFO("Saved config for {}", mod->name);
}


void ModSettings::load_ini(mod_setting* mod)
{
	// Create the path to the ini file for this mod

	// Load the ini file
	CSimpleIniA ini;
	ini.SetUnicode();
	SI_Error rc = ini.LoadFile(mod->ini_path.c_str());
	if (rc != SI_OK) {
		// Handle error loading file
		INFO(".ini file for {} not found. Creating a new .ini file.", mod->name);
		construct_ini(mod);
		return;
	}

	// Iterate through each group in the mod
	for (auto& group : mod->groups) {
		// Iterate through each setting in the group
		for (auto& setting_ptr : group->settings) {
			// Get the value of this setting from the ini file
			std::string value;
			if (ini.KeyExists(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str())) {
				value = ini.GetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), "");
			} else {
				INFO(".ini file for {} has no value for {}. Creating a new .ini file.", mod->name, setting_ptr->name.get());
				construct_ini(mod);
				return;
			}
			// Convert the value to the appropriate type and assign it to the setting
			if (setting_ptr->type == kSettingType_Checkbox) {
				dynamic_cast<setting_checkbox*>(setting_ptr)->value = (value == "true");
			} else if (setting_ptr->type == kSettingType_Slider) {
				dynamic_cast<setting_slider*>(setting_ptr)->value = std::stof(value);
			} else if (setting_ptr->type == kSettingType_Textbox) {
				dynamic_cast<setting_textbox*>(setting_ptr)->value = value;
			} else if (setting_ptr->type == kSettingType_Dropdown) {
				dynamic_cast<setting_dropdown*>(setting_ptr)->value = std::stoi(value);
			}
		}
	}
}

void ModSettings::construct_ini(mod_setting* mod)
{

	// Create a SimpleIni object to write to the ini file
	CSimpleIniA ini;
	ini.SetUnicode();

	// Iterate through each group in the mod
	for (auto& group : mod->groups) {
		// Iterate through each setting in the group
		for (auto& setting_ptr : group->settings) {
			// Write the default value of this setting to the ini file
			if (setting_ptr->type == kSettingType_Checkbox) {
				ini.SetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), dynamic_cast<setting_checkbox*>(setting_ptr)->value == true ? "true" : "false");
			} else if (setting_ptr->type == kSettingType_Slider) {
				ini.SetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), std::to_string(dynamic_cast<setting_slider*>(setting_ptr)->value).c_str());
			} else if (setting_ptr->type == kSettingType_Textbox) {
				ini.SetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), dynamic_cast<setting_textbox*>(setting_ptr)->value.c_str());
			} else if (setting_ptr->type == kSettingType_Dropdown) {
				ini.SetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), std::to_string(dynamic_cast<setting_dropdown*>(setting_ptr)->value).c_str());
			}
		}
	}

	// Save the ini file
	ini.SaveFile(mod->ini_path.c_str());
}

/* Flush changes to MOD into its .ini file. .ini file is guaranteed to exist.*/
void ModSettings::save_ini(mod_setting* mod)
{
	// Create a SimpleIni object to write to the ini file
	CSimpleIniA ini;
	ini.SetUnicode();

	// Iterate through each group in the mod
	for (auto& group : mod->groups) {
		// Iterate through each setting in the group
		for (auto& setting_ptr : group->settings) {
			// Write the current value of this setting to the ini file
			if (setting_ptr->type == kSettingType_Checkbox) {
				ini.SetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), dynamic_cast<setting_checkbox*>(setting_ptr)->value == true ? "true" : "false");
			} else if (setting_ptr->type == kSettingType_Slider) {
				ini.SetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), std::to_string(dynamic_cast<setting_slider*>(setting_ptr)->value).c_str());
			} else if (setting_ptr->type == kSettingType_Textbox) {
				ini.SetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), dynamic_cast<setting_textbox*>(setting_ptr)->value.c_str());
			} else if (setting_ptr->type == kSettingType_Dropdown) {
				ini.SetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), std::to_string(dynamic_cast<setting_dropdown*>(setting_ptr)->value).c_str());
			}
		}
	}

	// Save the ini file
	ini.SaveFile(mod->ini_path.c_str());
}

/* Flush changes to MOD's settings to game settings.*/
void ModSettings::save_game_setting(mod_setting* mod)
{
	auto gsc = RE::GameSettingCollection::GetSingleton();
	if (!gsc) {
		ERROR("Game setting collection not found when trying to save game setting.");
		return;
	}
	// Iterate through each group in the mod
	for (auto& group : mod->groups) {
		// Iterate through each setting in the group
		for (auto& setting_ptr : group->settings) {
			if (setting_ptr->gameSetting.empty()) { // no gamesetting mapping
				continue;
			}
			INFO("querying setting {}", setting_ptr->name.def);
			RE::Setting* s = gsc->GetSetting(setting_ptr->gameSetting.data());
			if (!s) {
				ERROR("Game setting not found when trying to save game setting.");
			}
			// Write the current value of this setting to the ini file
			if (setting_ptr->type == kSettingType_Checkbox) {
				s->SetBool(dynamic_cast<setting_checkbox*>(setting_ptr)->value);
			} else if (setting_ptr->type == kSettingType_Slider) {
				float val = dynamic_cast<setting_slider*>(setting_ptr)->value;
				switch (s->GetType()) {
				case RE::Setting::Type::kUnsignedInteger:
					s->SetUnsignedInteger((uint32_t)val);
					break;
				case RE::Setting::Type::kInteger:
					s->SetInteger(static_cast<std::int32_t>(val));
					break;
				case RE::Setting::Type::kFloat:
					s->SetFloat(val);
					break;
				default:
					ERROR("Game setting variable for slider has bad type prefix. Prefix it with f(float), i(integer), or u(unsigned integer) to specify the game setting type.");
					break;
				}
			} else if (setting_ptr->type == kSettingType_Textbox) {
				s->SetString(dynamic_cast<setting_textbox*>(setting_ptr)->value.c_str());
			} else if (setting_ptr->type == kSettingType_Dropdown) {
				//s->SetUnsignedInteger(dynamic_cast<setting_dropdown*>(setting_ptr)->value);
				s->SetInteger(dynamic_cast<setting_dropdown*>(setting_ptr)->value);
			}
		}
	}
}


void ModSettings::insert_game_setting(mod_setting* mod) 
{
	auto gsc = RE::GameSettingCollection::GetSingleton();
	if (!gsc) {
		ERROR("Game setting collection not found when trying to initialize game setting.");
		return;
	}
	// Iterate through each group in the mod
	for (auto& group : mod->groups) {
		// Iterate through each setting in the group
		for (auto& setting_ptr : group->settings) {
			if (setting_ptr->gameSetting.empty()) {  // no gamesetting mapping
				continue;
			}
			if (gsc->GetSetting(setting_ptr->gameSetting.data())) { // setting already exists
				continue;
			}
			RE::Setting* s = new RE::Setting(setting_ptr->gameSetting.c_str());
			if (!s) {
				ERROR("Failed to initialize setting when trying to insert game setting.");
			}
			gsc->InsertSetting(s);
			if (!gsc->GetSetting(setting_ptr->gameSetting.c_str())) {
				ERROR("Failed to insert game setting.");
			}
		}
	}
}

bool ModSettings::setting_base::incomplete()
{
	return this->name.def.empty() || this->ini_id.empty() || this->ini_section.empty();
}

void ModSettings::save_all_game_setting()
{
	for (auto mod : mods) {
		save_game_setting(mod);
	}
}

void ModSettings::insert_all_game_setting()
{
	for (auto mod : mods) {
		insert_game_setting(mod);
	}
}

bool ModSettings::API_RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback)
{
	INFO("Received registration for {} update.", a_mod);
	for (auto mod : mods) {
		if (mod->name == a_mod) {
			mod->callbacks.push_back(a_callback);
			INFO("Successfully added callback for {} update.", a_mod);
			return true;
		}
	}
	ERROR("{} mod not found.", a_mod);
	return false;
}



namespace Example
{
	bool RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback) 
	{
		static auto dMenu = GetModuleHandle("dmenu");
		using _RegisterForSettingUpdate = bool (*)(std::string, std::function<void()>);
		static auto func = reinterpret_cast<_RegisterForSettingUpdate>(GetProcAddress(dMenu, "RegisterForSettingUpdate"));
		if (func) {
			return func(a_mod, a_callback);
		}
		return false;
	}
}

extern "C" DLLEXPORT bool RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback)
{
	return ModSettings::API_RegisterForSettingUpdate(a_mod, a_callback);
}
