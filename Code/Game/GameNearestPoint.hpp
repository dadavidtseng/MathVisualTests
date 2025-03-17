//----------------------------------------------------------------------------------------------------
// GameNearestPoint.hpp
//----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
#pragma once
#include <cstdint>

#include "Engine/Math/Vec2.hpp"
#include "Game/Game.hpp"

//----------------------------------------------------------------------------------------------------
enum class eTestShape2DType : int8_t
{
    DISC2,
    LINESEGMENT2,
    INFINITE_LINESEGMENT2,
    TRIANGLE2,
    AABB2,
    OBB2,
    CAPSULE2,
    COUNT
};

//----------------------------------------------------------------------------------------------------
struct TestShape2D
{
    eTestShape2DType m_type;
    Vec2             m_startPosition  = Vec2::ZERO;
    Vec2             m_endPosition    = Vec2::ZERO;
    Vec2             m_thirdPosition  = Vec2::ZERO;
    Vec2             m_iBasisNormal   = Vec2::ZERO;
    Vec2             m_halfDimensions = Vec2::ZERO;
    float            m_radius         = 0.f;
    float            m_thickness      = 0.f;
    bool             m_isInfinite     = false;
};

//-----------------------------------------------------------------------------------------------
class GameNearestPoint final : public Game
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

    void RenderNearestPoints() const;
    void RenderReferencePoint() const;

    void GenerateRandomDisc2D();
    void GenerateRandomLineSegment2D();
    void GenerateRandomInfiniteLine2D();
    void GenerateRandomTriangle2D();
    void GenerateRandomAABB2D();
    void GenerateRandomOBB2D();
    void GenerateRandomCapsule2D();

    Vec2        m_referencePoint = Vec2::ZERO;
    TestShape2D m_testShapes[8]  = {};
};
