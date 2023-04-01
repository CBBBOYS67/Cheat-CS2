#include "Hooks.h"
#include "menus/Trainer.h"
void Hooks::Install()
{
	SKSE::AllocTrampoline(1 << 5);
	onWeatherChange::install();
}

void Hooks::onWeatherChange::updateWeather(RE::Sky* a_sky)
{
	if (Trainer::isWeatherLocked()) {
		return;
	}
	_updateWeather(a_sky);
}
