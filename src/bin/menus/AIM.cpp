#include "imgui.h"
#include "imgui_internal.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "AIM.h"

#include "bin/dMenu.h"
#include <unordered_set>

#include "bin/Utils.h"
// i swear i will RE this
inline void sendConsoleCommand(std::string a_command)
{
	const auto scriptFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>();
	const auto script = scriptFactory ? scriptFactory->Create() : nullptr;
	if (script) {
		const auto selectedRef = RE::Console::GetSelectedRef();
		script->SetCommand(a_command);
		script->CompileAndRun(selectedRef.get());
		delete script;
	}
}

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

static bool _init = false;


static std::vector<std::pair<std::string, bool>> _types = {
	{ "Weapon", false },
	{ "Armor", false },
	{ "Ammo", false },
	{ "Book", false }, 
	{ "Ingredient", false },
	{ "Key", false },
	{ "Misc", false },
	{ "NPC", false }
};

static std::vector<std::pair<RE::TESFile*, bool>> _mods;
static int _selectedModIndex = -1;

static std::vector<std::pair<std::string, RE::TESForm*>> _items; // items to show on AIM
static RE::TESForm* _selectedItem;

static bool _cached = false;

static ImGuiTextFilter _modFilter;
static ImGuiTextFilter _itemFilter;


template <class T>
inline void loadPlugins(std::unordered_set<RE::TESFile*>& mods)
{
	auto data = RE::TESDataHandler::GetSingleton();
	for (auto form : data->GetFormArray<T>()) {
		if (form && strl(form->GetName()) != 0) {
			if (!mods.contains(form->GetFile())) {
				mods.insert(form->GetFile());
			}
		}
	}
}
void AIM::init()
{
	if (_init) {
		return;
	}
	RE::TESDataHandler* data = RE::TESDataHandler::GetSingleton();
	std::unordered_set<RE::TESFile*> mods;

	// only show mods with valid items
	loadPlugins<RE::TESObjectWEAP>(mods);
	loadPlugins<RE::TESObjectARMO>(mods);
	loadPlugins<RE::TESAmmo>(mods);
	loadPlugins<RE::TESObjectBOOK>(mods);
	loadPlugins<RE::IngredientItem>(mods);
	loadPlugins<RE::TESKey>(mods);
	loadPlugins<RE::TESObjectMISC>(mods);
	loadPlugins<RE::TESNPC>(mods);

	for (auto mod : mods) {
		_mods.push_back({ mod, false });
	}

	_selectedModIndex = 0;

	_init = true;
	INFO("AIM initialized");
}


template <class T>
inline void cacheItems(RE::TESDataHandler* a_data)
{
	RE::TESFile* selectedMod = _mods[_selectedModIndex].first;
	for (auto form : a_data->GetFormArray<T>()) {
		if (form && form->GetFile() == selectedMod 
			&& strl(form->GetName()) != 0) {
			_items.push_back({ form->GetName(), form });
		}
	}
}
/* Present all filtered items to the user under the "Items" section*/
void cache()
{
	_items.clear();
	auto data = RE::TESDataHandler::GetSingleton();
	if (!data || _selectedModIndex == -1) {
		return;
	}
	if (_types[7].second) {
		cacheItems<RE::TESNPC>(data);
		return;
	}
	
	if (_types[0].second)
		cacheItems<RE::TESObjectWEAP>(data);
	if (_types[1].second)
		cacheItems<RE::TESObjectARMO>(data);
	if (_types[2].second)
		cacheItems<RE::TESAmmo>(data);
	if (_types[3].second)
		cacheItems<RE::TESObjectBOOK>(data);
	if (_types[4].second)
		cacheItems<RE::IngredientItem>(data);
	if (_types[5].second)
		cacheItems<RE::TESKey>(data);
	if (_types[6].second)
		cacheItems<RE::TESObjectMISC>(data);
	// filter out unplayable weapons in 2nd pass
	for (auto it = _items.begin(); it != _items.end();) {
		auto form = it->second;
		auto formtype = form->GetFormType();
		switch (formtype) {
		case RE::FormType::Weapon:
			if (form->As<RE::TESObjectWEAP>()->weaponData.flags.any(RE::TESObjectWEAP::Data::Flag::kNonPlayable)) {
				it = _items.erase(it);
				continue;
			}
			break;
		}
		it++;
	}

}

void AIM::show()
{
	// Begin the filter box
	_modFilter.Draw("Mod Name");
	// Render the dropdown menu
	if (ImGui::BeginCombo("Mods", _mods[_selectedModIndex].first->GetFilename().data())) {
		for (int i = 0; i < _mods.size(); i++) {
			if (_modFilter.PassFilter(_mods[i].first->GetFilename().data())) {
				bool isSelected = (_mods[_selectedModIndex].first == _mods[i].first);
				if (ImGui::Selectable(_mods[i].first->GetFilename().data(), isSelected)) {
					_selectedModIndex = i;
					_cached = false;
				}
				if (isSelected) {
					ImGui::SetItemDefaultFocus();
				}
			}
		}
		ImGui::EndCombo();
	}

	// item type filtering
	for (int i = 0; i < _types.size() - 1; i++) { // exclude NPC
		if (ImGui::Checkbox(_types[i].first.data(), &_types[i].second)) {  // Weapon | Armor | Consumables ....
			_cached = false;
		}
		ImGui::SameLine();
	}
	if (Utils::imgui::ToggleButton(_types[_types.size() - 1].first.c_str(), &_types[_types.size() - 1].second)) {
		_cached = false;
	}


	if (!_cached) {
		cache();
	}

	// Render the list of items with 
	_itemFilter.Draw("Item Name");

	ImGui::BeginChild("Items", DMenu::relativeSize(0.7, 0.5), true);

	for (int i = 0; i < _items.size(); i++) {
		// filter
		if (_itemFilter.PassFilter(_items[i].first.data())) {
			if (ImGui::Selectable(_items[i].first.data())) {
				// do something
				_selectedItem = _items[i].second;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::Text(fmt::format("{:x}", _items[i].second->GetFormID()).data());
				ImGui::EndTooltip();
			}
		}
	}
	ImGui::EndChild();
	
	
	if (_selectedItem != nullptr) {
		ImGui::Text(_selectedItem->GetName());
	}

	static char buf[16] = "1";
	// button to spawn selected item
	if (ImGui::Button("Spawn", DMenu::relativeSize(0.2, 0.1))) {
		if (_selectedItem != nullptr && RE::PlayerCharacter::GetSingleton() != nullptr) {
			// spawn item
			uint32_t formIDuint = _selectedItem->GetFormID();
			std::string formID = fmt::format("{:x}", formIDuint);
			std::string addCmd = _selectedItem->GetFormType() == RE::FormType::NPC ? "placeatme" : "additem";
			std::string cmd = "player." + addCmd + " " + formID + " " + buf;
			sendConsoleCommand(cmd);
		}
	}
	ImGui::SameLine();
	ImGui::InputText("Amount", buf, 16, ImGuiInputTextFlags_CharsDecimal);
}
