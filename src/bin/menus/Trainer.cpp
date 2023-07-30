#include "imgui.h"
#include "imgui_internal.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "bin/Utils.h"
#include "Trainer.h"

namespace World
{
	namespace Time
	{
		bool _sliderActive = false;
		void show()
		{
			auto calendar = RE::Calendar::GetSingleton();
			float* ptr = nullptr;
			if (calendar) {
				ptr = &(calendar->gameHour->value);
				if (ptr) {
					ImGui::SliderFloat("Time", ptr, 0.0f, 24.f);
					_sliderActive = ImGui::IsItemActive();
					return;
				}
			}
			ImGui::Text("Time address not found");
		}

	}

	namespace Weather
	{
		RE::TESRegion* _currRegionCache = nullptr;  // after fw `region` field of sky gets reset. restore from this.
		RE::TESRegion* getCurrentRegion()
		{
			if (RE::Sky::GetSingleton()) {
				auto ret =  RE::Sky::GetSingleton()->region;
				if (ret) {
					_currRegionCache = ret;
					return ret;
				}
			}
			return _currRegionCache;
		}
		// maps to store formEditorID as names for regions and weathers as they're discarded by bethesda
		std::unordered_map<RE::TESRegion*, std::string> _regionNames;
		std::unordered_map<RE::TESWeather*, std::string> _weatherNames;
		
		std::vector<RE::TESWeather*> _weathersToSelect;
		static std::vector<RE::TESFile*> _mods;  // plugins containing weathers
		static uint8_t _mods_i = 0;              // selected weather plugin
		bool _cached = false;
		
		bool _showCurrRegionOnly = true; // only show weather corresponding to current region
		
		bool _lockWeather = false;

		static std::vector<std::pair<std::string, bool>> _filters = {
			{ "Pleasant", false },
			{ "Cloudy", false },
			{ "Rainy", false },
			{ "Snow", false },
			{ "Permanent aurora", false },
			{ "Aurora follows sun", false }
		};

		void cache()
		{
			_weathersToSelect.clear();
			auto regionDataManager = RE::TESDataHandler::GetSingleton()->regionDataManager;
			if (!regionDataManager) {
				return;
			}
			for (auto weather : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESWeather>()) {
				if (!weather) {
					continue;
				}
				auto flags = weather->data.flags;
				if (_filters[0].second) {
					if (!flags.any(RE::TESWeather::WeatherDataFlag::kPleasant)) {
						continue;
					}
				}
				if (_filters[1].second) {
					if (!flags.any(RE::TESWeather::WeatherDataFlag::kCloudy)) {
						continue;
					}
				}
				if (_filters[2].second) {
					if (!flags.any(RE::TESWeather::WeatherDataFlag::kRainy)) {
						continue;
					}
				}
				if (_filters[3].second) {
					if (!flags.any(RE::TESWeather::WeatherDataFlag::kSnow)) {
						continue;
					}
				}
				if (_filters[4].second) {
					if (!flags.any(RE::TESWeather::WeatherDataFlag::kPermAurora)) {
						continue;
					}
				}
				if (_filters[5].second) {
					if (!flags.any(RE::TESWeather::WeatherDataFlag::kAuroraFollowsSun)) {
						continue;
					}
				}
				if (_showCurrRegionOnly) {
					bool belongsToCurrRegion = false;
					auto currRegion = getCurrentRegion();
					if (currRegion) { // could've cached regionData -> weathers at the cost of a bit of extra space, but this runs fine in O(n2) since there's limited # of weathers
						for (RE::TESRegionData* regionData : currRegion->dataList->regionDataList) {
							if (regionData->GetType() == RE::TESRegionData::Type::kWeather) {
								RE::TESRegionDataWeather* weatherData = regionDataManager->AsRegionDataWeather(regionData);
								for (auto t : weatherData->weatherTypes) {
									if (t->weather == weather) {
										belongsToCurrRegion = true;
									}
								}
							}
						}
					}
					if (!belongsToCurrRegion) {
						continue;
					}
				}
				_weathersToSelect.push_back(weather);
			}
		}
	

		void show()
		{
			RE::TESRegion* currRegion = nullptr;
			RE::TESWeather* currWeather = nullptr;
			if (RE::Sky::GetSingleton()) {
				currRegion = getCurrentRegion();
				currWeather = RE::Sky::GetSingleton()->currentWeather;
			}

			// List of weathers to choose
			if (ImGui::BeginCombo("##Weathers", _weatherNames[currWeather].c_str())) {
				for (int i = 0; i < _weathersToSelect.size(); i++) {
					bool isSelected = (_weathersToSelect[i] == currWeather);
					if (ImGui::Selectable(_weatherNames[_weathersToSelect[i]].c_str(), isSelected)) {
						if (!isSelected) {
							auto sky = RE::Sky::GetSingleton();
							sky->ForceWeather(_weathersToSelect[i], true);
						}
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			//ImGui::SameLine();
			//ImGui::Checkbox("Lock Weather", &_lockWeather);

			// Display filtering controls
			ImGui::Text("Flags:");

			for (int i = 0; i < _filters.size(); i++) {
				if (ImGui::Checkbox(_filters[i].first.c_str(), &_filters[i].second)) {
					_cached = false;
				}
				if (i < _filters.size() - 1) {
					ImGui::SameLine();
				}
			}

			// Display current region
			if (currRegion) {
				ImGui::Text("Current Region:");
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", _regionNames[currRegion]);
			} else {
				ImGui::Text("Current region not found");
			}

			ImGui::SameLine();

			ImGui::Checkbox("Current Region Only", &_showCurrRegionOnly);
			if (_showCurrRegionOnly) {
				ImGui::SameLine();
				ImGui::TextDisabled("(?!)");
				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text("Only display weather suitable for the current region.");
					ImGui::EndTooltip();
				}
			}

			if (!_cached) {
				cache();
			}
		}

		void init()
		{
			_cached = false;
			// load list of plugins with available weathers
			std::unordered_set<RE::TESFile*> plugins;
			Utils::loadUsefulPlugins<RE::TESWeather>(plugins);
			for (auto plugin : plugins) {
				_mods.push_back(plugin);
			}
			
			auto data = RE::TESDataHandler::GetSingleton();
			// load region&weather names
			for (RE::TESWeather* weather : data->GetFormArray<RE::TESWeather>()) {
				_weatherNames.insert({ weather, weather->GetFormEditorID() });
			}
			for (RE::TESRegion* region : data->GetFormArray<RE::TESRegion>()) {
				_regionNames.insert({ region, region->GetFormEditorID() });
			}
		}
	}

	void show() 
	{
		if (!RE::Sky::GetSingleton() || !RE::Sky::GetSingleton()->currentWeather) {
			ImGui::Text("World not loaded");
			return;
		}
		// Use consistent padding and alignment
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));

		// Indent the contents of the window
		ImGui::Indent();
		ImGui::Spacing();

		// Display time controls
		ImGui::Text("Time:");
		Time::show();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Display weather controls
		ImGui::Text("Weather:");
		Weather::show();

		// Unindent the contents of the window
		ImGui::Unindent();

		// Use consistent padding and alignment
		ImGui::PopStyleVar(2);
	}

	void init()
	{
		Weather::init();
	}
}


void Trainer::show() 
{
	ImGui::PushID("World");
	if (ImGui::CollapsingHeader("World")) {
		World::show();
	}
	ImGui::PopID();
}

void Trainer::init()
{
	// Weather
	World::init();


	INFO("Trainer initialized.");
}

bool Trainer::isWeatherLocked()
{
	return World::Time::_sliderActive;
}


// deprecated code
//if (ImGui::BeginCombo("WeatherMod", _mods[_mods_i]->GetFilename().data())) {
//	for (int i = 0; i < _mods.size(); i++) {
//		bool isSelected = (_mods[_mods_i] == _mods[i]);
//		if (ImGui::Selectable(_mods[i]->GetFilename().data(), isSelected)) {
//			_mods_i = i;
//			_cached = false;
//		}
//		if (isSelected) {
//			ImGui::SetItemDefaultFocus();
//		}
//	}
//	ImGui::EndCombo();
//}
