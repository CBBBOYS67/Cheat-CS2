#pragma once

namespace ui {
    enum class animation_type { highlight = 0 };

    class animation {
    public:
        animation(const animation&) = delete;
        animation& operator=(const animation&) = delete;
        animation(ImVec2 a_center,
            ImVec2 a_size,
            float a_angle,
            uint32_t a_alpha,
            uint32_t a_r_color,
            uint32_t a_g_color,
            uint32_t a_b_color,
            float a_duration,
            uint32_t a_length) {
            this->center = a_center;
            this->size = a_size;
            this->angle = a_angle;
            this->alpha = a_alpha;
            this->r_color = a_r_color;
            this->g_color = a_g_color;
            this->b_color = a_b_color;
            this->duration = a_duration;
            this->length = a_length;

            this->current_frame = 0;
            this->delta_time = 0.0f;
            this->single_frame_time = duration / static_cast<float>(length);
        };

        virtual void animate_action(float){};
        virtual bool is_over() { return current_frame >= length; }
        ImVec2 center;
        ImVec2 size;
        float angle;
        uint32_t current_frame;
        uint32_t alpha;
        uint32_t r_color;
        uint32_t g_color;
        uint32_t b_color;
        float duration;
        uint32_t length;

    protected:
        float delta_time;
        float single_frame_time;
    };

    class fade_framed_out_animation : public animation {
    public:
        fade_framed_out_animation(const fade_framed_out_animation&) = delete;
        fade_framed_out_animation& operator=(const fade_framed_out_animation&) = delete;
        fade_framed_out_animation(ImVec2 a_center,
            ImVec2 a_size,
            float a_angle,
            uint32_t a_alpha,
            uint32_t a_r_color,
            uint32_t a_g_color,
            uint32_t a_b_color,
            float a_duration,
            uint32_t a_length)
            : animation(a_center, a_size, a_angle, a_alpha, a_r_color, a_g_color, a_b_color, a_duration, a_length){};
        void animate_action(float a_delta_time) override {
            if (!is_over()) {
                this->delta_time += a_delta_time;
                if (this->delta_time >= this->single_frame_time) {
                    float frames_to_add = this->delta_time / this->single_frame_time;
                    this->current_frame += static_cast<uint32_t>(std::floor(frames_to_add));
                    this->delta_time -= this->current_frame * this->single_frame_time;
                }
            }
        }

        bool is_over() override { return this->current_frame >= this->length || this->alpha <= 0; }
    };
}
