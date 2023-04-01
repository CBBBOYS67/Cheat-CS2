#include "PCH.h"
namespace Hooks
{
	class onWeatherChange
	{
	public:
		static void install()
		{
			auto& trampoline = SKSE::GetTrampoline();
			REL::Relocation<std::uintptr_t> caller_setWeather{ RELOCATION_ID(25682, 26229) };  //Up	p	Sky__Update_1403B1670+29C	call    Sky__UpdateWeather_1403B1C80
			_updateWeather = trampoline.write_call<5>(caller_setWeather.address() + RELOCATION_OFFSET(0x29c, 0x000), updateWeather);
			logger::info("hook:onWeatherChange");
		}

	private:
		static void updateWeather(RE::Sky* a_sky);

		static inline REL::Relocation<decltype(updateWeather)> _updateWeather;
	};
	void Install();
}

