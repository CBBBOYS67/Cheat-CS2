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

void ModSettings::show_modSetting(mod_setting* mod)
{

	// Iterate through each group in the mod
	for (auto& group : mod->groups) {
		ImGui::Indent();
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));  // set background color alpha to 0
		if (ImGui::CollapsingHeader(group->name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::BeginChild((group->name + "##settings").c_str(), ImVec2(0, 200), true);
			// Iterate through each setting in the group
			for (auto& setting_ptr : group->settings) {
				switch (setting_ptr->type) {
				case kSettingType_Checkbox:
					{
						setting_checkbox* checkbox = dynamic_cast<setting_checkbox*>(setting_ptr);
						if (ImGui::Checkbox(checkbox->name.c_str(), &checkbox->value)) {
							mod->dirty = true;
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
						bool changed = false;

						if (ImGui::SliderFloatWithSteps(slider->name.c_str(), &slider->value, slider->min, slider->max, slider->step)) {
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
							mod->dirty = true;
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
							ImGui::HoverNote(dropdown->desc.c_str());
						}
					}
					break;
				default:
					break;
				}
			}
			ImGui::EndChild();
		}
		ImGui::PopStyleColor();  // restore original color

		ImGui::Unindent();
	}

}

void ModSettings::show()
{
	show_saveButton();
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	for (auto& mod : mods) {
		if (ImGui::CollapsingHeader(mod->name.c_str())) {
			//ImGui::BeginChild((mod->name + "##settings").c_str(), ImVec2(0, 200), true, ImGuiWindowFlags_NoScrollbar);
			show_modSetting(mod);
			//ImGui::EndChild();
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
		insert_game_setting(mod);
		save_game_setting(mod);
	}
	INFO("Mod settings initialized");
}

void ModSettings::load_mod(std::string mod_path)
{
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
	try {
		mod->name = mod_json["name"].get<std::string>();
		if (mod_json.contains("iniPath")) {
			mod->ini_path = mod_json["iniPath"].get<std::string>();
		}
		else {
			mod->ini_path = SETTINGS_DIR + "\\ini\\" + mod->name.data() + ".ini";
		}

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
					for (auto& option_json : setting_json["options"]) {
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

				if (setting_json.contains("gameSetting")) {
					s->gameSetting = setting_json["gameSetting"].get<std::string>();
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

	INFO("loading ini");
	// Iterate through each group in the mod
	for (auto& group : mod->groups) {
		// Iterate through each setting in the group
		for (auto& setting_ptr : group->settings) {
			// Get the value of this setting from the ini file
			std::string value;
			if (ini.KeyExists(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str())) {
				value = ini.GetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), "");
			} else {
				INFO(".ini file for {} has no value for {}. Creating a new .ini file.", mod->name, setting_ptr->name);
				construct_ini(mod);
				return;
			}
			INFO("got value {}, {}", setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str());
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
	INFO("ini loaded");
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
			RE::Setting* s = gsc->GetSetting(setting_ptr->gameSetting.data());
			if (!s) {
				ERROR("Game setting not found when trying to save game setting.");
			}
			// Write the current value of this setting to the ini file
			if (setting_ptr->type == kSettingType_Checkbox) {
				s->SetBool(dynamic_cast<setting_checkbox*>(setting_ptr)->value);
			} else if (setting_ptr->type == kSettingType_Slider) {
				float val = dynamic_cast<setting_slider*>(setting_ptr)->value;
				s->SetFloat(val);
				s->SetInteger((int)val);
				s->SetUnsignedInteger((uint32_t)val);
			} else if (setting_ptr->type == kSettingType_Textbox) {
				s->SetString(dynamic_cast<setting_textbox*>(setting_ptr)->value.c_str());
			} else if (setting_ptr->type == kSettingType_Dropdown) {
				s->SetUnsignedInteger(dynamic_cast<setting_dropdown*>(setting_ptr)->value);
				s->SetInteger(dynamic_cast<setting_dropdown*>(setting_ptr)->value); // why not
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

bool ModSettings::API_RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback)
{
	for (auto mod : mods) {
		if (mod->name == a_mod) {
			mod->callbacks.push_back(a_callback);
			return true;
		}
	}
	return false;
}



namespace Example
{
	void RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback) 
	{
		static auto dMenu = GetModuleHandle("dmenu");
		using _RegisterForSettingUpdate = const char* (*)(std::string, std::function<void()>);
		static auto func = reinterpret_cast<_RegisterForSettingUpdate>(GetProcAddress(dMenu, "RegisterForSettingUpdate"));
		if (func) {
			func(a_mod, a_callback);
		}
	}
}

extern "C" DLLEXPORT bool RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback)
{
	return ModSettings::API_RegisterForSettingUpdate(a_mod, a_callback);
}
