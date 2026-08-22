#ifndef UI_HPP
#define UI_HPP

#include <SDL3/SDL.h>

#include "asset.hpp"
#include "text.hpp"

#include "util/common.hpp"
#include "util/template.hpp"
#include "util/math_util.hpp"

namespace melv
{

struct Window {
    SDL_Window* window;
};

struct UiState;

using UiElementId = u32;

struct UiElementInfo {
    bool visible = false;
    bool active = false;

    UiElementInfo() {}
    UiElementInfo(bool vis) : visible(vis) {}
};

struct DragInfo {
    melv::vec2 start = {};
    bool drag = false;
};

struct ResizeInfo {
    melv::vec2 start = {};
    melv::Rectangle initialArea = {};
    melv::Direction direction = {};
    bool resize = false;

    melv::Rectangle calculate_new_area(melv::vec2 mouse_position, int min, int max) const;
};

struct Label {
    UiElementId id = {};
    Text text = {};
    melv::vec2 position = {};  // center
    melv::vec2 scale = {};
    melv::Color background = {};

    Label() {}
    Label(Text p_text, melv::vec2 pos, melv::vec2 sca, melv::Color back) : text(p_text), position(pos), scale(sca), background(back) {}
};

struct TextButton {
    UiElementId id = {};
    UiElementInfo info = {};
    UiUserData data = {};
    Text text = {};
    melv::vec2 position = {};
    melv::vec2 scale = {};
    melv::Color background = {};

    TextButton() {}
    TextButton(Text p_text, melv::vec2 pos, melv::vec2 sca, melv::Color back, bool visible = true) : info(visible), text(p_text), position(pos), scale(sca), background(back) {}
};

struct ImageButton {
    UiElementId id = {};
    UiElementInfo info = {};
    UiUserData data = {};
    Texture image = {};
    melv::vec2 position = {};
    melv::vec2 scale = {};
    melv::Color background = {};

    ImageButton() {}
    ImageButton(Texture image, melv::vec2 pos, melv::vec2 sca, melv::Color back, bool visible = true) : info(visible), image(image), position(pos), scale(sca), background(back) {}
};

struct ButtonGroup {
    UiElementId id = {};
    UiElementInfo info = {};
    UiUserData user = {};
    DArray<Texture> buttons = {};
    melv::vec2 button_scale = {};
    melv::vec2 position = {};
    melv::vec2 scale = {};
    melv::Color background = {};

    ButtonGroup() {}
    ButtonGroup(UiElementId ident, melv::vec2 pos, melv::vec2 sca, melv::Color back) : id(ident), position(pos), scale(sca), background(back) {}
};

struct GapBuffer {
    char* buffer = nullptr;
    int buffer_size = 0;
    int length = 0;
    int gap_index = 0;
    int end_gap = 0;

    GapBuffer() {}

    GapBuffer(GapBuffer& other) = delete;
    void operator=(GapBuffer& other) = delete;
    GapBuffer(GapBuffer&& other) noexcept {
        if (buffer) { std::free(buffer); }
        buffer = other.buffer;
        buffer_size = other.buffer_size;
        length = other.length;
        gap_index = other.gap_index;
        end_gap = other.end_gap;

        other.clear_values();
    }
    void operator=(GapBuffer&& other) noexcept {
        if (buffer) { std::free(buffer); }
        buffer = other.buffer;
        buffer_size = other.buffer_size;
        length = other.length;
        gap_index = other.gap_index;
        end_gap = other.end_gap;

        other.clear_values();
    }

    ~GapBuffer() {
        reset();
    }

    void initialize(int init_buffer_size);
    void append(String string, int where);
    void remove(int where, int amount);
    char get_character(int index);
    void move_gap(int position);
    void resize(int size);
    void get_string(String_Builder& sb);

    void reset();
private:
    void clear_values() {
        buffer = nullptr;
        length = 0;
        gap_index = 0;
        end_gap = 0;
        buffer_size = 0;
    }
};

enum Text_Input_Target : u8 {
    NO_TARGET,
};

constexpr melv::Color TextCursorColor (0x33, 0x56, 0x74, 0xDD);

struct TextSelection
{
    int start;
    int end;
};

struct CursorScreenPosition {
    int line;
    int pixel_x;
    int pixel_y;
};

struct Text_Field
{
    UiElementId id = {};
    UiElementInfo info = {};
    bool editable = false;

    melv::Rectangle m_area = {};
    melv::Color background = {};
    melv::Color text_color = {};

    GapBuffer m_buffer = {};
    String_Builder m_text = {};
    int text_gap = 0;
    AssetId fontId = {};
    int m_cursor_pixel_x = 0;
    int m_cursor_pixel_y = 0;
    int m_cursor_line = 0;
    int m_line_count = 0;

    // bytes
    int m_cursor = 0;
    int m_selection_point = 0;

    float mouse_x;
    float mouse_y;

    float m_font_size = 0.0;
    Texture m_texture = {};  // cached texture the text is rendered on, updated every text input event

    Text_Field() {}

    // height -> empty height
    Text_Field(AssetId font, float height, melv::Color background_color, melv::Color textColor, bool visible = true, bool is_editable = true, bool active = true)
    {
        m_font_size = height;
        fontId = font;
        background = background_color;
        text_color = textColor;
		info.visible = visible;
        info.active = active;
        editable = is_editable;
    }

    Text_Field(melv::Rectangle area, AssetId font, melv::Color background_color, melv::Color textColor, bool visible = true, bool is_editable = true, bool active = true)
    {
        fontId = font;
        background = background_color;
        text_color = textColor;
        m_area = area;
		info.visible = visible;
        info.active = active;
        editable = is_editable;
    }

    Text_Field(melv::Rectangle area, AssetId font, melv::Color background_color, melv::Color textColor, UiElementId ident, bool visible = true, bool is_editable = true, bool active = true)
        : id(ident)
    {
        fontId = font;
        background = background_color;
        text_color = textColor;
        m_area = area;
		info.visible = visible;
        info.active = active;
        editable = is_editable;
    }

    Text_Field(Text_Field&& other) = default;
    Text_Field& operator=(Text_Field&& other) = default;

    String get_string()
    {
        if (text_gap != m_buffer.gap_index || m_text.cursor != m_buffer.length)
        {
            m_buffer.get_string(m_text);
        }
        return m_text.to_string();
    }

    TextSelection get_selection()
    {
        if (m_cursor > m_selection_point)
        {
            return {m_selection_point, m_cursor};
        }
        else
        {
            return {m_cursor, m_selection_point};
        }
    }

    bool set_and_render_text(RenderContext& render, Font font, String s, bool wrapped)
    {
        set_string(s, render);
        return update_text(render, font, wrapped);
    }

    void set_string(String s, RenderContext& render)
    {
        clear(render);
        m_buffer.append(s, 0);
        text_gap = m_buffer.gap_index;
    }

    void append_string(String s)
    {
        if (m_cursor != m_selection_point)
        {
            TextSelection selection = get_selection();
            m_buffer.remove(selection.start, selection.end - selection.start);
            m_buffer.append(s, m_cursor);
            text_gap = m_buffer.gap_index;
        }
        else
        {
            m_buffer.append(s, m_cursor);
            text_gap = m_buffer.gap_index;
            m_cursor += s.size;
        }

        m_selection_point = m_cursor;
    }

    bool update_text(RenderContext& render, Font font, bool wrapped)
    {
        return render_text_field_texture(render, font, text_color, wrapped);
    }

    void clear(RenderContext& render) {
        m_buffer.remove(0, m_buffer.length);
        render.destroy_texture(m_texture);
        m_texture = {};
        m_cursor_pixel_x = 0;
        m_cursor_pixel_y = 0;
        m_cursor_line = 0;
        m_line_count = 0;
        m_cursor = 0;
        m_selection_point = 0;
        m_font_size = 0;
        text_gap = 0;
    }

    void reset(RenderContext& render)
    {
        clear(render);
        m_buffer.reset();
        m_text.free_buffer();
        text_gap = 0;
    }

    void delete_text()
    {
        TextSelection s = get_selection();
        int amount = s.end - s.start;
        m_buffer.remove(s.start, amount);
        text_gap = m_buffer.gap_index;

        m_selection_point = m_cursor;
    }

    void delete_at_cursor()
    {
        if (m_cursor != m_selection_point)
            return;

        ASSERT(m_cursor >= 0);

        if (m_cursor == 0)
            return;

        String current = this->get_string();
        int codepointSize = utf8_previous(current, m_cursor);

        m_cursor -= codepointSize;
        m_buffer.remove(m_cursor, codepointSize);
        text_gap = m_buffer.gap_index;
        m_selection_point = m_cursor;
    }

    void delete_after_cursor()
    {
        if (m_cursor != m_selection_point)
            return;
        if (m_cursor == m_buffer.length)
            return;

        String current = this->get_string();
        int codepointSize = utf8_next(current, m_cursor);

        m_buffer.remove(m_cursor, codepointSize);
        text_gap = m_buffer.gap_index;
        m_selection_point = m_cursor;
    }

    void delete_at_character(int character)
    {
        if (character >= m_buffer.length - 1)
            return;

        String current = this->get_string();
        int codepointSize = utf8_next(current, m_cursor);

        m_cursor = character;
        m_selection_point = character + codepointSize;
        delete_text();
        text_gap = m_buffer.gap_index;
    }

    void insert_tab(int tab_width);
    void insert_line();

    void set_text_input_area(SDL_Window* window, int line_skip)
    {
        const SDL_Rect area = { int(m_area.x), int(m_area.y) + m_cursor_line * line_skip, int(m_area.w), line_skip};
        SDL_SetTextInputArea(window, &area, m_cursor_pixel_x);
    }

    CursorScreenPosition get_cursor_from_selection(int cursor, String string, Font font, bool wrapped);
    size_t get_cursor_from_mouse(melv::vec2 mouse_position, String string, Font font, bool wrapped);

    void calculate_cursor_from_selection(String string, Font font, bool wrapped);
    size_t calculate_cursor_from_mouse(melv::vec2 mouse_position, String string, Font font, bool wrapped);

    bool render_text_field_texture(RenderContext& render, Font font, melv::Color color, bool wrapped);
};

struct TextEditor {
    Text_Field field = {};
    MutableString name = {};
    Texture title_texture = {};  // rendered name or something else
    float title_height = 0;
    melv::Color title_color = melv::Color();  // color of the title text
    melv::Color title_bar_color = melv::Color();

    Icon icon1 = {};
    Icon icon2 = {};
    Icon icon3 = {};
    int clicked_icon = 0;

    DragInfo drag = {};
    ResizeInfo resize = {};
    UiUserData user = {};

    TextEditor() {}
    TextEditor(melv::Rectangle area, AssetId font, melv::Color background_color, melv::Color textColor, melv::Color titleColor, melv::Color titleBarColor, String editor_name, float title_height)
        :
        field(area, font, background_color, textColor),
        name(editor_name),
        title_height(title_height),
        title_color(titleColor),
        title_bar_color(titleBarColor)
    {}
    TextEditor(UiElementId ident, melv::Rectangle area, AssetId font, melv::Color background_color, melv::Color textColor, melv::Color titleColor, melv::Color titleBarColor, String editor_name, float title_height)
        :
        field(area, font, background_color, textColor, ident),
        name(editor_name),
        title_height(title_height),
        title_color(titleColor),
        title_bar_color(titleBarColor)
    {}

    void rescale(melv::vec2 scale, RenderContext& render, const AssetCatalog& catalog);

    melv::Rectangle get_title_area() const {
        return melv::Rectangle(field.m_area.x, field.m_area.y - (field.m_area.h + title_height) / 2, field.m_area.w, title_height);
    }

    melv::Rectangle get_text_area() const {
        return field.m_area;
    }

    melv::Rectangle get_icon1_area() const {
        float iconScale = title_height;
        return melv::Rectangle(get_title_area().get_position() + melv::vec2(get_title_area().w / 2, 0) - melv::vec2(iconScale, 0) * 1, melv::vec2(iconScale));
    }
    melv::Rectangle get_icon2_area() const {
        float iconScale = title_height;
        return melv::Rectangle(get_title_area().get_position() + melv::vec2(get_title_area().w / 2, 0) - melv::vec2(iconScale, 0) * 3, melv::vec2(iconScale));
    }
    melv::Rectangle get_icon3_area() const {
        float iconScale = title_height;
        return melv::Rectangle(get_title_area().get_position() + melv::vec2(get_title_area().w / 2, 0) - melv::vec2(iconScale, 0) * 5, melv::vec2(iconScale));
    }

    void set_position(melv::vec2 pos) {
        field.m_area.x = pos.x;
        field.m_area.y = pos.y;
    }
};

// owns the text object inside it
struct Entry {
    Text label = {};
    union {
        void* data;
        int index;
        float number;
    };

    Entry() : label(), data(nullptr) {}
    Entry(Text text, void* p_data) : label(text), data(p_data) {}
    Entry(Text text, int p_index) : label(text), index(p_index) {}
};

#define DROP_DOWN_LIST_SELECTED_SENTINEL -1

struct Drop_Down_List {

    UiElementId id = {};

    melv::vec2 pos = {};
    melv::vec2 scale = {};
    int selected = DROP_DOWN_LIST_SELECTED_SENTINEL;
    Text title = {};
    DArray<Entry> options = {};
    melv::Color title_color = {};
    melv::Color option_color = {};
	melv::Color text_color = {};
    bool open = false;

    void toggle() {
        open = !open;
    }

    void set_area(melv::vec2 p_pos, melv::vec2 p_scale) {
        pos = p_pos; scale = p_scale;
    }

    void set_title(Text text) {
        title = text;
    }

    void add_option(Text text, void* data) {
        options.add(Entry(text, data));
    }

    void add_option(Text text, int index) {
        options.add(Entry(text, index));
    }

    Text get_option_text(int index) const {
        return options.get(index).label;
    }

    String get_option_name(int index) const {
        return options.get(index).label.string;
    }

    String get_selected_option_name() const {
        if (selected == DROP_DOWN_LIST_SELECTED_SENTINEL)
        {
            return String();
        }

        return get_option_name(selected);
    }

    void* get_option_data(int index) const {
        return options.get(index).data;
    }

    int get_option_data_index(int index) const {
        return options.get(index).index;
    }

    melv::Rectangle get_area() const
    {
        if (open) {
            int count = options.size();
            return melv::Rectangle(pos.x, pos.y + (float(count) / 2) * scale.y, scale.x, scale.y * count);
        }
        else {
            return melv::Rectangle(pos, scale);
        }
    }

    melv::Rectangle get_option_area(int i) const {
        return melv::Rectangle(pos.x, pos.y + scale.y * (i+1), scale.x, scale.y);
    }

    void remove_option(int index, RenderContext& render) {
        if (index == selected)
        {
            selected = DROP_DOWN_LIST_SELECTED_SENTINEL;
        }
        options.get_ref(index).label.clear();
        render.destroy_texture(options.get_ref(index).label.texture);
        options.remove_shift(index);
    }

    void clear()
    {
        selected = DROP_DOWN_LIST_SELECTED_SENTINEL;
        title.clear();
        options.reset();
        open = false;
    }

    Drop_Down_List() {}
    Drop_Down_List(melv::vec2 p_pos, melv::vec2 p_scale) : pos(p_pos), scale(p_scale) {}

    void reset() {
        title.clear();
        for (auto& entry : options)
        {
            entry.label.clear();
        }
        options.reset();
    }
};

struct PanelTab {
    Icon tabIcon = {};
    DArray<IconButton> icons = {};
    melv::Color color = {};

    PanelTab() {}
    PanelTab(Icon tab, DArray<IconButton> icons, melv::Color color) : tabIcon(tab), icons(icons), color(color) {}
};

struct Panel {
    UiElementId id = {};
    DragInfo drag = {};
    ResizeInfo resize = {};
    melv::Rectangle area = {};
    float title_height = 0;
    melv::Color title_bar_color = melv::Color();
    int activeTab = 0;
    float tabHeaderSize = 0;
    float iconSize = 0;
    float iconMargin = 0;
    DArray<PanelTab> tabs = {};

    Panel() {}
    Panel(UiElementId id, melv::Rectangle area, float headerSize, float icoSize, float margin) : id(id), area(area), tabHeaderSize(headerSize), iconSize(icoSize), iconMargin(margin) {}

    melv::Rectangle get_title_area() const;

    melv::Rectangle get_icon_area(int index) const;
    melv::Rectangle get_tab_header_area(int index) const;
};

enum UiValueType {
    // @todo rename to UiValue*
    ValueInteger,   // editable integer
    ValueNumber,    // editable number
    ValueString,    // editable string
    ValueLabel,     // a string value to be shown
    ValueButton,    // a single button
    ValueSelection, // a button group
};

struct ValueField {
    Text name = {};
    int ui_element = 0;
    int identifier = 0;  // user data
    UiValueType type = {};
    union {
        String string;
        u64 integer;
        double number;
        int selection = 0;
    } value = {};

    ValueField() : value{} {}
    ValueField(Text text, int ui, int ident, UiValueType type) : name(text), ui_element(ui), identifier(ident), type(type), value{} {}
};

struct ValuePanelTab {
    Icon tabIcon = {};
    melv::Color color = {};
    float field_height = 0;
    float field_margin = 0;
    DArray<ValueField> fields = {};
};

struct ValuePanel {
    UiElementId id = {};
    melv::Rectangle area = {};
    bool showTabs = false;
    int activeTab = 0;
    float fieldSize = 0;
    float tabHeaderSize = 0;
    melv::Direction direction = {};
    DArray<ValuePanelTab> tabs = {};

    ValuePanel() {}
    ValuePanel(UiElementId ident, melv::Rectangle area, float field_size, float tab_header_size, melv::Direction dir, bool show_tabs = true)
        :
        id(ident),
        area(area),
        showTabs(show_tabs),
        fieldSize(field_size),
        tabHeaderSize(tab_header_size),
        direction(dir)
    {}

    ValuePanelTab& get_active_tab() const;
    void switch_tabs(UiState& ui, int tabIndex);

    melv::Rectangle get_tab_header_area(int index) const;
    float get_field_width() const { return area.w * 0.95; }
    melv::Rectangle get_field_area(int tab, int field, const UiState* ui) const;
    melv::Rectangle get_field_title_area(RenderContext& render, int tab, int field) const;
};

struct DiscreteSlider {
    UiElementId id = {};

    melv::vec2 position = melv::vec2();
    melv::vec2 element_scale = {};
    int element_count = 0;
    int selected = 0;
    float element_gap = 0;
    bool vertical = false;
    Texture texture = {};
    melv::Colorf outlineColor = {};
    melv::Colorf buttonColor = {};
    melv::Colorf inactiveColor = {};
    melv::Colorf startColor = {};
    melv::Colorf endColor = {};

    DiscreteSlider() {}
    DiscreteSlider(UiElementId ident, melv::vec2 pos, melv::vec2 elem_scale, int elem_count, float elem_gap, bool vert, melv::Colorf outline_color, melv::Colorf button_color, melv::Colorf inactive_color, melv::Colorf start_color, melv::Colorf end_color)
        :
        id(ident),
        position(pos),
        element_scale(elem_scale),
        element_count(elem_count),
        element_gap(elem_gap),
        vertical(vert),
        outlineColor(outline_color),
        buttonColor(button_color),
        inactiveColor(inactive_color),
        startColor(start_color),
        endColor(end_color)
    {}

    melv::Rectangle get_bounds() const;
    melv::vec2 get_start() const;
    melv::vec2 get_step() const;
    melv::vec2 get_button_scale() const;
};

struct TextBox {
    Text text = {};
    melv::Colorf background = {};
};

#define TEXT_INPUT_TARGET_IS_VALID     BIT(0)
#define TEXT_INPUT_TARGET_IS_EDITOR    BIT(1)

struct TextInputTarget {
    u16 index = 0;
    u16 flags = 0;
};

struct UiState {
    DArray<TextEditor> editor = {};
    DArray<Text_Field> text_field = {};
    DArray<Drop_Down_List> drop_down = {};
    DArray<TextButton> button = {};
    DArray<ImageButton> image_button = {};
    DArray<Label> label = {};
    DArray<Panel> panel = {};
    DArray<ValuePanel> value_panel = {};
    DArray<DiscreteSlider> discrete_slider = {};
    DArray<ButtonGroup> button_group = {};

    TextBox hoverText = {};

    TextInputTarget text_input_target = {};
    melv::vec2 assumed_window_size = {};

    void reinit_text(RenderContext& render, Font font);

    void update_state(melv::vec2 window_size, RenderContext& render, const AssetCatalog& catalog);

    Text_Field* get_selected_text_field();

    bool doing_resize() const;
    DragInfo* get_drag_info();

    TextEditor* get_editor(UiElementId id);
    Text_Field* get_text_field(UiElementId id);
    Drop_Down_List* get_drop_down(UiElementId id);
    TextButton* get_button(UiElementId id);
    ImageButton* get_image_button(UiElementId id);
    Label* get_label(UiElementId id);
    ValuePanel* get_value_panel(UiElementId id);
    Panel* get_panel(UiElementId id);
    DiscreteSlider* get_discrete_slider(UiElementId id);

    ~UiState();
};

} // namespace

#endif // UI_HPP
