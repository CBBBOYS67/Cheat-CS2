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

	Language AsLanguage(const std::string& language);

	std::string AsLanguageStr(Language language);

	Language _language;  // language to translate to, from translation id

	std::unordered_map<TranslationID, std::string> _translations;


	Translator(Language language);

	// translate a string to the current language. Returns a null-pointer if the matching translation is not found.
	const char* Translate(TranslationID id);

	void LoadTranslation(std::filesystem::path txt_path);
};
