//----------------------------------------------------------------------------------------------------
// Game.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vec2.hpp"

//-Forward-Declaration--------------------------------------------------------------------------------
class Camera;
class Clock;

//----------------------------------------------------------------------------------------------------
enum class eGameMode : int8_t
{
    NEAREST_POINT,
    RAYCAST_VS_DISCS,
    RAYCAST_VS_LINESEGMENTS,
    RAYCAST_VS_AABBS,
    SHAPES_3D,
    CURVES_2D,
    PACHINKO_2D,
    COUNT
};

//----------------------------------------------------------------------------------------------------
class Game
{
public:
    virtual ~Game();

    virtual void Update() = 0;
    virtual void Render() const = 0;

protected:
    Vec2 GetMouseWorldPos() const;
    void RenderCurrentModeText(char const* currentModeText) const;
    void RenderControlText() const;
    Vec2 GenerateRandomPointInScreen() const;
    Vec2 ClampPointToScreen(Vec2 const& point, float radius) const;
    Vec2 ClampPointToScreen(Vec2 const& point, float halfWidth, float halfHeight) const;

    Clock*  m_gameClock     = nullptr;
    Camera* m_screenCamera  = nullptr;
    Camera* m_worldCamera   = nullptr;
    Vec2    m_baseCameraPos = Vec2::ZERO;
    float   m_moveSpeed     = 500.f;
    AABB2   m_space         = AABB2::ZERO_TO_ONE;

private:
    virtual void UpdateFromKeyboard(float deltaSeconds) = 0;
    virtual void UpdateFromController(float deltaSeconds) = 0;
};
