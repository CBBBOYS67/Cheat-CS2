#include "imgui_internal.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "imgui_stdlib.h"


#include "SimpleIni.h"
#include <fstream>
#include "ModSettings.h"

#include "bin/Utils.h"
#include "Settings.h"
static RE::GameSettingCollection* gsc = nullptr;
inline bool ModSettings::entry_base::Control::Req::satisfied()
{
	bool val = false;
	if (type == kReqType_Checkbox) {
		auto it = ModSettings::m_checkbox_toggle.find(id);
		if (it == ModSettings::m_checkbox_toggle.end()) {
			return false;
		}
		val = it->second->value;
	} else if (type == kReqType_GameSetting) {
		if (!gsc) {
			gsc = RE::GameSettingCollection::GetSingleton();
			if (!gsc)
				return false;
		}
		auto setting = gsc->GetSetting(id.c_str());
		if (!setting) 
			return false;
		val = setting->GetBool();
	}
	return this->_not ? !val : val;
}

inline bool ModSettings::entry_base::Control::satisfied()
{
	for (auto& req : reqs) {
		if (!req.satisfied()) {
			return false;
		}
	}
	return true;
}


using json = nlohmann::json;

void ModSettings::SendAllSettingsUpdateEvent()
{
	for (auto& mod : mods) {
		SendSettingsUpdateEvent(mod->name);
	}
}
void ModSettings::show_reloadTranslationButton()
{
	if (ImGui::Button("Reload Translation")) {
		Translator::ReLoadTranslations();
	}
}
  // not used anymore, we auto save.
void ModSettings::show_saveButton()
{
	bool unsaved_changes = !ini_dirty_mods.empty();
	
	if (unsaved_changes) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.9f, 0.5f, 1.0f));
    }

    if (ImGui::Button("Save")) {
		for (auto& mod : ini_dirty_mods) {
			flush_ini(mod);
			flush_game_setting(mod);
			for (auto callback : mod->callbacks) {
				callback();
			}
			SendSettingsUpdateEvent(mod->name);
		}
		ini_dirty_mods.clear();
    }

    if (unsaved_changes) {
        ImGui::PopStyleColor(3);
    }
}

void ModSettings::show_cancelButton()
{
	bool unsaved_changes = !ini_dirty_mods.empty();

	if (ImGui::Button("Cancel")) {
		for (auto& mod : ini_dirty_mods) {
			load_ini(mod);
		}
		ini_dirty_mods.clear();
	}
}

void ModSettings::show_saveJsonButton()
{
	bool unsaved_changes = !json_dirty_mods.empty();

	if (unsaved_changes) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.9f, 0.5f, 1.0f));
	}

	if (ImGui::Button("Save Config")) {
		for (auto& mod : json_dirty_mods) {
			flush_json(mod);
		}
		json_dirty_mods.clear();
	}

	if (unsaved_changes) {
		ImGui::PopStyleColor(3);
	}
}

void ModSettings::show_buttons_window()
{
	ImVec2 mainWindowSize = ImGui::GetWindowSize();
	ImVec2 mainWindowPos = ImGui::GetWindowPos();
	
	ImVec2 buttonsWindowSize = ImVec2(mainWindowSize.x * 0.15, mainWindowSize.y * 0.1);

	ImGui::SetNextWindowPos(ImVec2(mainWindowPos.x + mainWindowSize.x, mainWindowPos.y + mainWindowSize.y - buttonsWindowSize.y));
	ImGui::SetNextWindowSize(buttonsWindowSize);  // Adjust the height as needed
	ImGui::Begin("Buttons", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
	show_saveButton();
	ImGui::SameLine();
	show_cancelButton();
	if (edit_mode) {
		ImGui::SameLine();
		show_saveJsonButton();
		ImGui::SameLine();
		//show_reloadTranslationButton();
	}
	ImGui::End();
}


void ModSettings::show_entry_edit(entry_base* entry, mod_setting* mod)
{
	ImGui::PushID(entry);

	bool edited = false;

	// Show input fields to edit the setting name and ini id
	if (ImGui::InputTextWithPasteRequired("Name", entry->name.def))
		edited = true;
	if (ImGui::InputTextWithPaste("Description", entry->desc.def, ImVec2(ImGui::GetCurrentWindow()->Size.x * 0.8, ImGui::GetTextLineHeight() * 3), true, ImGuiInputTextFlags_AutoSelectAll))
		edited = true;
	int current_type = entry->type;

	// Show fields specific to the selected setting type
	switch (current_type) {
	case kEntryType_Checkbox:
		{
			setting_checkbox* checkbox = dynamic_cast<setting_checkbox*>(entry);
			if (ImGui::Checkbox("Default", &checkbox->default_value)) {
				edited = true;
			}
			std::string old_control_id = checkbox->control_id;
			if (ImGui::InputTextWithPaste("Control ID", checkbox->control_id)) {  // on change of control id
				if (!old_control_id.empty()) {                            // remove old control id
					m_checkbox_toggle.erase(old_control_id);
				}
				if (!checkbox->control_id.empty()) {
					m_checkbox_toggle[checkbox->control_id] = checkbox;  // update control id
				}
				edited = true;
			}
			break;
		}
	case kEntryType_Slider:
		{
			setting_slider* slider = dynamic_cast<setting_slider*>(entry);
			if (ImGui::InputFloat("Default", &slider->default_value))
				edited = true;
			if (ImGui::InputFloat("Min", &slider->min))
				edited = true;
			if (ImGui::InputFloat("Max", &slider->max))
				edited = true;
			if (ImGui::InputFloat("Step", &slider->step))
				edited = true;
			break;
		}
	case kEntryType_Textbox:
		{
			setting_textbox* textbox = dynamic_cast<setting_textbox*>(entry);
			if (ImGui::InputTextWithPaste("Default", textbox->default_value))
				edited = true;
			break;
		}
	case kEntryType_Dropdown:
		{
			setting_dropdown* dropdown = dynamic_cast<setting_dropdown*>(entry);
			ImGui::Text("Dropdown options");
			if (ImGui::BeginChild("##dropdown_items", ImVec2(0, 200), true, ImGuiWindowFlags_AlwaysAutoResize))
			{
				int buf;
				if (ImGui::InputInt("Default", &buf, 0, 100)) {
					dropdown->default_value = buf;
					edited = true;
				}
				if (ImGui::Button("Add")) {
					dropdown->options.emplace_back();
					edited = true;
				}
				for (int i = 0; i < dropdown->options.size(); i++) {
					ImGui::PushID(i);
					if (ImGui::InputText("Option", &dropdown->options[i]))
						edited = true;
					ImGui::SameLine();
					if (ImGui::Button("-")) {
						dropdown->options.erase(dropdown->options.begin() + i);
						if (dropdown->value == i) {  // erased current value
							dropdown->value = 0;     // reset to 1st value
						}
						edited = true;
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
			if (ImGui::BeginChild("##text_color", ImVec2(0, 100), true, ImGuiWindowFlags_AlwaysAutoResize))
			{
				entry_text* text = dynamic_cast<entry_text*>(entry);
				float colorArray[4] = { text->_color.x, text->_color.y, text->_color.z, text->_color.w };
				if (ImGui::ColorEdit4("Color", colorArray)) {
					text->_color = ImVec4(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
					edited = true;
				}
				ImGui::EndChild();
			}
			break;
		}
	case kEntryType_Color:
	{
			// color palette to set text color
			ImGui::Text("Color");
			if (ImGui::BeginChild("##color", ImVec2(0, 100), true, ImGuiWindowFlags_AlwaysAutoResize))
			{
				setting_color* color = dynamic_cast<setting_color*>(entry);
				float colorArray[4] = { color->default_color.x, color->default_color.y, color->default_color.z, color->default_color.w };
				if (ImGui::ColorEdit4("Default Color", colorArray)) {
					color->default_color = ImVec4(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
					edited = true;
				}
				ImGui::EndChild();
			}
			break;
	}
	case kEntryType_Keymap:
	{
			// color palette to set text color
			ImGui::Text("Keymap");
			if (ImGui::BeginChild("##keymap", ImVec2(0, 100), true, ImGuiWindowFlags_AlwaysAutoResize))
			{
				setting_keymap* keymap = dynamic_cast<setting_keymap*>(entry);
				ImGui::InputInt("Default Key ID", &(keymap->default_value));
				ImGui::EndChild();
			}
			break;
	}
	default:
		break;
	}

	ImGui::Text("Control");
	// choose fail action
	static const char* failActions[] = { "Disable", "Hide" };
	if (ImGui::BeginCombo("Fail Action", failActions[(int)entry->control.failAction])) {
		for (int i = 0; i < 2; i++) {
			bool is_selected = ((int)entry->control.failAction == i);
			if (ImGui::Selectable(failActions[i], is_selected))
				entry->control.failAction = static_cast<entry_base::Control::FailAction>(i);
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	if (ImGui::Button("Add")) {
		entry->control.reqs.emplace_back();
		edited = true;
	}

	if (ImGui::BeginChild("##control_requirements", ImVec2(0, 100), true, ImGuiWindowFlags_AlwaysAutoResize)) {
		// set the number of columns to 3


		for (int i = 0; i < entry->control.reqs.size(); i++) {
			auto& req = entry->control.reqs[i];  // get a reference to the requirement at index i
			ImGui::PushID(&req);
			const int numColumns = 3;
			ImGui::Columns(numColumns, nullptr, false);
			// set the width of each column to be the same
			ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.5f);
			ImGui::SetColumnWidth(1, ImGui::GetWindowWidth() * 0.25f);
			ImGui::SetColumnWidth(2, ImGui::GetWindowWidth() * 0.25f);

			if (ImGui::InputTextWithPaste("id", req.id)) {
				edited = true;
			}

			// add the second column
			ImGui::NextColumn();
			static const char* reqTypes[] = { "Checkbox", "GameSetting" };
			if (ImGui::BeginCombo("Type", reqTypes[(int)req.type])) {
				for (int i = 0; i < 2; i++) {
					bool is_selected = ((int)req.type == i);
					if (ImGui::Selectable(reqTypes[i], is_selected))
						req.type = static_cast<entry_base::Control::Req::ReqType>(i);
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			// add the third column
			ImGui::NextColumn();
			if (ImGui::Checkbox("not", &req._not)) {
				edited = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("-")) {
				entry->control.reqs.erase(entry->control.reqs.begin() + i);  // erase the requirement at index i
				edited = true;
				i--;  // update the loop index to account for the erased element
			}
			ImGui::Columns(1);

			ImGui::PopID();
		}

		ImGui::EndChild();
	}

	

	ImGui::Text("Localization");
	if (ImGui::BeginChild((std::string(entry->name.def) + "##Localization").c_str(), ImVec2(0, 100), true, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (ImGui::InputTextWithPaste("Name", entry->name.key))
			edited = true;
		if (ImGui::InputTextWithPaste("Description", entry->desc.key))
			edited = true;
		ImGui::EndChild();
	}

	if (entry->is_setting()) {
		setting_base* setting = dynamic_cast<setting_base*>(entry);
		ImGui::Text("Serialization");
		if (ImGui::BeginChild((std::string(setting->name.def) + "##serialization").c_str(), ImVec2(0, 100), true, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (ImGui::InputTextWithPasteRequired("ini ID", setting->ini_id))
				edited = true;

			if (ImGui::InputTextWithPasteRequired("ini Section", setting->ini_section))
				edited = true;

			if (ImGui::InputTextWithPaste("Game Setting", setting->gameSetting)) {
				edited = true;
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
	if (edited) {
		json_dirty_mods.insert(mod);
	}
	//ImGui::EndChild();
	ImGui::PopID();
}



void ModSettings::show_entry(entry_base* entry, mod_setting* mod)
{
	ImGui::PushID(entry);
	bool edited = false;
	
	// check if all control requirements are met
	bool available = entry->control.satisfied();
	if (!available) {
		switch (entry->control.failAction) {
		case entry_base::Control::FailAction::kFailAction_Hide:
			if (!edit_mode) { // use disable fail action under edit mode
				ImGui::PopID();
				return;
			}
		default:
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
		}
	}
		
	float width = ImGui::GetContentRegionAvail().x * 0.5f;
	switch (entry->type) {
	case kEntryType_Checkbox:
		{
			setting_checkbox* checkbox = dynamic_cast<setting_checkbox*>(entry);

			if (ImGui::Checkbox(checkbox->name.get(), &checkbox->value)) {
				edited = true;
			}
			if (ImGui::IsItemHovered()) {
				if (ImGui::IsKeyPressed(ImGuiKey_R)) {
					edited |= checkbox->reset();
				}
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

			// Set the width of the slider to a fraction of the available width
			ImGui::SetNextItemWidth(width);
			if (ImGui::SliderFloatWithSteps(slider->name.get(), &slider->value, slider->min, slider->max, slider->step)) {
				edited = true;
			}

			if (ImGui::IsItemHovered()) {
				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow))) {
					if (slider->value > slider->min) {
						slider->value -= slider->step;
						edited = true;
					}
				}
				if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow))) {
					if (slider->value < slider->max) {
						slider->value += slider->step;
						edited = true;
					}
				}

				if (ImGui::IsKeyPressed(ImGuiKey_R)) {
					edited |= slider->reset();
				}
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
				edited = true;
			}

			if (!textbox->desc.empty()) {
				ImGui::SameLine();
				ImGui::HoverNote(textbox->desc.get());
			}

			if (ImGui::IsItemHovered()) {
				if (ImGui::IsKeyPressed(ImGuiKey_R)) {
					edited |= textbox->reset();

				}
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
			const char* preview_value = "";
			if (selected >= 0 && selected < options.size()) {
				preview_value = cstrings[selected];
			}
			ImGui::SetNextItemWidth(width);
			if (ImGui::BeginCombo(name, preview_value)) {
				for (int i = 0; i < options.size(); i++) {
					bool is_selected = (selected == i);

					if (ImGui::Selectable(cstrings[i], is_selected)) {
						selected = i;
						dropdown->value = selected;
						edited = true;
					}

					if (is_selected) {
						ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}

			if (ImGui::IsItemHovered()) {
				if (ImGui::IsKeyPressed(ImGuiKey_R)) {
					edited |= dropdown->reset();
				}
			}

			if (!dropdown->desc.empty()) {
				ImGui::SameLine();
				ImGui::HoverNote(dropdown->desc.get());
			}
		}
		break;
	case kEntryType_Text:
		{
			entry_text* t = dynamic_cast<entry_text*>(entry);
			ImGui::TextColored(t->_color, t->name.get());
		}
		break;
	case kEntryType_Group:
		{
			entry_group* g = dynamic_cast<entry_group*>(entry);
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			if (ImGui::CollapsingHeader(g->name.get())) {
				if (ImGui::IsItemHovered() && !g->desc.empty()) {
					ImGui::SetTooltip(g->desc.get());
				}
				ImGui::PopStyleColor();
				show_entries(g->entries, mod);
			} else {
				ImGui::PopStyleColor();
			}
		}
		break;
	case kEntryType_Keymap:
		{
			setting_keymap* k = dynamic_cast<setting_keymap*>(entry);
			std::string hash = std::to_string((unsigned long long)(void**)k);
			if (ImGui::Button("Remap")) {
				ImGui::OpenPopup(hash.data());
				keyMapListening = k;
			}
			ImGui::SameLine();
			if (ImGui::Button("Unmap")) {
				k->value = 0;
				edited = true;
			}
			ImGui::SameLine();
			ImGui::Text("%s:", k->name.get());
			ImGui::SameLine();
			ImGui::Text(setting_keymap::keyid_to_str(k->value));

			if (!k->desc.empty()) {
				ImGui::SameLine();
				ImGui::HoverNote(k->desc.get());
			}

			if (ImGui::BeginPopupModal(hash.data())) {
				ImGui::Text("Enter the key you wish to map");
				if (keyMapListening == nullptr) {
					edited = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
		}
		break;
	case kEntryType_Color:
		{
			setting_color* c = dynamic_cast<setting_color*>(entry);
			ImGui::SetNextItemWidth(width);
			setting_color* color = dynamic_cast<setting_color*>(entry);
			float colorArray[4] = { color->color.x, color->color.y, color->color.z, color->color.w };
			if (ImGui::ColorEdit4(color->name.get(), colorArray)) {
				color->color = ImVec4(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
				edited = true;
			}
			if (ImGui::IsItemHovered()) {
				if (ImGui::IsKeyPressed(ImGuiKey_R)) {
					edited |= color->reset();
				}
			}
			if (!c->desc.empty()) {
				ImGui::SameLine();
				ImGui::HoverNote(c->desc.get());
			}
		}
		break;
	default:
		break;
	}
	if (!available) { // disabled before
		ImGui::PopStyleVar();
		ImGui::PopItemFlag();
		ImGui::PopStyleColor();
	}
	if (edited) {
		ini_dirty_mods.insert(mod);
	}
	ImGui::PopID();
}

void ModSettings::show_entries(std::vector<entry_base*>& entries, mod_setting* mod)
{
	ImGui::PushID(&entries);
	bool edited = false;

	
	for (auto it = entries.begin(); it != entries.end(); it++) {
		ModSettings::entry_base* entry = *it;

		ImGui::PushID(entry);

		ImGui::Indent();
		// edit entry
		bool entry_deleted = false;
		bool all_entries_deleted = false;

		if (edit_mode) {
			if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {
				if (it != entries.begin()) {
					std::iter_swap(it, it - 1);
					edited = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::ArrowButton("##down", ImGuiDir_Down)) {
				if (it != entries.end() - 1) {
					std::iter_swap(it, it + 1);
					edited = true;
				}
			}
			ImGui::SameLine();

			if (ImGui::Button("Edit")) {  // Get the size of the current window
				ImGui::OpenPopup("Edit Setting");
				ImVec2 windowSize = ImGui::GetWindowSize();
				// Set the size of the pop-up to be proportional to the window size
				ImVec2 popupSize(windowSize.x * 0.5f, windowSize.y * 0.5f);
				ImGui::SetNextWindowSize(popupSize);
			}
			if (ImGui::BeginPopup("Edit Setting", ImGuiWindowFlags_AlwaysAutoResize)) {
				// Get the size of the current window
				show_entry_edit(entry, mod);
				ImGui::EndPopup();
			}

			
			ImGui::SameLine();

			// delete  button
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
			if (ImGui::Button("Delete")) {
				// delete entry
				ImGui::OpenPopup("Delete Confirmation");
				// move popup mousepos
				ImVec2 mousePos = ImGui::GetMousePos();
				ImGui::SetNextWindowPos(mousePos);
			}
			if (ImGui::BeginPopupModal("Delete Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Are you sure you want to delete this setting?");
				ImGui::Separator();

				if (ImGui::Button("Yes", ImVec2(120, 0))) {
					if (entry->type == entry_type::kEntryType_Checkbox) {
						setting_checkbox* checkbox = (setting_checkbox*)entry;
						m_checkbox_toggle.erase(checkbox->control_id);
					}
					bool should_decrement = it != entries.begin();
					it = entries.erase(it);
					if (should_decrement) {
						it--;
					}
					if (it == entries.end()) {
						all_entries_deleted = true;
					}
					edited = true;
					delete entry;
					entry_deleted = true;
					// TODO: delete everything in a group if deleting group.
					ImGui::CloseCurrentPopup();
				}
				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("No", ImVec2(120, 0))) {
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::PopStyleColor();
			ImGui::SameLine();

		}

		if (!entry_deleted) {
			show_entry(entry, mod);
		}

		ImGui::Unindent();
		ImGui::PopID();
		if (all_entries_deleted) {
			break;
		}
	}

		// add entry
	if (edit_mode) {
		if (ImGui::Button("New Entry")) {
			// correct popup position
			ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());
			ImGui::OpenPopup("Add Entry");
		}
		if (ImGui::BeginPopup("Add Entry")) {
			if (ImGui::Selectable("Checkbox")) {
				setting_checkbox* checkbox = new setting_checkbox();
				entries.push_back(checkbox);
				edited = true;
				INFO("added entry");
			}
			if (ImGui::Selectable("Slider")) {
				setting_slider* slider = new setting_slider();
				entries.push_back(slider);
				edited = true;
				INFO("added entry");
			}
			if (ImGui::Selectable("Textbox")) {
				setting_textbox* textbox = new setting_textbox();
				entries.push_back(textbox);
				edited = true;
				INFO("added entry");
			}
			if (ImGui::Selectable("Dropdown")) {
				setting_dropdown* dropdown = new setting_dropdown();
				dropdown->options.push_back("Option 0");
				dropdown->options.push_back("Option 1");
				dropdown->options.push_back("Option 2");
				entries.push_back(dropdown);
				edited = true;
				INFO("added entry");
			}
			if (ImGui::Selectable("Keymap")) {
				setting_keymap* keymap = new setting_keymap();
				entries.push_back(keymap);
				edited = true;
				INFO("added entry");
			}
			if (ImGui::Selectable("Text")) {
				entry_text* text = new entry_text();
				entries.push_back(text);
				edited = true;
				INFO("added entry");
			}
			if (ImGui::Selectable("Group")) {
				entry_group* group = new entry_group();
				entries.push_back(group);
				edited = true;
				INFO("added entry");
			}
			if (ImGui::Selectable("Color")) {
				setting_color* color = new setting_color();
				entries.push_back(color);
				edited = true;
				INFO("added entry");
			}

			ImGui::EndPopup();
		}
	}
	if (edited) {
		json_dirty_mods.insert(mod);
	}
	ImGui::PopID();
}

inline void ModSettings::SendSettingsUpdateEvent(std::string& modName)
{
	auto eventSource = SKSE::GetModCallbackEventSource();
	if (!eventSource) {
		return;
	}
	SKSE::ModCallbackEvent callbackEvent;
	callbackEvent.eventName = "dmenu_updateSettings";
	callbackEvent.strArg = modName;
	eventSource->SendEvent(&callbackEvent);
}



void ModSettings::show_modSetting(mod_setting* mod)
{

	show_entries(mod->entries, mod);

}



void ModSettings::submitInput(uint32_t id)
{
	if (keyMapListening == nullptr) {
		return;
	}

	
	keyMapListening->value = id;
	keyMapListening = nullptr;
}

inline std::string ModSettings::get_type_str(entry_type t)
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
	case entry_type::kEntryType_Color:
		return "color";
	case entry_type::kEntryType_Keymap:
		return "keymap";
	default:
		return "invalid";
	}
}

void ModSettings::show()
{
	
	// a button on the rhs of this same line
	ImGui::SameLine(ImGui::GetWindowWidth() - 100.0f);  // Move cursor to the right side of the window
	ImGui::ToggleButton("Edit Config", &edit_mode);

		// Set window padding and item spacing
	const float padding = 8.0f;
	const float spacing = 8.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, spacing));

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	for (auto& mod : mods) {
		if (ImGui::CollapsingHeader(mod->name.c_str())) {
			show_modSetting(mod);
		}
	}
	ImGui::PopStyleColor();
	
	ImGui::PopStyleVar();

	show_buttons_window();
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
		insert_game_setting(mod);
		flush_game_setting(mod);
	}
	INFO("Mod settings initialized");
}

ModSettings::entry_base* ModSettings::load_json_non_group(nlohmann::json& json)
{
	entry_base* e = nullptr;
	std::string type_str = json["type"].get<std::string>();
	if (type_str == "checkbox") {
		setting_checkbox* scb = new setting_checkbox();
		scb->value = json.contains("default") ? json["default"].get<bool>() : false;
		scb->default_value = scb->value;
		if (json.contains("control")) {
			if (json["control"].contains("id")) {
				scb->control_id = json["control"]["id"].get<std::string>();
				m_checkbox_toggle[scb->control_id] = scb;
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
		entry_text* et = new entry_text();
		if (json.contains("style") && json["style"].contains("color")) {
			et->_color = ImVec4(json["style"]["color"]["r"].get<float>(), json["style"]["color"]["g"].get<float>(), json["style"]["color"]["b"].get<float>(), json["style"]["color"]["a"].get<float>());
		}
		e = et;
	} else if (type_str == "color") {
		setting_color* sc = new setting_color();
		sc->default_color = ImVec4(json["default"]["r"].get<float>(), json["default"]["g"].get<float>(), json["default"]["b"].get<float>(), json["default"]["a"].get<float>());
		sc->color = sc->default_color;
		e = sc;
	} else if (type_str == "keymap") {
		setting_keymap* skm = new setting_keymap();
		skm->default_value = json["default"].get<int>();
		skm->value = skm->default_value;
		e = skm;
	} else {
		INFO("Unknown setting type: {}", type_str);
		return nullptr;
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

ModSettings::entry_group* ModSettings::load_json_group(nlohmann::json& group_json)
{
	entry_group* group = new entry_group();
	for (auto& entry_json : group_json["entries"]) {
		entry_base* entry = load_json_entry(entry_json);
		if (entry) {
			group->entries.push_back(entry);
		}
	}
	return group;
}

ModSettings::entry_base* ModSettings::load_json_entry(nlohmann::json& entry_json)
{
	ModSettings::entry_base* entry = nullptr;
	if (entry_json["type"].get<std::string>() == "group") {
		entry = load_json_group(entry_json);
	} else {
		entry = load_json_non_group(entry_json);
	}
	if (entry == nullptr) {
		INFO("ERROR: Failed to load json entry.");
		return nullptr;
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
	entry->control.failAction = entry_base::Control::kFailAction_Disable;

	if (entry_json.contains("control")) {
		if (entry_json["control"].contains("requirements")) {
			for (auto& req_json : entry_json["control"]["requirements"]) {
				entry_base::Control::Req req;
				req.id = req_json["id"].get<std::string>();
				req._not = !req_json["value"].get<bool>();
				std::string req_type = req_json["type"].get<std::string>();
				if (req_type == "checkbox") {
					req.type = entry_base::Control::Req::ReqType::kReqType_Checkbox;
				} else if (req_type == "gameSetting") {
					req.type = entry_base::Control::Req::ReqType::kReqType_GameSetting;
				}
				else {
					INFO("Error: unknown requirement type: {}", req_type);
					continue;
				}
				entry->control.reqs.push_back(req);
			}
		}
		if (entry_json["control"].contains("failAction")) {
			std::string failAction = entry_json["control"]["failAction"].get<std::string>();
			if (failAction == "disable") {
				entry->control.failAction = entry_base::Control::kFailAction_Disable;
			} else if (failAction == "hide") {
				entry->control.failAction = entry_base::Control::kFailAction_Hide;
			}
		}
	}
	
	return entry;
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
			entry_base* entry = load_json_entry(entry_json);
			if (entry) {
				mod->entries.push_back(entry);
			}
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

void ModSettings::populate_non_group_json(entry_base* entry, nlohmann::json& json)
{
	if (!entry->is_setting()) {
		if (entry->type == entry_type::kEntryType_Text) {
			json["style"]["color"]["r"] = dynamic_cast<entry_text*>(entry)->_color.x;
			json["style"]["color"]["g"] = dynamic_cast<entry_text*>(entry)->_color.y;
			json["style"]["color"]["b"] = dynamic_cast<entry_text*>(entry)->_color.z;
			json["style"]["color"]["a"] = dynamic_cast<entry_text*>(entry)->_color.w;
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
	} else if (setting->type == entry_type::kEntryType_Slider) {
		auto slider_setting = dynamic_cast<setting_slider*>(entry);
		json["default"] = slider_setting->default_value;
		json["style"]["min"] = slider_setting->min;
		json["style"]["max"] = slider_setting->max;
		json["style"]["step"] = slider_setting->step;

	} else if (setting->type == entry_type::kEntryType_Textbox) {
		auto textbox_setting = dynamic_cast<setting_textbox*>(entry);
		json["default"] = textbox_setting->default_value;

	} else if (setting->type == entry_type::kEntryType_Dropdown) {
		auto dropdown_setting = dynamic_cast<setting_dropdown*>(entry);
		json["default"] = dropdown_setting->default_value;
		for (auto& option : dropdown_setting->options) {
			json["options"].push_back(option);
		}
	} else if (setting->type == entry_type::kEntryType_Color) {
		auto color_setting = dynamic_cast<setting_color*>(entry);
		json["default"]["r"] = color_setting->default_color.x;
		json["default"]["g"] = color_setting->default_color.y;
		json["default"]["b"] = color_setting->default_color.z;
		json["default"]["a"] = color_setting->default_color.w;
	} else if (setting->type == entry_type::kEntryType_Keymap) {
		auto keymap_setting = dynamic_cast<setting_keymap*>(entry);
		json["default"] = keymap_setting->default_value;
	}
	
}

void ModSettings::populate_group_json(entry_group* group, nlohmann::json& group_json)
{
	for (auto& entry : group->entries) {
		nlohmann::json entry_json;
		populate_entry_json(entry, entry_json);
		group_json["entries"].push_back(entry_json);
	}
}
void ModSettings::populate_entry_json(entry_base* entry, nlohmann::json& entry_json)
{
	// common fields for entry
	entry_json["text"]["name"] = entry->name.def;
	entry_json["text"]["desc"] = entry->desc.def;
	entry_json["translation"]["name"] = entry->name.key;
	entry_json["translation"]["desc"] = entry->desc.key;
	entry_json["type"] = get_type_str(entry->type);

	auto control_json = entry_json["control"];

	for (auto& req : entry->control.reqs) {
		nlohmann::json req_json;
		std::string req_type = "";
		switch (req.type) {
		case entry_base::Control::Req::kReqType_Checkbox:
			req_type = "checkbox";
			break;
		case entry_base::Control::Req::kReqType_GameSetting:
			req_type = "gameSetting";
			break;
		default:
			req_type = "ERRORTYPE";
			break;
		}
		req_json["type"] = req_type;
		req_json["value"] = !req._not;
		req_json["id"] = req.id;
		control_json["requirements"].push_back(req_json);
	}
	switch (entry->control.failAction) {
	case entry_base::Control::kFailAction_Disable:
		control_json["failAction"] = "disable";
		break;
	case entry_base::Control::kFailAction_Hide:
		control_json["failAction"] = "hide";
		break;
	default:
		control_json["failAction"] = "ERROR";
		break;
	}
	entry_json["control"] = control_json;
	if (entry->is_group()) {
		populate_group_json(dynamic_cast<entry_group*>(entry), entry_json);
	} else {
		populate_non_group_json(entry, entry_json);
	}
}
/* Serialize config to .json*/
void ModSettings::flush_json(mod_setting* mod)
{
	nlohmann::json mod_json;
	mod_json["name"] = mod->name;
	mod_json["ini"] = mod->ini_path;

	nlohmann::json data_json;

	for (auto& entry : mod->entries) {
		nlohmann:json entry_json;
		populate_entry_json(entry, entry_json);
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
	flush_game_setting(mod);

	INFO("Saved config for {}", mod->name);
}

void ModSettings::get_all_settings(mod_setting* mod, std::vector<ModSettings::setting_base*>& r_vec)
{
	std::stack<entry_group*> group_stack;
	for (auto& entry : mod->entries) {
		if (entry->is_group()) {
			group_stack.push(dynamic_cast<entry_group*>(entry));
		} else if (entry->is_setting()) {
			r_vec.push_back(dynamic_cast<setting_base*>(entry));
		}
	}
	while (!group_stack.empty()) {  // get all groups
		auto group = group_stack.top();
		group_stack.pop();
		for (auto& entry : group->entries) {
			if (entry->is_group()) {
				group_stack.push(dynamic_cast<entry_group*>(entry));
			} else if (entry->is_setting()) {
				r_vec.push_back(dynamic_cast<setting_base*>(entry));
			}
		}
	}
}

void ModSettings::get_all_entries(mod_setting* mod, std::vector<ModSettings::entry_base*>& r_vec)
{
	std::stack<entry_group*> group_stack;
	for (auto& entry : mod->entries) {
		if (entry->is_group()) {
			group_stack.push(dynamic_cast<entry_group*>(entry));
		}
		r_vec.push_back(entry);
	}
	while (!group_stack.empty()) {  // get all groups
		auto group = group_stack.top();
		group_stack.pop();
		for (auto& entry : group->entries) {
			if (entry->is_group()) {
				group_stack.push(dynamic_cast<entry_group*>(entry));
			}
			r_vec.push_back(entry);
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
		flush_ini(mod);
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
		bool use_default = false;
		if (ini.KeyExists(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str())) {
			value = ini.GetValue(setting_ptr->ini_section.c_str(), setting_ptr->ini_id.c_str(), "");
		} else {
			INFO("Value not found for setting {} in .ini file. Using default entry value.", setting_ptr->name.def);
			use_default = true;
		}
		if (use_default) {
			// Convert the value to the appropriate type and assign it to the setting
			if (setting_ptr->type == kEntryType_Checkbox) {
				dynamic_cast<setting_checkbox*>(setting_ptr)->value = dynamic_cast<setting_checkbox*>(setting_ptr)->default_value;
			} else if (setting_ptr->type == kEntryType_Slider) {
				dynamic_cast<setting_slider*>(setting_ptr)->value = dynamic_cast<setting_slider*>(setting_ptr)->default_value;
			} else if (setting_ptr->type == kEntryType_Textbox) {
				dynamic_cast<setting_textbox*>(setting_ptr)->value = dynamic_cast<setting_textbox*>(setting_ptr)->default_value;
			} else if (setting_ptr->type == kEntryType_Dropdown) {
				dynamic_cast<setting_dropdown*>(setting_ptr)->value = dynamic_cast<setting_dropdown*>(setting_ptr)->default_value;
			} else if (setting_ptr->type == kEntryType_Color) {
				dynamic_cast<setting_color*>(setting_ptr)->color.x = dynamic_cast<setting_color*>(setting_ptr)->default_color.x;
				dynamic_cast<setting_color*>(setting_ptr)->color.y = dynamic_cast<setting_color*>(setting_ptr)->default_color.y;
				dynamic_cast<setting_color*>(setting_ptr)->color.z = dynamic_cast<setting_color*>(setting_ptr)->default_color.z;
				dynamic_cast<setting_color*>(setting_ptr)->color.w = dynamic_cast<setting_color*>(setting_ptr)->default_color.w;
			} else if (setting_ptr->type == kEntryType_Keymap) {
				dynamic_cast<setting_keymap*>(setting_ptr)->value = dynamic_cast<setting_keymap*>(setting_ptr)->default_value;
			}
		} else {
			// Convert the value to the appropriate type and assign it to the setting
			if (setting_ptr->type == kEntryType_Checkbox) {
				dynamic_cast<setting_checkbox*>(setting_ptr)->value = (value == "true");
			} else if (setting_ptr->type == kEntryType_Slider) {
				dynamic_cast<setting_slider*>(setting_ptr)->value = std::stof(value);
			} else if (setting_ptr->type == kEntryType_Textbox) {
				dynamic_cast<setting_textbox*>(setting_ptr)->value = value;
			} else if (setting_ptr->type == kEntryType_Dropdown) {
				dynamic_cast<setting_dropdown*>(setting_ptr)->value = std::stoi(value);
			} else if (setting_ptr->type == kEntryType_Color) {
				uint32_t colUInt = std::stoul(value);
				dynamic_cast<setting_color*>(setting_ptr)->color.x = ((colUInt >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f;
				dynamic_cast<setting_color*>(setting_ptr)->color.y = ((colUInt >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f;
				dynamic_cast<setting_color*>(setting_ptr)->color.z = ((colUInt >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f;
				dynamic_cast<setting_color*>(setting_ptr)->color.w = ((colUInt >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;
			} else if (setting_ptr->type == kEntryType_Keymap) {
				dynamic_cast<setting_keymap*>(setting_ptr)->value = std::stoi(value);
			}
		}

	}
	INFO(".ini loaded.");
}



/* Flush changes to MOD into its .ini file*/
void ModSettings::flush_ini(mod_setting* mod)
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
		else if (setting->type == kEntryType_Color) {
			// from imvec4 float to imu32
			auto sc = dynamic_cast<setting_color*>(setting);
			// Step 1: Extract components
			uint32_t r = sc->color.x * 255.0f;
			uint32_t g = sc->color.y * 255.f;
			uint32_t b = sc->color.z * 255.f;
			uint32_t a = sc->color.w * 255.f;
			ImU32 col = IM_COL32(r, g, b, a);
			value = std::to_string(col);
		} else if (setting->type == kEntryType_Keymap) {
			value = std::to_string(dynamic_cast<setting_keymap*>(setting)->value);
		}
		ini.SetValue(setting->ini_section.c_str(), setting->ini_id.c_str(), value.c_str());
	}

	// Save the ini file
	ini.SaveFile(mod->ini_path.c_str());
}




/* Flush changes to MOD's settings to game settings.*/
void ModSettings::flush_game_setting(mod_setting* mod)
{
	std::vector<ModSettings::setting_base*> settings;
	get_all_settings(mod, settings);
	auto gsc = RE::GameSettingCollection::GetSingleton();
	if (!gsc) {
		INFO("Game setting collection not found when trying to save game setting.");
		return;
	}
	for (auto& setting : settings) {
		if (setting->gameSetting.empty()) {
			continue;
		}
		RE::Setting* s = gsc->GetSetting(setting->gameSetting.c_str());
		if (!s) {
			INFO("Error: Game setting not found when trying to save game setting {} for setting {}", setting->gameSetting, setting->name.def);
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
		} else if (setting->type == kEntryType_Keymap) {
			s->SetInteger(dynamic_cast<setting_keymap*>(setting)->value);
		} else if (setting->type == kEntryType_Color) {
			setting_color* sc = dynamic_cast<setting_color*>(setting);
			uint32_t r = sc->color.x * 255.0f;
			uint32_t g = sc->color.y * 255.f;
			uint32_t b = sc->color.z * 255.f;
			uint32_t a = sc->color.w * 255.f;
			ImU32 col = IM_COL32(r, g, b, a);
			s->SetUnsignedInteger(col);
		}
	}
}

void ModSettings::insert_game_setting(mod_setting* mod) 
{
	std::vector<ModSettings::entry_base*> entries;
	get_all_entries(mod, entries); // must get all entries to inject settings for the entry's control requirements
	auto gsc = RE::GameSettingCollection::GetSingleton();
	if (!gsc) {
		INFO("Game setting collection not found when trying to insert game setting.");
		return;
	}
	for (auto& entry : entries) {
		if (entry->is_setting()) {
			auto es = dynamic_cast<ModSettings::setting_base*>(entry);
			// insert setting setting
			if (!es->gameSetting.empty()) {                     // gamesetting mapping
				if (gsc->GetSetting(es->gameSetting.c_str())) {  // setting already exists
					return;
				}
				RE::Setting* s = new RE::Setting(es->gameSetting.c_str());
				gsc->InsertSetting(s);
				if (!gsc->GetSetting(es->gameSetting.c_str())) {
					INFO("ERROR: Failed to insert game setting.");
				}
			}
		}
		
		// inject setting for setting's req
		for (auto req : entry->control.reqs) {
			if (req.type == entry_base::Control::Req::kReqType_GameSetting) {
				if (!gsc->GetSetting(req.id.c_str())) {
					RE::Setting* s = new RE::Setting(req.id.c_str());
					gsc->InsertSetting(s);
				}
			}
		}
	}
}

void ModSettings::save_all_game_setting()
{
	for (auto mod : mods) {
		flush_game_setting(mod);
	}
}

void ModSettings::insert_all_game_setting()
{
	for (auto mod : mods) {
		insert_game_setting(mod);
	}
}
//
//bool ModSettings::API_RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback)
//{
//	INFO("Received registration for {} update.", a_mod);
//	for (auto mod : mods) {
//		if (mod->name == a_mod) {
//			mod->callbacks.push_back(a_callback);
//			INFO("Successfully added callback for {} update.", a_mod);
//			return true;
//		}
//	}
//	ERROR("{} mod not found.", a_mod);
//	return false;
//}
//
//
//
//namespace Example
//{
//	bool RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback) 
//	{
//		static auto dMenu = GetModuleHandle("dmenu");
//		using _RegisterForSettingUpdate = bool (*)(std::string, std::function<void()>);
//		static auto func = reinterpret_cast<_RegisterForSettingUpdate>(GetProcAddress(dMenu, "RegisterForSettingUpdate"));
//		if (func) {
//			return func(a_mod, a_callback);
//		}
//		return false;
//	}
//}
//
//extern "C" DLLEXPORT bool RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback)
//{
//	return ModSettings::API_RegisterForSettingUpdate(a_mod, a_callback);
//}

const char* ModSettings::setting_keymap::keyid_to_str(int key_id)
{
	switch (key_id) {
	case 0: 
		return "Unmapped";
	case 1:
		return "Escape";
	case 2:
		return "1";
	case 3:
		return "2";
	case 4:
		return "3";
	case 5:
		return "4";
	case 6:
		return "5";
	case 7:
		return "6";
	case 8:
		return "7";
	case 9:
		return "8";
	case 10:
		return "9";
	case 11:
		return "0";
	case 12:
		return "Minus";
	case 13:
		return "Equals";
	case 14:
		return "Backspace";
	case 15:
		return "Tab";
	case 16:
		return "Q";
	case 17:
		return "W";
	case 18:
		return "E";
	case 19:
		return "R";
	case 20:
		return "T";
	case 21:
		return "Y";
	case 22:
		return "U";
	case 23:
		return "I";
	case 24:
		return "O";
	case 25:
		return "P";
	case 26:
		return "Left Bracket";
	case 27:
		return "Right Bracket";
	case 28:
		return "Enter";
	case 29:
		return "Left Control";
	case 30:
		return "A";
	case 31:
		return "S";
	case 32:
		return "D";
	case 33:
		return "F";
	case 34:
		return "G";
	case 35:
		return "H";
	case 36:
		return "J";
	case 37:
		return "K";
	case 38:
		return "L";
	case 39:
		return "Semicolon";
	case 40:
		return "Apostrophe";
	case 41:
		return "~ (Console)";
	case 42:
		return "Left Shift";
	case 43:
		return "Back Slash";
	case 44:
		return "Z";
	case 45:
		return "X";
	case 46:
		return "C";
	case 47:
		return "V";
	case 48:
		return "B";
	case 49:
		return "N";
	case 50:
		return "M";
	case 51:
		return "Comma";
	case 52:
		return "Period";
	case 53:
		return "Forward Slash";
	case 54:
		return "Right Shift";
	case 55:
		return "NUM*";
	case 56:
		return "Left Alt";
	case 57:
		return "Spacebar";
	case 58:
		return "Caps Lock";
	case 59:
		return "F1";
	case 60:
		return "F2";
	case 61:
		return "F3";
	case 62:
		return "F4";
	case 63:
		return "F5";
	case 64:
		return "F6";
	case 65:
		return "F7";
	case 66:
		return "F8";
	case 67:
		return "F9";
	case 68:
		return "F10";
	case 69:
		return "Num Lock";
	case 70:
		return "Scroll Lock";
	case 71:
		return "NUM7";
	case 72:
		return "NUM8";
	case 73:
		return "NUM9";
	case 74:
		return "NUM-";
	case 75:
		return "NUM4";
	case 76:
		return "NUM5";
	case 77:
		return "NUM6";
	case 78:
		return "NUM+";
	case 79:
		return "NUM1";
	case 80:
		return "NUM2";
	case 81:
		return "NUM3";
	case 82:
		return "NUM0";
	case 83:
		return "NUM.";
	case 87:
		return "F11";
	case 88:
		return "F12";
	case 156:
		return "NUM Enter";
	case 157:
		return "Right Control";
	case 181:
		return "NUM/";
	case 183:
		return "SysRq / PtrScr";
	case 184:
		return "Right Alt";
	case 197:
		return "Pause";
	case 199:
		return "Home";
	case 200:
		return "Up Arrow";
	case 201:
		return "PgUp";
	case 203:
		return "Left Arrow";
	case 205:
		return "Right Arrow";
	case 207:
		return "End";
	case 208:
		return "Down Arrow";
	case 209:
		return "PgDown";
	case 210:
		return "Insert";
	case 211:
		return "Delete";
	case 256:
		return "Left Mouse Button";
	case 257:
		return "Right Mouse Button";
	case 258:
		return "Middle/Wheel Mouse Button";
	case 259:
		return "Mouse Button 3";
	case 260:
		return "Mouse Button 4";
	case 261:
		return "Mouse Button 5";
	case 262:
		return "Mouse Button 6";
	case 263:
		return "Mouse Button 7";
	case 264:
		return "Mouse Wheel Up";
	case 265:
		return "Mouse Wheel Down";
	case 266:
		return "DPAD_UP";
	case 267:
		return "DPAD_DOWN";
	case 268:
		return "DPAD_LEFT";
	case 269:
		return "DPAD_RIGHT";
	case 270:
		return "START";
	case 271:
		return "BACK";
	case 272:
		return "LEFT_THUMB";
	case 273:
		return "RIGHT_THUMB";
	case 274:
		return "LEFT_SHOULDER";
	case 275:
		return "RIGHT_SHOULDER";
	case 276:
		return "A";
	case 277:
		return "B";
	case 278:
		return "X";
	case 279:
		return "Y";
	case 280:
		return "LT";
	case 281:
		return "RT";
	default:
		return "Unknown Key";
	}
}
