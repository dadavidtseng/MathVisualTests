//----------------------------------------------------------------------------------------------------
// GameRaycastVsLineSegments.hpp
//----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/LineSegment2.hpp"
#include "Game/Game.hpp"

//-----------------------------------------------------------------------------------------------
class GameRaycastVsLineSegments : public Game
{
public:
    GameRaycastVsLineSegments();
    ~GameRaycastVsLineSegments() override;

    void Update() override;
    void Render() const override;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;

    void RenderRaycastResult() const;
    void RenderLineSegments2D() const;

    Vec2 GenerateRandomPointInScreen() const;
    void GenerateRandomLineSegmentInScreen();
    void GenerateRandomLineSegment2D();

    LineSegment2 m_lineSegments[8] = {};
    LineSegment2 m_lineSegment;
};
