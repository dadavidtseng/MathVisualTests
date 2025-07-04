//----------------------------------------------------------------------------------------------------
// GameRaycastVsLineSegments.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameRaycastVsLineSegments.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
GameRaycastVsLineSegments::GameRaycastVsLineSegments()
{
    m_screenCamera = new Camera();

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));
    m_screenCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);
    m_gameClock = new Clock(Clock::GetSystemClock());

    GenerateRandomLineSegmentInScreen();
    GenerateRandomLineSegment2D();
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsLineSegments::Update()
{
    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    UpdateFromKeyboard(deltaSeconds);
    UpdateFromController(deltaSeconds);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsLineSegments::Render() const
{
    g_theRenderer->BeginCamera(*m_screenCamera);

    RenderLineSegments2D();
    RenderRaycastResult();
    RenderCurrentModeText("CurrentMode: RaycastVsLineSegments");
    RenderControlText();

    g_theRenderer->EndCamera(*m_screenCamera);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsLineSegments::UpdateFromKeyboard(float const deltaSeconds)
{
    if (g_theInput->WasKeyJustPressed(KEYCODE_O)) m_gameClock->StepSingleFrame();
    if (g_theInput->WasKeyJustPressed(KEYCODE_T)) m_gameClock->SetTimeScale(0.1f);
    if (g_theInput->WasKeyJustReleased(KEYCODE_T)) m_gameClock->SetTimeScale(1.f);
    if (g_theInput->WasKeyJustPressed(KEYCODE_P)) m_gameClock->TogglePause();
    if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) App::RequestQuit();

    if (g_theInput->WasKeyJustPressed(KEYCODE_F8)) GenerateRandomLineSegment2D();
    if (g_theInput->IsKeyDown(KEYCODE_W)) m_lineSegment.m_startPosition.y += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_S)) m_lineSegment.m_startPosition.y -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_A)) m_lineSegment.m_startPosition.x -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_D)) m_lineSegment.m_startPosition.x += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_I)) m_lineSegment.m_endPosition.y += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_K)) m_lineSegment.m_endPosition.y -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_J)) m_lineSegment.m_endPosition.x -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_L)) m_lineSegment.m_endPosition.x += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE)) m_lineSegment.m_startPosition = GetMouseWorldPos();
    if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE)) m_lineSegment.m_endPosition = GetMouseWorldPos();
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsLineSegments::UpdateFromController(float const deltaSeconds)
{
    XboxController const controller = g_theInput->GetController(0);

    m_lineSegment.m_startPosition += controller.GetLeftStick().GetPosition() * m_moveSpeed * deltaSeconds;
    m_lineSegment.m_endPosition += controller.GetRightStick().GetPosition() * m_moveSpeed * deltaSeconds;
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsLineSegments::GenerateRandomLineSegmentInScreen()
{
    m_lineSegment = LineSegment2(GenerateRandomPointInScreen(), GenerateRandomPointInScreen(), 2.f, false);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsLineSegments::GenerateRandomLineSegment2D()
{
    for (int i = 0; i < 8; ++i)
    {
        Vec2 startPosition = GenerateRandomPointInScreen();
        Vec2 endPosition   = GenerateRandomPointInScreen();

        m_lineSegments[i] = LineSegment2(startPosition, endPosition, 3.f, false);
    }
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsLineSegments::RenderLineSegments2D() const
{
    VertexList_PCU verts;

    for (int i = 0; i < 8; ++i)
    {
        AddVertsForLineSegment2D(verts, m_lineSegments[i].m_startPosition, m_lineSegments[i].m_endPosition, 3.f, false, Rgba8::BLUE);
    }

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsLineSegments::RenderRaycastResult() const
{
    VertexList_PCU verts;

    // Ray direction and starting position
    Vec2 const  forwardNormal = (m_lineSegment.m_endPosition - m_lineSegment.m_startPosition).GetNormalized();
    Vec2 const  tailPosition  = m_lineSegment.m_startPosition;
    float const maxDistance   = m_lineSegment.GetLength();

    // To store the closest collision result
    RaycastResult2D closestResult;
    closestResult.m_didImpact    = false;
    closestResult.m_impactLength = maxDistance;
    int closestDiscIndex         = -1; // Index of the closest disc


    // Check collisions with all discs and find the closest one
    for (int i = 0; i < 8; ++i)
    {
        RaycastResult2D const result = RaycastVsLineSegment2D(tailPosition, forwardNormal, maxDistance, m_lineSegments[i].m_startPosition, m_lineSegments[i].m_endPosition);

        if (result.m_didImpact && result.m_impactLength < closestResult.m_impactLength)
        {
            closestResult    = result;
            closestDiscIndex = i;
        }
    }

    // Draw the ray as a white arrow
    AddVertsForArrow2D(verts, tailPosition, tailPosition + forwardNormal * maxDistance, 50.f, m_lineSegment.m_thickness);

    // If a collision occurred, draw the closest collision result
    if (closestResult.m_didImpact)
    {
        // Mark the closest collision disc in blue
        AddVertsForLineSegment2D(verts, m_lineSegments[closestDiscIndex].m_startPosition, m_lineSegments[closestDiscIndex].m_endPosition, 3.f, false, Rgba8::LIGHT_BLUE);

        // 1. Dark gray arrow: represents the full ray distance
        AddVertsForArrow2D(verts, tailPosition, tailPosition + forwardNormal * maxDistance, 50.f, m_lineSegment.m_thickness, Rgba8::GREY);

        // 2. Orange arrow: represents the distance from the start to the impact point
        AddVertsForArrow2D(verts, tailPosition, closestResult.m_impactPosition, 50.f, m_lineSegment.m_thickness, Rgba8::ORANGE);

        // 3. Cyan arrow: represents the normal vector at the impact point
        AddVertsForArrow2D(verts, closestResult.m_impactPosition, closestResult.m_impactPosition + closestResult.m_impactNormal * 100.0f, 50.f, m_lineSegment.m_thickness, Rgba8::CYAN);

        // 4. Small white circle: represents the impact point location
        AddVertsForDisc2D(verts, closestResult.m_impactPosition, 5.f);
    }

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}
