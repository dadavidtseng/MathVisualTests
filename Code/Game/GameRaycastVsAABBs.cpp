//----------------------------------------------------------------------------------------------------
// GameRaycastVsAABBs.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameRaycastVsAABBs.hpp"

#include "Engine/Core/EngineCommon.hpp"
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
}

//----------------------------------------------------------------------------------------------------
GameRaycastVsAABBs::~GameRaycastVsAABBs()
{
}

void GameRaycastVsAABBs::Update()
{
}

void GameRaycastVsAABBs::Render() const
{
    g_theRenderer->BeginCamera(*m_screenCamera);

    RenderCurrentModeText("CurrentMode: RaycastVsAABBs");

    g_theRenderer->EndCamera(*m_screenCamera);
}

void GameRaycastVsAABBs::UpdateFromKeyboard(float deltaSeconds)
{
}

void GameRaycastVsAABBs::UpdateFromController(float deltaSeconds)
{
}
