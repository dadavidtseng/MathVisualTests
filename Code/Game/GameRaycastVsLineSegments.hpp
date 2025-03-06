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
    Vec2 GetMouseWorldPos() const;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;

    void RenderShapes() const;
    void RenderLineSegment2D() const;

    void GenerateRandom();
    Vec2 GenerateRandomPointInScreen() const;
    void GenerateRandomLineSegmentInScreen();
    void RenderRaycastResult() const;


    LineSegment2 m_lineSegment[8] = {};
    LineSegment2 m_referenceLine;
};
