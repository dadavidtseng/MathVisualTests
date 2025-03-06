//----------------------------------------------------------------------------------------------------
// GameRaycastVsAABBs.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameRaycastVsAABBs.hpp"

#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
GameRaycastVsAABBs::GameRaycastVsAABBs()
{
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
    std::vector<Vertex_PCU> titleVerts;
    g_theBitmapFont->AddVertsForTextInBox2D(titleVerts,
                                            "CURRENT MODE: RaycastVsAABBs",
                                            AABB2(Vec2(0.f, 750.f), Vec2(1600.f, 800.f)),
                                            10.f);

    g_theRenderer->BindTexture(&g_theBitmapFont->GetTexture());

    g_theRenderer->DrawVertexArray(static_cast<int>(titleVerts.size()), titleVerts.data());
}

void GameRaycastVsAABBs::UpdateFromKeyboard(float deltaSeconds)
{
}

void GameRaycastVsAABBs::UpdateFromController(float deltaSeconds)
{
}
