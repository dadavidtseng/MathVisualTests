//----------------------------------------------------------------------------------------------------
// GameRaycastVsAABBs.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Game/Game.hpp"

//-----------------------------------------------------------------------------------------------
class GameRaycastVsAABBs final : public Game
{
public:
    GameRaycastVsAABBs();
    ~GameRaycastVsAABBs() override;

    void Update() override;
    void Render() const override;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;

    void RenderAABB2s2D() const;
    void RenderRaycastResult()const ;

    void GenerateRandomLineSegmentInScreen();
    void GenerateRandomAABB2s2D();

    AABB2        m_AABB2[8] = {};
    LineSegment2 m_lineSegment;
};
