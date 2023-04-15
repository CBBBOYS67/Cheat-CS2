#include "imgui_internal.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "imgui_stdlib.h"


#include "SimpleIni.h"
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


void ModSettings::show_entry_edit(Entry* entry, mod_setting* mod)
{
	ImGui::PushID(entry);


	// Show input fields to edit the setting name and ini id
	if (ImGui::InputTextRequired("Name", &entry->name.def, ImGuiInputTextFlags_AutoSelectAll))
		mod->json_dirty = true;
	if (ImGui::InputText("Description", &entry->desc.def, ImGuiInputTextFlags_AutoSelectAll))
		mod->json_dirty = true;

	int current_type = entry->type;

	// Show fields specific to the selected setting type
	switch (current_type) {
	case kEntryType_Checkbox:
		{
			setting_checkbox* checkbox = dynamic_cast<setting_checkbox*>(entry);
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
	case kEntryType_Slider:
		{
			setting_slider* slider = dynamic_cast<setting_slider*>(entry);
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
	case kEntryType_Textbox:
		{
			setting_textbox* textbox = dynamic_cast<setting_textbox*>(entry);
			if (ImGui::InputText("Default", &textbox->default_value))
				mod->json_dirty = true;
			if (ImGui::InputInt("Size", &textbox->buf_size))
				mod->json_dirty = true;
			break;
		}
	case kEntryType_Dropdown:
		{
			setting_dropdown* dropdown = dynamic_cast<setting_dropdown*>(entry);
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
	case kEntryType_Text:
		{	
			// color palette to set text color
			ImGui::Text("Text color");
			ImGui::BeginChild("##text_color", ImVec2(0, 100), true, ImGuiWindowFlags_AlwaysAutoResize);
			{
				Entry_text* text = dynamic_cast<Entry_text*>(entry);
				float colorArray[4] = { text->_color.x, text->_color.y, text->_color.z, text->_color.w };
				if (ImGui::ColorEdit4("Color", colorArray)) {
					text->_color = ImVec4(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
					mod->json_dirty = true;
				}
				ImGui::EndChild();
			}
			break;
		}
	default:
		break;
	}

	ImGui::Text("Control");
	ImGui::BeginChild("##control_requirements", ImVec2(0, 100), true, ImGuiWindowFlags_AlwaysAutoResize);
	
	if (ImGui::Button("Add")) {
		entry->req.emplace_back();
		mod->json_dirty = true;
	}
	for (int i = 0; i < entry->req.size(); i++) {
		ImGui::PushID(i);
		if (ImGui::InputText("Option", &entry->req[i]))
			mod->json_dirty = true;
		ImGui::SameLine();
		if (ImGui::Button("-")) {
			entry->req.erase(entry->req.begin() + i);
			mod->json_dirty = true;
		}
		ImGui::PopID();
	}
	ImGui::EndChild();
	
	
	ImGui::Text("Localization");
	if (ImGui::BeginChild((std::string(entry->name.def) + "##Localization").c_str(), ImVec2(0, 200), true, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (ImGui::InputText("Name", &entry->name.key, ImGuiInputTextFlags_AutoSelectAll))
			mod->json_dirty = true;
		if (ImGui::InputText("Description", &entry->desc.key, ImGuiInputTextFlags_AutoSelectAll))
			mod->json_dirty = true;
		ImGui::EndChild();
	}
	
	if (entry->is_setting()) {
		setting_base* setting = dynamic_cast<setting_base*>(entry);
		ImGui::Text("Serialization");
		if (ImGui::BeginChild((std::string(setting->name.def) + "##serialization").c_str(), ImVec2(0, 200), true, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (ImGui::InputTextRequired("ini ID", &setting->ini_id))
				mod->json_dirty = true;

			if (ImGui::InputTextRequired("ini Section", &setting->ini_section))
				mod->json_dirty = true;

			if (ImGui::InputText("Game Setting", &setting->gameSetting)) {
				mod->json_dirty = true;
			}
			if (setting->type == kEntryType_Slider) {
				if (!setting->gameSetting.empty()) {
					switch (setting->gameSetting[0]) {
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
		}
	}
	
	
	

	//ImGui::EndChild();
	ImGui::PopID();
}

void ModSettings::show_entry(Entry* entry, mod_setting* mod)
{
	ImGui::PushID(entry);		bool available = true;
		for (auto& r : entry->req) {
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
		
	float width = ImGui::GetContentRegionAvail().x * 0.5f;
	switch (entry->type) {
	case kEntryType_Checkbox:
		{
			setting_checkbox* checkbox = dynamic_cast<setting_checkbox*>(entry);

			if (ImGui::Checkbox(checkbox->name.get(), &checkbox->value)) {
				mod->dirty = true;
			}

			if (!checkbox->desc.empty()) {
				ImGui::SameLine();
				ImGui::HoverNote(checkbox->desc.get());
			}
		}
		break;

	case kEntryType_Slider:
		{
			setting_slider* slider = dynamic_cast<setting_slider*>(entry);
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

			if (!slider->desc.empty()) {
				ImGui::SameLine();
				ImGui::HoverNote(slider->desc.get());
			}
		}
		break;

	case kEntryType_Textbox:
		{
			setting_textbox* textbox = dynamic_cast<setting_textbox*>(entry);

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

	case kEntryType_Dropdown:
		{
			setting_dropdown* dropdown = dynamic_cast<setting_dropdown*>(entry);
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
	case kEntryType_Text:
		{
			Entry_text* t = dynamic_cast<Entry_text*>(entry);
			ImGui::TextColored(t->_color, t->name.get());
		}
		break;
	case kEntryType_Group:
		{
			Entry_group* g = dynamic_cast<Entry_group*>(entry);
			if (ImGui::CollapsingHeader(entry->name.get())) {
				show_entries(g->entries, mod);
			}
		}
		break;
	default:
		break;
	}
	if (!available) {
		ImGui::PopStyleVar();
		ImGui::PopItemFlag();
		ImGui::PopStyleColor();
	}
	ImGui::PopID();
}

void ModSettings::show_entries(std::vector<Entry*>& entries, mod_setting* mod)
{
	if (edit_mode) {
		// button to add entry
		if (ImGui::Button("+")) {
			// correct popup position
			ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());
			ImGui::OpenPopup("Add Entry");
		}
		if (ImGui::BeginPopup("Add Entry")) {
			if (ImGui::Selectable("Checkbox")) {
				setting_checkbox* checkbox = new setting_checkbox();
				checkbox->editing = true;
				entries.push_back(checkbox);
				mod->json_dirty = true;
				INFO("added entry");
			}
			if (ImGui::Selectable("Slider")) {
				setting_slider* slider = new setting_slider();
				slider->editing = true;
				entries.push_back(slider);
				mod->json_dirty = true;
				INFO("added entry");

			}
			if (ImGui::Selectable("Textbox")) {
				setting_textbox* textbox = new setting_textbox();
				textbox->editing = true;
				entries.push_back(textbox);
				mod->json_dirty = true;
				INFO("added entry");

			}
			if (ImGui::Selectable("Dropdown")) {
				setting_dropdown* dropdown = new setting_dropdown();
				dropdown->editing = true;
				dropdown->options.push_back("Option 0");
				dropdown->options.push_back("Option 1");
				dropdown->options.push_back("Option 2");
				entries.push_back(dropdown);
				mod->json_dirty = true;
				INFO("added entry");

			}
			if (ImGui::Selectable("Text")) {
				Entry_text* text = new Entry_text();
				text->editing = true;
				entries.push_back(text);
				mod->json_dirty = true;
				INFO("added entry");

			}
			if (ImGui::Selectable("Group")) {
				Entry_group* group = new Entry_group();
				group->editing = true;
				entries.push_back(group);
				mod->json_dirty = true;
				INFO("added entry");

			}

			ImGui::EndPopup();
		}
	}
	
	for (auto it = entries.begin(); it != entries.end(); it++) {
		ModSettings::Entry* entry = *it;

		ImGui::PushID(entry);

		ImGui::Indent();
		// Set collapsing header background color to transparent
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

		// Add edit button next to the entry
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
				show_entry_edit(entry, mod);
				ImGui::EndPopup();
			}

			// up/down the entry
			ImGui::SameLine();
			if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {
				if (entry != entries.front()) {
					auto it = std::find(entries.begin(), entries.end(), entry);
					std::iter_swap(it, it - 1);
					mod->json_dirty = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::ArrowButton("##down", ImGuiDir_Down)) {
				if (entry != entries.back()) {
					auto it = std::find(entries.begin(), entries.end(), entry);
					std::iter_swap(it, it + 1);
					mod->json_dirty = true;
				}
			}
			ImGui::SameLine();
			// delete  button
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
			if (ImGui::Button("Delete")) {
				// delete entry
				
				if (entry->type == entry_type::kEntryType_Checkbox) {
					setting_checkbox* checkbox = (setting_checkbox*)entry;
					m_controls.erase(checkbox->control_id);
				}
				it = entries.erase(it);
				it--;
				mod->json_dirty = true;
				delete entry;
				// TODO: delete everything in a group if deleting group.
				continue;
			}
			ImGui::PopStyleColor();

			ImGui::SameLine();
		}

		show_entry(entry, mod);

		ImGui::PopStyleColor();

		ImGui::Unindent();
		ImGui::PopID();
	}
}



void ModSettings::show_modSetting(mod_setting* mod)
{
	// Set window padding and item spacing
	const float padding = 8.0f;
	const float spacing = 8.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

	show_entries(mod->entries, mod);

	ImGui::PopStyleVar(2);
}



inline std::string ModSettings::getEntryStr(entry_type t)
{
	switch (t) {
	case entry_type::kEntryType_Checkbox:
		return "checkbox";
	case entry_type::kEntryType_Slider:
		return "slider";
	case entry_type::kEntryType_Textbox:
		return "textbox";
	case entry_type::kEntryType_Dropdown:
		return "dropdown";
	case entry_type::kEntryType_Text:
		return "text";
	case entry_type::kEntryType_Group:
		return "group";
	default:
		return "invalid";
	}
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

	INFO("Loading .json configurations...");
	namespace fs = std::filesystem;
	for (auto& file : std::filesystem::directory_iterator(SETTINGS_DIR)) {
		if (file.path().extension() == ".json") {
			load_json(file.path());
		}
	}

	INFO("Loading .ini config serializations...");
	// Read serialized settings from the .ini file for each mod
	for (auto& mod : mods) {
		load_ini(mod);
		//insert_game_setting(mod);
		//save_game_setting(mod);
	}
	INFO("Mod settings initialized");
}

ModSettings::Entry* ModSettings::load_json_non_group(nlohmann::json& json)
{
	INFO("Loading json {}", json["text"]["name"].get<std::string>());

	Entry* e = nullptr;
	std::string type_str = json["type"].get<std::string>();
	if (type_str == "checkbox") {
		setting_checkbox* scb = new setting_checkbox();
		scb->value = json.contains("default") ? json["default"].get<bool>() : false;
		scb->default_value = scb->value;
		if (json.contains("control")) {
			if (json["control"].contains("id")) {
				scb->control_id = json["control"]["id"].get<std::string>();
				m_controls[scb->control_id] = scb;
			}
		}
		e = scb;
	} else if (type_str == "slider") {
		setting_slider* ssl = new setting_slider();
		ssl->value = json.contains("default") ? json["default"].get<float>() : 0;
		ssl->min = json["style"]["min"].get<float>();
		ssl->max = json["style"]["max"].get<float>();
		ssl->step = json["style"]["step"].get<float>();
		ssl->default_value = ssl->value;
		e = ssl;
	} else if (type_str == "textbox") {
		setting_textbox* stb = new setting_textbox();
		size_t buf_size = json.contains("size") ? json["size"].get<size_t>() : 64;
		stb->value = json.contains("default") ? json["default"].get<std::string>() : "";
		stb->default_value = stb->value;
		e = stb;
	} else if (type_str == "dropdown") {
		setting_dropdown* sdd = new setting_dropdown();
		sdd->value = json.contains("default") ? json["default"].get<int>() : 0;
		for (auto& option_json : json["options"]) {
			sdd->options.push_back(option_json.get<std::string>());
		}
		sdd->default_value = sdd->value;
		e = sdd;
	} else if (type_str == "text") {
		Entry_text* et = new Entry_text();
		if (json.contains("style") && json["style"].contains("color")) {
			et->_color = ImVec4(json["style"]["color"]["r"].get<float>(), json["style"]["color"]["g"].get<float>(), json["style"]["color"]["b"].get<float>(), json["style"]["color"]["a"].get<float>());
		}
		e = et;
	} else {
		INFO("Unknown setting type: {}", type_str);
	}
	
	if (e->is_setting()) {
		setting_base* s = dynamic_cast<setting_base*>(e);
		if (json.contains("gameSetting")) {
			s->gameSetting = json["gameSetting"].get<std::string>();
		}
		s->ini_section = json["ini"]["section"].get<std::string>();
		s->ini_id = json["ini"]["id"].get<std::string>();
	}
	return e;

}

ModSettings::Entry_group* ModSettings::load_json_group(nlohmann::json& group_json)
{
	INFO("Loading group json {}", group_json["text"]["name"].get<std::string>());
	Entry_group* group = new Entry_group();
	for (auto& entry_json : group_json["entries"]) {
		Entry* entry = nullptr;

		if (entry_json["type"].get<std::string>() == "group") {
			entry = load_json_group(entry_json);
		} else {
			entry = load_json_non_group(entry_json);
		}

		entry->name.def = entry_json["text"]["name"].get<std::string>();
		entry->desc.def = entry_json["text"]["desc"].get<std::string>();

		if (entry_json.contains("translation")) {
			if (entry_json["translation"].contains("name")) {
				entry->name.key = entry_json["translation"]["name"].get<std::string>();
			}
			if (entry_json["translation"].contains("desc")) {
				entry->desc.key = entry_json["translation"]["desc"].get<std::string>();
			}
		}

		if (entry_json.contains("control")) {
			if (entry_json["control"].contains("need")) {
				for (auto& r : entry_json["control"]["need"]) {
					std::string id = r.get<std::string>();
					entry->req.push_back(id);
				}
			}
		}
		INFO("adding {}", entry->name.def);
		group->entries.push_back(entry);
	}
	return group;
}

void ModSettings::load_json(std::filesystem::path path)
{
	std::string mod_path = path.string();
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
	try {
		if (mod_json.contains("ini")) {
			mod->ini_path = mod_json["ini"].get<std::string>();
		}
		else {
			mod->ini_path = SETTINGS_DIR + "\\ini\\" + mod->name.data() + ".ini";
		}

		for (auto& entry_json : mod_json["data"]) {
			Entry* entry = nullptr;

			if (entry_json["type"].get<std::string>() ==  "group") {
				entry = load_json_group(entry_json);
			} else {
				entry = load_json_non_group(entry_json);
			}
			
			entry->name.def = entry_json["text"]["name"].get<std::string>();
			if (entry_json["text"].contains("desc")) {
				entry->desc.def = entry_json["text"]["desc"].get<std::string>();
			}

			if (entry_json.contains("translation")) {
				if (entry_json["translation"].contains("name")) {
					entry->name.key = entry_json["translation"]["name"].get<std::string>();
				}
				if (entry_json["translation"].contains("desc")) {
					entry->desc.key = entry_json["translation"]["desc"].get<std::string>();
				}
			}
			
			if (entry_json.contains("control")) {
				if (entry_json["control"].contains("need")) {
					for (auto& r : entry_json["control"]["need"]) {
						std::string id = r.get<std::string>();
						entry->req.push_back(id);
					}
				}
			}
			mod->entries.push_back(entry);
		}
		// Add the mod to the list of mods
		mods.push_back(mod);
	} catch (const nlohmann::json::exception& e) {
		// Handle error parsing JSON
		INFO("Exception parsing {} : {}", mod_path, e.what());
		return;
	}
	INFO("Loaded mod {}", mod->name);
}

void ModSettings::populate_non_group_json(Entry* entry, nlohmann::json& json)
{
	std::string type_str = "";
	if (!entry->is_setting()) {
		if (entry->type == entry_type::kEntryType_Text) {
			type_str = "text";
			json["style"]["color"]["r"] = dynamic_cast<Entry_text*>(entry)->_color.x;
			json["style"]["color"]["g"] = dynamic_cast<Entry_text*>(entry)->_color.y;
			json["style"]["color"]["b"] = dynamic_cast<Entry_text*>(entry)->_color.z;
			json["style"]["color"]["a"] = dynamic_cast<Entry_text*>(entry)->_color.w;
		}
		return;  // no need to continue, the following fields are only for settings
	}
	
	setting_base* setting = dynamic_cast<setting_base*>(entry);
	json["ini"]["section"] = setting->ini_section;
	json["ini"]["id"] = setting->ini_id;
	if (setting->gameSetting != "") {
		json["gameSetting"] = dynamic_cast<setting_base*>(entry)->gameSetting;
	}
	if (setting->type == entry_type::kEntryType_Checkbox) {
		auto cb_setting = dynamic_cast<setting_checkbox*>(entry);
		json["default"] = cb_setting->default_value;
		if (cb_setting->control_id != "") {
			json["control"]["id"] = cb_setting->control_id;
		}
		type_str = "checkbox";
	} else if (setting->type == entry_type::kEntryType_Slider) {
		auto slider_setting = dynamic_cast<setting_slider*>(entry);
		json["default"] = slider_setting->default_value;
		json["style"]["min"] = slider_setting->min;
		json["style"]["max"] = slider_setting->max;
		json["style"]["step"] = slider_setting->step;
		type_str = "slider";

	} else if (setting->type == entry_type::kEntryType_Textbox) {
		auto textbox_setting = dynamic_cast<setting_textbox*>(entry);
		json["default"] = textbox_setting->default_value;
		json["size"] = textbox_setting->buf_size;
		type_str = "textbox";

	} else if (setting->type == entry_type::kEntryType_Dropdown) {
		auto dropdown_setting = dynamic_cast<setting_dropdown*>(entry);
		json["default"] = dropdown_setting->default_value;
		for (auto& option : dropdown_setting->options) {
			json["options"].push_back(option);
		}
		type_str = "dropdown";
	}
	json["type"] = type_str;
	
}


void ModSettings::populate_group_json(Entry_group* group, nlohmann::json& json)
{
	json["type"] = "group";
	for (auto& entry : group->entries) {
		nlohmann::json entry_json;
		// common fields for entry
		entry_json["text"]["name"] = entry->name.def;
		entry_json["text"]["desc"] = entry->desc.def;
		entry_json["translation"]["name"] = entry->name.key;
		entry_json["translation"]["desc"] = entry->desc.key;
		if (entry->is_group()) {
			populate_group_json(dynamic_cast<Entry_group*>(entry), entry_json);
		} else {
			populate_non_group_json(entry, entry_json);
		}
		json["entries"].push_back(entry_json);
	}
}
/* Serialize config to .json*/
void ModSettings::save_mod_config(mod_setting* mod)
{
	nlohmann::json mod_json;
	mod_json["name"] = mod->name;
	mod_json["ini"] = mod->ini_path;

	nlohmann::json data_json;

	for (auto& entry : mod->entries) {
		nlohmann:json entry_json;
		entry_json["text"]["name"] = entry->name.def;
		entry_json["text"]["desc"] = entry->desc.def;
		entry_json["translation"]["name"] = entry->name.key;
		entry_json["translation"]["desc"] = entry->desc.key;

		if (entry->is_group()) {
			populate_group_json(dynamic_cast<Entry_group*>(entry), entry_json);
		} else {
			populate_non_group_json(entry, entry_json);
		}
		data_json.push_back(entry_json);
	}
	
	
	std::ofstream json_file(mod->json_path);
	if (!json_file.is_open()) {
		// Handle error opening file
		INFO("error: failed to open {}", mod->json_path);
		return;
	}
	
	mod_json["data"] = data_json;

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

void ModSettings::get_all_settings(mod_setting* mod, std::vector<ModSettings::setting_base*>& r_vec)
{
	std::stack<Entry_group*> group_stack;
	for (auto& entry : mod->entries) {
		if (entry->is_group()) {
			group_stack.push(dynamic_cast<Entry_group*>(entry));
		} else if (entry->is_setting()) {
			r_vec.push_back(dynamic_cast<setting_base*>(entry));
		}
	}
	while (!group_stack.empty()) {  // get all groups
		auto group = group_stack.top();
		group_stack.pop();
		for (auto& entry : group->entries) {
			if (entry->is_group()) {
				group_stack.push(dynamic_cast<Entry_group*>(entry));
			} else if (entry->is_setting()) {
				r_vec.push_back(dynamic_cast<setting_base*>(entry));
			}
		}
	}
}


void ModSettings::load_ini(mod_setting* mod)
{
	// Create the path to the ini file for this mod
	INFO("loading .ini for {}", mod->name);
	// Load the ini file
	CSimpleIniA ini;
	ini.SetUnicode();
	SI_Error rc = ini.LoadFile(mod->ini_path.c_str());
	if (rc != SI_OK) {
		// Handle error loading file
		INFO(".ini file for {} not found. Creating a new .ini file.", mod->name);
		save_ini(mod);
		return;
	}

	std::vector<ModSettings::setting_base*> settings;
	get_all_settings(mod, settings);
	// Iterate through each setting in the group
	for (auto& setting_ptr : settings) {
		if (setting_ptr->ini_id.empty() || setting_ptr->ini_section.empty()) {
			ERROR("Undefined .ini serialization for setting {}; failed to load value.", setting_ptr->name.def);
			continue;
		}
		// Get the value of this setting from the ini file
		std::string value;
		if (ini.KeyExists(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str())) {
			value = ini.GetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), "");
		} else {
			INFO(".ini file for {} has no value for {}. Creating a new .ini file.", mod->name, setting_ptr->name.get());
			save_ini(mod);
			return;
		}
		// Convert the value to the appropriate type and assign it to the setting
		if (setting_ptr->type == kEntryType_Checkbox) {
			dynamic_cast<setting_checkbox*>(setting_ptr)->value = (value == "true");
		} else if (setting_ptr->type == kEntryType_Slider) {
			dynamic_cast<setting_slider*>(setting_ptr)->value = std::stof(value);
		} else if (setting_ptr->type == kEntryType_Textbox) {
			dynamic_cast<setting_textbox*>(setting_ptr)->value = value;
		} else if (setting_ptr->type == kEntryType_Dropdown) {
			dynamic_cast<setting_dropdown*>(setting_ptr)->value = std::stoi(value);
		}
	}
	INFO(".ini loaded.");
}



/* Flush changes to MOD into its .ini file*/
void ModSettings::save_ini(mod_setting* mod)
{
	// Create a SimpleIni object to write to the ini file
	CSimpleIniA ini;
	ini.SetUnicode();
	
	std::vector<ModSettings::setting_base*> settings;
	get_all_settings(mod, settings);
	for (auto& setting : settings) {
		if (setting->ini_id.empty() || setting->ini_section.empty()) {
			ERROR("Undefined .ini serialization for setting {}; failed to save value.", setting->name.def);
			continue;
		}
		// Get the value of this setting from the ini file
		std::string value;
		if (setting->type == kEntryType_Checkbox) {
			value = dynamic_cast<setting_checkbox*>(setting)->value ? "true" : "false";
		} else if (setting->type == kEntryType_Slider) {
			value = std::to_string(dynamic_cast<setting_slider*>(setting)->value);
		} else if (setting->type == kEntryType_Textbox) {
			value = dynamic_cast<setting_textbox*>(setting)->value;
		} else if (setting->type == kEntryType_Dropdown) {
			value = std::to_string(dynamic_cast<setting_dropdown*>(setting)->value);
		}
		ini.SetValue(setting->ini_section.c_str(), setting->ini_id.c_str(), value.c_str());
	}

	// Save the ini file
	ini.SaveFile(mod->ini_path.c_str());
}




/* Flush changes to MOD's settings to game settings.*/
void ModSettings::save_game_setting(mod_setting* mod)
{
	std::vector<ModSettings::setting_base*> settings;
	get_all_settings(mod, settings);
	auto gsc = RE::GameSettingCollection::GetSingleton();
	if (!gsc) {
		INFO("Game setting collection not found when trying to save game setting.");
		return;
	}
	for (auto& setting : settings) {
		RE::Setting* s = gsc->GetSetting(setting->gameSetting.data());
		if (!s) {
			INFO("Error: Game setting not found when trying to save game setting.");
			return;
		}
		if (setting->type == kEntryType_Checkbox) {
			s->SetBool(dynamic_cast<setting_checkbox*>(setting)->value);
		} else if (setting->type == kEntryType_Slider) {
			float val = dynamic_cast<setting_slider*>(setting)->value;
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
		} else if (setting->type == kEntryType_Textbox) {
			s->SetString(dynamic_cast<setting_textbox*>(setting)->value.c_str());
		} else if (setting->type == kEntryType_Dropdown) {
			//s->SetUnsignedInteger(dynamic_cast<setting_dropdown*>(setting_ptr)->value);
			s->SetInteger(dynamic_cast<setting_dropdown*>(setting)->value);
		}
	}
}

void ModSettings::insert_game_setting(mod_setting* mod) 
{
	std::vector<ModSettings::setting_base*> settings;
	get_all_settings(mod, settings);
	auto gsc = RE::GameSettingCollection::GetSingleton();
	if (!gsc) {
		INFO("Game setting collection not found when trying to insert game setting.");
		return;
	}
	for (auto& setting : settings) {
		if (setting->gameSetting.empty()) {  // no gamesetting mapping
			return;
		}
		if (gsc->GetSetting(setting->gameSetting.data())) {  // setting already exists
			return;
		}
		RE::Setting* s = new RE::Setting(setting->gameSetting.c_str());
		if (!s) {
			INFO("Failed to initialize setting when trying to insert game setting.");
			return;
		}
		gsc->InsertSetting(s);
		if (!gsc->GetSetting(setting->gameSetting.c_str())) {
			INFO("Failed to insert game setting.");
			return;
		}
	}
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
