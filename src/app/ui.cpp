#include "ui.hpp"
#include "util/log.hpp"

void GapBuffer::initialize(int init_buffer_size) {
    buffer = (char*) std::malloc(init_buffer_size);
    buffer_size = init_buffer_size;
    gap_index = 0;
    end_gap = init_buffer_size;
    length = 0;
}

void GapBuffer::reset() {
    std::free(buffer);
    buffer = nullptr;
    buffer_size = 0;
    length = 0;
    gap_index = 0;
    end_gap = 0;
}

void GapBuffer::append(String string, int where)
{
    if (!(where <= length && where >= 0)) {
        return;
    }

    if (length + string.size > buffer_size) {
        ASSERT(buffer_size >= 0);
        if (buffer_size == 0)
        {
            initialize(256);
        }
        else
        {
            resize(buffer_size * 2);
        }
    }

    if (where != gap_index)
    {
        move_gap(where);
    }

    for (int i = 0; i < string.size; i++)
    {
        buffer[gap_index + i] = string.data[i];
    }

    length += string.size;
    gap_index += string.size;
}

void GapBuffer::remove(int where, int amount)
{
    if (where + amount > length)
    {
        amount = length - where;
    }

    if (!(where < length && where >= 0))
        return;

    move_gap(where);
    end_gap += amount;
    length -= amount;
}

char GapBuffer::get_character(int index)
{
    if (index < gap_index)
    {
        return buffer[index];
    }
    else
    {
        return buffer[end_gap + index - gap_index];
    }
}

void GapBuffer::move_gap(int position)
{
    if (!(position <= length && position >= 0)) {
        return;
    }

    int start = 0;
    int dest = 0;
    int amount = 0;
    if (position < gap_index) {
        amount = gap_index - position;
        start = position;
        dest = end_gap - amount;
    }
    else {
        amount = position - gap_index;
        start = end_gap;
        dest = gap_index;
    }

    std::memmove(buffer + dest, buffer + start, amount);

    int gap_size = end_gap - gap_index;
    gap_index = position;
    end_gap = gap_index + gap_size;
}

void GapBuffer::resize(int size)
{
    if (size < length) {
        panic("Invalid GapBuffer");
        return;  // failure
    }

    int start_chars = gap_index;
    int end_chars = buffer_size - end_gap;

    char* nbuffer = (char*) std::malloc(size);
    ASSERT(nbuffer);
    std::memcpy(nbuffer, buffer, start_chars);
    std::memcpy(nbuffer + (size - end_chars), buffer + end_gap, end_chars);
    std::free(buffer);

    buffer = nbuffer;
    end_gap = size - end_chars;
    buffer_size = size;
}

void GapBuffer::get_string(String_Builder& sb)
{
	sb.clear_and_append(String(buffer, gap_index));
	sb.append(String(buffer + end_gap, length - gap_index));
}

void Text_Field::calculate_cursor_from_selection(String string, Font font, bool wrapped)
{
    auto cursorPosition = get_cursor_from_selection(m_cursor, string, font, wrapped);

    m_cursor_line = cursorPosition.line;
    m_cursor_pixel_x = cursorPosition.pixel_x;
    m_cursor_pixel_y = cursorPosition.pixel_y;
}

CursorScreenPosition Text_Field::get_cursor_from_selection(int cursor, String string, Font font, bool wrapped)
{
    CursorScreenPosition pos = {};

    int line_skip = TTF_GetFontLineSkip(font.font);

    // calculate cursor position
    int cursor_line = 0;
    int cursor_pixel_x = 0;
    size_t cursor_byte = 0;

    melv::Rectangle area = m_area;

    if (wrapped)
    {
        while (cursor_byte < cursor)
        {
            size_t to_next_newline = 0;
            size_t cursor_character_this_line = 0;

            // measure distance to linebreak
            while (cursor_byte + to_next_newline < cursor && string.data[cursor_byte + to_next_newline] != '\n') {
                to_next_newline += 1;
            }

            if (to_next_newline == 0) {
                // a line that is just a newline character
                cursor_byte += 1;
                cursor_line += 1;
            }

            if (string.data[cursor_byte + to_next_newline] == '\n') {
                to_next_newline += 1;  // skip the newline character as well. It is not rendered and don't leave the cursor on it because then it will cause the loop to break
            }

            // measure distance to end of text render area
            TTF_MeasureString(font.font, string.data + cursor_byte, cursor - cursor_byte, area.w, &cursor_pixel_x, &cursor_character_this_line);

            // take the minimum
            cursor_character_this_line = MIN(cursor_character_this_line, to_next_newline);

            if (cursor_character_this_line == 0)
            {
                break;
            }

            cursor_byte += cursor_character_this_line;

            cursor_line += 1;
        }

        if (cursor_line)
        {
            cursor_line -= 1;  // 0 based indexing instead of 1 based indexing
        }

        int cursor_pixel_y = cursor_line * line_skip;

        pos.line = cursor_line;
        pos.pixel_x = cursor_pixel_x;
        pos.pixel_y = cursor_pixel_y;
        return pos;
    }
    else {
        TTF_MeasureString(font.font, string.data, cursor, MAX_INTEGER, &cursor_pixel_x, nullptr);

        pos.line = 0;
        pos.pixel_x = cursor_pixel_x;
        pos.pixel_y = 0;
    }

    return pos;
}

size_t Text_Field::calculate_cursor_from_mouse(melv::vec2 position, String string, Font font, bool wrapped)
{
    int line_skip = TTF_GetFontLineSkip(font.font);
    melv::Rectangle area = m_area;
    int line_count = m_line_count;

    int cursor_line = position.y / line_skip;

    if (cursor_line >= line_count)
    {
        m_cursor_line = m_line_count - 1;
        return m_buffer.length;
    }

    size_t cursor_character = 0;
    int pixel_x = 0;
    int pixel_y = cursor_line * line_skip;

    if (wrapped)
    {
        // calculate what the lines above us add up to in character count
        for (int i = 0; i < cursor_line; i++)
        {
            size_t cursor_character_this_line = 0;

            TTF_MeasureString(font.font, string.data + cursor_character, string.size - cursor_character, area.w, nullptr, &cursor_character_this_line);

            cursor_character += cursor_character_this_line;
        }
    }

    size_t last_line_character = 0;
    TTF_MeasureString(font.font, string.data + cursor_character, string.size - cursor_character, position.x, &pixel_x, &last_line_character);

    if (cursor_character + last_line_character < string.size)
    {
        int next_pixel_x = 0;

        TTF_MeasureString(font.font, string.data + cursor_character, last_line_character + 1, area.w, &next_pixel_x, NULL);

        if (position.x > (pixel_x + next_pixel_x) / 2)
        {
            last_line_character += 1;
            pixel_x = next_pixel_x;
        }
    }

    cursor_character += last_line_character;

    m_cursor_line = cursor_line;
    m_cursor_pixel_x = pixel_x;
    m_cursor_pixel_y = pixel_y;

    return cursor_character;
}

bool Text_Field::render_text_field_texture(SDL_Renderer* renderer, Font font, melv::Color color, bool wrapped)
{
    SDL_DestroyTexture(m_texture);  // old texture
    m_texture = nullptr;

    String str = get_string();
    if (str.size == 0)
    {
        return true;
    }

    const SDL_Color text_color = {color.r, color.g, color.b, color.a};
    SDL_Surface* text_surface;

    if (wrapped) {
        text_surface = TTF_RenderText_Solid_Wrapped(font.font, str.data, str.size, text_color, m_area.w);
    } else {
        text_surface = TTF_RenderText_Solid(font.font, str.data, str.size, text_color);
    }

    if (!text_surface)
    {
        return false;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    SDL_DestroySurface(text_surface);

    if (!texture)
    {
        return false;
    }

    float texture_width, texture_height;
    SDL_GetTextureSize(texture, &texture_width, &texture_height);
    int line_skip = TTF_GetFontLineSkip(font.font);
    int line_count = (wrapped) ? MAX(1, (int)(texture_height / line_skip)) : (1);

    calculate_cursor_from_selection(str, font, wrapped);

    m_line_count = line_count;
    m_texture = texture;
    m_font_size = font.size;

    return true;
}

void Text_Field::insert_tab(int tab_width)
{
    if (tab_width > 16)
    {
        tab_width = 16;
        log_warning("Tab width set to larger than 16");
    }

    char buffer[16] = {};
    for (int i = 0; i < tab_width; i++)
    {
        buffer[i] = ' ';
    }

    delete_text();
    m_buffer.append(String(buffer, tab_width), m_cursor);

    m_cursor += tab_width;
    m_selection_point = m_cursor;
}

void Text_Field::insert_line()
{
    delete_text();
    m_buffer.append(String("\n"), m_cursor);

    m_cursor += 1;
    m_selection_point = m_cursor;
}

melv::Rectangle ResizeInfo::calculate_new_area(melv::vec2 mouse_position, int min, int max) const
{
    melv::Rectangle area = initialArea;
    melv::vec2 p = area.get_point_at_direction(direction);
    melv::vec2 d = mouse_position - p;

    if (direction & melv::DirEast)
    {
        area.x += d.x / 2;
        area.w += d.x;
    }
    else if (direction & melv::DirWest)
    {
        area.x += d.x / 2;
        area.w -= d.x;
    }

    if (direction & melv::DirNorth)
    {
        area.y += d.y / 2;
        area.h += d.y;
    }
    else if (direction & melv::DirSouth)
    {
        area.y += d.y / 2;
        area.h -= d.y;
    }

    area.w = melv::clamp(min, max, area.w);
    area.h = melv::clamp(min, max, area.h);

    return area;
}

void UiState::reinit_text(RenderContext& render, Font font)
{
    for (auto& lbl : label) {
        SDL_DestroyTexture(lbl.text.texture);
        lbl.text = create_text(render.renderer, lbl.text.string, font, lbl.text.color);
    }

    for (auto& but : button) {
        SDL_DestroyTexture(but.text.texture);
        but.text = create_text(render.renderer, but.text.string, font, but.text.color);
    }

    for (auto& drop : drop_down) {
        for (auto& e : drop.options)
        {
            SDL_DestroyTexture(e.label.texture);
            e.label = create_text(render.renderer, e.label.string, font, e.label.color);
        }

        SDL_DestroyTexture(drop.title.texture);
        drop.title = create_text(render.renderer, drop.title.string, font, drop.title.color);
    }

    for (auto& vp : value_panel)
    {
        for (auto& tab : vp.tabs)
        {
            for (auto& field : tab.fields)
            {
                SDL_DestroyTexture(field.name.texture);
                field.name = create_text(render.renderer, field.name.string, font, field.name.color);
            }
        }
    }
}

bool UiState::doing_resize() const
{
    for (auto& e : editor)
    {
        if (e.resize.resize)
        {
            return true;
        }
    }

    for (auto& p : panel)
    {
        if (p.resize.resize)
        {
            return true;
        }
    }

    return false;
}

DragInfo* UiState::get_drag_info()
{
    for (auto& e : editor)
    {
        if (e.drag.drag)
        {
            return &e.drag;
        }
    }

    for (auto& p : panel)
    {
        if (p.drag.drag)
        {
            return &p.drag;
        }
    }

    for (auto& c : control)
    {
        if (c.drag.drag)
        {
            return &c.drag;
        }
    }

    return nullptr;
}

void UiState::update_state(melv::vec2 window_size, const RenderContext& render, const AssetCatalog& catalog) {
    float y_factor = window_size.y / assumed_window_size.y;
    float x_factor = window_size.x / assumed_window_size.x;

    for (auto& ed : editor) {
        ed.rescale(melv::vec2(x_factor, y_factor), render, catalog);
    }

    for (auto& field : text_field) {
        Font font = catalog.get_font(field.fontId);

        field.m_area.x *= x_factor;
        field.m_area.y *= y_factor;
        field.m_area.w *= x_factor;
        field.m_area.h *= y_factor;
        field.render_text_field_texture(render.renderer, font, field.text_color, true);
    }

    for (auto& drop : drop_down) {
        drop.pos.x *= x_factor;
        drop.pos.y *= y_factor;
        drop.scale.x *= x_factor;
        drop.scale.y *= y_factor;
    }

    for (auto& but : button) {
        but.position.x *= x_factor;
        but.position.y *= y_factor;
        but.scale.x *= x_factor;
        but.scale.y *= y_factor;
    }

    for (auto& img : image_button) {
        img.position.x *= x_factor;
        img.position.y *= y_factor;
        img.scale.x *= x_factor;
        img.scale.y *= y_factor;
    }

    for (auto& lbl : label) {
        lbl.position.x *= x_factor;
        lbl.position.y *= y_factor;
        lbl.scale.x *= x_factor;
        lbl.scale.y *= y_factor;
    }

    for (auto& c : control)
    {
        c.position.x *= x_factor;
        c.position.y *= y_factor;
        c.scale.x *= x_factor;
        c.scale.y *= y_factor;
    }

    for (auto& ds : discrete_slider)
    {
        ds.position.x *= x_factor;
        ds.position.y *= y_factor;
        ds.element_scale.x *= x_factor;
        ds.element_scale.y *= y_factor;
        ds.element_gap *= ds.vertical ? y_factor : x_factor;
    }

    for (auto& p : panel)
    {
        p.area.x *= x_factor;
        p.area.y *= y_factor;
        p.area.w *= x_factor;
        p.area.h *= y_factor;
    }

    for (auto& vp : value_panel)
    {
        vp.area.x *= x_factor;
        vp.area.y *= y_factor;
        vp.area.w *= x_factor;
        vp.area.h *= y_factor;
        vp.fieldSize *= direction_is_horizontal(vp.direction) ? y_factor : x_factor;
        vp.tabHeaderSize *= direction_is_horizontal(vp.direction) ? y_factor : x_factor;
    }

    assumed_window_size = window_size;
}

bool load_font(Font* font, String_Builder& path, String font_folder, String font_file, float size)
{
    path.append(font_folder);
    path.append(make_string(PathSeparator));

    path.append(font_file);

    TTF_Font* ttf_font = TTF_OpenFont(path.c_string(), size);

    bool success = load_font_file(font, path.c_string(), size);

    path.remove(font_folder.size + 1 + font_file.size);

    return success;
}

bool load_font_file(Font* font, const char* path, float size)
{
    TTF_Font* ttf_font = TTF_OpenFont(path, size);
    if (!ttf_font)
    {
        fprintf(stderr, "Could not load font %s\n", path);
        fprintf(stderr, "%s\n", SDL_GetError());
        return false;
    }

    font->font = ttf_font;
    font->size = size;

    return true;
}

void TextEditor::rescale(melv::vec2 scale, const RenderContext& render, const AssetCatalog& catalog)
{
    Font font = catalog.get_font(field.fontId);

    title_height *= scale.y;
    if (title_texture) {
        SDL_DestroyTexture(title_texture);
        title_texture = render_text(render.renderer, name.to_string(), font, title_color);
    }

    field.m_area.x *= scale.x;
    field.m_area.y *= scale.y;
    field.m_area.w *= scale.x;
    field.m_area.h *= scale.y;
    field.render_text_field_texture(render.renderer, font, field.text_color, true);
}

UiState::~UiState()
{
    for (auto& dd : drop_down) {
        dd.reset();
    }

    editor.reset();
    text_field.reset();
    drop_down.reset();
    button.reset();
    image_button.reset();
    label.reset();
    value_panel.reset();
    control.reset();
    discrete_slider.reset();
    button_group.reset();

    for (auto& l : label)
    {
        l.text.clear();
    }
}

Text_Field* UiState::get_selected_text_field()
{
    if (!(text_input_target.flags & TEXT_INPUT_TARGET_IS_VALID)) {
        return nullptr;
    }
    else if (text_input_target.flags & TEXT_INPUT_TARGET_IS_EDITOR) {
        return &editor.get_ref(text_input_target.index).field;
    }
    else {
        return text_field.get_ptr(text_input_target.index);
    }
}

TextEditor* UiState::get_editor(UiElementId id)
{
    for (auto& element : editor) {
        if (element.field.id == id) {
            return &element;
        }
    }

    return nullptr;
}

Text_Field* UiState::get_text_field(UiElementId id) {
    for (auto& element : text_field)
    {
        if (element.id == id)
        {
            return &element;
        }
    }

    return nullptr;
}

Drop_Down_List* UiState::get_drop_down(UiElementId id) {
    for (auto& element : drop_down)
    {
        if (element.id == id)
        {
            return &element;
        }
    }

    return nullptr;
}

TextButton* UiState::get_button(UiElementId id) {
    for (auto& element : button)
    {
        if (element.id == id)
        {
            return &element;
        }
    }

    return nullptr;
}

ImageButton* UiState::get_image_button(UiElementId id) {
    for (auto& element : image_button)
    {
        if (element.id == id)
        {
            return &element;
        }
    }

    return nullptr;
}

Panel* UiState::get_panel(UiElementId id)
{
    for (auto& element : panel)
    {
        if (element.id == id)
        {
            return &element;
        }
    }

    return nullptr;
}

Label* UiState::get_label(UiElementId id) {
    for (auto& element : label)
    {
        if (element.id == id)
        {
            return &element;
        }
    }

    return nullptr;
}

ValuePanel* UiState::get_value_panel(UiElementId id)
{
    for (auto& element : value_panel)
    {
        if (element.id == id)
        {
            return &element;
        }
    }

    return nullptr;
}

DiscreteSlider* UiState::get_discrete_slider(UiElementId id)
{
    for (auto& element : discrete_slider)
    {
        if (element.id == id)
        {
            return &element;
        }
    }

    return nullptr;
}

melv::Rectangle Panel::get_title_area() const
{
    return melv::Rectangle(area.x, area.y - (area.h + title_height) / 2, area.w, title_height);
}

melv::Rectangle Panel::get_icon_area(int index) const {
    ASSERT(index < tabs.get(activeTab).icons.size());

    float length_per_icon = iconSize + iconMargin;
    int rowCount = melv::max(area.h / length_per_icon, 1);
    int row = index % rowCount;
    int column = index / rowCount;
    return melv::Rectangle(area.get_top_left() + melv::vec2(column, row) * length_per_icon, melv::vec2(iconSize));
}

melv::Rectangle Panel::get_tab_header_area(int index) const {
    ASSERT(index < tabs.size());

    int rowCount = melv::max(area.h / (tabHeaderSize * 2), 1);
    int row = index % rowCount;
    int column = index / rowCount;
    return melv::Rectangle(area.x + area.w / 2 + tabHeaderSize / 2 + column * tabHeaderSize,
                     area.y - area.h / 2 + tabHeaderSize / 2 + row * tabHeaderSize * 2,
                     tabHeaderSize, tabHeaderSize);
}

ValuePanelTab& ValuePanel::get_active_tab() const
{
    return tabs.get_ref(activeTab);
}

void ValuePanel::switch_tabs(UiState& ui, int tabIndex)
{
    if (!tabs.in_bounds(tabIndex))
    {
        panic("Invalid value panel tab index to switch");
    }

    ValuePanelTab& active = get_active_tab();

    for (auto& field : active.fields)
    {
        switch (field.type)
        {
            case ValueInteger:  // fallthrough
            case ValueNumber:   // fallthrough
            case ValueString:
            {
                ui.text_field.get_ref(field.ui_element).info.active = false;
                break;
            }
            case ValueButton:
            {
                // nothing
                break;
            }
            case ValueLabel: break;
            case ValueSelection:
            {
                ui.button_group.get_ref(field.ui_element).info.active = false;
                break;
            }
        }
    }

    activeTab = tabIndex;
}

melv::Rectangle ValuePanel::get_field_title_area(int tabIndex, int fieldIndex) const
{
    ValuePanelTab& tab = tabs.get_ref(tabIndex);
    ValueField& field = tab.fields.get_ref(fieldIndex);

    melv::vec2 text_scale = {};
    SDL_GetTextureSize(field.name.texture, &text_scale.x, &text_scale.y);
    float factor = fieldSize / text_scale.y;
    text_scale.x *= factor;
    text_scale.y = fieldSize;
    melv::vec2 text_position (area.x, area.y - area.h / 2 + text_scale.y / 2);
    return melv::Rectangle(text_position, text_scale);
}

melv::Rectangle ValuePanel::get_field_area(int tabIndex, int fieldIndex, const UiState* ui) const
{
    ValuePanelTab& tab = tabs.get_ref(tabIndex);
    ValueField& field = tab.fields.get_ref(fieldIndex);

    float width = get_field_width();

    melv::vec2 top_left = area.get_top_left();

    switch (field.type)
    {
        case ValueInteger: {
            // fallthrough
        }
        case ValueNumber: {
            // fallthrough
        }
        case ValueString: {
            Text_Field& text_field = ui->text_field.get_ref(field.ui_element);
            int line_count = text_field.m_line_count;
            float font_size = text_field.m_font_size;
            float tf_height = (line_count == 0) ? font_size : font_size * line_count;
            melv::vec2 tf_scale = melv::vec2(width, tf_height);
            melv::vec2 tf_pos = melv::vec2(area.x, top_left.y + tf_height / 2);
            return melv::Rectangle(tf_pos, tf_scale);
        }
        case ValueSelection: {
            ButtonGroup& group = ui->button_group.get_ref(field.ui_element);
            melv::vec2 scale = melv::vec2(width, group.button_scale.y * group.buttons.size());
            melv::vec2 position = melv::vec2(area.x, top_left.y + group.scale.y / 2);
            return melv::Rectangle(position, scale);
        }
        case ValueLabel: // fallthrough
        case ValueButton: {
            return melv::Rectangle();
        }
        default: {
            return melv::Rectangle();
        }
    }
}

melv::Rectangle ValuePanel::get_tab_header_area(int index) const
{
    ASSERT(index < tabs.size());

    if (direction & melv::DirWest || direction & melv::DirEast)
    {
        int rowCount = melv::max(area.h / (tabHeaderSize * 2), 1);
        int row = index % rowCount;
        int column = index / rowCount;
        float x = area.x + area.w / 2 * (direction & melv::DirEast ? 1 : -1) + (tabHeaderSize / 2 + column * tabHeaderSize) * (direction & melv::DirEast ? 1 : -1);
        float y = area.y - area.h / 2 + tabHeaderSize / 2 + row * tabHeaderSize * 2;
        return melv::Rectangle(x, y,
                         tabHeaderSize, tabHeaderSize);
    }
    else if (direction & melv::DirNorth || direction & melv::DirSouth)
    {
        int columnCount = melv::max(area.w / (tabHeaderSize * 2), 1);
        int column = index % columnCount;
        int row = index / columnCount;
        float x = area.x + tabHeaderSize / 2 + column * tabHeaderSize * 2;
        float y = area.y + (direction & melv::DirSouth ? area.h : 0) + (tabHeaderSize / 2 + row * tabHeaderSize) * (direction & melv::DirSouth ? 1 : -1);
        return melv::Rectangle(x, y,
                         tabHeaderSize, tabHeaderSize);
    }
    else {
        return melv::Rectangle();
    }
}

melv::Rectangle DiscreteSlider::get_bounds() const
{
    float elem = vertical ? element_scale.y : element_scale.x;
    float long_axis = element_count * (elem + element_gap);
    melv::vec2 scale = vertical ? melv::vec2(element_scale.x, long_axis) : melv::vec2(long_axis, element_scale.y);
    return melv::Rectangle(position - scale / 2, scale);
}

melv::vec2 DiscreteSlider::get_start() const
{
    float elem = vertical ? element_scale.y + element_gap : element_scale.x + element_gap;
    melv::vec2 step = vertical ? melv::vec2(0, elem) : melv::vec2(elem, 0);
    float long_axis = element_count * elem;
    melv::vec2 offset = vertical ? melv::vec2(0, long_axis / 2) : melv::vec2(long_axis / 2, 0);
    return position - offset + step / 2;
}

melv::vec2 DiscreteSlider::get_step() const
{
    float elem = vertical ? element_scale.y + element_gap : element_scale.x + element_gap;
    return vertical ? melv::vec2(0, elem) : melv::vec2(elem, 0);
}

melv::vec2 DiscreteSlider::get_button_scale() const
{
    melv::vec2 extraButtonSpace = vertical ? melv::vec2(0, element_gap) : melv::vec2(element_gap, 0);
    return element_scale + extraButtonSpace;
}
