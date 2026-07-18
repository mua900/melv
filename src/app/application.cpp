#include "application.hpp"
#include "util/log.hpp"

#include <iostream>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

bool Application::initialize()
{
    // @todo ask the game what to initialize

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("Failed to init SDL: %s\n", SDL_GetError());
        return false;
    }

    String_Builder path(256);

    get_base_path(path);
    if (!read_asset_catalog(path)) {
        log_error("Could not read asset catalog\n");
        return false;
    }

    {
        if (!TTF_Init())
        {
            fprintf(stderr, "Could not initialize TTF: %s\n", SDL_GetError());
            return false;
        }

        if (!MIX_Init())
        {
            fprintf(stderr, "Could not initialize MIX: %s\n", SDL_GetError());
            return false;
        }
    }

    // window
    {
        float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE |
                                SDL_WINDOW_HIDDEN;  // show the window after the initialization is complete
        SDL_Window* window = SDL_CreateWindow("cobot", INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, flags);
        if (!window)
        {
            SDL_Log("Failed to create window with SDL: %s\n", SDL_GetError());
            return false;
        }

        // minimum aspect ratio of 1 and maximum aspect ratio of 2 default 1.6
        SDL_SetWindowAspectRatio(window, 1.0, 2.0);

        m_window = { window };

        if (!init_render())
        {
            return false;
        }

        SDL_ShowWindow(window);
    }

    if (!m_audio_player.initialize())
    {
        return false;
    }

    {
        SDL_Cursor* normal = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
        SDL_Cursor* text = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
        SDL_Cursor* resize_ew = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
        SDL_Cursor* resize_ns = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
        if (!(normal && text && resize_ew && resize_ns))
        {
            return false;
        }

        m_input.mouse.cursor.normal = normal;
        m_input.mouse.cursor.text = text;
        m_input.mouse.cursor.resize_ew = resize_ew;
        m_input.mouse.cursor.resize_ns = resize_ns;
    }

    if (!load_assets())
    {
        return false;
    }

    for (auto& cam : cameras)
    {
        cam.zoom = 1;
        cam.offset = m_render.get_center();
    }

    {
        int num_keys = 0;
        m_input.keyboard.keys = SDL_GetKeyboardState(&num_keys);
        m_input.keyboard.num_keys = num_keys;
        m_input.keyboard.do_input = true;
    }

	if (!load_default_font())
	{
		return false;
	}
	
	quit = false;

	if (user.init)
	{
		if (!user.init(user.userdata, this))
		{
			return false;
		}
	}
	
    return true;
}

bool Application::load_default_font()
{
	m_font = get_asset(String("FiraSans"), m_catalog);
	m_editor_font = get_asset(String("FiraCode"), m_catalog);

	return m_font.is_valid() && m_editor_font.is_valid();
}

bool Application::read_asset_catalog(String_Builder& path)
{
	// @todo let the user rename or provide a static string for this
    const char* desc_name = "run_tree.txt";
    path.append(make_string(desc_name));
    bool parse_description = parse_assets(path.c_string(), m_catalog);

    m_catalog.load_context.render = &m_render;
    m_catalog.load_context.audio = &m_audio_player;

    return parse_description;
}

bool Application::reload_assets()
{
    for (int i = 0; i < m_catalog.catalogEntryCount; i++)
    {
        if (!m_catalog.reload_asset_at_index(i))
        {
            return false;
        }
    }

    update_assets();

    return true;
}

// literally reinitialize everything that touches any asset
bool Application::update_assets()
{
    AssetId fontId = get_asset(String("FiraSans"), m_catalog);
    AssetId editorFontId = get_asset(String("FiraSans"), m_catalog);
    return true;
}

bool Application::load_assets()
{
    // the size can actually change when we are trying to load assets since folder references will expand and include arbitrary amount of files
    // so save the amount we need to iterate
    int count = m_catalog.assets.count();
    for (int i = 0; i < count; i++)
    {
        if (!(m_catalog.assets[i].flags & ASSET_IS_LAZY))
        {
            AssetId id = get_asset_at_index(i, m_catalog);
            if (!id.is_valid())
            {
                auto asset_name = m_catalog.get_asset_name_at_index(i);
                SCOPE_STRING(asset_name, name);
                if (!(m_catalog.assets[i].flags & ASSET_IS_OPTIONAL)) {
                    log_error("Couldn't load asset %s", name);
                    return false;
                }
                else {
                    log_warning("Couldn't load asset %s", name);
                }
            }
        }
    }

    m_catalog.path.free_buffer();

    return true;
}

UiState* Application::get_active_ui()
{
    // @todo ask the game
    return nullptr;
}

Camera* Application::get_active_camera()
{
    // @todo ask the game
    return nullptr;
}

void Application::handle_events()
{
    SDL_Event e = {};
    while (SDL_PollEvent(&e))
    {
		if (user.event)
		{
			if (user.event(e, user.userdata, this))
			{
				continue;
			}
		}
		
        switch (e.type)
        {
            case SDL_EVENT_QUIT:
            {
                quit = true;
                break;
            }
            case SDL_EVENT_KEY_DOWN:
            {
                SDL_KeyboardEvent keyboard = e.key;
                keyboard_input_down(keyboard);
                break;
            }
            case SDL_EVENT_KEY_UP:
            {
                SDL_KeyboardEvent keyboard = e.key;
                keyboard_input_up(keyboard);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                SDL_MouseButtonEvent mouse = e.button;
                m_input.mouse.down = true;
                m_input.mouse.buttonFlags = SDL_GetMouseState(&m_input.mouse.pos.x, &m_input.mouse.pos.y);

                on_mouse_down();

                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                SDL_MouseButtonEvent mouse = e.button;

                m_input.mouse.down = false;
                m_input.mouse.buttonFlags = SDL_GetMouseState(&m_input.mouse.pos.x, &m_input.mouse.pos.y);

                on_mouse_up(mouse.button);

                break;
            }
            case SDL_EVENT_MOUSE_MOTION:
            {
                m_input.mouse.buttonFlags = SDL_GetMouseState(&m_input.mouse.pos.x, &m_input.mouse.pos.y);
                on_mouse_move();
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL:
            {
                const float mouseSensitivity = 0.1;
                SDL_MouseWheelEvent wheel = e.wheel;

                Camera* camera = get_active_camera();
                if (camera)
                {
                    camera->zoom += wheel.y * mouseSensitivity;
                    camera->zoom = cobot::clamp(0.1, 10, camera->zoom);
                }
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED:
            {
                int render_size_x, render_size_y;
                SDL_GetRenderOutputSize(m_render.renderer, &render_size_x, &render_size_y);
                m_render.render_size = cobot::vec2(render_size_x, render_size_y);
                for (auto& c : cameras)
                {
                    c.offset = cobot::vec2(render_size_x / 2, render_size_y / 2);
                }

                update_ui_state(cobot::vec2(render_size_x, render_size_y));

                break;
            }
            case SDL_EVENT_TEXT_INPUT:
            {
                SDL_TextInputEvent text = e.text;
                String input_text = make_string(text.text);

                UiState* ui = get_active_ui();
                Text_Field* text_field = ui->get_selected_text_field();
                if (text_field)
                {
                    Font font = m_catalog.get_font(m_editor_font);

                    text_field->append_string(input_text);
                    text_field->update_text(m_render.renderer, font, true);
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

    update_keyboard_state();

	if (user.input)
	{		
		user.input(user.userdata, this);
	}
}

void Application::mouse_move_ui(UiState& ui)
{
    cobot::vec2 mouse_pos = m_input.mouse.pos;

    for (auto& editor : ui.editor)
    {
        cobot::Rectangle text_area = editor.get_text_area();
        cobot::Rectangle title_area = editor.get_title_area();

        if (editor.resize.resize)
        {
            editor.field.m_area = editor.resize.calculate_new_area(mouse_pos, 50, 2000);
        }
        else
        {
            cobot::Direction dir = text_area.on_edge(mouse_pos, 3);
            set_text_editor_cursor(text_area, dir);
        }
    }

    for (auto& panel : ui.panel)
    {
        if (panel.resize.resize)
        {
            panel.area = panel.resize.calculate_new_area(mouse_pos, 50, 2000);
        }
        else
        {
            cobot::Direction dir = panel.area.on_edge(mouse_pos, 3);
            if (cobot::direction_is_horizontal(dir))
            {
                SDL_SetCursor(m_input.mouse.cursor.resize_ew);
            }
            else if (cobot::direction_is_vertical(dir))
            {
                SDL_SetCursor(m_input.mouse.cursor.resize_ns);
            }
            else
            {
                SDL_SetCursor(m_input.mouse.cursor.normal);
            }
        }
    }
}

void Application::set_text_editor_cursor(cobot::Rectangle text_area, cobot::Direction dir)
{
    cobot::vec2 mouse_pos = m_input.mouse.pos;

    if (dir) {
        if (direction_is_vertical(dir))
        {
            SDL_SetCursor(m_input.mouse.cursor.resize_ns);
        }
        else if (direction_is_horizontal(dir))
        {
            SDL_SetCursor(m_input.mouse.cursor.resize_ew);
        }
    }
    else if (text_area.contains_centered(mouse_pos)) {
        SDL_SetCursor(m_input.mouse.cursor.text);
    }
    else {
        // SDL checks wheter the cursor set is different so no unnecessary redraws here
        SDL_SetCursor(m_input.mouse.cursor.normal);
    }
}

bool Application::keyboard_input_up(SDL_KeyboardEvent keyboard)
{
    switch (keyboard.scancode)
    {
        case SDL_SCANCODE_DOWN: // fallthrough
        case SDL_SCANCODE_UP: {
            return true;
        }
    }

    return false;
}

bool Application::keyboard_input_down(SDL_KeyboardEvent keyboard)
{
    if (keyboard_input_down_common(keyboard))
    {
        return true;
    }
    else
    {
		if (!m_input.keyboard.do_input)
		{
			return false;
		}

        return false;
    }
}

bool Application::keyboard_input_down_common(KeyboardEvent keyboard)
{
    UiState* ui = get_active_ui();
    if (!ui)
    {
        return false;
    }

    switch (keyboard.scancode)
    {
        case SDL_SCANCODE_ESCAPE:
        {
            if (doing_text_input)
            {
                text_input_stop();
                return true;
            }

            quit = true;
            return true;
        }
        case SDL_SCANCODE_RETURN:
        {
            if (doing_text_input)
            {
                auto field = ui->get_selected_text_field();
                if (field)
                {
                    field->insert_line();
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_TAB:
        {
            if (doing_text_input)
            {
                auto field = ui->get_selected_text_field();
                if (field)
                {
                    field->insert_tab(4);
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_BACKSPACE:
        {
            if (doing_text_input)
            {
                auto field = ui->get_selected_text_field();
                if (field)
                {
                    if (field->m_cursor == field->m_selection_point)
                    {
                        field->delete_at_cursor();
                    }
                    else
                    {
                        field->delete_text();
                    }

                    Font font = m_catalog.get_font(m_editor_font);
                    field->update_text(m_render.renderer, m_catalog.get_font(field->fontId), true);
                }
            }
            return true;
        }
        case SDL_SCANCODE_DELETE:
        {
            if (doing_text_input)
            {
                auto field = ui->get_selected_text_field();
                if (field)
                {
                    if (field->m_cursor == field->m_selection_point)
                    {
                        field->delete_after_cursor();
                    }
                    else
                    {
                        field->delete_text();
                    }
                    field->update_text(m_render.renderer, m_catalog.get_font(field->fontId), true);
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_HOME:
        {
            if (doing_text_input)
            {
                auto field = ui->get_selected_text_field();
                if (field) {
                    field->m_cursor = 0;
                    field->m_selection_point = 0;

                    Font font = m_catalog.get_font(field->fontId);
                    field->update_text(m_render.renderer, font, true);
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_END:
        {
            if (doing_text_input)
            {
                auto field = ui->get_selected_text_field();
                if (field) {
                    field->m_cursor = field->m_buffer.length;
                    field->m_selection_point = field->m_buffer.length;

                    Font font = m_catalog.get_font(field->fontId);
                    field->update_text(m_render.renderer, font, true);
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_LEFT: {
            if (doing_text_input) {
                auto field = ui->get_selected_text_field();
                if (field) {
                    String s = field->get_string();
                    int step = utf8_previous(s, field->m_cursor);

                    int selectionPos = field->m_selection_point;
                    field->m_cursor = MAX(0, field->m_cursor - step);

                    Font font = m_catalog.get_font(field->fontId);
                    field->update_text(m_render.renderer, font, true);

                    if (m_input.keyboard.mod_state & KEYMOD_LEFT_SHIFT)
                    {
                        field->m_selection_point = selectionPos;
                    }
                    else
                    {
                        field->m_selection_point = field->m_cursor;
                    }
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_RIGHT: {
            if (doing_text_input) {
                auto field = ui->get_selected_text_field();
                if (field) {
                    String s = field->get_string();
                    int step = utf8_next(s, field->m_cursor);

                    int selectionPos = field->m_selection_point;
                    field->m_cursor = MIN(field->m_cursor + step, field->m_buffer.length);

                    Font font = m_catalog.get_font(field->fontId);
                    field->update_text(m_render.renderer, font, true);

                    if (m_input.keyboard.mod_state & KEYMOD_LEFT_SHIFT)
                    {
                        field->m_selection_point = selectionPos;
                    }
                    else
                    {
                        field->m_selection_point = field->m_cursor;
                    }
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_F11:
        {
            SDL_SetWindowFullscreen(m_window.window, !is_fullscreen());
            return true;
        }
        case SDL_SCANCODE_F4:
        {
            if (!reload_assets())
            {
                log_error("Couldn't reload assets");
                quit = true;
            }

            return true;
        }
    }

    return false;
}

void Application::on_mouse_move()
{
    cobot::vec2 mouse_pos = m_input.mouse.pos;

    if (m_input.mouse.drag)
    {
        Camera* camera = get_active_camera();
        if (camera)
        {
            cobot::vec2 move = (m_input.mouse.dragPosition - mouse_pos);
            camera->position += cobot::vec2(move.x, -move.y) / camera->zoom;
            m_input.mouse.dragPosition = mouse_pos;
        }
        return;
    }

    UiState* ui = get_active_ui();
    if (ui)
    {
        mouse_move_ui(*ui);
    }
}

bool Application::on_mouse_down()
{
    UiState* ui = get_active_ui();

	if (mouse_input_common())
	{
		return true;
	}

    if ((m_input.mouse.buttonFlags & MOUSE_LEFT_MASK) && get_active_camera() && ui && !ui->get_drag_info() && !ui->doing_resize())
    {
        m_input.mouse.dragPosition = m_input.mouse.pos;
        m_input.mouse.drag = true;
        return true;
    }
    else
    {
        return false;
    }
}

void Application::on_mouse_up(int button)
{
    UiState* ui = get_active_ui();
    if (!ui)
    {
        return;
    }

    if (button & MOUSE_LEFT_MASK)
    {
        // @todo maybe button interactions should be on button up

        for (auto& editor : ui->editor) {
            if (editor.drag.drag) {
                editor.drag.drag = false;
            }

            if (editor.resize.resize) {
                editor.resize = {};
            }

            editor.clicked_icon = 0;
        }

        for (auto& panel : ui->panel)
        {
            if (panel.drag.drag) {
                panel.drag.drag = false;
            }

            if (panel.resize.resize) {
                panel.resize = {};
            }
        }

        m_input.mouse.drag = false;
    }
}

bool Application::mouse_input_common()
{
	cobot::vec2 mouse_pos = m_input.mouse.pos;
	UiState* ui = get_active_ui();

    if (ui)
    {
    	for (int it = 0; it < ui->text_field.size(); it++)
    	{
    		auto& field = ui->text_field.get_ref(it);

            if (!field.editable)
            {
                continue;
            }

            if (!field.info.active)
            {
                continue;
            }

    		cobot::Rectangle area = field.m_area;
    		if (area.contains_centered(mouse_pos)) {
                cobot::vec2 relative = mouse_pos - area.get_top_left();
                if (doing_text_input && (ui->text_input_target.flags & TEXT_INPUT_TARGET_IS_VALID) && ui->text_input_target.index == it)
                {
                    // if we are already doing text input
                    field.mouse_x = relative.x;
                    field.mouse_y = relative.y;
                }
                else
                {
                    text_input_start();

                    ui->text_input_target.index = it;
                    ui->text_input_target.flags = TEXT_INPUT_TARGET_IS_VALID;
                }

                Font font = m_catalog.get_font(field.fontId);
                field.m_cursor = field.calculate_cursor_from_mouse(relative, field.get_string(), font, true);
                field.m_selection_point = field.m_cursor;

    			return true;
    		}
    	}
    }

	return false;
}

void Application::update_keyboard_state()
{
    m_input.keyboard.keys = SDL_GetKeyboardState(&m_input.keyboard.num_keys);
    m_input.keyboard.mod_state = SDL_GetModState();
}

void Application::update()
{
    // update time
    SDL_Time time = SDL_GetTicks();
    double time_sec = (double)time / MILLISECONDS_PER_SECOND;
    m_time.deltaTime = time - m_time.time;
    m_time.deltaTimeSeconds = time_sec - m_time.timeSeconds;
    m_time.time = time;
    m_time.timeSeconds = time_sec;

    update_ui_pos();
    timeout();

	user_update();
}

void Application::user_update()
{
	auto time = m_time;

	if (!user.update_state)
	{
		return;
	}
	
	constexpr int maxIterationsPerFrame = 50;
    int iterations = 0;
	double timeStep = user.update_state->calculateTimeStep();
    while ((user.update_state->elapsed < time.timeSeconds + time.deltaTimeSeconds) && iterations < maxIterationsPerFrame)
    {
		if (user.update_state->fixedUpdate)
		{
			user.update_state->fixedUpdate(user.userdata);
		}
        user.update_state->elapsed += timeStep;
        user.update_state->ticks += 1;

        iterations += 1;
    }

	if (user.update_state->update)
	{
		user.update_state->update(user.userdata, time);
	}
}

void Application::timeout()
{
    for (int i = 0; i < ARRAY_SIZE(m_events); i++)
    {
        if (m_events[i].active)
        {
            if (m_events[i].event < m_time.time)
            {
                m_events[i].active = false;
            }
        }
    }
}

void Application::update_ui_state(cobot::vec2 window_size) {
    for (int i = 0; i < m_ui.size(); i++)
    {
        cobot::vec2 assumed = m_ui[i].assumed_window_size;
        float x_factor = window_size.x / assumed.x;
        float y_factor = window_size.y / assumed.y;
        if ((fabsf(x_factor - 1.0f) >= 0.1f) || (fabsf(y_factor - 1.0f) >= 0.1f)) {
            m_ui[i].update_state(window_size, m_render, m_catalog);
        }
    }
}

void Application::update_ui_pos()
{
    UiState* ui = get_active_ui();
    if (!ui)
    {
        return;
    }

    cobot::vec2 mouse_pos = m_input.mouse.pos;

    for (auto& editor : ui->editor)
    {
        if (editor.drag.drag)
        {
            cobot::Rectangle area = editor.get_text_area();
            cobot::vec2 half_scale = cobot::vec2(area.w / 2, area.h / 2);
            cobot::vec2 dst = (mouse_pos - editor.drag.start) + half_scale;
            dst.y += editor.title_height;
            editor.set_position(dst);
        }
    }

    for (auto& panel : ui->panel)
    {
        if (panel.drag.drag)
        {
            cobot::vec2 half_scale = panel.area.get_scale() / 2;
            cobot::vec2 pos = (mouse_pos - panel.drag.start) + half_scale;
            panel.area.x = pos.x;
            panel.area.y = pos.y;
        }
    }
}

void Application::set_event_active(int event_index, double timeout_seconds)
{
    s64 timeout = (s64)(timeout_seconds * MILLISECONDS_PER_SECOND);
    m_events[event_index].active = true;
    m_events[event_index].event = m_time.time + timeout;
}

void Application::set_event_deactive(int event_index)
{
    m_events[event_index].active = false;
}

void Application::cleanup()
{
	user.before_cleanup(user.userdata, this);
	
    MIX_Quit();
    SDL_Quit();
    TTF_Quit();

	user.after_cleanup(user.userdata, this);
}

bool Application::init_render()
{
    if (!initialize_render_context(&m_render, m_window.window))
    {
        return false;
    }
	
    return true;
}

void Application::draw()
{
    SDL_Renderer* renderer = m_render.renderer;

    if (SDL_GetWindowFlags(m_window.window) & SDL_WINDOW_MINIMIZED) {
        // don't draw anything if the window is minimized
        return;
    }

    cobot::Color edit_color = cobot::Color(0x77, 0x55, 0x66);
    cobot::Color background = doing_text_input ? edit_color : m_clear_color;
    SDL_SetRenderDrawColor(renderer, COLOR_ARG(background));
    SDL_RenderClear(renderer);

    // SDL_FlushRenderer(m_render.renderer);

    // game graphics
    m_render.space = CoordinateSpace::World;

	user.draw(user.userdata, this);
	
    // ui
    m_render.space = CoordinateSpace::Screen;

    SDL_RenderPresent(renderer);
}

bool Application::is_minimized() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(m_window.window);
    return flags & SDL_WINDOW_MINIMIZED;
}

bool Application::is_maximized() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(m_window.window);
    return flags & SDL_WINDOW_MAXIMIZED;
}

bool Application::is_fullscreen() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(m_window.window);
    return flags & SDL_WINDOW_FULLSCREEN;
}

cobot::vec2 Application::get_window_size() const {
    cobot::ivec2 s;
    SDL_GetWindowSize(m_window.window, &s.x, &s.y);
    return cobot::vec2(s.x, s.y);
}

void Application::draw_ui_state(UiState& state)
{
	cobot::vec2 mouse_pos = m_input.mouse.pos;

    for (TextEditor& editor : state.editor)
    {
        render_text_editor(editor);
    }

    for (Text_Field& field : state.text_field)
    {
        if (field.info.visible)
        {
            render_text_field(field);
        }
    }

    for (const Drop_Down_List& list : state.drop_down)
    {
        render_dropdown(list);
    }

    for (const TextButton& button : state.button)
    {
        if (button.info.visible)
        {
            render_textured_rectangle(m_render, cobot::Rectangle(button.position, button.scale), button.text.texture, button.background, true);
        }
    }

    for (const ImageButton& button : state.image_button)
    {
        if (button.info.visible)
        {
            render_textured_rectangle(m_render, cobot::Rectangle(button.position, button.scale), button.image, button.background, true);
        }
    }

    for (const Label& label : state.label)
    {
        render_textured_rectangle(m_render, cobot::Rectangle(label.position, label.scale), label.text.texture, label.background, false);
    }

    for (const ControlMenu& menu : state.control)
    {
        render_control_menu(menu);
    }

    for (const DiscreteSlider& slider : state.discrete_slider)
    {
        render_discrete_slider(slider);
    }

    for (const Panel& panel : state.panel)
    {
        render_panel(panel);
    }

    for (const ValuePanel& panel : state.value_panel)
    {
        render_value_panel(state, panel);
    }

    for (const ButtonGroup& group : state.button_group)
    {
        if (group.info.visible)
        {
            render_button_group(group);
        }
    }

	float hoverWidth, hoverHeight = 0;
	SDL_GetTextureSize(state.hoverText.text.texture, &hoverWidth, &hoverHeight);
	render_textured_rectangle(m_render, cobot::Rectangle(mouse_pos.x, mouse_pos.y, hoverWidth, hoverHeight), state.hoverText.text.texture, state.hoverText.background);
}

void Application::render_rectangle(cobot::Rectangle rect, cobot::Color color, bool center) const
{
    SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(color));
    SDL_FRect area = center ?
                    SDL_FRect { rect.x - rect.w / 2, rect.y - rect.h / 2, rect.w, rect.h } :
                    SDL_FRect { rect.x, rect.y, rect.w, rect.h };
    SDL_RenderFillRect(m_render.renderer, &area);
}

void Application::render_rectangle_outline(cobot::Rectangle rect, cobot::Color color, bool center) const
{
    SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(color));
    SDL_FRect area = center ?
                    SDL_FRect { rect.x - rect.w / 2, rect.y - rect.h / 2, rect.w, rect.h } :
                    SDL_FRect { rect.x, rect.y, rect.w, rect.h };
    SDL_RenderRect(m_render.renderer, &area);
}

void Application::render_discrete_slider(const DiscreteSlider& slider) const
{
    cobot::vec2 start = slider.get_start();
    cobot::vec2 step = slider.get_step();

    cobot::Rectangle area = slider.get_bounds();
    render_rectangle_outline(area, slider.outlineColor, false);

    for (int i = 0; i < slider.element_count; i++)
    {
        cobot::Rectangle area (start + i * step, slider.element_scale);
        float t = float (i) / slider.element_count;
        cobot::ColorF color = i <= slider.selected ? cobot::mixColors(slider.startColor, slider.endColor, t) : slider.inactiveColor;

        if (slider.texture)
        {
            render_texture_with_tint(m_render, area, slider.texture, color, true);
        }
        else
        {
            render_rectangle(area, cobot::Color(color));
        }

        render_rectangle_outline(area, cobot::Color(slider.buttonColor));
    }
}

void Application::render_slider(cobot::Rectangle area, cobot::vec2 knob_scale, float value, cobot::Color slider_color, cobot::Color knob_color, const Text& text) const
{
    float slider_knob_width = area.w * knob_scale.x;
    float slider_knob_height = area.h * knob_scale.y;

    SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(slider_color));
    SDL_FRect slider = { area.x, area.y, area.w, area.h };
    SDL_RenderFillRect(m_render.renderer, &slider);
    float percentage = value;
    SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(knob_color));
    SDL_FRect slider_knob = {
        slider.x - (slider_knob_width / 2) + (slider.w * percentage), slider.y + slider.h / 2 - slider_knob_height / 2,
        slider_knob_width, slider_knob_height
    };
    SDL_RenderFillRect(m_render.renderer, &slider_knob);

    // text
    {
        const int margin = 10;
        render_text_scale(m_render.renderer, text,
            cobot::vec2(slider.x + slider.w / 2, slider.y + slider.h * 2 + margin), cobot::vec2(0.6, 0.6));
    }
}

void Application::render_panel(const Panel& panel) const
{
    render_rectangle(panel.get_title_area(), panel.title_bar_color);

    auto& tab = panel.tabs.get_ref(panel.activeTab);
    render_rectangle(panel.area, tab.color);

    const float margin = 16;
    const float iconSize = 32;
    for (int i = 0; i < tab.icons.size(); i++) {
        cobot::Rectangle area = panel.get_icon_area(i);
        render_textured_rectangle(m_render, area, tab.icons.get(i).icon.texture, tab.icons.get(i).icon.background, true, false);
    }

    for (int i = 0; i < panel.tabs.size(); i++) {
        cobot::Rectangle area = panel.get_tab_header_area(i);
        render_textured_rectangle(m_render, area, panel.tabs.get(i).tabIcon.texture, panel.tabs.get(i).tabIcon.background, true);
    }
}

void Application::render_value_panel(const UiState& ui, const ValuePanel& panel) const
{
    auto& tab = panel.tabs.get_ref(panel.activeTab);
    render_rectangle(panel.area, tab.color, true);

    float height = 0;
    for (int i = 0; i < tab.fields.size(); i++)
    {
        ValueField& value = tab.fields[i];

        cobot::Rectangle text_area = panel.get_field_title_area(panel.activeTab, i);
        text_area.y += height;
        height += text_area.h;

        render_texture(m_render, text_area, value.name.texture, true);

        cobot::Rectangle area = panel.get_field_area(panel.activeTab, i, &ui);
        area.y += height;

        switch (value.type)
        {
            case ValueInteger: {
                // fallthrough
            }
            case ValueNumber: {
                // fallthrough
            }
            case ValueString: {
                Text_Field& text_field = ui.text_field.get_ref(value.ui_element);
                int line_count = text_field.m_line_count;

                text_field.m_area = area;

                if (line_count == 0)
                {
                    render_rectangle(area, text_field.background);
                }
                else {
                    render_text_field(text_field);
                }

                height += area.h;
                break;
            }
            case ValueLabel: { /* nothing extra to display */ break; }
            case ValueSelection: {
                ButtonGroup& group = ui.button_group.get_ref(value.ui_element);
                group.position = cobot::vec2(area.x, area.y);
                group.scale = cobot::vec2(area.w, area.h);
                render_button_group(group);

                height += group.scale.y;
                break;
            }
            case ValueButton: {
                render_rectangle_outline(text_area, cobot::Color(0x99, 0x55, 0x66));
                break;
            }
        }

        height += tab.field_margin;
    }

    if (panel.showTabs && panel.tabs.size() > 1)
    {
        for (int i = 0; i < panel.tabs.size(); i++) {
            cobot::Rectangle area = panel.get_tab_header_area(i);
            render_textured_rectangle(m_render, area, panel.tabs.get(i).tabIcon.texture, panel.tabs.get(i).tabIcon.background, true);
        }
    }
}

void Application::render_button_group(const ButtonGroup& group) const
{
    render_rectangle(cobot::Rectangle(group.position, group.scale), group.background);
    cobot::vec2 top_left = group.position - group.scale / 2;
    int numColumns = std::floor(group.scale.x / group.button_scale.x);
    int row = 0;
    int column = 0;
    for (auto& texture : group.buttons)
    {
        draw_texture(m_render, cobot::Rectangle(top_left + cobot::vec2(column * group.button_scale.x, row * group.button_scale.y) + group.button_scale / 2, group.button_scale), texture);
        column += 1;
        row = (column == numColumns) ? row + 1 : row;
    }
}

void Application::render_control_menu(const ControlMenu& menu) const
{
    if (menu.visible)
    {
        if (menu.anchorPosition)
        {
            draw_segment(m_render, *menu.anchorPosition, menu.position, 2, menu.background);
        }

        int index = 0;
        for (auto& button : menu.buttons)
        {
            render_textured_rectangle(m_render, cobot::Rectangle(menu.position + cobot::vec2(0, menu.scale.y * index), menu.scale), button.label.texture, menu.background, true);
            index += 1;
        }
    }
}

void Application::render_text_editor(TextEditor& editor) const
{
    cobot::Rectangle text_area = editor.field.m_area;
    cobot::Rectangle title_area = editor.get_title_area();
    render_textured_rectangle(m_render, title_area, editor.title_texture, editor.title_bar_color);

    cobot::Rectangle area = editor.get_title_area();
    cobot::vec2 iconPos = area.get_position() + cobot::vec2(area.get_scale().x / 2, 0);
    cobot::vec2 iconScale = cobot::vec2(editor.title_height, editor.title_height);

    cobot::Color clicked_background = cobot::Color(0xAA, 0x55, 0x33);
    render_textured_rectangle(m_render, editor.get_icon1_area(), editor.icon1.texture, (editor.clicked_icon == 1) ? clicked_background : editor.icon1.background, true);
    render_textured_rectangle(m_render, editor.get_icon2_area(), editor.icon2.texture, (editor.clicked_icon == 2) ? clicked_background : editor.icon2.background, true);
    render_textured_rectangle(m_render, editor.get_icon3_area(), editor.icon3.texture, (editor.clicked_icon == 3) ? clicked_background : editor.icon3.background, true);

    render_text_field(editor.field);
}

void Application::render_text_field(Text_Field& text_field) const
{
    cobot::Rectangle area = text_field.m_area;
    render_rectangle(area, text_field.background);

    SDL_Texture* text_texture = text_field.m_texture;

    Font font = m_catalog.get_font(text_field.fontId);

    if (text_texture)
    {
        cobot::vec2 top_left = area.get_top_left();
        cobot::vec2 text_scale = {};
        SDL_GetTextureSize(text_texture, &text_scale.x, &text_scale.y);

        int line_count = text_field.m_line_count;
        float font_size = text_field.m_font_size;

        int line_skip = TTF_GetFontLineSkip(font.font);

        SDL_Rect clip = {
            int(area.x - area.w / 2),
            int(area.y - area.h / 2),
            int(area.w),
            int(area.h)
        };
        SDL_SetRenderClipRect(m_render.renderer, &clip);

        draw_texture(m_render, cobot::Rectangle(top_left, text_scale), text_texture);

        SDL_SetRenderClipRect(m_render.renderer, nullptr);

        if (doing_text_input)
        {
            String string = text_field.get_string();

            cobot::ColorF highlightColor (0.2, 0.2, 0.6, 0.5);

            // selected area
            CursorScreenPosition cursorPos = text_field.get_cursor_from_selection(text_field.m_selection_point, string, font, true);
            int lineCount = std::abs(text_field.m_cursor_line - cursorPos.line);
            if (lineCount == 0)
            {
                float width = std::fabsf(text_field.m_cursor_pixel_x - cursorPos.pixel_x);
                int pixelX = cobot::min(text_field.m_cursor_pixel_x, cursorPos.pixel_x);
                cobot::Rectangle area = {
                    top_left.x + pixelX, top_left.y + cursorPos.pixel_y,
                    width, float(line_skip)
                };
                render_rectangle(area, highlightColor, false);
            }
            else
            {
                CursorScreenPosition start;
                CursorScreenPosition end;
                if (text_field.m_cursor_line > cursorPos.line)
                {
                    start = cursorPos;
                    end = { text_field.m_cursor_line, text_field.m_cursor_pixel_x, text_field.m_cursor_pixel_y };
                }
                else
                {
                    start = { text_field.m_cursor_line, text_field.m_cursor_pixel_x, text_field.m_cursor_pixel_y };
                    end = cursorPos;
                }

                render_rectangle(cobot::Rectangle(top_left.x + start.pixel_x, top_left.y + start.line * line_skip, area.w - start.pixel_x, line_skip), highlightColor, false);
                for (int i = start.line + 1; i < end.line; i++)
                {
                    log_info("%d", i);
                    render_rectangle(cobot::Rectangle(top_left.x, top_left.y + i * line_skip, area.w, line_skip), highlightColor, false);
                }
                render_rectangle(cobot::Rectangle(top_left.x, top_left.y +  end.line * line_skip, end.pixel_x, line_skip), highlightColor, false);
            }

            // cursor
            float cursor_width = 5;
            render_rectangle(
                cobot::Rectangle(cobot::vec2(top_left.x + text_field.m_cursor_pixel_x - cursor_width / 2,
                                            top_left.y + text_field.m_cursor_pixel_y + font_size / 2),
                                cobot::vec2(cursor_width, font_size)),
                                TextCursorColor);
        }
    }
}

void Application::render_dropdown(const Drop_Down_List& list) const {
    SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(list.title_color));

    SDL_FRect header_area = {
        list.pos.x - list.scale.x/2, list.pos.y - list.scale.y / 2,
        list.scale.x, list.scale.y
    };
    SDL_RenderFillRect(m_render.renderer, &header_area);
    Text title_text = list.selected == DROP_DOWN_LIST_SELECTED_SENTINEL ? list.title : list.get_option_text(list.selected);
    render_text_size(m_render.renderer, title_text,
        cobot::vec2(header_area.x + header_area.w / 2, header_area.y + header_area.h / 2), cobot::vec2(header_area.w, header_area.h));

    if (list.open) {
        SDL_SetRenderDrawColor(m_render.renderer, COLOR_ARG(list.option_color));

        for (int i = 0; i < list.options.size(); i++) {
            SDL_FRect area = header_area;
            area.y += area.h * (i + 1);
            SDL_RenderFillRect(m_render.renderer, &area);
            render_text_size(m_render.renderer, list.get_option_text(i),
                cobot::vec2(area.x + area.w/2, area.y + area.h/2), cobot::vec2(area.w, area.h));
        }
    }
}

Icon Application::create_icon(AssetId image, cobot::Color background) {
    SDL_Texture* texture = m_catalog.get_image(image);
    return Icon(texture, background);
}

void Application::text_input_stop()
{
    SDL_StopTextInput(m_window.window);
    doing_text_input = false;
    m_input.keyboard.do_input = true;

    for (int i = 0; i < m_ui.size(); i++)
    {
        m_ui[i].text_input_target = {};
    }

    m_clear_color = DEFAULT_BACKGROUND_COLOR;
}

void Application::text_input_start()
{
    SDL_StartTextInput(m_window.window);
    doing_text_input = true;
    m_input.keyboard.do_input = false;

    m_clear_color = {0, 0x44, 0x66, 0xff};
}

void Application::toggle_text_input()
{
    if (!doing_text_input)
    {
        text_input_start();
    }
    else
    {
        text_input_stop();
    }
}
