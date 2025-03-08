//----------------------------------------------------------------------------------------------------
// GameNearestPoint.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameNearestPoint.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/SimpleTriangleFont.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
GameNearestPoint::GameNearestPoint()
{
    m_screenCamera = new Camera();
    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

    m_referencePoint = Vec2(SCREEN_SIZE_X / 2.f, SCREEN_SIZE_Y / 2.f);

    m_gameClock = new Clock(Clock::GetSystemClock());

    GenerateRandomShapes();
}

//----------------------------------------------------------------------------------------------------
GameNearestPoint::~GameNearestPoint()
{
    SafeDelete(m_screenCamera);
    SafeDelete(m_gameClock);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::Update()
{
    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    UpdateFromKeyboard(deltaSeconds);
    UpdateFromController(deltaSeconds);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::Render() const
{
    g_theRenderer->BeginCamera(*m_screenCamera);

    g_theRenderer->SetBlendMode(BlendMode::ALPHA);
    g_theRenderer->SetDepthMode(DepthMode::DISABLED);
    g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
    g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
    RenderShapes();

    std::vector<Vertex_PCU> titleVerts;
    g_theBitmapFont->AddVertsForTextInBox2D(titleVerts,
                                            "CURRENT MODE: NearestPoint",
                                            AABB2(Vec2(0.f, 750.f), Vec2(1600.f, 800.f)),
                                            10.f);

    g_theRenderer->BindTexture(&g_theBitmapFont->GetTexture());
    g_theRenderer->DrawVertexArray(static_cast<int>(titleVerts.size()), titleVerts.data());

    g_theRenderer->EndCamera(*m_screenCamera);
}

//------------------------------------------------------------------------------------------------
Vec2 GameNearestPoint::GetMouseWorldPos() const
{
    Vec2 const  mouseUV    = g_theWindow->GetNormalizedMouseUV();
    Vec2 const  bottomLeft = m_screenCamera->GetOrthographicBottomLeft();
    Vec2 const  topRight   = m_screenCamera->GetOrthographicTopRight();
    AABB2 const orthoBounds(bottomLeft, topRight);
    return orthoBounds.GetPointAtUV(mouseUV);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::UpdateFromKeyboard(float const deltaSeconds)
{
    if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
    {
        GenerateRandomShapes();
        m_referencePoint = Vec2(SCREEN_SIZE_X / 2.f, SCREEN_SIZE_Y / 2.f);
    }

    if (g_theInput->IsKeyDown(KEYCODE_W)) m_referencePoint.y += m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_S)) m_referencePoint.y -= m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_A)) m_referencePoint.x -= m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_D)) m_referencePoint.x += m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE)) m_referencePoint = GetMouseWorldPos();
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::UpdateFromController(float const deltaSeconds)
{
    UNUSED(deltaSeconds)
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderShapes() const
{
    RenderDisc2();
    RenderLineSegment2();
    RenderLineInfinite2D();
    RenderTriangle2D();
    RenderAABB2();
    RenderOBB2();
    RenderCapsule2D();
    RenderReferencePoint();
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomShapes()
{
    GenerateRandomDisc2();
    GenerateRandomLineSegment2();
    GenerateRandomInfiniteLine2();
    GenerateRandomTriangle2D();
    GenerateRandomAABB2();
    GenerateRandomOBB2();
    GenerateRandomCapsule2D();
}

//----------------------------------------------------------------------------------------------------
Vec2 GameNearestPoint::GenerateRandomPointInScreen() const
{
    const float randomX = g_theRNG->RollRandomFloatInRange(0, SCREEN_SIZE_X);
    const float randomY = g_theRNG->RollRandomFloatInRange(0, SCREEN_SIZE_Y);

    return Vec2(randomX, randomY);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomDisc2()
{
    const float randomRadius = g_theRNG->RollRandomFloatInRange(10.f, 200.f);
    Vec2        center       = GenerateRandomPointInScreen();
    center                   = ClampPointToScreen(center, randomRadius);

    m_randomDisc = Disc2(center, randomRadius);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomLineSegment2()
{
    Vec2        start           = GenerateRandomPointInScreen();
    Vec2        end             = GenerateRandomPointInScreen();
    const float randomThickness = g_theRNG->RollRandomFloatInRange(1.f, 5.f);

    // Ensure the line is within screen bounds
    start = ClampPointToScreen(start, randomThickness);
    end   = ClampPointToScreen(end, randomThickness);

    m_randomLineSegment = LineSegment2(start, end, randomThickness, false);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomInfiniteLine2()
{
    Vec2        start        = GenerateRandomPointInScreen();
    Vec2        end          = GenerateRandomPointInScreen();
    const float randomRadius = g_theRNG->RollRandomFloatInRange(1.f, 5.f);

    // Ensure the line is within screen bounds
    start = ClampPointToScreen(start, randomRadius);
    end   = ClampPointToScreen(end, randomRadius);

    m_randomInfiniteLine = LineSegment2(start, end, randomRadius, true);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomTriangle2D()
{
    Vec2 const randomA = GenerateRandomPointInScreen();
    Vec2 const randomB = GenerateRandomPointInScreen();
    Vec2 const randomC = GenerateRandomPointInScreen();

    m_randomTriangle = Triangle2(randomA, randomB, randomC);

    // Ensure the triangle is valid
    Vec2 const edgeAB = randomB - randomA;
    Vec2 const edgeAC = randomC - randomA;

    // Points A, B, and C are collinear.
    if (CrossProduct2D(edgeAB, edgeAC) == 0.f)
    {
        GenerateRandomTriangle2D();
        return;
    }

    // Ensure the triangle has counter-clockwise (CCW) winding order
    if (CrossProduct2D(edgeAB, edgeAC) < 0.f)
    {
        m_randomTriangle = Triangle2(randomA, randomC, randomB);
    }
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomAABB2()
{
    const float randomWidth  = g_theRNG->RollRandomFloatInRange(0.1f, SCREEN_SIZE_X / 2);
    const float randomHeight = g_theRNG->RollRandomFloatInRange(0.1f, SCREEN_SIZE_Y / 2);

    Vec2 center     = GenerateRandomPointInScreen();
    Vec2 randomMins = ClampPointToScreen(center - Vec2(randomWidth / 2, randomHeight / 2), randomWidth / 2,
                                         randomHeight / 2);
    Vec2 randomMaxs = ClampPointToScreen(center + Vec2(randomWidth / 2, randomHeight / 2), randomWidth / 2,
                                         randomHeight / 2);

    m_randomAABB2 = AABB2(randomMins, randomMaxs);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomOBB2()
{
    const Vec2 center         = GenerateRandomPointInScreen();
    const Vec2 halfDimensions = Vec2(g_theRNG->RollRandomFloatInRange(10.f, SCREEN_SIZE_X / 4),
                                     g_theRNG->RollRandomFloatInRange(10.f, SCREEN_SIZE_Y / 4));
    m_randomOBB2 = OBB2(center, Vec2(1.f, 0.f), halfDimensions); // Angle is set to 0 for simplicity
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomCapsule2D()
{
    Vec2        start        = GenerateRandomPointInScreen();
    Vec2        end          = GenerateRandomPointInScreen();
    const float randomRadius = g_theRNG->RollRandomFloatInRange(10.f, 200.f);

    start = ClampPointToScreen(start, randomRadius);
    end   = ClampPointToScreen(end, randomRadius);

    m_randomCapsule2 = Capsule2(start, end, randomRadius);
}

//----------------------------------------------------------------------------------------------------
Vec2 GameNearestPoint::ClampPointToScreen(const Vec2& point, float radius) const
{
    Vec2 clampedPoint = point;
    clampedPoint.x    = GetClamped(clampedPoint.x, radius, SCREEN_SIZE_X - radius);
    clampedPoint.y    = GetClamped(clampedPoint.y, radius, SCREEN_SIZE_Y - radius);
    return clampedPoint;
}

//----------------------------------------------------------------------------------------------------
Vec2 GameNearestPoint::ClampPointToScreen(const Vec2& point, float halfWidth, float halfHeight) const
{
    Vec2 clampedPoint = point;
    clampedPoint.x    = GetClamped(clampedPoint.x, halfWidth, SCREEN_SIZE_X - halfWidth);
    clampedPoint.y    = GetClamped(clampedPoint.y, halfHeight, SCREEN_SIZE_Y - halfHeight);
    return clampedPoint;
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderDisc2() const
{
    const Vec2 nearestPoint = m_randomDisc.GetNearestPoint(m_referencePoint);
    DrawDisc2D(m_randomDisc, m_randomDisc.IsPointInside(m_referencePoint) ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.0f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderLineSegment2() const
{
    const Vec2 nearestPoint = m_randomLineSegment.GetNearestPoint(m_referencePoint);
    DrawLineSegment2D(m_randomLineSegment, m_randomLineSegment.m_thickness, m_randomLineSegment.m_isInfinite, Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.0f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderLineInfinite2D() const
{
    const Vec2 nearestPoint = m_randomInfiniteLine.GetNearestPoint(m_referencePoint);
    DrawLineSegment2D(m_randomInfiniteLine, m_randomInfiniteLine.m_thickness, m_randomInfiniteLine.m_isInfinite, Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.0f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderTriangle2D() const
{
    Vec2 const nearestPoint = m_randomTriangle.GetNearestPoint(m_referencePoint);
    DrawTriangle2D(m_randomTriangle, m_randomTriangle.IsPointInside(m_referencePoint) ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.0f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderAABB2() const
{
    Vec2 const nearestPoint = m_randomAABB2.GetNearestPoint(m_referencePoint);
    DrawAABB2D(m_randomAABB2, m_randomAABB2.IsPointInside(m_referencePoint) ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.0f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderOBB2() const
{
    Vec2 const nearestPoint = m_randomOBB2.GetNearestPoint(m_referencePoint);
    DrawOBB2D(m_randomOBB2, m_randomOBB2.IsPointInside(m_referencePoint) ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.0f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderCapsule2D() const
{
    Vec2 const nearestPoint = m_randomCapsule2.GetNearestPoint(m_referencePoint);
    DrawCapsule2D(m_randomCapsule2, m_randomCapsule2.IsPointInside(m_referencePoint) ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.0f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderReferencePoint() const
{
    DrawDisc2D(m_referencePoint, 3.f, Rgba8::WHITE);
}
