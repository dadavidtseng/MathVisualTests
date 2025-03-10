//----------------------------------------------------------------------------------------------------
// GameRaycastVsAABBs.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameRaycastVsAABBs.hpp"

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
GameRaycastVsAABBs::GameRaycastVsAABBs()
{
    m_screenCamera = new Camera();

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);

    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));

    m_gameClock = new Clock(Clock::GetSystemClock());

    GenerateRandomLineSegmentInScreen();
    GenerateRandomAABB2s2D();
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsAABBs::Update()
{
    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    UpdateFromKeyboard(deltaSeconds);
    UpdateFromController(deltaSeconds);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsAABBs::Render() const
{
    g_theRenderer->BeginCamera(*m_screenCamera);

    RenderAABB2s2D();
    RenderRaycastResult();
    RenderCurrentModeText("CurrentMode: RaycastVsAABBs");
    RenderControlText();

    g_theRenderer->EndCamera(*m_screenCamera);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsAABBs::UpdateFromKeyboard(float const deltaSeconds)
{
    if (g_theInput->WasKeyJustPressed(KEYCODE_O)) m_gameClock->StepSingleFrame();
    if (g_theInput->WasKeyJustPressed(KEYCODE_T)) m_gameClock->SetTimeScale(0.1f);
    if (g_theInput->WasKeyJustReleased(KEYCODE_T)) m_gameClock->SetTimeScale(1.f);
    if (g_theInput->WasKeyJustPressed(KEYCODE_P)) m_gameClock->TogglePause();
    if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) App::RequestQuit();

    if (g_theInput->WasKeyJustPressed(KEYCODE_F8)) GenerateRandomAABB2s2D();
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
void GameRaycastVsAABBs::UpdateFromController(float const deltaSeconds)
{
    XboxController const controller = g_theInput->GetController(0);

    m_lineSegment.m_start += controller.GetLeftStick().GetPosition() * m_moveSpeed * deltaSeconds;
    m_lineSegment.m_end += controller.GetRightStick().GetPosition() * m_moveSpeed * deltaSeconds;
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsAABBs::RenderAABB2s2D() const
{
    for (int i = 0; i < 8; ++i)
    {
        DrawAABB2D(m_AABB2[i], Rgba8::BLUE);
    }
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsAABBs::RenderRaycastResult() const
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


    // Check collisions with all discs and find the closest one
    for (int i = 0; i < 8; ++i)
    {
        RaycastResult2D const result = RayCastVsAABB2D(tailPosition, forwardNormal, maxDistance, m_AABB2[i]);

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
        DrawAABB2D(m_AABB2[closestDiscIndex], Rgba8::LIGHT_BLUE);

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

//----------------------------------------------------------------------------------------------------
void GameRaycastVsAABBs::GenerateRandomLineSegmentInScreen()
{
    m_lineSegment = LineSegment2(GenerateRandomPointInScreen(), GenerateRandomPointInScreen(), 2.f, false);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsAABBs::GenerateRandomAABB2s2D()
{
    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);

    for (int i = 0; i < 8; ++i)
    {
        float const randomWidth  = g_theRNG->RollRandomFloatInRange(10.f, screenSizeX / 5.f);
        float const randomHeight = g_theRNG->RollRandomFloatInRange(10.f, screenSizeY / 5.f);

        Vec2 center     = GenerateRandomPointInScreen();
        Vec2 randomMins = ClampPointToScreen(center - Vec2(randomWidth / 2.f, randomHeight / 2.f), randomWidth / 2.f, randomHeight / 2.f);
        Vec2 randomMaxs = ClampPointToScreen(center + Vec2(randomWidth / 2.f, randomHeight / 2.f), randomWidth / 2.f, randomHeight / 2.f);

        m_AABB2[i] = AABB2(randomMins, randomMaxs);
    }
}
