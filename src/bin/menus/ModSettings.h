#include "PCH.h"
#include <unordered_set>
#include "Translator.h"
class ModSettings
{
	enum entry_type
	{
		kEntryType_Checkbox,
		kEntryType_Slider,
		kEntryType_Textbox,
		kEntryType_Dropdown,
		kEntryType_Text,
		kEntryType_Header,
		kSettingType_Invalid
	};

	class entry
	{
	public:
		entry_type type;
		Translatable name;
		Translatable desc;
		std::vector<std::string> req;
		bool editing = false;
		constexpr bool is_setting() const { return type != kEntryType_Header && type != kEntryType_Text; }
		virtual ~entry() = default;
	};

	class text : entry
	{
	public:
		text()
		{
			type = kEntryType_Text;
			name = Translatable("New Text");
		}
	};

	class header : entry
	{
	public:
		header()
		{
			type = kEntryType_Header;
			name = Translatable("New Header");
		}
	};
	
	class setting_checkbox;

	class setting_base : public entry
	{
	public:
		std::string ini_section;
		std::string ini_id;

		std::string gameSetting;
		virtual ~setting_base() = default;
	};

	class setting_checkbox : public setting_base
	{
	public:
		setting_checkbox()
		{
			type = kEntryType_Checkbox;
			name = Translatable("New Checkbox");
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
			type = kEntryType_Slider; 
			name = Translatable("New Slider");
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
			type = kEntryType_Textbox; 
			name = Translatable("New Textbox");
			value = "";
			default_value = "";
		}
	};

	class setting_dropdown : public setting_base
	{
	public:
		setting_dropdown() 
		{ 
			type = kEntryType_Dropdown; 
			name = Translatable("New Dropdown");
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
			Translatable name;
			Translatable desc;
			std::vector<entry*> entries;
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
	static inline std::vector<mod_setting*> mods;

public:

	static void show(); // called by imgui per tick
	
	/* Load settings config from .json files and saved settings from .ini files*/
	static void init();
	
	private:
	/* Load a single mod from .json file*/
	static void load_json(std::filesystem::path a_path);
	static void save_mod_config(mod_setting* mod);
	static void load_ini(mod_setting* mod);
	static void construct_ini(mod_setting* mod);
	static void save_ini(mod_setting* mod);

	static void save_game_setting(mod_setting* mod);

	static void insert_game_setting(mod_setting* mod);

public:
	static void save_all_game_setting();
	static void insert_all_game_setting();

public:
	static bool API_RegisterForSettingUpdate(std::string a_mod, std::function<void()> a_callback);

private:
	static void show_reloadTranslationButton();
	static void show_saveButton();
	static void show_saveJsonButton();
	static void show_modSetting(mod_setting* mod);
	static void show_entry_edit(entry* base, mod_setting* mod);
	static void show_entry(entry* base, mod_setting* mod);

	static inline std::unordered_map<std::string, setting_checkbox*> m_controls;

	static inline bool edit_mode = false;
};
