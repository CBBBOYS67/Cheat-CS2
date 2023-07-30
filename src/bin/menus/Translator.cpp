#include "Translator.h"

void Translator::ReLoadTranslations()
{
	_translations.clear();
}

const char* Translator::Translate(std::string id)
{
	auto it = _translations.find(id);
	if (it != _translations.end()) {
		return it->second.first ? it->second.second.c_str() : nullptr;
	} else {
		std::string res = "";
		SKSE::Translation::Translate(id, res);
		bool hasTranslation = !res.empty();
		_translations[id] = { hasTranslation, res };
		return hasTranslation ? res.c_str() : nullptr;
	}
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
