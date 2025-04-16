//----------------------------------------------------------------------------------------------------
// GamePachinkoMachine2D.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Game/Game.hpp"

//----------------------------------------------------------------------------------------------------
class GamePachinkoMachine2D final : public Game
{
public:
    GamePachinkoMachine2D();

    void Update() override;
    void Render() const override;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;

    Camera* m_worldCamera = nullptr;
};
