#include "app/draw.hpp"
#include "game.hpp"
#include "util/log.hpp"

void GameState::update(TimeInfo time)
{
    constexpr int maxIterationsPerFrame = 50;
    int iterations = 0;
	double timeStep = updateState->calculateTimeStep();
    while ((updateState->elapsed < time.timeSeconds + time.deltaTimeSeconds) && iterations < maxIterationsPerFrame)
    {
        updateState->fixedUpdate(this);
        updateState->elapsed += timeStep;
        updateState->ticks += 1;

        iterations += 1;
    }

    updateState->update(this, time);
}

void GameState::set_default_game_data()
{

}

void idleUpdate(GameState* game, TimeInfo time) {}
void idleFixedUpdate(GameState* game) {}

void keyboardIdle(GameState* game, KeyboardState* keyboard) {}

bool initialize_ui(cobot::vec2 windowSize, RenderContext& render, AssetCatalog& catalog, UiState& ui)
{
    return true;
}
