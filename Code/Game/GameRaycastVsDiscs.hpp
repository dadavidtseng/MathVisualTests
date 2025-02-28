//----------------------------------------------------------------------------------------------------
// GameRaycastVsDiscs.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Game/Game.hpp"

//----------------------------------------------------------------------------------------------------
class GameRaycastVsDiscs : public Game
{
public:
    GameRaycastVsDiscs();
    ~GameRaycastVsDiscs() override;

    void Update() override;
    void Render() const override;
    Vec2 GetMouseWorldPos() const;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;
    Vec2 GenerateRandomPointInScreen() const;
    void GenerateRandomLineSegmentInScreen();
    void GenerateRandomDiscs();
    void RenderDisc2() const;
    void RenderRaycastResult() const;
    Vec2 ClampPointToScreen(Vec2 const& point, float radius) const;
    bool IsTailPosInsideDisc(Vec2 const& startPos) const;

    Disc2        m_randomDisc[8] = {};
    LineSegment2 m_lineSegment;
};
