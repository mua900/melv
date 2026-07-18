#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "ui.hpp"
#include "asset.hpp"
#include "input.hpp"
#include "draw.hpp"
#include "time.hpp"
#include "camera.hpp"

#include "util/common.hpp"
#include "util/template.hpp"
#include "util/math_util.hpp"

class Application;

struct Event_Timeout {
    s64 event = 0;
    bool active = false;
};

#define EVENT_TIMEOUT_LONG 1000000.0

enum Events {
    EVENT_DUMMY,
    EVENT_COUNT,
};

typedef bool (*InitCallback)(void *userdata, Application* app);
typedef bool (*EventCallback)(SDL_Event event, void *userdata, Application* app);
typedef void (*InputCallback)(void *userdata, Application* app);
typedef void (*DrawCallback)(void *userdata, Application* app);
typedef void (*BeforeCleanupCallback)(void *userdata, Application* app);
typedef void (*AfterCleanupCallback)(void *userdata, Application* app);

typedef void (*UpdateFunction)(void *userdata, TimeInfo time);
typedef void (*FixedUpdateFunction)(void *userdata);

struct UpdateState {
    UpdateFunction update = nullptr;
    FixedUpdateFunction fixedUpdate = nullptr;
    s64 ticks = 0;
    double elapsed = 0;
    double timeScale = 0;
	int updateRate = 0;

    double calculateTimeStep() { return 1.0 / updateRate; }
};

// these callbacks won't be called if they are null
// user code can set them however it wants
struct UserData {
	// passed to every single user function
	void* userdata = nullptr;

	UpdateState* update_state = {};

	InitCallback init = nullptr;
	EventCallback event = nullptr;
	InputCallback input = nullptr;
	DrawCallback draw = nullptr;
	BeforeCleanupCallback before_cleanup = nullptr;
	AfterCleanupCallback after_cleanup = nullptr;

	KeyboardCallback keyboard = nullptr;
	MouseCallback mouse = nullptr;
};

class Application {
public:
    Window m_window = {};
    RenderContext m_render = {};
    AudioPlayer m_audio_player = {};
    Input m_input = {};
    AssetCatalog m_catalog = {};

    DArray<UiState> m_ui = {};
    DArray<UpdateState> m_update_states = {};

    cobot::Color m_clear_color = DEFAULT_BACKGROUND_COLOR;

    TimeInfo m_time = {};

    Event_Timeout m_events[EVENT_COUNT] = {};

    DArray<Camera> cameras = {};

    AssetId m_font = {};
    AssetId m_editor_font = {};

    bool quit = false;
    bool doing_text_input = false;

	// fill this out
	UserData user = {};

    bool initialize();

    void handle_events();
    void update();
    void draw();

    void cleanup();
private:
    bool init_render();
    bool init_shaders();

    Camera* get_active_camera();

	bool load_default_font();

	bool load_assets();
    bool reload_assets();

    bool update_assets();

    UiState* get_active_ui();

	void user_update();
	
    void timeout();
    void update_ui_state(cobot::vec2 window_size);
    void update_ui_pos();

    void set_event_active(int event_index, double timeout_seconds);
    void set_event_deactive(int event_index);

    void draw_ui_state(UiState& state);

    bool on_mouse_down();
    void on_mouse_up(int button);
    void on_mouse_move();
    void mouse_move_ui(UiState& ui);

    void set_text_editor_cursor(cobot::Rectangle text_area, cobot::Direction dir);

	bool mouse_input_common();

    void update_keyboard_state();
    bool keyboard_input_down(KeyboardEvent keyboard);
    bool keyboard_input_up(KeyboardEvent keyboard);

    bool keyboard_input_down_common(KeyboardEvent keyboard);

    void text_input_start();
    void text_input_stop();
    void toggle_text_input();

    bool read_asset_catalog(String_Builder& path);

    void render_rectangle_outline(cobot::Rectangle rect, cobot::Color color, bool center = true) const;
    void render_rectangle(cobot::Rectangle rect, cobot::Color color, bool center = true) const;

    Icon create_icon(AssetId image, cobot::Color background);

    void render_slider(cobot::Rectangle area, cobot::vec2 knob_scale, float value, cobot::Color slider_color, cobot::Color knob_color, const Text& text) const;
    void render_text_field(Text_Field& text_field) const;
    void render_text_editor(TextEditor& editor) const;
    void render_dropdown(const Drop_Down_List& list) const;
    void render_control_menu(const ControlMenu& menu) const;
    void render_discrete_slider(const DiscreteSlider& slider) const;
    void render_panel(const Panel& panel) const;
    void render_value_panel(const UiState& ui, const ValuePanel& panel) const;
    void render_button_group(const ButtonGroup& group) const;

    bool is_minimized() const;
    bool is_maximized() const;
    bool is_fullscreen() const;
    cobot::vec2 get_window_size() const;
};

void get_base_path(String_Builder& builder);
void get_pref_path(String_Builder& builder, const char *org, const char *app);

#endif // APPLICATION_HPP
