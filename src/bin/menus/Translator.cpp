#include "Translator.h"

std::string Translator::AsLanguageStr(Language language)
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

void Translator::LoadTranslations(Language language)
{
	INFO("(re)Initializing {} translator.", AsLanguageStr(language));
	_translations.clear();
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
		return it->second.c_str();
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

Translatable::Translatable(std::string def, std::string key)
{
	this->def = def;
	this->key = key;
}

Translatable::Translatable(std::string def)
{
	this->def = def;
	this->key = "";
}
Translatable::Translatable()
{
	this->def = "";
	this->key = "";
}

const char* Translatable::get() const
{
	const char* ret = Translator::Translate(key);
	return ret ? ret : def.c_str();
}

bool Translatable::empty()
{
	return def.empty() && key.empty();
}
