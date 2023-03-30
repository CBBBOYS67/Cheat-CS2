#pragma once

class Settings
{
public:
	enum class Language
	{
		English = 0,
		ChineseSimplified = 1,
		ChineseTraditional = 2
	};

	static inline float relative_window_size_h = 1.f;
	static inline float relative_window_size_v = 1.f;
	static inline float windowPos_x = 0.f;
	static inline float windowPos_y = 0.f;

	static inline bool lockWindowPos = false;
	static inline bool lockWindowSize = false;

	static inline Language language = Language::English;

	static void show();

	static void init();
};
