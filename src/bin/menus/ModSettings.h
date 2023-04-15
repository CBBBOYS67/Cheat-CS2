#include "PCH.h"
#include <unordered_set>
#include "Translator.h"
#include "imgui.h"
#include "nlohmann/json.hpp"

class ModSettings
{
public:
	enum entry_type
	{
		kEntryType_Checkbox,
		kEntryType_Slider,
		kEntryType_Textbox,
		kEntryType_Dropdown,
		kEntryType_Text,
		kEntryType_Group,
		kSettingType_Invalid
	};

	static std::string get_type_str(entry_type t);

	class Entry
	{
	public:
		entry_type type;
		Translatable name;
		Translatable desc;
		std::vector<std::string> req;
		bool editing = false;
		virtual bool is_setting() const { return false; }
		virtual ~Entry() = default;
		virtual bool is_group() const { return false; }
	};

	class Entry_text : public Entry
	{
	public:
		ImVec4 _color;

		Entry_text()
		{
			type = kEntryType_Text;
			name = Translatable("New Text");
			_color = ImVec4(1, 1, 1, 1);
		}

	};


	class Entry_group : public Entry
	{
	public:
		std::vector<Entry*> entries;
		
		Entry_group()
		{
			type = kEntryType_Group;
			name = Translatable("New Group");
		}

		bool is_group() const override { return true; }
	};

	
	class setting_checkbox;

	class setting_base : public Entry
	{
	public:
		std::string ini_section;
		std::string ini_id;

		std::string gameSetting;
		bool is_setting() const override { return true; }
		virtual ~setting_base() = default;
		virtual bool reset() { return false; };
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
		bool reset() override
		{
			bool changed = value != default_value;
			value = default_value;
			return changed;
		}
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
		bool reset() override
		{
			bool changed = value != default_value;
			value = default_value;
			return changed;
		}
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
		bool reset() override 
		{ 
			bool changed = value != default_value;
			value = default_value;
			return changed;
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
		bool reset() override
		{
			bool changed = value != default_value;
			value = default_value;
			return changed;
		}
	};

	/* Settings of one mod, represented by one .json file and serialized to one .ini file.*/
	class mod_setting
	{
	public:
		std::string name;
		std::vector<Entry*> entries;
		std::string ini_path;
		std::string json_path;

		std::vector<std::function<void()>> callbacks;
	};

	/* Settings of all mods*/
	static inline std::vector<mod_setting*> mods;
	
	static inline std::unordered_set<mod_setting*> json_dirty_mods;  // mods whose changes need to be flushed to .json file. i.e. author has changed its setting
	static inline std::unordered_set<mod_setting*> ini_dirty_mods;   // mods whose changes need to be flushed to .ini or gamesetting. i.e.  user has changed its setting

public:

	static void show(); // called by imgui per tick
	
	/* Load settings config from .json files and saved settings from .ini files*/
	static void init();
	
	private:
	/* Load a single mod from .json file*/
		
	/* Read everything in group_json and populate entries*/
	static Entry* load_json_non_group(nlohmann::json& json);
	static Entry_group* load_json_group(nlohmann::json& group_json);
	static Entry* load_json_entry(nlohmann::json& json);
	static void load_json(std::filesystem::path a_path);
	
	static void populate_non_group_json(Entry* group, nlohmann::json& group_json);
	static void populate_group_json(Entry_group* group, nlohmann::json& group_json);
	static void populate_entry_json(Entry* entry, nlohmann::json& entry_json);

	static void flush_json(mod_setting* mod);
	
	static void get_all_settings(mod_setting* mod, std::vector<ModSettings::setting_base*>& r_vec);


	static void load_ini(mod_setting* mod);
	static void flush_ini(mod_setting* mod);
	
	static void flush_game_setting(mod_setting* mod);

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
	static void show_entry_edit(Entry* base, mod_setting* mod);
	static void show_entry(Entry* base, mod_setting* mod);
	static void show_entries(std::vector<Entry*>& entries, mod_setting* mod);

	static inline std::unordered_map<std::string, setting_checkbox*> m_controls;

	static inline bool edit_mode = false;
};
