//----------------------------------------------------------------------------------------------------
// GameRaycastVsDiscs.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameRaycastVsDiscs.hpp"

#include "App.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
GameRaycastVsDiscs::GameRaycastVsDiscs()
{
    m_screenCamera = new Camera();

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));

    m_gameClock = new Clock(Clock::GetSystemClock());

    GenerateRandomDiscs2D();
    GenerateRandomLineSegmentInScreen();
}

//----------------------------------------------------------------------------------------------------
GameRaycastVsDiscs::~GameRaycastVsDiscs()
{
    SafeDelete(m_screenCamera);
    SafeDelete(m_gameClock);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsDiscs::Update()
{
    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    UpdateFromKeyboard(deltaSeconds);
    UpdateFromController(deltaSeconds);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsDiscs::Render() const
{
    g_theRenderer->BeginCamera(*m_screenCamera);

    RenderDisc2();
    RenderRaycastResult();
    RenderCurrentModeText("CurrentMode: RaycastVsDiscs");

    g_theRenderer->EndCamera(*m_screenCamera);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsDiscs::UpdateFromKeyboard(float const deltaSeconds)
{
    if (g_theInput->WasKeyJustPressed('O')) m_gameClock->StepSingleFrame();
    if (g_theInput->WasKeyJustPressed('T')) m_gameClock->SetTimeScale(0.1f);
    if (g_theInput->WasKeyJustReleased('T')) m_gameClock->SetTimeScale(1.f);
    if (g_theInput->WasKeyJustPressed('P')) m_gameClock->TogglePause();
    if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) App::RequestQuit();

    if (g_theInput->WasKeyJustPressed(KEYCODE_F8)) GenerateRandomDiscs2D();
    if (g_theInput->IsKeyDown(KEYCODE_W)) m_lineSegment.m_start.y += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_S)) m_lineSegment.m_start.y -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_A)) m_lineSegment.m_start.x -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_D)) m_lineSegment.m_start.x += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_I)) m_lineSegment.m_end.y += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_K)) m_lineSegment.m_end.y -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_J)) m_lineSegment.m_end.x -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_L)) m_lineSegment.m_end.x += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE)) m_lineSegment.m_start = GetMouseWorldPos();
    if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE)) m_lineSegment.m_end = GetMouseWorldPos();
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsDiscs::UpdateFromController(float const deltaSeconds)
{
    UNUSED(deltaSeconds)
}

//----------------------------------------------------------------------------------------------------
Vec2 GameRaycastVsDiscs::GenerateRandomPointInScreen() const
{
    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    float const randomX     = g_theRNG->RollRandomFloatInRange(0, screenSizeX);
    float const randomY     = g_theRNG->RollRandomFloatInRange(0, screenSizeY);

    return Vec2(randomX, randomY);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsDiscs::GenerateRandomLineSegmentInScreen()
{
    m_lineSegment = LineSegment2(GenerateRandomPointInScreen(), GenerateRandomPointInScreen(), 2.f, false);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsDiscs::GenerateRandomDiscs2D()
{
    for (int i = 0; i < 8; ++i)
    {
        float const randomRadius = g_theRNG->RollRandomFloatInRange(10.f, 200.f);
        Vec2        center       = GenerateRandomPointInScreen();
        center                   = ClampPointToScreen(center, randomRadius);

        m_randomDisc[i] = Disc2(center, randomRadius);
    }
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsDiscs::RenderDisc2() const
{
    for (int i = 0; i < 8; ++i)
    {
        DrawDisc2D(m_randomDisc[i].m_position, m_randomDisc[i].m_radius, Rgba8::BLUE);
    }
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsDiscs::RenderRaycastResult() const
{
    // Ray direction and starting position
    Vec2 const  forwardNormal = (m_lineSegment.m_end - m_lineSegment.m_start).GetNormalized();
    Vec2 const  tailPosition  = m_lineSegment.m_start;
    float const maxDistance   = m_lineSegment.GetLength();

    // To store the closest collision result
    RaycastResult2D closestResult;
    closestResult.m_didImpact  = false;
    closestResult.m_impactDist = maxDistance;
    int closestDiscIndex       = -1; // Index of the closest disc

    // If the start point is inside at least one disc, draw a white arrow and stop further checks
    if (IsTailPosInsideDisc(tailPosition))
    {
        DrawDisc2D(tailPosition, 5.0f, Rgba8::WHITE); // Mark the start point with a small white circle
        DrawArrow2D(tailPosition, tailPosition + forwardNormal * maxDistance, 50.f, m_lineSegment.m_thickness, Rgba8::WHITE);
    }
    else
    {
        // Check collisions with all discs and find the closest one
        for (int i = 0; i < 8; ++i)
        {
            RaycastResult2D const result = RaycastVsDisc2D(tailPosition, forwardNormal, maxDistance, m_randomDisc[i].GetCenter(), m_randomDisc[i].GetRadius());

            if (result.m_didImpact && result.m_impactDist < closestResult.m_impactDist)
            {
                closestResult    = result;
                closestDiscIndex = i;
            }
        }

        // Draw the ray as a white arrow
        DrawArrow2D(tailPosition,
                    tailPosition + forwardNormal * maxDistance,
                    50.f,
                    m_lineSegment.m_thickness,
                    Rgba8::WHITE);

        // If a collision occurred, draw the closest collision result
        if (closestResult.m_didImpact)
        {
            // Mark the closest collision disc in blue
            DrawDisc2D(m_randomDisc[closestDiscIndex].GetCenter(),
                       m_randomDisc[closestDiscIndex].GetRadius(),
                       Rgba8::LIGHT_BLUE);

            // 1. Dark gray arrow: represents the full ray distance
            DrawArrow2D(tailPosition,
                        tailPosition + forwardNormal * maxDistance, 50.f,
                        m_lineSegment.m_thickness,
                        Rgba8::GREY);

            // 2. Orange arrow: represents the distance from the start to the impact point
            DrawArrow2D(tailPosition,
                        closestResult.m_impactPos,
                        50.f,
                        m_lineSegment.m_thickness,
                        Rgba8::ORANGE);

            // 3. Cyan arrow: represents the normal vector at the impact point
            DrawArrow2D(closestResult.m_impactPos,
                        closestResult.m_impactPos + closestResult.m_impactNormal * 100.0f,
                        50.f,
                        m_lineSegment.m_thickness,
                        Rgba8::CYAN);

            // 4. Small white circle: represents the impact point location
            DrawDisc2D(closestResult.m_impactPos,
                       5.0f,
                       Rgba8::WHITE);
        }
    }
}

//----------------------------------------------------------------------------------------------------
Vec2 GameRaycastVsDiscs::ClampPointToScreen(Vec2 const& pointPosition,
                                            float const radius) const
{
    Vec2 clampedPoint = pointPosition;

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    clampedPoint.x          = GetClamped(clampedPoint.x, radius, screenSizeX - radius);
    clampedPoint.y          = GetClamped(clampedPoint.y, radius, screenSizeY - radius);

    return clampedPoint;
}

//----------------------------------------------------------------------------------------------------
bool GameRaycastVsDiscs::IsTailPosInsideDisc(Vec2 const& startPosition) const
{
    for (int i = 0; i < 8; ++i)
    {
        if (m_randomDisc[i].IsPointInside(startPosition))
        {
            DrawDisc2D(m_randomDisc[i].GetCenter(), m_randomDisc[i].GetRadius(), Rgba8::LIGHT_BLUE);
            return true;
        }
    }

    return false;
}
