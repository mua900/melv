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

namespace melv
{

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

typedef void (*UpdateFunction)(void *userdata, Application* app);
typedef void (*FixedUpdateFunction)(void *userdata, Application* app);

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

	// called once after SDL and everything is initialized
	InitCallback init = nullptr;
	// called with an event before application tries to process it itself
	// use this to react to one time events
	// if this returns true, the application will consider that event consumed and won't process it
	EventCallback event = nullptr;
	// called after all events are processed.
	// you can read a snapshot of the input state as it is recorded in application
	InputCallback input = nullptr;
	// called with user data in draw()
	DrawCallback draw = nullptr;
	// called before SDL and everything else is quitted
	BeforeCleanupCallback before_cleanup = nullptr;
	// called after SDL and everything else is quitted
	AfterCleanupCallback after_cleanup = nullptr;

	// called with SDL_KeyboardEvent
	// you can also access this from event callback but this is something more specific
	KeyboardCallback keyboard = nullptr;
	// called with SDL_MouseEvent
	// you can also access this from event callback but this is something more specific
	MouseCallback mouse = nullptr;
};

struct InitConfiguration {
	// 1440, 810
	int window_width;
	int window_height;

	// VIDEO | AUDIO
	SDL_InitFlags flags;

	// Default Name
	const char* name;
};

InitConfiguration get_default_init_configuration();

class Application {
public:
	// you can directly access everything here
	// maybe be careful with doing_text_input
	// also if you set render context's coordinate space to be world, you need to give it a camera pointer
	
    Window window = {};
    RenderContext render = {};
    AudioPlayer audio_player = {};
    Input input = {};
    AssetCatalog catalog = {};

    DArray<UiState> uiStates = {};
	
    melv::Color clear_color = {};

    TimeInfo timeInfo = {};

    DArray<Event_Timeout> events = {};

    DArray<Camera> cameras = {};

    AssetId font = {};
    AssetId editor_font = {};

    bool quit = false;
    bool doing_text_input = false;  // don't mess with this.

	// fill this out
	UserData user = {};

    bool initialize(InitConfiguration conf);

    void handle_events();
    void update();
    void draw();

    void cleanup();
private:
    bool init_render();

    Camera* get_active_camera();

	bool load_assets();
    bool reload_assets();

    bool update_assets();

    UiState* get_active_ui();

	void user_update();
	
    void timeout();
    void update_ui_state(melv::vec2 window_size);
    void update_ui_pos();

    void set_event_active(int event_index, double timeout_seconds);
    void set_event_deactive(int event_index);

    void draw_ui_state(UiState& state);

    bool on_mouse_down();
    void on_mouse_up(int button);
    void on_mouse_move();
    void mouse_move_ui(UiState& ui);

    void set_text_editor_cursor(melv::Rectangle text_area, melv::Direction dir);

	bool mouse_input_common();

    void update_keyboard_state();
    bool keyboard_input_down(KeyboardEvent keyboard);
    bool keyboard_input_up(KeyboardEvent keyboard);

    bool keyboard_input_down_common(KeyboardEvent keyboard);

    void text_input_start();
    void text_input_stop();
    void toggle_text_input();

    bool read_asset_catalog(String_Builder& path);

    void render_rectangle_outline(melv::Rectangle rect, melv::Color color, bool center = true) const;
    void render_rectangle(melv::Rectangle rect, melv::Color color, bool center = true) const;

    Icon create_icon(AssetId image, melv::Color background);

    void render_slider(melv::Rectangle area, melv::vec2 knob_scale, float value, melv::Color slider_color, melv::Color knob_color, const Text& text) const;
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
    melv::vec2 get_window_size() const;
};

void get_base_path(String_Builder& builder);
void get_pref_path(String_Builder& builder, const char *org, const char *app);

} // namespace

#endif // APPLICATION_HPP

