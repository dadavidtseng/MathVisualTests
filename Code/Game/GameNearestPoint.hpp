//----------------------------------------------------------------------------------------------------
// GameNearestPoint.hpp
//----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Game/Game.hpp"

//-----------------------------------------------------------------------------------------------
class GameNearestPoint : public Game
{
public:
    GameNearestPoint();

    void Update() override;
    void Render() const override;

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;

    void RenderShapes() const;
    void GenerateRandomShapes();

    void GenerateRandomDisc2D();
    void GenerateRandomLineSegment2D();
    void GenerateRandomInfiniteLine2D();
    void GenerateRandomTriangle2D();
    void GenerateRandomAABB2D();
    void GenerateRandomOBB2D();
    void GenerateRandomCapsule2D();

    void RenderDisc2D() const;
    void RenderLineSegment2D() const;
    void RenderLineInfinite2D() const;
    void RenderTriangle2D() const;
    void RenderAABB2D() const;
    void RenderOBB2D() const;
    void RenderCapsule2D() const;

    void RenderReferencePoint() const;

    Vec2         m_referencePoint;
    Disc2        m_randomDisc;
    LineSegment2 m_randomLineSegment;
    LineSegment2 m_randomInfiniteLine;
    Triangle2    m_randomTriangle;
    AABB2        m_randomAABB2;
    OBB2         m_randomOBB2;
    Capsule2     m_randomCapsule2;
};
