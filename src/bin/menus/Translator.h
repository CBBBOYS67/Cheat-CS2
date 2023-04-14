class Translator
{
    using TranslationID = std::string;
    public:
    enum class Language{
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

    Language AsLanguage(const std::string& language)
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

    std::string AsLanguageStr(Language language)
    {
        switch (language)
        {
        case Language::English:
            return "EN";
        case Language::German:
            return "GE";
        case Language::French:
            return "FR";
        case Language::Spanish:
            return "SP";
        case Language::Italian:
            return "IT";
        case Language::Russian:
            return "RU";
        case Language::Polish:
            return "PL";
        case Language::Portuguese:
            return "PO";
        case Language::Japanese:
            return "JA";
        case Language::Chinese:
            return "CH";
        case Language::Korean:
            return "KO";
        case Language::Turkish:
            return "TU";
        case Language::Arabic:
            return "AR";
        default:
            return "Invalid";
        }
    }

    Language _language; // language to translate to, from translation id

    std::unordered_map<TranslationID, std::string> _translations;

static const std::string TRANSLATION_DIR = "Data\\SKSE\\Plugins\\\dmenu\\translations";

    Translator(Language language)
    {
        std::string language_id = "_" + AsLanguageStr(language);
        for (auto& file : std::filesystem::directory_iterator(TRANSLATION_DIR)) {
            if (file.path().extension() == ".txt" && file.path().stem().string().ends_with(language_id)) {
                LoadTranslation(file.path());
            }
        }
    }

    const char* Translate(TranslationID id)
    {
        auto it = _translations.find(id);
        if (it != _translations.end())
            return it->second.c_str();
        else
            return id.c_str();
    }

    void LoadTranslation(std::filesystem::path txt_path)
    {
        std::ifstream file(txt_path);
        std::string line;
        while (std::getline(file, line))
        {
            size_t tab_pos = line.find('\t');
            if (tab_pos != std::string::npos)
            {
                TranslationID key = line.substr(0, tab_pos);
                std::string value = line.substr(tab_pos + 1);
                _translations[key] = value;
            }
        }
        file.close();
    }

}