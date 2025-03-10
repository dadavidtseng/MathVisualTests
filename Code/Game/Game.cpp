//----------------------------------------------------------------------------------------------------
// Game.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Game.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
Game::~Game()
{
    SafeDelete(m_screenCamera);
    SafeDelete(m_gameClock);
}

//----------------------------------------------------------------------------------------------------
Vec2 Game::GetMouseWorldPos() const
{
    Vec2 const  mouseUV    = g_theInput->GetCursorNormalizedPosition();
    Vec2 const  bottomLeft = m_screenCamera->GetOrthographicBottomLeft();
    Vec2 const  topRight   = m_screenCamera->GetOrthographicTopRight();
    AABB2 const orthoBounds(bottomLeft, topRight);

    return orthoBounds.GetPointAtUV(mouseUV);
}

//----------------------------------------------------------------------------------------------------
void Game::RenderCurrentModeText(char const* currentModeText) const
{
    VertexList verts;

    float const currentModeTextBoxMinX = g_gameConfigBlackboard.GetValue("currentModeTextBoxMinX", 0.f);
    float const currentModeTextBoxMinY = g_gameConfigBlackboard.GetValue("currentModeTextBoxMinY", 780.f);
    float const currentModeTextBoxMaxX = g_gameConfigBlackboard.GetValue("currentModeTextBoxMaxX", 1600.f);
    float const currentModeTextBoxMaxY = g_gameConfigBlackboard.GetValue("currentModeTextBoxMaxY", 800.f);
    AABB2 const currentModeTextBox(Vec2(currentModeTextBoxMinX, currentModeTextBoxMinY), Vec2(currentModeTextBoxMaxX, currentModeTextBoxMaxY));

    g_theBitmapFont->AddVertsForTextInBox2D(verts, currentModeText, currentModeTextBox, 20.f, Rgba8::YELLOW);
    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(BlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(DepthMode::DISABLED);
    g_theRenderer->BindTexture(&g_theBitmapFont->GetTexture());
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void Game::RenderControlText() const
{
    VertexList verts;

    float const currentControlTextBoxMinX = g_gameConfigBlackboard.GetValue("currentControlTextBoxMinX", 0.f);
    float const currentControlTextBoxMinY = g_gameConfigBlackboard.GetValue("currentControlTextBoxMinY", 760.f);
    float const currentControlTextBoxMaxX = g_gameConfigBlackboard.GetValue("currentControlTextBoxMaxX", 1600.f);
    float const currentControlTextBoxMaxY = g_gameConfigBlackboard.GetValue("currentControlTextBoxMaxY", 780.f);
    AABB2 const currentModeTextBox(Vec2(currentControlTextBoxMinX, currentControlTextBoxMinY), Vec2(currentControlTextBoxMaxX, currentControlTextBoxMaxY));

    const char* currentControlText = "F8 to randomize; LMB/RMB set ray start/end, IJKL move end, arrows move ray, hold T = slow";
    g_theBitmapFont->AddVertsForTextInBox2D(verts, currentControlText, currentModeTextBox, 20.f, Rgba8::GREEN);
    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(BlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(DepthMode::DISABLED);
    g_theRenderer->BindTexture(&g_theBitmapFont->GetTexture());
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
Vec2 Game::GenerateRandomPointInScreen() const
{
    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    float const randomX     = g_theRNG->RollRandomFloatInRange(0, screenSizeX);
    float const randomY     = g_theRNG->RollRandomFloatInRange(0, screenSizeY);

    return Vec2(randomX, randomY);
}

//----------------------------------------------------------------------------------------------------
Vec2 Game::ClampPointToScreen(Vec2 const& point, float const radius) const
{
    Vec2 clampedPoint = point;

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    clampedPoint.x          = GetClamped(clampedPoint.x, radius, screenSizeX - radius);
    clampedPoint.y          = GetClamped(clampedPoint.y, radius, screenSizeY - radius);

    return clampedPoint;
}

//----------------------------------------------------------------------------------------------------
Vec2 Game::ClampPointToScreen(Vec2 const& point, float const halfWidth, float const halfHeight) const
{
    Vec2 clampedPoint = point;

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    clampedPoint.x          = GetClamped(clampedPoint.x, halfWidth, screenSizeX - halfWidth);
    clampedPoint.y          = GetClamped(clampedPoint.y, halfHeight, screenSizeY - halfHeight);

    return clampedPoint;
}