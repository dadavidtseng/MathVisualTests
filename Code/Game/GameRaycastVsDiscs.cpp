//----------------------------------------------------------------------------------------------------
// GameRaycastVsDiscs.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameRaycastVsDiscs.hpp"

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
    m_screenCamera->SetOrthoGraphicView(Vec2(0.f, 0.f), Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

    m_gameClock = new Clock(Clock::GetSystemClock());

    GenerateRandomDiscs();
    GenerateRandomLineSegmentInScreen();
}

//----------------------------------------------------------------------------------------------------
GameRaycastVsDiscs::~GameRaycastVsDiscs()
{
    delete m_screenCamera;
    m_screenCamera = nullptr;
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


    std::vector<Vertex_PCU> titleVerts;
    g_theBitmapFont->AddVertsForTextInBox2D(titleVerts,
                                            "CURRENT MODE: RaycastVsDiscs",
                                            AABB2(Vec2(0.f, 750.f), Vec2(1600.f, 800.f)),
                                            10.f);

    g_theRenderer->BindTexture(&g_theBitmapFont->GetTexture());

    g_theRenderer->DrawVertexArray(static_cast<int>(titleVerts.size()), titleVerts.data());

    RenderDisc2();
    RenderRaycastResult();

    g_theRenderer->EndCamera(*m_screenCamera);
}


//------------------------------------------------------------------------------------------------
Vec2 GameRaycastVsDiscs::GetMouseWorldPos() const
{
    Vec2 const  mouseUV    = g_theWindow->GetNormalizedMouseUV();
    Vec2        const bottomLeft = m_screenCamera->GetOrthographicBottomLeft();
    Vec2        const topRight   = m_screenCamera->GetOrthographicTopRight();
    AABB2 const orthoBounds(bottomLeft, topRight);
    return orthoBounds.GetPointAtUV(mouseUV);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsDiscs::UpdateFromKeyboard(float const deltaSeconds)
{
    if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
        GenerateRandomDiscs();

    if (g_theInput->IsKeyDown(KEYCODE_W))
        m_lineSegment.m_start.y += m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_S))
        m_lineSegment.m_start.y -= m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_A))
        m_lineSegment.m_start.x -= m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_D))
        m_lineSegment.m_start.x += m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_I))
        m_lineSegment.m_end.y += m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_K))
        m_lineSegment.m_end.y -= m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_J))
        m_lineSegment.m_end.x -= m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_L))
        m_lineSegment.m_end.x += m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE))
        m_lineSegment.m_start = GetMouseWorldPos();

    if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE))
        m_lineSegment.m_end = GetMouseWorldPos();
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsDiscs::UpdateFromController(float const deltaSeconds)
{
    UNUSED(deltaSeconds)
}

//----------------------------------------------------------------------------------------------------
Vec2 GameRaycastVsDiscs::GenerateRandomPointInScreen() const
{
    const float randomX = g_theRNG->RollRandomFloatInRange(0, SCREEN_SIZE_X);
    const float randomY = g_theRNG->RollRandomFloatInRange(0, SCREEN_SIZE_Y);

    return Vec2(randomX, randomY);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsDiscs::GenerateRandomLineSegmentInScreen()
{
    m_lineSegment = LineSegment2(GenerateRandomPointInScreen(), GenerateRandomPointInScreen(), 2.f, false);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsDiscs::GenerateRandomDiscs()
{
    for (int i = 0; i < 8; ++i)
    {
        const float randomRadius = g_theRNG->RollRandomFloatInRange(10.f, 200.f);
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
        DrawDisc2(m_randomDisc[i].m_position, m_randomDisc[i].m_radius, BLUE);
    }
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsDiscs::RenderRaycastResult() const
{
    // Ray direction and starting position
    const Vec2  fwdNormal = (m_lineSegment.m_end - m_lineSegment.m_start).GetNormalized();
    const Vec2  tailPos   = m_lineSegment.m_start;
    const float maxDist   = m_lineSegment.GetLength();

    // To store the closest collision result
    RaycastResult2D closestResult;
    closestResult.m_didImpact  = false;
    closestResult.m_impactDist = maxDist;
    int closestDiscIndex       = -1; // Index of the closest disc

    // If the start point is inside at least one disc, draw a white arrow and stop further checks
    if (IsTailPosInsideDisc(tailPos))
    {
        DrawDisc2(tailPos, 5.0f, WHITE); // Mark the start point with a small white circle
        DrawArrow2D(tailPos, tailPos + fwdNormal * maxDist, 50.f, m_lineSegment.m_thickness, WHITE);
    }
    else
    {
        // Check collisions with all discs and find the closest one
        for (int i = 0; i < 8; ++i)
        {
            const RaycastResult2D result = RaycastVsDisc2D(tailPos, fwdNormal, maxDist, m_randomDisc[i].GetCenter(), m_randomDisc[i].GetRadius());

            if (result.m_didImpact && result.m_impactDist < closestResult.m_impactDist)
            {
                closestResult    = result;
                closestDiscIndex = i;
            }
        }

        // Draw the ray as a white arrow
        DrawArrow2D(tailPos,
                    tailPos + fwdNormal * maxDist,
                    50.f,
                    m_lineSegment.m_thickness,
                    WHITE);

        // If a collision occurred, draw the closest collision result
        if (closestResult.m_didImpact)
        {
            // Mark the closest collision disc in blue
            DrawDisc2(m_randomDisc[closestDiscIndex].GetCenter(),
                      m_randomDisc[closestDiscIndex].GetRadius(),
                      LIGHT_BLUE);

            // 1. Dark gray arrow: represents the full ray distance
            DrawArrow2D(tailPos,
                        tailPos + fwdNormal * maxDist, 50.f,
                        m_lineSegment.m_thickness,
                        GREY);

            // 2. Orange arrow: represents the distance from the start to the impact point
            DrawArrow2D(tailPos,
                        closestResult.m_impactPos,
                        50.f,
                        m_lineSegment.m_thickness,
                        ORANGE);

            // 3. Cyan arrow: represents the normal vector at the impact point
            DrawArrow2D(closestResult.m_impactPos,
                        closestResult.m_impactPos + closestResult.m_impactNormal * 100.0f,
                        50.f,
                        m_lineSegment.m_thickness,
                        CYAN);

            // 4. Small white circle: represents the impact point location
            DrawDisc2(closestResult.m_impactPos,
                      5.0f,
                      WHITE);
        }
    }
}

//----------------------------------------------------------------------------------------------------
Vec2 GameRaycastVsDiscs::ClampPointToScreen(const Vec2& point,
                                            const float radius) const
{
    Vec2 clampedPoint = point;
    clampedPoint.x    = GetClamped(clampedPoint.x, radius, SCREEN_SIZE_X - radius);
    clampedPoint.y    = GetClamped(clampedPoint.y, radius, SCREEN_SIZE_Y - radius);
    return clampedPoint;
}

//----------------------------------------------------------------------------------------------------
bool GameRaycastVsDiscs::IsTailPosInsideDisc(Vec2 const& startPos) const
{
    for (int i = 0; i < 8; ++i)
    {
        if (m_randomDisc[i].IsPointInside(startPos))
        {
            DrawDisc2(m_randomDisc[i].GetCenter(), m_randomDisc[i].GetRadius(), LIGHT_BLUE);
            return true;
        }
    }
    return false;
}
