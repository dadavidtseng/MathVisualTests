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
class GameRaycastVsDiscs final : public Game
{
public:
    GameRaycastVsDiscs();

    void Update() override;
    void Render() const override;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;

    void GenerateRandomLineSegmentInScreen();
    void GenerateRandomDiscs2D();

    void RenderDisc2() const;
    void RenderRaycastResult() const;

    bool IsTailPosInsideDisc(Vec2 const& startPosition) const;

    Disc2        m_randomDisc[8] = {};
    LineSegment2 m_lineSegment;
};
