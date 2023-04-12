#include "PCH.h"
#include <unordered_set>
class ModSettings
{
	class Translations
	{
		struct StringHash
		{
			std::size_t operator()(std::string_view str) const
			{
				std::size_t hash = 0;
				for (char c : str) {
					hash = (hash * 31) + c;
				}
				return hash;
			}
		};
		static std::unordered_map<std::string_view, std::string, StringHash> translation_EN;
		static std::unordered_map<std::string_view, std::string, StringHash> translation_FR;
		static std::unordered_map<std::string_view, std::string, StringHash> translation_CN;
		static std::unordered_map<std::string_view, std::string, StringHash> translation_RU;

		public:
		static void init();
		static std::string_view lookup(std::string_view a_key);
	};

	
	enum setting_type
	{
		kSettingType_Checkbox,
		kSettingType_Slider,
		kSettingType_Textbox,
		kSettingType_Dropdown,
		kSettingType_Invalid
	};

	class setting_base
	{
	public:
		setting_type type;
		std::string name;
		std::string desc;
		
		std::string ini_section;
		std::string ini_id;

		std::string gameSetting;
		
		std::vector<std::string> req;
		std::vector<std::string> req_not;
		bool req_met = false;
		bool hide_if_no_req = false;
		virtual ~setting_base() = default;
	};

	class setting_checkbox : public setting_base
	{
	public:
		setting_checkbox()
		{
			type = kSettingType_Checkbox;
		}
		bool value;
	};

	class setting_slider : public setting_base
	{
	public:
		setting_slider() { type = kSettingType_Slider; }
		float value = 0.0f;
		float min = 0.0f;
		float max = 1.0f;
		float step = 0.01f;
		uint8_t precision = 2;  // number of decimal places
	};
	
	class setting_textbox : public setting_base
	{
	public:
		std::string value;
		char* buf;
		size_t buf_size;
		setting_textbox() { type = kSettingType_Textbox; }
	};

	class setting_dropdown : public setting_base
	{
	public:
		setting_dropdown() { type = kSettingType_Dropdown; }
		std::vector<std::string> options;
		uint8_t value = 0;  // index into options
	};

	/* Settings of one mod, represented by one .json file and serialized to one .ini file.*/
	class mod_setting
	{
	public:
		class mod_setting_group
		{
		public:
			std::string name;
			std::vector<setting_base*> settings;
		};
		std::string name;
		std::vector<mod_setting_group*> groups;
		bool dirty;
		std::string ini_path;

		std::vector<std::function<void()>> callbacks;
	};

	/* Settings of all mods*/
	static std::vector<mod_setting*> mods;

	static std::vector<mod_setting*> mods_dirty;  // mods that need to be serialized back to disk

public:

	static void show(); // called by imgui per tick
	
	/* Load settings config from .json files and saved settings from .ini files*/
	static void init();
	
	private:
	/* Load a single mod from .json file*/
	static void load_mod(std::string mod_path);
	static void load_ini(mod_setting* mod);
	static void construct_ini(mod_setting* mod);
	static void save_ini(mod_setting* mod);

	static void save_game_setting(mod_setting* mod);
	static void insert_game_setting(mod_setting* mod);

public:
	static bool API_RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback);

private:
	// for show()
	static void show_saveButton();
	static void show_modSetting(mod_setting* mod);
};
