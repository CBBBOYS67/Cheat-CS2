#pragma once

namespace Settings
{
	enum Language
	{
		English,
		ChineseSimplified,
		ChineseTraditional
	};

	float relative_window_size_h;
	float relative_window_size_v;

	Language language;

	void show();

	void init();
}
