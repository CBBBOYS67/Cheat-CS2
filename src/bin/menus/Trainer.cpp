#include "imgui.h"
#include "imgui_internal.h"
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "Trainer.h"
inline void _time() 
{
	auto calendar = RE::Calendar::GetSingleton();
	float* ptr = nullptr;
	if (calendar) {
		ptr = &(calendar->gameHour->value);
	}
	if (_time) {
		ImGui::SliderFloat("Time", ptr, 0.0f, 24.f);
	}
}

inline void _weather() 
{
	//RE::PlayerCharacter::GetSingleton()->GetWorldspace()->
}


void Trainer::show() 
{
	ImGui::PushID("World");
	if (ImGui::CollapsingHeader("World")) {
		_time();
	}
	ImGui::PopID();
}
