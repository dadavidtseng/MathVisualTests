//----------------------------------------------------------------------------------------------------
// GameShapes3D.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameShapes3D.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
GameShapes3D::GameShapes3D()
{
    m_screenCamera = new Camera();
    m_worldCamera  = new Camera();

    m_player.m_position    = Vec3(-2.f, 0.f, 1.f);
    m_player.m_orientation = EulerAngles::ZERO;
    m_sphere.m_position    = Vec3(-2.f, 0.f, 1.f);
    m_sphere.m_orientation = EulerAngles::ZERO;

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);

    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));
    m_worldCamera->SetPerspectiveGraphicView(2.f, 60.f, 0.1f, 100.f);
    m_worldCamera->SetPosition(Vec3(-2, 0, 1.f));

    m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");
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
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::Update()
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

    // m_worldCamera->SetPositionAndOrientation(m_testShapes.m_position-Vec3::ONE,m_testShapes.m_orientation);
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::Render() const
{
    //-Start-of-World-Camera--------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_worldCamera);

    RenderEntities();

    g_theRenderer->EndCamera(*m_worldCamera);

    //-End-of-World-Camera----------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    DebugRenderWorld(*m_worldCamera);
    //------------------------------------------------------------------------------------------------
    //-Start-of-Screen-Camera-------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_screenCamera);

    RenderCurrentModeText("CurrentMode: 3D Shapes");

    g_theRenderer->EndCamera(*m_screenCamera);

    //-End-of-Screen-Camera---------------------------------------------------------------------------

    DebugRenderScreen(*m_screenCamera);
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::UpdateFromKeyboard(float deltaSeconds)
{
    if (g_theInput->WasKeyJustPressed(KEYCODE_O)) m_gameClock->StepSingleFrame();
    if (g_theInput->WasKeyJustPressed(KEYCODE_T)) m_gameClock->SetTimeScale(0.1f);
    if (g_theInput->WasKeyJustReleased(KEYCODE_T)) m_gameClock->SetTimeScale(1.f);
    if (g_theInput->WasKeyJustPressed(KEYCODE_P)) m_gameClock->TogglePause();
    if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) App::RequestQuit();

    XboxController const& controller = g_theInput->GetController(0);

    // if (g_theInput->WasKeyJustPressed(KEYCODE_H) || controller.WasButtonJustPressed(XBOX_BUTTON_START))
    // {
    //     if (m_game->IsAttractMode() == false)
    //     {
    //         m_position    = Vec3::ZERO;
    //         m_orientation = EulerAngles::ZERO;
    //     }
    // }

    Vec3 forward;
    Vec3 left;
    Vec3 up;
    m_player.m_orientation.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);

    m_player.m_velocity       = Vec3::ZERO;
    float constexpr moveSpeed = 2.f;

    Vec2 const leftStickInput = controller.GetLeftStick().GetPosition();
    m_player.m_velocity += Vec3(leftStickInput.y, -leftStickInput.x, 0) * moveSpeed;

    if (g_theInput->IsKeyDown(KEYCODE_W)) m_player.m_velocity += forward * moveSpeed;
    if (g_theInput->IsKeyDown(KEYCODE_S)) m_player.m_velocity -= forward * moveSpeed;
    if (g_theInput->IsKeyDown(KEYCODE_A)) m_player.m_velocity += left * moveSpeed;
    if (g_theInput->IsKeyDown(KEYCODE_D)) m_player.m_velocity -= left * moveSpeed;
    if (g_theInput->IsKeyDown(KEYCODE_Z) || controller.IsButtonDown(XBOX_BUTTON_LSHOULDER)) m_player.m_velocity -= Vec3(0.f, 0.f, 1.f) * moveSpeed;
    if (g_theInput->IsKeyDown(KEYCODE_C) || controller.IsButtonDown(XBOX_BUTTON_RSHOULDER)) m_player.m_velocity += Vec3(0.f, 0.f, 1.f) * moveSpeed;
    if (g_theInput->IsKeyDown(KEYCODE_Z)) m_player.m_velocity -= Vec3(0.f, 0.f, 1.f) * moveSpeed;
    if (g_theInput->IsKeyDown(KEYCODE_C)) m_player.m_velocity += Vec3(0.f, 0.f, 1.f) * moveSpeed;

    if (g_theInput->IsKeyDown(KEYCODE_SHIFT) || controller.IsButtonDown(XBOX_BUTTON_A)) deltaSeconds *= 10.f;

    m_player.m_position += m_player.m_velocity * deltaSeconds;

    Vec2 const rightStickInput = controller.GetRightStick().GetPosition();
    m_player.m_orientation.m_yawDegrees -= rightStickInput.x * 0.125f;
    m_player.m_orientation.m_pitchDegrees -= rightStickInput.y * 0.125f;

    m_player.m_orientation.m_yawDegrees -= g_theInput->GetCursorClientDelta().x * 0.125f;
    m_player.m_orientation.m_pitchDegrees += g_theInput->GetCursorClientDelta().y * 0.125f;
    m_player.m_orientation.m_pitchDegrees = GetClamped(m_player.m_orientation.m_pitchDegrees, -85.f, 85.f);

    m_player.m_angularVelocity.m_rollDegrees = 0.f;

    float const leftTriggerInput  = controller.GetLeftTrigger();
    float const rightTriggerInput = controller.GetRightTrigger();

    if (leftTriggerInput != 0.f)
    {
        m_player.m_angularVelocity.m_rollDegrees -= 90.f;
    }

    if (rightTriggerInput != 0.f)
    {
        m_player.m_angularVelocity.m_rollDegrees += 90.f;
    }

    if (g_theInput->IsKeyDown(KEYCODE_Q)) m_player.m_angularVelocity.m_rollDegrees = 90.f;
    if (g_theInput->IsKeyDown(KEYCODE_E)) m_player.m_angularVelocity.m_rollDegrees = -90.f;

    m_player.m_orientation.m_rollDegrees += m_player.m_angularVelocity.m_rollDegrees * deltaSeconds;
    m_player.m_orientation.m_rollDegrees = GetClamped(m_player.m_orientation.m_rollDegrees, -45.f, 45.f);

    m_worldCamera->SetPositionAndOrientation(m_player.m_position, m_player.m_orientation);
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::UpdateFromController(float deltaSeconds)
{
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::RenderEntities() const
{
    VertexList verts;

    AddVertsForSphere3D(verts, Vec3::ZERO, 0.3f);

    Mat44 m2w;

    m2w.SetTranslation3D(m_sphere.m_position);
    m2w.AppendZRotation(m_sphere.m_orientation.m_yawDegrees);
    m2w.AppendYRotation(m_sphere.m_orientation.m_pitchDegrees);
    m2w.AppendXRotation(m_sphere.m_orientation.m_rollDegrees);

    g_theRenderer->SetModelConstants(m2w);
    g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
    g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
    g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
    g_theRenderer->BindTexture(m_texture);
    // g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());

    RenderPlayerBasis();
}

void GameShapes3D::RenderPlayerBasis() const
{
    VertexList verts;

    AddVertsForArrow3D(verts, m_player.m_position, m_player.m_position+Vec3::X_BASIS, 0.8f, 0.03f, 0.06f, Rgba8::RED);
    AddVertsForArrow3D(verts, m_player.m_position, m_player.m_position+Vec3::Y_BASIS, 0.8f, 0.03f, 0.06f, Rgba8::GREEN);
    AddVertsForArrow3D(verts, m_player.m_position, m_player.m_position+Vec3::Z_BASIS, 0.8f, 0.03f, 0.06f, Rgba8::BLUE);

    Mat44 m2w;

    m2w.SetTranslation3D(m_player.m_position+Vec3::Y_BASIS*0.5f);
    m2w.Append(m_player.m_orientation.GetAsMatrix_IFwd_JLeft_KUp());


    g_theRenderer->SetModelConstants(m2w);
    g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
    g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
    g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}