//----------------------------------------------------------------------------------------------------
// Game.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class Camera;
class Clock;

//----------------------------------------------------------------------------------------------------
enum class eGameMode
{
    NEAREST_POINT,
    RAYCAST_VS_DISCS,
    RAYCAST_VS_LINESEGMENTS,
    RAYCAST_VS_AABBS
};

//----------------------------------------------------------------------------------------------------
class Game
{
public:
    virtual ~Game() = default;

    virtual void Update() = 0;
    virtual void Render() const = 0;

protected:
    Clock*  m_gameClock     = nullptr;
    Camera* m_screenCamera  = nullptr;
    Vec2    m_baseCameraPos = Vec2::ZERO;
    float   m_moveSpeed     = 500.f;

private:
    virtual void UpdateFromKeyboard(float deltaSeconds) = 0;
    virtual void UpdateFromController(float deltaSeconds) = 0;
};
