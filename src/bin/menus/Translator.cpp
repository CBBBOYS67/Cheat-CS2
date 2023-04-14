#include "Translator.h"

Translator::Language Translator::AsLanguage(const std::string& language)
{
	if (language == "EN")
		return Language::English;
	else if (language == "GE")
		return Language::German;
	else if (language == "FR")
		return Language::French;
	else if (language == "SP")
		return Language::Spanish;
	else if (language == "IT")
		return Language::Italian;
	else if (language == "RU")
		return Language::Russian;
	else if (language == "PL")
		return Language::Polish;
	else if (language == "PO")
		return Language::Portuguese;
	else if (language == "JA")
		return Language::Japanese;
	else if (language == "CH")
		return Language::Chinese;
	else if (language == "KO")
		return Language::Korean;
	else if (language == "TU")
		return Language::Turkish;
	else if (language == "AR")
		return Language::Arabic;
	else
		return Language::Invalid;
}

inline std::string Translator::AsLanguageStr(Language language)
{
	switch (language) {
	case Language::English:
		return "english";
	case Language::German:
		return "german";
	case Language::French:
		return "french";
	case Language::Spanish:
		return "spanish";
	case Language::Italian:
		return "italian";
	case Language::Russian:
		return "russian";
	case Language::Polish:
		return "polish";
	case Language::Portuguese:
		return "protuguese";
	case Language::Japanese:
		return "japanese";
	case Language::Chinese:
		return "chinese";
	case Language::Korean:
		return "korean";
	case Language::Turkish:
		return "turkish";
	case Language::Arabic:
		return "arabic";
	default:
		return "Invalid";
	}
}
static const std::string TRANSLATION_DIR = "Data\\SKSE\\Plugins\\\dmenu\\customSettings\\translations";

Translator::Translator(Language language)
{
	INFO("Initializing translator for %s", AsLanguageStr(language));
	std::string language_id = "_" + AsLanguageStr(language);
	for (auto& file : std::filesystem::directory_iterator(TRANSLATION_DIR)) {
		if (file.path().extension() == ".txt" && file.path().stem().string().ends_with(language_id)) {
			INFO("loading translation {}", file.path().string());
			LoadTranslation(file.path());
		}
	}
}

const char* Translator::Translate(TranslationID id)
{
	auto it = _translations.find(id);
	if (it != _translations.end())
		return it->second.data();
	else
		return nullptr;
}

void Translator::LoadTranslation(std::filesystem::path txt_path)
{
	std::ifstream file(txt_path);
	std::string line;
	while (std::getline(file, line)) {
		size_t tab_pos = line.find('\t');
		if (tab_pos != std::string::npos) {
			TranslationID key = line.substr(0, tab_pos);
			std::string value = line.substr(tab_pos + 1);
			_translations[key] = value;
		}
	}
	file.close();
}
