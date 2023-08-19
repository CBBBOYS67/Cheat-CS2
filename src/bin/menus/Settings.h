#pragma once
#include "Translator.h"
class Settings
{
public:
	static inline float relative_window_size_h = 1.f;
	static inline float relative_window_size_v = 1.f;
	static inline float windowPos_x = 0.f;
	static inline float windowPos_y = 0.f;

	static inline bool lockWindowPos = false;
	static inline bool lockWindowSize = false;

	static inline float fontScale = 1.f;

	static inline uint32_t key_toggle_dmenu = 199;

	static void show();

	static void init();
};
