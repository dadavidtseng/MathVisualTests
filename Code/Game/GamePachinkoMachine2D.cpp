//----------------------------------------------------------------------------------------------------
// GamePachinkoMachine2D.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GamePachinkoMachine2D.hpp"

#include "GameCommon.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"

//----------------------------------------------------------------------------------------------------
GamePachinkoMachine2D::GamePachinkoMachine2D()
{
    m_screenCamera = new Camera();
    m_worldCamera  = new Camera();

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);

    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));
    m_worldCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));

    m_gameClock = new Clock(Clock::GetSystemClock());
}

void GamePachinkoMachine2D::Update()
{
}

void GamePachinkoMachine2D::Render() const
{
    //-Start-of-World-Camera--------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_worldCamera);

    // RenderShapes();

    g_theRenderer->EndCamera(*m_worldCamera);

    //-End-of-World-Camera----------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //-Start-of-Screen-Camera-------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_screenCamera);

    RenderCurrentModeText("CurrentMode: GamePachinkoMachine2D");

    VertexList_PCU verts;

    float const currentControlTextBoxMinX = g_gameConfigBlackboard.GetValue("currentControlTextBoxMinX", 0.f);
    float const currentControlTextBoxMinY = g_gameConfigBlackboard.GetValue("currentControlTextBoxMinY", 760.f);
    float const currentControlTextBoxMaxX = g_gameConfigBlackboard.GetValue("currentControlTextBoxMaxX", 1600.f);
    float const currentControlTextBoxMaxY = g_gameConfigBlackboard.GetValue("currentControlTextBoxMaxY", 780.f);
    AABB2 const currentModeTextBox(Vec2(currentControlTextBoxMinX, currentControlTextBoxMinY - 20.f), Vec2(currentControlTextBoxMaxX, currentControlTextBoxMaxY - 20.f));

    String const currentControlText = Stringf("F8 to randomize; W/E=pre/next easing function,\nN/M=curve subdivisions(), hold T=slow");
    g_theBitmapFont->AddVertsForTextInBox2D(verts, currentControlText, currentModeTextBox, 20.f, Rgba8::GREEN, 1.f, Vec2::ZERO, eTextBoxMode::OVERRUN);
    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(&g_theBitmapFont->GetTexture());
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());

    g_theRenderer->EndCamera(*m_screenCamera);

    //-End-of-Screen-Camera---------------------------------------------------------------------------

    DebugRenderScreen(*m_screenCamera);
}

void GamePachinkoMachine2D::UpdateFromKeyboard(float deltaSeconds)
{
}

void GamePachinkoMachine2D::UpdateFromController(float deltaSeconds)
{
}
