//----------------------------------------------------------------------------------------------------
// GameRaycastVsLineSegments.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameRaycastVsLineSegments.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
GameRaycastVsLineSegments::GameRaycastVsLineSegments()
{
    m_screenCamera = new Camera();
    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(SCREEN_SIZE_X, SCREEN_SIZE_Y));

    m_gameClock = new Clock(Clock::GetSystemClock());

    GenerateRandomLineSegmentInScreen();
}

//----------------------------------------------------------------------------------------------------
GameRaycastVsLineSegments::~GameRaycastVsLineSegments()
{
}

void GameRaycastVsLineSegments::Update()
{
}

void GameRaycastVsLineSegments::Render() const
{
    g_theRenderer->BeginCamera(*m_screenCamera);

    DrawLineSegment2D(m_referenceLine.m_start, m_referenceLine.m_end, m_referenceLine.m_thickness, m_referenceLine.m_isInfinite, Rgba8::WHITE);

    std::vector<Vertex_PCU> titleVerts;
    g_theBitmapFont->AddVertsForTextInBox2D(titleVerts,
                                            "CURRENT MODE: RaycastVsLineSegments",
                                            AABB2(Vec2(0.f, 750.f), Vec2(1600.f, 800.f)),
                                            10.f);

    g_theRenderer->BindTexture(&g_theBitmapFont->GetTexture());

    g_theRenderer->DrawVertexArray(static_cast<int>(titleVerts.size()), titleVerts.data());

    g_theRenderer->EndCamera(*m_screenCamera);
}

void GameRaycastVsLineSegments::UpdateFromKeyboard(float deltaSeconds)
{
}

void GameRaycastVsLineSegments::UpdateFromController(float deltaSeconds)
{
}

//----------------------------------------------------------------------------------------------------
Vec2 GameRaycastVsLineSegments::GenerateRandomPointInScreen() const
{
    float const randomX = g_theRNG->RollRandomFloatInRange(0, SCREEN_SIZE_X);
    float const randomY = g_theRNG->RollRandomFloatInRange(0, SCREEN_SIZE_Y);

    return Vec2(randomX, randomY);
}

//----------------------------------------------------------------------------------------------------
void GameRaycastVsLineSegments::GenerateRandomLineSegmentInScreen()
{
    m_referenceLine = LineSegment2(GenerateRandomPointInScreen(), GenerateRandomPointInScreen(), 2.f, false);
}
