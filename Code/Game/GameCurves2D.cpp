//----------------------------------------------------------------------------------------------------
// GameCurves2D.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameCurves2D.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Cylinder3.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/Sphere3.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
GameCurves2D::GameCurves2D()
{
    m_screenCamera = new Camera();
    m_worldCamera  = new Camera();

    // m_player.m_startPosition = Vec3(0, 0.f, 0);
    // m_player.m_orientation   = EulerAngles::ZERO;

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);

    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));
    m_worldCamera->SetPerspectiveGraphicView(2.f, 60.f, 0.1f, 100.f);
    m_worldCamera->SetPosition(Vec3(-2.f, 0.f, 1.f));

    // m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");
    Mat44 c2r;

    c2r.m_values[Mat44::Ix] = 0.f;
    c2r.m_values[Mat44::Iz] = 1.f;
    c2r.m_values[Mat44::Jx] = -1.f;
    c2r.m_values[Mat44::Jy] = 0.f;
    c2r.m_values[Mat44::Ky] = 1.f;
    c2r.m_values[Mat44::Kz] = 0.f;

    m_worldCamera->SetCameraToRenderTransform(c2r);

    m_gameClock = new Clock(Clock::GetSystemClock());

    DebugAddWorldBasis(Mat44(), -1.f);

    Mat44 transform;

    transform.SetIJKT3D(-Vec3::Y_BASIS, Vec3::X_BASIS, Vec3::Z_BASIS, Vec3(0.25f, 0.f, 0.25f));
    DebugAddWorldText("X-Forward", transform, 0.25f, Vec2::ONE, -1.f, Rgba8::RED);

    transform.SetIJKT3D(-Vec3::X_BASIS, -Vec3::Y_BASIS, Vec3::Z_BASIS, Vec3(0.f, 0.25f, 0.5f));
    DebugAddWorldText("Y-Left", transform, 0.25f, Vec2::ZERO, -1.f, Rgba8::GREEN);

    transform.SetIJKT3D(-Vec3::X_BASIS, Vec3::Z_BASIS, Vec3::Y_BASIS, Vec3(0.f, -0.25f, 0.25f));
    DebugAddWorldText("Z-Up", transform, 0.25f, Vec2(1.f, 0.f), -1.f, Rgba8::BLUE);

    // GenerateRandomShapes();
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::Update()
{
    g_theInput->SetCursorMode(CursorMode::FPS);

    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    DebugAddScreenText(Stringf("Time: %.2f\nFPS: %.2f\nScale: %.1f", m_gameClock->GetTotalSeconds(), 1.f / m_gameClock->GetDeltaSeconds(), m_gameClock->GetTimeScale()), m_screenCamera->GetOrthographicTopRight() - Vec2(250.f, 60.f), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);
    UpdateFromKeyboard(deltaSeconds);
    UpdateFromController(deltaSeconds);

    if (g_theInput->WasKeyJustPressed(NUMCODE_6))
    {
        DebugAddWorldWireCylinder(m_worldCamera->GetPosition(), m_worldCamera->GetPosition() + Vec3::Z_BASIS * 2, 1.f, 10.f, Rgba8::WHITE, Rgba8::RED);
    }

    // UpdateShapes();
    // m_worldCamera->SetPositionAndOrientation(m_player.m_startPosition, m_player.m_orientation);
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::Render() const
{
    //-Start-of-World-Camera--------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_worldCamera);

    // RenderShapes();
    // RenderPlayerBasis();

    g_theRenderer->EndCamera(*m_worldCamera);

    //-End-of-World-Camera----------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    DebugRenderWorld(*m_worldCamera);
    //------------------------------------------------------------------------------------------------
    //-Start-of-Screen-Camera-------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_screenCamera);

    RenderCurrentModeText("CurrentMode: Curves 2D");

    VertexList_PCU verts;

    float const currentControlTextBoxMinX = g_gameConfigBlackboard.GetValue("currentControlTextBoxMinX", 0.f);
    float const currentControlTextBoxMinY = g_gameConfigBlackboard.GetValue("currentControlTextBoxMinY", 760.f);
    float const currentControlTextBoxMaxX = g_gameConfigBlackboard.GetValue("currentControlTextBoxMaxX", 1600.f);
    float const currentControlTextBoxMaxY = g_gameConfigBlackboard.GetValue("currentControlTextBoxMaxY", 780.f);
    AABB2 const currentModeTextBox(Vec2(currentControlTextBoxMinX, currentControlTextBoxMinY), Vec2(currentControlTextBoxMaxX, currentControlTextBoxMaxY));

    String const currentControlText = "F8 to randomize; WASD:fly, ZC:fly vertical, hold T=slow";
    g_theBitmapFont->AddVertsForTextInBox2D(verts, currentControlText, currentModeTextBox, 20.f, Rgba8::GREEN);
    // g_theBitmapFont->AddVertsForTextInBox2D(verts, m_raycastResultText + m_grabbedShapeText, AABB2(Vec2(currentModeTextBox.m_mins.x, currentModeTextBox.m_mins.y - 20.f), Vec2(currentModeTextBox.m_maxs.x, currentModeTextBox.m_maxs.y - 20.f)), 20.f, Rgba8::GREEN);
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

//----------------------------------------------------------------------------------------------------
void GameCurves2D::UpdateFromKeyboard(float deltaSeconds)
{
    if (g_theInput->WasKeyJustPressed(KEYCODE_O)) m_gameClock->StepSingleFrame();
    if (g_theInput->WasKeyJustPressed(KEYCODE_T)) m_gameClock->SetTimeScale(0.1f);
    if (g_theInput->WasKeyJustReleased(KEYCODE_T)) m_gameClock->SetTimeScale(1.f);
    if (g_theInput->WasKeyJustPressed(KEYCODE_P)) m_gameClock->TogglePause();
    if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) App::RequestQuit();
    // if (g_theInput->WasKeyJustPressed(KEYCODE_F8)) GenerateRandomShapes();

    XboxController const& controller = g_theInput->GetController(0);

    if (g_theInput->WasKeyJustPressed(KEYCODE_H) || controller.WasButtonJustPressed(XBOX_BUTTON_START))
    {
        m_worldCamera->SetPosition(Vec3::ZERO);
        m_worldCamera->SetOrientation(EulerAngles::ZERO);
    }

    Vec3 forward;
    Vec3 left;
    Vec3 up;
    m_worldCamera->GetOrientation().GetAsVectors_IFwd_JLeft_KUp(forward, left, up);

    float constexpr moveSpeed = 2.f;

    Vec2 const leftStickInput = controller.GetLeftStick().GetPosition();
    Vec3       targetPosition = m_worldCamera->GetPosition();

    if (g_theInput->IsKeyDown(KEYCODE_SHIFT) || controller.IsButtonDown(XBOX_BUTTON_A)) deltaSeconds *= 10.f;
    targetPosition += Vec3(leftStickInput.y, -leftStickInput.x, 0.f) * moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_W)) targetPosition += forward * moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_S)) targetPosition -= forward * moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_A)) targetPosition += left * moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_D)) targetPosition -= left * moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_Z) || controller.IsButtonDown(XBOX_BUTTON_LSHOULDER)) targetPosition -= Vec3::Z_BASIS * moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_C) || controller.IsButtonDown(XBOX_BUTTON_RSHOULDER)) targetPosition += Vec3::Z_BASIS * moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_Z)) targetPosition -= Vec3::Z_BASIS * moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_C)) targetPosition += Vec3::Z_BASIS * moveSpeed * deltaSeconds;


    Vec2 const rightStickInput = controller.GetRightStick().GetPosition();

    EulerAngles targetEulerAngles = m_worldCamera->GetOrientation();

    targetEulerAngles.m_yawDegrees -= rightStickInput.x * 0.125f;
    targetEulerAngles.m_pitchDegrees -= rightStickInput.y * 0.125f;

    targetEulerAngles.m_yawDegrees -= g_theInput->GetCursorClientDelta().x * 0.125f;
    targetEulerAngles.m_pitchDegrees += g_theInput->GetCursorClientDelta().y * 0.125f;
    targetEulerAngles.m_pitchDegrees = GetClamped(targetEulerAngles.m_pitchDegrees, -89.9f, 89.9f);

    float const leftTriggerInput  = controller.GetLeftTrigger();
    float const rightTriggerInput = controller.GetRightTrigger();

    if (leftTriggerInput != 0.f || g_theInput->IsKeyDown(KEYCODE_E))
    {
        targetEulerAngles.m_rollDegrees -= 90.f * deltaSeconds;
    }

    if (rightTriggerInput != 0.f || g_theInput->IsKeyDown(KEYCODE_Q))
    {
        targetEulerAngles.m_rollDegrees += 90.f * deltaSeconds;
    }

    targetEulerAngles.m_rollDegrees = GetClamped(targetEulerAngles.m_rollDegrees, -45.f, 45.f);

    m_worldCamera->SetPositionAndOrientation(targetPosition, targetEulerAngles);

    //----------------------------------------------------------------------------------------------------
    Vec3 forwardNormal = m_worldCamera->GetOrientation().GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D().GetNormalized();

    float closestDistance = FLOAT_MAX;
    int   closestIndex    = -1;


}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::UpdateFromController(float const deltaSeconds)
{
    UNUSED(deltaSeconds)
}