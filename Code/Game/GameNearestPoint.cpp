//----------------------------------------------------------------------------------------------------
// GameNearestPoint.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameNearestPoint.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
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
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
GameNearestPoint::GameNearestPoint()
{
    m_screenCamera = new Camera();

    float const screenSizeX   = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY   = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    float const screenCenterX = g_gameConfigBlackboard.GetValue("screenCenterX", 800.f);
    float const screenCenterY = g_gameConfigBlackboard.GetValue("screenCenterY", 400.f);

    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));
    m_referencePoint = Vec2(screenCenterX, screenCenterY);
    m_gameClock      = new Clock(Clock::GetSystemClock());

    GenerateRandomShapes();
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

    RenderShapes();
    RenderCurrentModeText("CurrentMode: NearestPoint");

    g_theRenderer->EndCamera(*m_screenCamera);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::UpdateFromKeyboard(float const deltaSeconds)
{
    if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
    {
        GenerateRandomShapes();

        float const screenCenterX = g_gameConfigBlackboard.GetValue("screenCenterX", 800.f);
        float const screenCenterY = g_gameConfigBlackboard.GetValue("screenCenterY", 400.f);
        m_referencePoint          = Vec2(screenCenterX, screenCenterY);
    }

    if (g_theInput->WasKeyJustPressed(KEYCODE_O)) m_gameClock->StepSingleFrame();
    if (g_theInput->WasKeyJustPressed(KEYCODE_T)) m_gameClock->SetTimeScale(0.1f);
    if (g_theInput->WasKeyJustReleased(KEYCODE_T)) m_gameClock->SetTimeScale(1.f);
    if (g_theInput->WasKeyJustPressed(KEYCODE_P)) m_gameClock->TogglePause();
    if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) App::RequestQuit();

    if (g_theInput->IsKeyDown(KEYCODE_W)) m_referencePoint.y += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_S)) m_referencePoint.y -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_A)) m_referencePoint.x -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_D)) m_referencePoint.x += m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE)) m_referencePoint = GetMouseWorldPos();
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::UpdateFromController(float const deltaSeconds)
{
    XboxController const controller = g_theInput->GetController(0);

    m_referencePoint += controller.GetLeftStick().GetPosition() * m_moveSpeed * deltaSeconds;
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderShapes() const
{
    RenderDisc2D();
    RenderLineSegment2D();
    RenderLineInfinite2D();
    RenderTriangle2D();
    RenderAABB2D();
    RenderOBB2D();
    RenderCapsule2D();
    RenderReferencePoint();
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomShapes()
{
    GenerateRandomDisc2D();
    GenerateRandomLineSegment2D();
    GenerateRandomInfiniteLine2D();
    GenerateRandomTriangle2D();
    GenerateRandomAABB2D();
    GenerateRandomOBB2D();
    GenerateRandomCapsule2D();
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomDisc2D()
{
    Vec2        centerPosition = GenerateRandomPointInScreen();
    float const randomRadius   = g_theRNG->RollRandomFloatInRange(10.f, 100.f);

    centerPosition = ClampPointToScreen(centerPosition, randomRadius);

    m_randomDisc = Disc2(centerPosition, randomRadius);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomLineSegment2D()
{
    Vec2        startPosition   = GenerateRandomPointInScreen();
    Vec2        endPosition     = GenerateRandomPointInScreen();
    float const randomThickness = g_theRNG->RollRandomFloatInRange(1.f, 5.f);

    startPosition = ClampPointToScreen(startPosition, randomThickness);
    endPosition   = ClampPointToScreen(endPosition, randomThickness);

    m_randomLineSegment = LineSegment2(startPosition, endPosition, randomThickness, false);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomInfiniteLine2D()
{
    Vec2        startPosition   = GenerateRandomPointInScreen();
    Vec2        endPosition     = GenerateRandomPointInScreen();
    float const randomThickness = g_theRNG->RollRandomFloatInRange(1.f, 5.f);

    startPosition = ClampPointToScreen(startPosition, randomThickness);
    endPosition   = ClampPointToScreen(endPosition, randomThickness);

    m_randomInfiniteLine = LineSegment2(startPosition, endPosition, randomThickness, true);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomTriangle2D()
{
    Vec2 const randomA = GenerateRandomPointInScreen();
    Vec2 const randomB = GenerateRandomPointInScreen();
    Vec2 const randomC = GenerateRandomPointInScreen();

    // Ensure the triangle is valid.
    Vec2 const edgeAB = randomB - randomA;
    Vec2 const edgeAC = randomC - randomA;

    // Points A, B, and C are collinear and the triangle's length is not too long.
    if (CrossProduct2D(edgeAB, edgeAC) == 0.f ||
        edgeAB.GetLengthSquared() > 100000.f ||
        edgeAC.GetLengthSquared() > 100000.f)
    {
        GenerateRandomTriangle2D();

        return;
    }

    // Ensure the triangle has counter-clockwise (CCW) winding order.
    if (CrossProduct2D(edgeAB, edgeAC) < 0.f)
    {
        m_randomTriangle = Triangle2(randomA, randomC, randomB);
    }
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomAABB2D()
{
    float const screenSizeX  = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY  = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    float const randomWidth  = g_theRNG->RollRandomFloatInRange(10.f, screenSizeX / 5.f);
    float const randomHeight = g_theRNG->RollRandomFloatInRange(10.f, screenSizeY / 5.f);

    Vec2 const centerPosition = GenerateRandomPointInScreen();
    Vec2 const randomMins     = ClampPointToScreen(centerPosition - Vec2(randomWidth / 2.f, randomHeight / 2.f), randomWidth / 2.f, randomHeight / 2.f);
    Vec2 const randomMaxs     = ClampPointToScreen(centerPosition + Vec2(randomWidth / 2.f, randomHeight / 2.f), randomWidth / 2.f, randomHeight / 2.f);

    m_randomAABB2 = AABB2(randomMins, randomMaxs);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomOBB2D()
{
    float const screenSizeX    = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY    = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    float const randomIBasisX  = g_theRNG->RollRandomFloatZeroToOne();
    float const randomIBasisY  = g_theRNG->RollRandomFloatZeroToOne();
    Vec2 const  center         = GenerateRandomPointInScreen();
    Vec2 const  halfDimensions = Vec2(g_theRNG->RollRandomFloatInRange(10.f, screenSizeX / 5.f),
                                     g_theRNG->RollRandomFloatInRange(10.f, screenSizeY / 5.f));

    m_randomOBB2 = OBB2(center, Vec2(randomIBasisX, randomIBasisY), halfDimensions);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomCapsule2D()
{
    Vec2        startPosition = GenerateRandomPointInScreen();
    Vec2        endPosition   = GenerateRandomPointInScreen();
    float const randomRadius  = g_theRNG->RollRandomFloatInRange(10.f, 100.f);

    startPosition = ClampPointToScreen(startPosition, randomRadius);
    endPosition   = ClampPointToScreen(endPosition, randomRadius);

    m_randomCapsule2 = Capsule2(startPosition, endPosition, randomRadius);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderDisc2D() const
{
    Vec2 const nearestPoint = m_randomDisc.GetNearestPoint(m_referencePoint);
    DrawDisc2D(m_randomDisc, m_randomDisc.IsPointInside(m_referencePoint) ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.0f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderLineSegment2D() const
{
    Vec2 const nearestPoint = m_randomLineSegment.GetNearestPoint(m_referencePoint);
    DrawLineSegment2D(m_randomLineSegment, m_randomLineSegment.m_thickness, m_randomLineSegment.m_isInfinite, Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.0f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderLineInfinite2D() const
{
    Vec2 const nearestPoint = m_randomInfiniteLine.GetNearestPoint(m_referencePoint);
    DrawLineSegment2D(m_randomInfiniteLine, m_randomInfiniteLine.m_thickness, m_randomInfiniteLine.m_isInfinite, Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.0f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderTriangle2D() const
{
    Vec2 const nearestPoint = m_randomTriangle.GetNearestPoint(m_referencePoint);
    DrawTriangle2D(m_randomTriangle, m_randomTriangle.IsPointInside(m_referencePoint) ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderAABB2D() const
{
    Vec2 const nearestPoint = m_randomAABB2.GetNearestPoint(m_referencePoint);
    DrawAABB2D(m_randomAABB2, m_randomAABB2.IsPointInside(m_referencePoint) ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderOBB2D() const
{
    Vec2 const nearestPoint = m_randomOBB2.GetNearestPoint(m_referencePoint);
    DrawOBB2D(m_randomOBB2, m_randomOBB2.IsPointInside(m_referencePoint) ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderCapsule2D() const
{
    Vec2 const nearestPoint = m_randomCapsule2.GetNearestPoint(m_referencePoint);
    DrawCapsule2D(m_randomCapsule2, m_randomCapsule2.IsPointInside(m_referencePoint) ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
    DrawLineSegment2D(m_referencePoint, nearestPoint, 3.f, false, Rgba8::TRANSLUCENT_WHITE);
    DrawDisc2D(nearestPoint, 5.0f, Rgba8::ORANGE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderReferencePoint() const
{
    DrawDisc2D(m_referencePoint, 3.f, Rgba8::WHITE);
}
