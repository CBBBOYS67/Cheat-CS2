#pragma once
#include "PCH.h"

class InputListener : public RE::BSTEventSink<RE::InputEvent*>
{
public:
	static InputListener* GetSingleton()
	{
		static InputListener listener;
		return std::addressof(listener);
	}

	virtual RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;
};
