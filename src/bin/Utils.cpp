#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include "imgui.h"
#include "Utils.h"
namespace Utils
{
	namespace imgui
	{
		

		void HoverNote(const char* text, const char* note)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::Text(note);
			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text(text);
				ImGui::EndTooltip();
			}
		}

	}

	std::string getFormEditorID(const RE::TESForm* a_form)
	{
		switch (a_form->GetFormType()) {
		case RE::FormType::Keyword:
		case RE::FormType::LocationRefType:
		case RE::FormType::Action:
		case RE::FormType::MenuIcon:
		case RE::FormType::Global:
		case RE::FormType::HeadPart:
		case RE::FormType::Race:
		case RE::FormType::Sound:
		case RE::FormType::Script:
		case RE::FormType::Navigation:
		case RE::FormType::Cell:
		case RE::FormType::WorldSpace:
		case RE::FormType::Land:
		case RE::FormType::NavMesh:
		case RE::FormType::Dialogue:
		case RE::FormType::Quest:
		case RE::FormType::Idle:
		case RE::FormType::AnimatedObject:
		case RE::FormType::ImageAdapter:
		case RE::FormType::VoiceType:
		case RE::FormType::Ragdoll:
		case RE::FormType::DefaultObject:
		case RE::FormType::MusicType:
		case RE::FormType::StoryManagerBranchNode:
		case RE::FormType::StoryManagerQuestNode:
		case RE::FormType::StoryManagerEventNode:
		case RE::FormType::SoundRecord:
			return a_form->GetFormEditorID();
		default:
			{
				static auto tweaks = GetModuleHandle("po3_Tweaks");
				static auto func = reinterpret_cast<_GetFormEditorID>(GetProcAddress(tweaks, "GetFormEditorID"));
				if (func) {
					return func(a_form->formID);
				}
				return {};
			}
		}
	}


}

settingsLoader::settingsLoader(const char* settingsFile)
{
	_ini.LoadFile(settingsFile);
	if (_ini.IsEmpty()) {
		logger::info("Warning: {} is empty.", settingsFile);
	}
	_settingsFile = settingsFile;
}

settingsLoader::~settingsLoader()
{
	log();
}


/*Set the active section. Load() will load keys from this section.*/

void settingsLoader::setActiveSection(const char* section)
{
	_section = section;
}

/*Load a boolean value if present.*/

void settingsLoader::load(bool& settingRef, const char* key)
{
	if (_ini.GetValue(_section, key)) {
		bool val = _ini.GetBoolValue(_section, key);
		settingRef = val;
		_loadedSettings++;
	}
}

/*Load a float value if present.*/

void settingsLoader::load(float& settingRef, const char* key)
{
	if (_ini.GetValue(_section, key)) {
		float val = static_cast<float>(_ini.GetDoubleValue(_section, key));
		settingRef = val;
		_loadedSettings++;
	}
}

/*Load an unsigned int value if present.*/

void settingsLoader::load(uint32_t& settingRef, const char* key)
{
	if (_ini.GetValue(_section, key)) {
		uint32_t val = static_cast<uint32_t>(_ini.GetDoubleValue(_section, key));
		settingRef = val;
		_loadedSettings++;
	}
}

void settingsLoader::save(bool& settingRef, const char* key)
{
	if (_ini.GetValue(_section, key)) {
		if (settingRef) {
			_ini.SetValue(_section, key, "true");
		} else {
			_ini.SetValue(_section, key, "false");
		}
		_savedSettings++;
	}
}

void settingsLoader::save(float& settingRef, const char* key)
{
	if (_ini.GetValue(_section, key)) {
		_ini.SetValue(_section, key, std::to_string(settingRef).data());
		_savedSettings++;
	}
}

void settingsLoader::save(uint32_t& settingRef, const char* key)
{
	if (_ini.GetValue(_section, key)) {
		_ini.SetValue(_section, key, std::to_string(settingRef).data());
		_savedSettings++;
	}
}

/*Load an integer value if present.*/

void settingsLoader::load(int& settingRef, const char* key)
{
	if (_ini.GetValue(_section, key)) {
		int val = static_cast<int>(_ini.GetDoubleValue(_section, key));
		settingRef = val;
		_loadedSettings++;
	}
}

void settingsLoader::flush()
{
	_ini.SaveFile(_settingsFile);
}

namespace ImGui
{
	bool SliderFloatWithSteps(const char* label, float* v, float v_min, float v_max, float v_step)
	{
		char text_buf[64] = {};
		ImFormatString(text_buf, IM_ARRAYSIZE(text_buf), "%g", *v);

		// Map from [v_min,v_max] to [0,N]
		const int countValues = int((v_max - v_min) / v_step);
		int v_i = int((*v - v_min) / v_step);
		const bool value_changed = SliderInt(label, &v_i, 0, countValues, text_buf);

		// Remap from [0,N] to [v_min,v_max]
		*v = v_min + float(v_i) * v_step;
		return value_changed;
	}

	void HoverNote(const char* text, const char* note)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
		ImGui::Text(note);
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text(text);
			ImGui::EndTooltip();
		}
	}


	bool ToggleButton(const char* str_id, bool* v)
	{
		bool ret = false;
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		float height = ImGui::GetFrameHeight();
		float width = height * 1.55f;
		float radius = height * 0.50f;

		ImGui::InvisibleButton(str_id, ImVec2(width, height));
		if (ImGui::IsItemClicked()) {
			*v = !*v;
			ret = true;
		}

		float t = *v ? 1.0f : 0.0f;

		ImGuiContext& g = *GImGui;
		float ANIM_SPEED = 0.08f;
		if (g.LastActiveId == g.CurrentWindow->GetID(str_id))  // && g.LastActiveIdTimer < ANIM_SPEED)
		{
			float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
			t = *v ? (t_anim) : (1.0f - t_anim);
		}

		ImU32 col_bg;
		if (ImGui::IsItemHovered())
			col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.78f, 0.78f, 0.78f, 1.0f), ImVec4(0.64f, 0.83f, 0.34f, 1.0f), t));
		else
			col_bg = ImGui::GetColorU32(ImLerp(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), ImVec4(0.56f, 0.83f, 0.26f, 1.0f), t));

		draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
		draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));

		return ret;
	}

	bool InputTextRequired(const char* label, std::string* str, ImGuiInputTextFlags flags) 
	{
		bool empty = str->empty();
		if (empty) {
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
		}
		bool ret = ImGui::InputText(label, str, flags);
		if (empty) {
			ImGui::PopStyleColor(3);
		}
		return ret;
	}

// Callback function to handle resizing the std::string buffer
	static int InputTextCallback(ImGuiInputTextCallbackData* data)
	{
		std::string* str = static_cast<std::string*>(data->UserData);
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
			str->resize(data->BufTextLen);
			data->Buf = &(*str)[0];
		}
		return 0;
	}

	
	bool InputTextWithPaste(const char* label, std::string& text, const ImVec2& size, bool multiline, ImGuiInputTextFlags flags)
	{
		ImGui::PushID(&text);
		// Add the ImGuiInputTextFlags_CallbackResize flag to allow resizing the std::string buffer
		flags |= ImGuiInputTextFlags_CallbackResize;
	
		// Call the InputTextWithCallback function with the std::string buffer and callback function
		bool result;
		if (multiline) {
			result = ImGui::InputTextMultiline(label, &text[0], text.capacity()+1, size, flags, InputTextCallback, &text);

		} else {
			result = ImGui::InputText(label, &text[0], text.capacity() + 1, flags, InputTextCallback, &text);
		}
		
		// Check if the InputText is hovered and the right mouse button is clicked
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
			// Set the context menu to be shown
			ImGui::OpenPopup("InputTextContextMenu");
		}

		// Display the context menu
		if (ImGui::BeginPopup("InputTextContextMenu")) {
			if (ImGui::MenuItem("Copy")) {
				// Copy the selected text to the clipboard
				const char* selected_text = text.c_str();
				if (selected_text) {
					ImGui::SetClipboardText(selected_text);
				}
			}
			if (ImGui::MenuItem("Paste")) {
				// Read the clipboard content
				const char* clipboard = ImGui::GetClipboardText();

				if (clipboard) {
					// Insert the clipboard content into the text buffer
					text.append(clipboard);
				}
			}
			ImGui::EndPopup();
		}
		ImGui::PopID();
		return result;
	}

	bool InputTextWithPasteRequired(const char* label, std::string& text, const ImVec2& size, bool multiline, ImGuiInputTextFlags flags)
	{
		ImGui::PushID(&text);
		// Add the ImGuiInputTextFlags_CallbackResize flag to allow resizing the std::string buffer
		flags |= ImGuiInputTextFlags_CallbackResize;

		// Call the InputTextWithCallback function with the std::string buffer and callback function
		bool empty = text.empty();
		if (empty) {
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.2f));
		}
		bool result;
		if (multiline) {
			result = ImGui::InputTextMultiline(label, &text[0], text.capacity() + 1, size, flags, InputTextCallback, &text);

		} else {

			result = ImGui::InputText(label, &text[0], text.capacity() + 1, flags, InputTextCallback, &text);
		}
		if (empty) {
			ImGui::PopStyleColor(3);
		}

		// Check if the InputText is hovered and the right mouse button is clicked
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
			// Set the context menu to be shown
			ImGui::OpenPopup("InputTextContextMenu");
		}

		// Display the context menu
		if (ImGui::BeginPopup("InputTextContextMenu")) {
			if (ImGui::MenuItem("Copy")) {
				// Copy the selected text to the clipboard
				const char* selected_text = text.c_str();
				if (selected_text) {
					ImGui::SetClipboardText(selected_text);
				}
			}
			if (ImGui::MenuItem("Paste")) {
				// Read the clipboard content
				const char* clipboard = ImGui::GetClipboardText();

				if (clipboard) {
					// Insert the clipboard content into the text buffer
					text.append(clipboard);
				}
			}
			ImGui::EndPopup();
		}
		ImGui::PopID();
		return result;
	}
}
