#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "imgui_internal.h"
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

		// Calculate epsilon value as a fraction of the step size
		const float epsilon = v_step * 0.001f;

		// Map from [v_min,v_max] to [0,N] with epsilon correction
		const int countValues = int((v_max - v_min) / v_step + epsilon);
		int v_i = int((*v - v_min) / v_step + epsilon);
		const bool value_changed = SliderInt(label, &v_i, 0, countValues, text_buf);
		// Remap from [0,N] to [v_min,v_max] with epsilon correction
		*v = v_min + (float(v_i) + epsilon) * v_step;
		// eliminate rounding errors to v_step precision
		*v = roundf(*v / v_step) * v_step;
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
	void InputTextStr(const char* label, std::string* dest, size_t max_len)
	{
		char* buffer = new char[max_len];
		strncpy(buffer, dest->c_str(), max_len);
		buffer[max_len] = '\0';

		ImGui::InputText(label, buffer, max_len);

		*dest = buffer;
		dest->resize(strnlen(buffer, max_len));
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
}
