//----------------------------------------------------------------------------------------------------
// GameRaycastVsAABBs.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Game/Game.hpp"



//-----------------------------------------------------------------------------------------------
class GameRaycastVsAABBs : public Game
{
public:
    GameRaycastVsAABBs();
    ~GameRaycastVsAABBs() override;

    void Update() override;
    void Render() const override;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;

    void RenderShapes() const;
    void GenerateRandomShapes();

    Vec2 GenerateRandomPointInScreen() const;
    void GenerateRandomDisc2();
    void GenerateRandomLineSegment2();
    void GenerateRandomInfiniteLine2();
    void GenerateRandomTriangle2D();
    void GenerateRandomAABB2();
    void GenerateRandomOBB2();
    void GenerateRandomCapsule2D();
    Vec2 ClampPointToScreen(const Vec2& point, float radius) const;
    Vec2 ClampPointToScreen(const Vec2& point, float halfWidth, float halfHeight) const;
    void RenderDisc2() const;
    void RenderLineSegment2() const;
    void RenderLineInfinite2D() const;
    void RenderTriangle2D() const;
    void RenderAABB2() const;
    void RenderOBB2() const;
    void RenderCapsule2D() const;

    void RenderReferencePoint() const;

    Vec2         m_referencePoint;
};
