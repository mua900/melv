#include "application.hpp"
#include "util/log.hpp"

#include <iostream>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

namespace melv
{

InitConfiguration get_default_init_configuration()
{
	InitConfiguration conf;

	conf.flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO;

	conf.window_width = 1440;
	conf.window_height = 810;

	conf.name = "Default Name";

	conf.gpuDebug = false;

	return conf;
}

bool Application::initialize(InitConfiguration conf)
{
    if (!SDL_Init(conf.flags)) {
        SDL_Log("Failed to init SDL: %s\n", SDL_GetError());
        return false;
    }

    String_Builder path(256);

    get_base_path(path);
    if (!read_asset_catalog(path)) {
        log_error("Could not read asset catalog");
        return false;
    }

    {
        if (!TTF_Init())
        {
            log_error("Could not initialize TTF: %s", SDL_GetError());
            return false;
        }

        if (!MIX_Init())
        {
            log_error("Could not initialize MIX: %s", SDL_GetError());
            return false;
        }
    }

    // window
    {
        float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE |
                                SDL_WINDOW_HIDDEN;  // show the window after the initialization is complete
        SDL_Window* w = SDL_CreateWindow(conf.name, conf.window_width, conf.window_height, flags);
        if (!w)
        {
            SDL_Log("Failed to create window with SDL: %s\n", SDL_GetError());
            return false;
        }

        // minimum aspect ratio of 1 and maximum aspect ratio of 2 default 1.6
        SDL_SetWindowAspectRatio(w, 1.0, 2.0);

        window = { w };

        if (!init_render(conf.gpuDebug))
        {
            return false;
        }

        SDL_ShowWindow(w);
    }

    if (!audio_player.initialize())
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

        input.mouse.cursor.normal = normal;
        input.mouse.cursor.text = text;
        input.mouse.cursor.resize_ew = resize_ew;
        input.mouse.cursor.resize_ns = resize_ns;
    }

    if (!init_gpu_renderer(&render, window.window))
    {
        return false;
    }

    if (!load_assets())
    {
        return false;
    }

    for (auto& cam : cameras)
    {
		cam = init_camera();
    }

    {
        int num_keys = 0;
        input.keyboard.keys = SDL_GetKeyboardState(&num_keys);
        input.keyboard.num_keys = num_keys;
        input.keyboard.do_input = true;
    }

	if (user.init)
	{
		if (!user.init(user.userdata, this))
		{
			return false;
		}
	}

	quit = false;

    return true;
}

Camera Application::init_camera() const
{
    melv::vec2 center = render.get_center();
	Camera cam;
	// this weird inverted signs are because of coordinate system mismatch between
	// render.get_center which is is screen space and the camera position which is in world space
	cam.position = melv::vec2(-center.x, center.y);
	cam.zoom = 1;
	cam.rotation = 0;
	return cam;
}

bool Application::read_asset_catalog(String_Builder& path)
{
	// @todo let the user rename or provide a static string for this
    const char* desc_name = "run_tree.txt";
    path.append(make_string(desc_name));
    bool parse_description = parse_assets(path.c_string(), catalog);

    catalog.load_context.render = &render;
    catalog.load_context.audio = &audio_player;

    return parse_description;
}

bool Application::reload_assets()
{
    for (int i = 0; i < catalog.catalogEntryCount; i++)
    {
        if (!catalog.reload_asset_at_index(i))
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
    AssetId fontId = get_asset(String("FiraSans"), catalog);
    AssetId editorFontId = get_asset(String("FiraCode"), catalog);
    return true;
}

bool Application::load_assets()
{
    // the size can actually change when we are trying to load assets since folder references will expand and include arbitrary amount of files
    // so save the amount we need to iterate
    int count = catalog.assets.count();
    for (int i = 0; i < count; i++)
    {
        if (!(catalog.assets[i].flags & ASSET_IS_LAZY))
        {
            AssetId id = get_asset_at_index(i, catalog);
            if (!id.is_valid())
            {
                auto asset_name = catalog.get_asset_name_at_index(i);
                SCOPE_STRING(asset_name, name);
                if (!(catalog.assets[i].flags & ASSET_IS_OPTIONAL)) {
                    log_error("Couldn't load asset %s", name);
                    return false;
                }
                else {
                    log_warning("Couldn't load asset %s", name);
                }
            }
        }
    }

    catalog.path.free_buffer();

    return true;
}

UiState* Application::get_active_ui()
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
				// continue;
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
                input.mouse.down = true;
                input.mouse.buttonFlags = SDL_GetMouseState(&input.mouse.pos.x, &input.mouse.pos.y);

                on_mouse_down();

                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                SDL_MouseButtonEvent mouse = e.button;

                input.mouse.down = false;
                input.mouse.buttonFlags = SDL_GetMouseState(&input.mouse.pos.x, &input.mouse.pos.y);

                on_mouse_up(mouse.button);

                break;
            }
            case SDL_EVENT_MOUSE_MOTION:
            {
                input.mouse.buttonFlags = SDL_GetMouseState(&input.mouse.pos.x, &input.mouse.pos.y);
                on_mouse_move();
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED:
            {
				int render_size_x, render_size_y;
				SDL_GetWindowSize(window.window, &render_size_x, &render_size_y);

                update_ui_state(melv::vec2(render_size_x, render_size_y));

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
                    Font editorFont = catalog.get_font(editor_font);

                    text_field->append_string(input_text);
                    text_field->update_text(render, editorFont, true);
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
    melv::vec2 mouse_pos = input.mouse.pos;

    for (auto& editor : ui.editor)
    {
        melv::Rectangle text_area = editor.get_text_area();
        melv::Rectangle title_area = editor.get_title_area();

        if (editor.resize.resize)
        {
            editor.field.m_area = editor.resize.calculate_new_area(mouse_pos, 50, 2000);
        }
        else
        {
            melv::Direction dir = text_area.on_edge(mouse_pos, 3);
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
            melv::Direction dir = panel.area.on_edge(mouse_pos, 3);
            if (melv::direction_is_horizontal(dir))
            {
                SDL_SetCursor(input.mouse.cursor.resize_ew);
            }
            else if (melv::direction_is_vertical(dir))
            {
                SDL_SetCursor(input.mouse.cursor.resize_ns);
            }
            else
            {
                SDL_SetCursor(input.mouse.cursor.normal);
            }
        }
    }
}

void Application::set_text_editor_cursor(melv::Rectangle text_area, melv::Direction dir)
{
    melv::vec2 mouse_pos = input.mouse.pos;

    if (dir) {
        if (direction_is_vertical(dir))
        {
            SDL_SetCursor(input.mouse.cursor.resize_ns);
        }
        else if (direction_is_horizontal(dir))
        {
            SDL_SetCursor(input.mouse.cursor.resize_ew);
        }
    }
    else if (text_area.contains_centered(mouse_pos)) {
        SDL_SetCursor(input.mouse.cursor.text);
    }
    else {
        // SDL checks wheter the cursor set is different so no unnecessary redraws here
        SDL_SetCursor(input.mouse.cursor.normal);
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
		if (!input.keyboard.do_input)
		{
			return false;
		}

        return false;
    }
}

bool Application::keyboard_input_down_common(KeyboardEvent keyboard)
{
    UiState* ui = get_active_ui();

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
            if (ui && doing_text_input)
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
            if (ui && doing_text_input)
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
            if (ui && doing_text_input)
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

                    field->update_text(render, catalog.get_font(field->fontId), true);
                }
            }
            return true;
        }
        case SDL_SCANCODE_DELETE:
        {
            if (ui && doing_text_input)
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
                    field->update_text(render, catalog.get_font(field->fontId), true);
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_HOME:
        {
            if (ui && doing_text_input)
            {
                auto field = ui->get_selected_text_field();
                if (field) {
                    field->m_cursor = 0;
                    field->m_selection_point = 0;

                    Font f = catalog.get_font(field->fontId);
                    field->update_text(render, f, true);
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_END:
        {
            if (ui && doing_text_input)
            {
                auto field = ui->get_selected_text_field();
                if (field) {
                    field->m_cursor = field->m_buffer.length;
                    field->m_selection_point = field->m_buffer.length;

                    Font f = catalog.get_font(field->fontId);
                    field->update_text(render, f, true);
                }
                return true;
            }

            break;
        }
        case SDL_SCANCODE_LEFT: {
            if (ui && doing_text_input) {
                auto field = ui->get_selected_text_field();
                if (field) {
                    String s = field->get_string();
                    int step = utf8_previous(s, field->m_cursor);

                    int selectionPos = field->m_selection_point;
                    field->m_cursor = MAX(0, field->m_cursor - step);

                    Font f = catalog.get_font(field->fontId);
                    field->update_text(render, f, true);

                    if (input.keyboard.mod_state & KEYMOD_LEFT_SHIFT)
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
            if (ui && doing_text_input) {
                auto field = ui->get_selected_text_field();
                if (field) {
                    String s = field->get_string();
                    int step = utf8_next(s, field->m_cursor);

                    int selectionPos = field->m_selection_point;
                    field->m_cursor = MIN(field->m_cursor + step, field->m_buffer.length);

                    Font f = catalog.get_font(field->fontId);
                    field->update_text(render, f, true);

                    if (input.keyboard.mod_state & KEYMOD_LEFT_SHIFT)
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
            SDL_SetWindowFullscreen(window.window, !is_fullscreen());
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
    melv::vec2 mouse_pos = input.mouse.pos;

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

    if ((input.mouse.buttonFlags & MOUSE_LEFT_MASK) && ui && !ui->get_drag_info() && !ui->doing_resize())
    {
        input.mouse.dragPosition = input.mouse.pos;
        input.mouse.drag = true;
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

        input.mouse.drag = false;
    }
}

bool Application::mouse_input_common()
{
	melv::vec2 mouse_pos = input.mouse.pos;
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

    		melv::Rectangle area = field.m_area;
    		if (area.contains_centered(mouse_pos)) {
                melv::vec2 relative = mouse_pos - area.get_top_left();
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

                Font f = catalog.get_font(field.fontId);
                field.m_cursor = field.calculate_cursor_from_mouse(relative, field.get_string(), f, true);
                field.m_selection_point = field.m_cursor;

    			return true;
    		}
    	}
    }

	return false;
}

void Application::update_keyboard_state()
{
    input.keyboard.keys = SDL_GetKeyboardState(&input.keyboard.num_keys);
    input.keyboard.mod_state = SDL_GetModState();
}

void Application::update()
{
    // update time
    SDL_Time time = SDL_GetTicks();
    double time_sec = (double)time / MILLISECONDS_PER_SECOND;
    timeInfo.deltaTime = time - timeInfo.time;
    timeInfo.deltaTimeSeconds = time_sec - timeInfo.timeSeconds;
    timeInfo.time = time;
    timeInfo.timeSeconds = time_sec;

    update_ui_pos();
    timeout();

	user_update();
}

void Application::user_update()
{
	if (!user.update_state)
	{
		return;
	}

	constexpr int maxIterationsPerFrame = 50;
    int iterations = 0;
	double timeStep = user.update_state->calculateTimeStep();
    while ((user.update_state->elapsed < timeInfo.timeSeconds + timeInfo.deltaTimeSeconds) && iterations < maxIterationsPerFrame)
    {
		if (user.update_state->fixedUpdate)
		{
			user.update_state->fixedUpdate(user.userdata, this);
		}
        user.update_state->elapsed += timeStep * user.update_state->timeScale;
        user.update_state->ticks += 1;

        iterations += 1;
    }

	if (user.update_state->update)
	{
		user.update_state->update(user.userdata, this);
	}
}

void Application::timeout()
{
    for (int i = 0; i < events.size(); i++)
    {
        if (events[i].active)
        {
            if (events[i].event < timeInfo.time)
            {
                events[i].active = false;
            }
        }
    }
}

void Application::update_ui_state(melv::vec2 window_size) {
    for (int i = 0; i < uiStates.size(); i++)
    {
        melv::vec2 assumed = uiStates[i].assumed_window_size;
        float x_factor = window_size.x / assumed.x;
        float y_factor = window_size.y / assumed.y;
        if ((fabsf(x_factor - 1.0f) >= 0.1f) || (fabsf(y_factor - 1.0f) >= 0.1f)) {
            uiStates[i].update_state(window_size, render, catalog);
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

    melv::vec2 mouse_pos = input.mouse.pos;

    for (auto& editor : ui->editor)
    {
        if (editor.drag.drag)
        {
            melv::Rectangle area = editor.get_text_area();
            melv::vec2 half_scale = melv::vec2(area.w / 2, area.h / 2);
            melv::vec2 dst = (mouse_pos - editor.drag.start) + half_scale;
            dst.y += editor.title_height;
            editor.set_position(dst);
        }
    }

    for (auto& panel : ui->panel)
    {
        if (panel.drag.drag)
        {
            melv::vec2 half_scale = panel.area.get_scale() / 2;
            melv::vec2 pos = (mouse_pos - panel.drag.start) + half_scale;
            panel.area.x = pos.x;
            panel.area.y = pos.y;
        }
    }
}

void Application::set_event_active(int event_index, double timeout_seconds)
{
    s64 timeout = (s64)(timeout_seconds * MILLISECONDS_PER_SECOND);
    events[event_index].active = true;
    events[event_index].event = timeInfo.time + timeout;
}

void Application::set_event_deactive(int event_index)
{
    events[event_index].active = false;
}

void Application::cleanup()
{
	if (user.before_cleanup)
	{
		user.before_cleanup(user.userdata, this);
	}

    MIX_Quit();
    SDL_Quit();
    TTF_Quit();

	if (user.after_cleanup)
	{
		user.after_cleanup(user.userdata, this);
	}
}

bool Application::init_render(bool enableDebug)
{
    if (!initialize_render_context(&render, window.window, enableDebug))
    {
        return false;
    }

    return true;
}

void Application::draw()
{
    if (SDL_GetWindowFlags(window.window) & SDL_WINDOW_MINIMIZED) {
        // don't draw anything if the window is minimized
        return;
    }

    if (!start_frame(render, window.window))
    {
        // couldn't get command buffer or the swapchain texture
        return;
    }

	if (user.draw)
	{
		user.draw(user.userdata, this);
	}

	end_frame(render, window.window);
}

bool Application::is_minimized() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(window.window);
    return flags & SDL_WINDOW_MINIMIZED;
}

bool Application::is_maximized() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(window.window);
    return flags & SDL_WINDOW_MAXIMIZED;
}

bool Application::is_fullscreen() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(window.window);
    return flags & SDL_WINDOW_FULLSCREEN;
}

melv::vec2 Application::get_window_size() const {
    melv::ivec2 s;
    SDL_GetWindowSize(window.window, &s.x, &s.y);
    return melv::vec2(s.x, s.y);
}

void Application::draw_ui_state(UiState& state)
{
	melv::vec2 mouse_pos = input.mouse.pos;

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
            render_textured_rectangle(render, melv::Rectangle(button.position, button.scale), render.get_texture(button.text.texture), button.background, true);
        }
    }

    for (const ImageButton& button : state.image_button)
    {
        if (button.info.visible)
        {
            render_textured_rectangle(render, melv::Rectangle(button.position, button.scale), render.get_texture(button.image), button.background, true);
        }
    }

    for (const Label& label : state.label)
    {
        render_textured_rectangle(render, melv::Rectangle(label.position, label.scale), render.get_texture(label.text.texture), label.background, false);
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

    GPUTexture tex = render.get_texture(state.hoverText.text.texture);
	float hoverWidth = tex.width;
	float hoverHeight = tex.height;
	render_textured_rectangle(render, melv::Rectangle(mouse_pos.x, mouse_pos.y, hoverWidth, hoverHeight), tex, state.hoverText.background);
}

// @todo port these

void Application::render_rectangle(melv::Rectangle rect, melv::Color color, bool center) const
{
/*
    SDL_SetRenderDrawColor(render.renderer, COLOR_ARG(color));
    SDL_FRect area = center ?
                    SDL_FRect { rect.x - rect.w / 2, rect.y - rect.h / 2, rect.w, rect.h } :
                    SDL_FRect { rect.x, rect.y, rect.w, rect.h };
    SDL_RenderFillRect(render.renderer, &area);
*/
}

void Application::render_rectangle_outline(melv::Rectangle rect, melv::Color color, bool center) const
{
/*
    SDL_SetRenderDrawColor(render.renderer, COLOR_ARG(color));
    SDL_FRect area = center ?
                    SDL_FRect { rect.x - rect.w / 2, rect.y - rect.h / 2, rect.w, rect.h } :
                    SDL_FRect { rect.x, rect.y, rect.w, rect.h };
    SDL_RenderRect(render.renderer, &area);
*/
}

void Application::render_discrete_slider(const DiscreteSlider& slider) const
{
/*
    melv::vec2 start = slider.get_start();
    melv::vec2 step = slider.get_step();

    melv::Rectangle area = slider.get_bounds();
    render_rectangle_outline(area, slider.outlineColor, false);

    for (int i = 0; i < slider.element_count; i++)
    {
        melv::Rectangle area (start + i * step, slider.element_scale);
        float t = float (i) / slider.element_count;
        melv::ColorF color = i <= slider.selected ? melv::mixColors(slider.startColor, slider.endColor, t) : slider.inactiveColor;

        if (slider.texture.texture)
        {
            render_texture_with_tint(render, area, slider.texture, color, true);
        }
        else
        {
            render_rectangle(area, melv::Color(color));
        }

        render_rectangle_outline(area, melv::Color(slider.buttonColor));
    }
*/
}

void Application::render_slider(melv::Rectangle area, melv::vec2 knob_scale, float value, melv::Color slider_color, melv::Color knob_color, const Text& text) const
{
/*
    float slider_knob_width = area.w * knob_scale.x;
    float slider_knob_height = area.h * knob_scale.y;

    SDL_SetRenderDrawColor(render.renderer, COLOR_ARG(slider_color));
    SDL_FRect slider = { area.x, area.y, area.w, area.h };
    SDL_RenderFillRect(render.renderer, &slider);
    float percentage = value;
    SDL_SetRenderDrawColor(render.renderer, COLOR_ARG(knob_color));
    SDL_FRect slider_knob = {
        slider.x - (slider_knob_width / 2) + (slider.w * percentage), slider.y + slider.h / 2 - slider_knob_height / 2,
        slider_knob_width, slider_knob_height
    };
    SDL_RenderFillRect(render.renderer, &slider_knob);

    // text
    {
        const int margin = 10;
        render_text_scale(render, text,
            melv::vec2(slider.x + slider.w / 2, slider.y + slider.h * 2 + margin), melv::vec2(0.6, 0.6));
    }
*/
}

void Application::render_panel(const Panel& panel) const
{
/*
    render_rectangle(panel.get_title_area(), panel.title_bar_color);

    auto& tab = panel.tabs.get_ref(panel.activeTab);
    render_rectangle(panel.area, tab.color);

    const float margin = 16;
    const float iconSize = 32;
    for (int i = 0; i < tab.icons.size(); i++) {
        melv::Rectangle area = panel.get_icon_area(i);
        render_textured_rectangle(render, area, tab.icons.get(i).icon.texture, tab.icons.get(i).icon.background, true, false);
    }

    for (int i = 0; i < panel.tabs.size(); i++) {
        melv::Rectangle area = panel.get_tab_header_area(i);
        render_textured_rectangle(render, area, panel.tabs.get(i).tabIcon.texture, panel.tabs.get(i).tabIcon.background, true);
    }
*/
}

void Application::render_value_panel(const UiState& ui, const ValuePanel& panel) const
{
/*
    auto& tab = panel.tabs.get_ref(panel.activeTab);
    render_rectangle(panel.area, tab.color, true);

    float height = 0;
    for (int i = 0; i < tab.fields.size(); i++)
    {
        ValueField& value = tab.fields[i];

        melv::Rectangle text_area = panel.get_field_title_area(panel.activeTab, i);
        text_area.y += height;
        height += text_area.h;

        render_texture(render, text_area, value.name.texture, true);

        melv::Rectangle area = panel.get_field_area(panel.activeTab, i, &ui);
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
            case ValueLabel: { break; } //nothing extra to display
            case ValueSelection: {
                ButtonGroup& group = ui.button_group.get_ref(value.ui_element);
                group.position = melv::vec2(area.x, area.y);
                group.scale = melv::vec2(area.w, area.h);
                render_button_group(group);

                height += group.scale.y;
                break;
            }
            case ValueButton: {
                render_rectangle_outline(text_area, melv::Color(0x99, 0x55, 0x66));
                break;
            }
        }

        height += tab.field_margin;
    }

    if (panel.showTabs && panel.tabs.size() > 1)
    {
        for (int i = 0; i < panel.tabs.size(); i++) {
            melv::Rectangle area = panel.get_tab_header_area(i);
            render_textured_rectangle(render, area, panel.tabs.get(i).tabIcon.texture, panel.tabs.get(i).tabIcon.background, true);
        }
    }
*/
}

void Application::render_button_group(const ButtonGroup& group) const
{
/*
    render_rectangle(melv::Rectangle(group.position, group.scale), group.background);
    melv::vec2 top_left = group.position - group.scale / 2;
    int numColumns = std::floor(group.scale.x / group.button_scale.x);
    int row = 0;
    int column = 0;
    for (auto& texture : group.buttons)
    {
        draw_texture(render, melv::Rectangle(top_left + melv::vec2(column * group.button_scale.x, row * group.button_scale.y) + group.button_scale / 2, group.button_scale), texture);
        column += 1;
        row = (column == numColumns) ? row + 1 : row;
    }
*/
}

void Application::render_text_editor(TextEditor& editor) const
{
/*
    melv::Rectangle text_area = editor.field.m_area;
    melv::Rectangle title_area = editor.get_title_area();
    render_textured_rectangle(render, title_area, editor.title_texture, editor.title_bar_color);

    melv::Rectangle area = editor.get_title_area();
    melv::vec2 iconPos = area.get_position() + melv::vec2(area.get_scale().x / 2, 0);
    melv::vec2 iconScale = melv::vec2(editor.title_height, editor.title_height);

    melv::Color clicked_background = melv::Color(0xAA, 0x55, 0x33);
    render_textured_rectangle(render, editor.get_icon1_area(), editor.icon1.texture, (editor.clicked_icon == 1) ? clicked_background : editor.icon1.background, true);
    render_textured_rectangle(render, editor.get_icon2_area(), editor.icon2.texture, (editor.clicked_icon == 2) ? clicked_background : editor.icon2.background, true);
    render_textured_rectangle(render, editor.get_icon3_area(), editor.icon3.texture, (editor.clicked_icon == 3) ? clicked_background : editor.icon3.background, true);

    render_text_field(editor.field);
*/
}

void Application::render_text_field(Text_Field& text_field) const
{
/*
    melv::Rectangle area = text_field.m_area;
    render_rectangle(area, text_field.background);

    SDL_Texture* text_texture = text_field.m_texture;

    Font font = catalog.get_font(text_field.fontId);

    if (text_texture)
    {
        melv::vec2 top_left = area.get_top_left();
        melv::vec2 text_scale = {};
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
        SDL_SetRenderClipRect(render.renderer, &clip);

        draw_texture(render, melv::Rectangle(top_left, text_scale), text_texture);

        SDL_SetRenderClipRect(render.renderer, nullptr);

        if (doing_text_input)
        {
            String string = text_field.get_string();

            melv::ColorF highlightColor (0.2, 0.2, 0.6, 0.5);

            // selected area
            CursorScreenPosition cursorPos = text_field.get_cursor_from_selection(text_field.m_selection_point, string, font, true);
            int lineCount = std::abs(text_field.m_cursor_line - cursorPos.line);
            if (lineCount == 0)
            {
                float width = std::fabsf(text_field.m_cursor_pixel_x - cursorPos.pixel_x);
                int pixelX = melv::min(text_field.m_cursor_pixel_x, cursorPos.pixel_x);
                melv::Rectangle area = {
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

                render_rectangle(melv::Rectangle(top_left.x + start.pixel_x, top_left.y + start.line * line_skip, area.w - start.pixel_x, line_skip), highlightColor, false);
                for (int i = start.line + 1; i < end.line; i++)
                {
                    log_info("%d", i);
                    render_rectangle(melv::Rectangle(top_left.x, top_left.y + i * line_skip, area.w, line_skip), highlightColor, false);
                }
                render_rectangle(melv::Rectangle(top_left.x, top_left.y +  end.line * line_skip, end.pixel_x, line_skip), highlightColor, false);
            }

            // cursor
            float cursor_width = 5;
            render_rectangle(
                melv::Rectangle(melv::vec2(top_left.x + text_field.m_cursor_pixel_x - cursor_width / 2,
                                            top_left.y + text_field.m_cursor_pixel_y + font_size / 2),
                                melv::vec2(cursor_width, font_size)),
                                TextCursorColor);
        }
    }
*/
}

void Application::render_dropdown(const Drop_Down_List& list) const {
/*
    SDL_SetRenderDrawColor(render.renderer, COLOR_ARG(list.title_color));

    SDL_FRect header_area = {
        list.pos.x - list.scale.x/2, list.pos.y - list.scale.y / 2,
        list.scale.x, list.scale.y
    };
    SDL_RenderFillRect(render.renderer, &header_area);
    Text title_text = list.selected == DROP_DOWN_LIST_SELECTED_SENTINEL ? list.title : list.get_option_text(list.selected);
    render_text_size(render.renderer, title_text,
        melv::vec2(header_area.x + header_area.w / 2, header_area.y + header_area.h / 2), melv::vec2(header_area.w, header_area.h));

    if (list.open) {
        SDL_SetRenderDrawColor(render.renderer, COLOR_ARG(list.option_color));

        for (int i = 0; i < list.options.size(); i++) {
            SDL_FRect area = header_area;
            area.y += area.h * (i + 1);
            SDL_RenderFillRect(render.renderer, &area);
            render_text_size(render.renderer, list.get_option_text(i),
                melv::vec2(area.x + area.w/2, area.y + area.h/2), melv::vec2(area.w, area.h));
        }
    }
*/
}

Icon Application::create_icon(AssetId image, melv::Color background) {
    TextureHandle texture = catalog.get_image(image);
    return Icon(texture, background);
}

void Application::text_input_stop()
{
    SDL_StopTextInput(window.window);
    doing_text_input = false;
    input.keyboard.do_input = true;

    for (int i = 0; i < uiStates.size(); i++)
    {
        uiStates[i].text_input_target = {};
    }

    render.clear_color = {};
}

void Application::text_input_start()
{
    SDL_StartTextInput(window.window);
    doing_text_input = true;
    input.keyboard.do_input = false;

    // @todo not hardcode
    render.clear_color = {0, 0x44, 0x66, 0xff};
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

} // namespace
