//----------------------------------------------------------------------------------------------------
// GameCurves2D.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Texture.hpp"
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
