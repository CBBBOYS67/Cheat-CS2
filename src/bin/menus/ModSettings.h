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
	class setting_checkbox;

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

		bool editing = false;

		bool incomplete();

		virtual ~setting_base() = default;
	};

	class setting_checkbox : public setting_base
	{
	public:
		setting_checkbox()
		{
			type = kSettingType_Checkbox;
			name = "New Checkbox";
			value = true;
			default_value = true;
		}
		bool value;
		bool default_value;
		std::string control_id;

	};

	class setting_slider : public setting_base
	{
	public:
		setting_slider() 
		{
			type = kSettingType_Slider; 
			name = "New Slider";
			value = 0.0f;
			min = 0.0f;
			max = 1.0f;
			step = 0.1f;
			default_value = 0.f;
		}
		float value;
		float min;
		float max;
		float step;
		float default_value;
		uint8_t precision = 2;  // number of decimal places
	};
	
	class setting_textbox : public setting_base
	{
	public:
		std::string value;
		char* buf;
		int buf_size;
		std::string default_value;
		setting_textbox() 
		{ 
			type = kSettingType_Textbox; 
			name = "New Textbox";
			value = "";
			default_value = "";
		}
	};

	class setting_dropdown : public setting_base
	{
	public:
		setting_dropdown() 
		{ 
			type = kSettingType_Dropdown; 
			name = "New Dropdown";
			value = 0;
			default_value = 0;
		}
		std::vector<std::string> options;
		int value;  // index into options
		int default_value;
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
		bool json_dirty;
		std::string ini_path;
		std::string json_path;

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
	static void load_json(std::filesystem::path a_path);
	static void save_json(mod_setting* mod);
	static void load_ini(mod_setting* mod);
	static void construct_ini(mod_setting* mod);
	static void save_ini(mod_setting* mod);

	static void save_game_setting(mod_setting* mod);
	static void insert_game_setting(mod_setting* mod);

public:
	static void save_all_game_setting();

public:
	static bool API_RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback);

private:
	// for show()
	static void show_saveButton();
	static void show_saveJsonButton();
	static void show_modSetting(mod_setting* mod);
	static void show_setting_author(setting_base* base, mod_setting* mod);
	static void show_setting_user(setting_base* base, mod_setting* mod);
	static inline std::unordered_map<std::string, setting_checkbox*> m_controls;

	static inline bool edit_mode = false;
};
