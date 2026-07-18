#ifndef GAME_HPP
#define GAME_HPP

#include "app/text.hpp"
#include "app/time.hpp"
#include "app/input.hpp"
#include "app/ui.hpp"

#include "util/common.hpp"
#include "util/template.hpp"
#include "util/math_util.hpp"

struct GameState;

typedef void (*KeyboardCallback)(GameState* game, KeyboardState* keyboard);

typedef void (*UpdateFunction)(GameState* game, TimeInfo time);
typedef void (*FixedUpdateFunction)(GameState* game);

struct UpdateState {
    UpdateFunction update = nullptr;
    FixedUpdateFunction fixedUpdate = nullptr;
    s64 ticks = 0;
    double elapsed = 0;
    double timeScale = 0;
	int updateRate = 0;

    double calculateTimeStep() { return 1.0 / updateRate; }
};

struct GameState {
    // game state

    UpdateState* updateState = nullptr;
    KeyboardCallback keyboard = nullptr;

    String_Builder builder;

    void update(TimeInfo time);

    void set_default_game_data();
};

void idleUpdate(GameState* game, TimeInfo time);
void idleFixedUpdate(GameState* game);

void keyboardIdle(GameState* game, KeyboardState* keyboard);

void draw_game_state(const RenderContext& context, const AssetCatalog& catalog);

bool initialize_ui(cobot::vec2 windowSize, RenderContext& render, AssetCatalog& catalog, UiState& ui);

#endif // GAME_HPP
