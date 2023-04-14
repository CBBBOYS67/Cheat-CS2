#pragma once

class Translator
{
	using TranslationID = std::string;

public:
	enum Language
	{
		English,
		German,
		French,
		Spanish,
		Italian,
		Russian,
		Polish,
		Portuguese,
		Japanese,
		Chinese,
		Korean,
		Turkish,
		Arabic,
		Invalid
	};

	static std::string AsLanguageStr(Language language);

	static Language _language;  // language to translate to, from translation id

	static void LoadTranslations(Language language);

	// translate a string to the current language. Returns a null-pointer if the matching translation is not found.
	static const char* Translate(TranslationID id);

	static void LoadTranslation(std::filesystem::path txt_path);

private:
	static inline std::unordered_map<TranslationID, std::string> _translations;
};


struct Translatable
{
	Translatable();
	Translatable(std::string def, std::string key);
	Translatable(std::string def);
	std::string def;              // default string
	std::string key;  // key to look up in the translation file
	const char* get() const;      // get the translated string, or the default if no translation is found note the ptr points to the original data.
	bool empty();
};
