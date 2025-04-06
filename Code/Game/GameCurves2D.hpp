//----------------------------------------------------------------------------------------------------
// GameCurves2D.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Game/Game.hpp"

struct Rgba8;



//----------------------------------------------------------------------------------------------------
class GameCurves2D final : public Game
{
public:
    GameCurves2D();

    void Update() override;
    void Render() const override;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;


    Camera*     m_worldCamera    = nullptr;
};
