#pragma once
#include <unordered_set>
inline size_t strl(const char* a_str)
{
	if (a_str == nullptr)
		return 0;
	size_t len = 0;
	while (a_str[len] != '\0' && len < 0x7FFFFFFF) {
		len++;
	}
	return len;
}

namespace Utils
{
	namespace imgui
	{
		bool ToggleButton(const char* str_id, bool* v);
	}


	template <class T>
	void loadUsefulPlugins(std::unordered_set<RE::TESFile*>& mods)
	{
		auto data = RE::TESDataHandler::GetSingleton();
		for (auto form : data->GetFormArray<T>()) {
			if (form) {
				if (!mods.contains(form->GetFile())) {
					mods.insert(form->GetFile());
				}
			}
		}
	};

	using _GetFormEditorID = const char* (*)(std::uint32_t);
	
	std::string getFormEditorID(const RE::TESForm* a_form);
}

