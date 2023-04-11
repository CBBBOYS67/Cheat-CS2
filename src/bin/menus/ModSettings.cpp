#include "imgui.h"
#include "imgui_internal.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>


#include "SimpleIni.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "ModSettings.h"

#include "bin/Utils.h"

using json = nlohmann::json;

// Initialize static members
std::vector<ModSettings::mod_setting*> ModSettings::mods;

std::unordered_map<std::string_view, std::string, ModSettings::Translations::StringHash> ModSettings::Translations::translation_EN;
std::unordered_map<std::string_view, std::string, ModSettings::Translations::StringHash> ModSettings::Translations::translation_FR;
std::unordered_map<std::string_view, std::string, ModSettings::Translations::StringHash> ModSettings::Translations::translation_CN;
std::unordered_map<std::string_view, std::string, ModSettings::Translations::StringHash> ModSettings::Translations::translation_RU;

void ModSettings::Translations::init()
{
	// Load translations from files here
	// ...
}

std::string_view ModSettings::Translations::lookup(std::string_view a_key)
{
	// Lookup key in selected translation map and return value
	// ...
	return a_key;
}

void ModSettings::flag_dirty(mod_setting* mod) 
{
	mod->dirty = true;
}

void ModSettings::show()
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	for (auto& mod : mods) {
		if (ImGui::CollapsingHeader(mod->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Spacing();
			ImGui::Separator();
			// Iterate through each group in the mod
			for (auto& group : mod->groups) {
				ImGui::Indent();
				if (ImGui::CollapsingHeader(group->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::Spacing();
					ImGui::Separator();
					// Iterate through each setting in the group
					for (auto& setting_ptr : group->settings) {
						switch (setting_ptr->type) {
						case kSettingType_Checkbox:
							{
								setting_checkbox* checkbox = dynamic_cast<setting_checkbox*>(setting_ptr);
								if (ImGui::Checkbox(checkbox->name.c_str(), &checkbox->value)) {
									flag_dirty(mod);
								}
								if (!checkbox->desc.empty()) {
									ImGui::SameLine();
									ImGui::HoverNote(checkbox->desc.c_str());
								}
							}
							break;
						case kSettingType_Slider:
							{
								setting_slider* slider = dynamic_cast<setting_slider*>(setting_ptr);
								
								if (ImGui::SliderFloatWithSteps(slider->name.c_str(), &slider->value, slider->min, slider->max, slider->step)) {
									flag_dirty(mod);
								}
								if (!slider->desc.empty()) {
									ImGui::SameLine();
									ImGui::HoverNote(slider->desc.c_str());
								}
							}
							break;
						case kSettingType_Textbox:
							{
								setting_textbox* textbox = dynamic_cast<setting_textbox*>(setting_ptr);
								if (ImGui::InputText(textbox->name.c_str(), textbox->buf, textbox->buf_size)) {
									flag_dirty(mod);
								}
								if (!textbox->desc.empty()) {
									ImGui::SameLine();
									ImGui::HoverNote(textbox->desc.c_str());
								}
							}
							break;
						case kSettingType_Dropdown:
							{
								setting_dropdown* dropdown = dynamic_cast<setting_dropdown*>(setting_ptr);
								const char* name = dropdown->name.c_str();
								int selected = dropdown->value;
								std::vector<std::string> options = dropdown->options;
								std::vector<const char*> cstrings;
								for (auto& option : options) {
									cstrings.push_back(option.c_str());
								}
								if (ImGui::Combo(name, &selected, cstrings.data(), cstrings.size())) {
									dropdown->value = selected;
									flag_dirty(mod);
								}
								if (!dropdown->desc.empty()) {
									ImGui::SameLine();
									ImGui::HoverNote(dropdown->desc.c_str());
								}
							}
							break;
						default:
							break;
						}
					}
					ImGui::Spacing();
					ImGui::Separator();
				}
				ImGui::Unindent();
			}
			ImGui::Spacing();
			ImGui::Separator();
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
			load_mod(file.path().string());
		}
	}

	// Read serialized settings from the .ini file for each mod
	for (auto& mod : mods) {
		load_ini(mod);
	}
	INFO("Mod settings initialized");
}

void ModSettings::load_mod(std::string mod_path)
{
	INFO("Loading mod {}", mod_path);
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
	try {
		mod->name = mod_json["name"].get<std::string>();

		// Iterate through each setting group in the JSON
		for (auto& group_json : mod_json["Groups"]) {
			mod_setting::mod_setting_group* group = new mod_setting::mod_setting_group();
			group->name = group_json["group"].get<std::string>();

			// Iterate through each setting in the group
			for (auto& setting_json : group_json["settings"]) {
				setting_base* s = nullptr;
				std::string type_str = setting_json["type"].get<std::string>();
				if (type_str == "checkBox") {
					setting_checkbox* scb = new setting_checkbox();
					scb->value = setting_json.contains("default") ? setting_json["default"].get<bool>() : false;
					s = scb;
				} else if (type_str == "slider") {
					setting_slider* ssl = new setting_slider();
					ssl->value = setting_json.contains("default") ? setting_json["default"].get<float>() : 0;
					ssl->min = setting_json["style"]["min"].get<float>();
					ssl->max = setting_json["style"]["max"].get<float>();
					ssl->step = setting_json["style"]["step"].get<float>();
					s = ssl;
				} else if (type_str == "textbox") {
					setting_textbox* stb = new setting_textbox();
					size_t buf_size = setting_json.contains("size") ? setting_json["size"].get<size_t>() : 16;
					stb->buf = (char*)calloc(buf_size, sizeof(char));
					std::string data = setting_json.contains("default") ? setting_json["default"].get<std::string>() : "";
					strncpy(stb->buf, data.c_str(), max(buf_size, data.size()));
					s = stb;
				} else if (type_str == "dropdown") {
					setting_dropdown* sdd = new setting_dropdown();
					sdd->value = setting_json.contains("default") ? setting_json["default"].get<int>() : 0;
					for (auto& option_json : setting_json["style"]["options"]) {
						sdd->options.push_back(option_json.get<std::string>());
					}
					s = sdd;
				} else {
					INFO("Unknown setting type: {}", type_str);
					continue;
				}

				s->name = setting_json["text"]["name"].get<std::string>();
				if (setting_json["text"].contains("desc")) {
					s->desc = setting_json["text"]["desc"].get<std::string>();
				}
				
				s->ini_section = setting_json["ini"]["section"].get<std::string>();
				s->ini_id = setting_json["ini"]["id"].get<std::string>();
				group->settings.push_back(s);
			}
			mod->groups.push_back(group);
		}
		INFO("mod loaded");
		// Add the mod to the list of mods
		mods.push_back(mod);
	} catch (const nlohmann::json::exception& e) {
		// Handle error parsing JSON
		INFO("Error parsing {} : {}", mod_path, e.what());
		return;
	}

}


void ModSettings::load_ini(mod_setting* mod)
{
	// Create the path to the ini file for this mod
	std::string ini_path = SETTINGS_DIR + mod->name.data() + "_i.ini";

	// Load the ini file
	CSimpleIniA ini;
	ini.SetUnicode();
	SI_Error rc = ini.LoadFile(ini_path.c_str());
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
			if (ini.GetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), "")) {
				value = ini.GetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), "");
			} else {
				INFO(".ini file for {} has no value for {}. Creating a new .ini file.", mod->name, setting_ptr->name);
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
	// Create the path to the ini file for this mod
	std::string ini_path = SETTINGS_DIR + mod->name.data() + ".ini";

	// Create a SimpleIni object to write to the ini file
	CSimpleIniA ini;
	ini.SetUnicode();

	// Iterate through each group in the mod
	for (auto& group : mod->groups) {
		INFO("saving group {}", group->name);
		// Iterate through each setting in the group
		for (auto& setting_ptr : group->settings) {
			INFO("saving setting {}", setting_ptr->name);
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
	ini.SaveFile(ini_path.c_str());
}
