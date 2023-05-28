#pragma once
#include "animation_handler.h"
#include "handle/data/page/position_setting.h"
#include "image_path.h"

namespace ui {
    struct image {
        ID3D11ShaderResourceView* texture = nullptr;
        int32_t width = 0;
        int32_t height = 0;
    };

    class ui_renderer {
        using page_setting = handle::position_setting;
        using slot_type = handle::slot_setting::slot_type;
        using position_type = handle::position_setting::position_type;

        struct wnd_proc_hook {
            static LRESULT thunk(HWND h_wnd, UINT u_msg, WPARAM w_param, LPARAM l_param);
            static inline WNDPROC func;
        };

        ui_renderer();

        static void draw_animations_frame();
        static void draw_text(float a_x,
            float a_y,
            float a_offset_x,
            float a_offset_y,
            float a_offset_extra_x,
            float a_offset_extra_y,
            const char* a_text,
            uint32_t a_alpha,
            uint32_t a_red,
            uint32_t a_green,
            uint32_t a_blue,
            float a_font_size = 20.f,
            bool a_center_text = true,
            bool a_deduct_text_x = false,
            bool a_deduct_text_y = false,
            bool a_add_text_x = false,
            bool a_add_text_y = false);
        static void draw_element(ID3D11ShaderResourceView* a_texture,
            ImVec2 a_center,
            ImVec2 a_size,
            float a_angle,
            ImU32 a_color = IM_COL32_WHITE);
        static void draw_hud(float a_x, float a_y, float a_scale_x, float a_scale_y, uint32_t a_alpha);
        static void draw_slot(float a_screen_x,
            float a_screen_y,
            float a_scale_x,
            float a_scale_y,
            float a_offset_x,
            float a_offset_y,
            uint32_t a_modify,
            uint32_t a_alpha);
        static void init_animation(animation_type animation_type,
            float a_screen_x,
            float a_screen_y,
            float a_scale_x,
            float a_scale_y,
            float a_offset_x,
            float a_offset_y,
            uint32_t a_modify,
            uint32_t a_alpha,
            float a_duration);
        static void draw_slots(float a_x, float a_y, const std::map<position_type, page_setting*>& a_settings);
        static void draw_key(float a_x,
            float a_y,
            float a_scale_x,
            float a_scale_y,
            float a_offset_x,
            float a_offset_y,
            uint32_t a_alpha = 255);
        static void draw_keys(float a_x, float a_y, const std::map<position_type, page_setting*>& a_settings);
        static void draw_icon(float a_x,
            float a_y,
            float a_scale_x,
            float a_scale_y,
            float a_offset_x,
            float a_offset_y,
            icon_image_type a_type,
            uint32_t a_alpha);
        static void draw_key_icon(float a_x,
            float a_y,
            float a_scale_x,
            float a_scale_y,
            float a_offset_x,
            float a_offset_y,
            uint32_t a_key,
            uint32_t a_alpha);
        static void draw_ui();

        static bool load_texture_from_file(const char* filename,
            ID3D11ShaderResourceView** out_srv,
            std::int32_t& out_width,
            std::int32_t& out_height);

        static inline bool show_ui_ = false;
        static inline ID3D11Device* device_ = nullptr;
        static inline ID3D11DeviceContext* context_ = nullptr;

        template <typename T>
        static void
            load_images(std::map<std::string, T>& a_map, std::map<uint32_t, image>& a_struct, std::string& file_path);

        static void load_animation_frames(std::string& file_path, std::vector<image>& frame_list);

        static image get_key_icon(uint32_t a_key);
        static void load_font();

    public:
        static float get_resolution_scale_width();
        static float get_resolution_scale_height();

        static float get_resolution_width();
        static float get_resolution_height();

        static void set_fade(bool a_in, float a_value);
        static bool get_fade();

        static void toggle_show_ui();
        static void set_show_ui(bool a_show);

        static void load_all_images();

        struct d_3d_init_hook {
            static void thunk();
            static inline REL::Relocation<decltype(thunk)> func;

            static constexpr auto id = REL::RelocationID(75595, 77226);
            static constexpr auto offset = REL::VariantOffset(0x9, 0x275, 0x00);  // VR unknown

            static inline std::atomic<bool> initialized = false;
        };

        struct dxgi_present_hook {
            static void thunk(std::uint32_t a_p1);
            static inline REL::Relocation<decltype(thunk)> func;

            static constexpr auto id = REL::RelocationID(75461, 77246);
            static constexpr auto offset = REL::Offset(0x9);
        };
    };
}
