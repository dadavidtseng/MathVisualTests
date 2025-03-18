//----------------------------------------------------------------------------------------------------
// GameShapes3D.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameShapes3D.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
GameShapes3D::GameShapes3D()
{
    m_screenCamera = new Camera();
    m_worldCamera  = new Camera();

    // m_player.m_startPosition = Vec3(0, 0.f, 0);
    // m_player.m_orientation   = EulerAngles::ZERO;

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

    GenerateRandomShapes();
    // GenerateRandomShapes(m_spheres);
    // GenerateRandomShapes(m_cylinders);


    GenerateTest();
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

    // m_worldCamera->SetPositionAndOrientation(m_player.m_startPosition, m_player.m_orientation);
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::Render() const
{
    //-Start-of-World-Camera--------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_worldCamera);

    // RenderShapes();
    RenderPlayerBasis();
    RenderTest();

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
    targetPosition += Vec3(leftStickInput.y, -leftStickInput.x, 0) * moveSpeed * deltaSeconds;

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
    targetEulerAngles.m_pitchDegrees = GetClamped(targetEulerAngles.m_pitchDegrees, -85.f, 85.f);

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
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::UpdateFromController(float deltaSeconds)
{
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::RenderShapes() const
{
    for (int i = 0; i < 15; i++)
    {
        VertexList verts;
        AABB3      aabb3     = AABB3(m_testShapes[i].m_startPosition - Vec3::ONE, m_testShapes[i].m_startPosition + Vec3::ONE);
        Sphere3    sphere3   = Sphere3(m_testShapes[i].m_startPosition, m_testShapes[i].m_radius);
        Cylinder3  cylinder3 = Cylinder3(m_testShapes[i].m_startPosition, m_testShapes[i].m_endPosition, m_testShapes[i].m_radius);
        Vec3       nearestPoint;

        if (m_testShapes[i].m_type == TestShapeType::AABB3)
        {
            AddVertsForAABB3D(verts, aabb3);
        }

        if (m_testShapes[i].m_type == TestShapeType::SPHERE3)
        {
            AddVertsForSphere3D(verts, sphere3.m_centerPosition, sphere3.m_radius);
        }
        if (m_testShapes[i].m_type == TestShapeType::CYLINDER3)
        {
            AddVertsForCylinder3D(verts, cylinder3.m_startPosition, cylinder3.m_endPosition, cylinder3.m_radius, Rgba8::WHITE, AABB2(Vec2::ZERO, Vec2::ONE));
        }

        g_theRenderer->SetModelConstants();
        g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
        g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
        g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
        g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
        g_theRenderer->BindTexture(m_texture);
        g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());

        VertexList nearestPointVerts;

        if (m_testShapes[i].m_type == TestShapeType::AABB3)
        {
            nearestPoint = aabb3.GetNearestPoint(m_worldCamera->GetPosition());
            AddVertsForSphere3D(nearestPointVerts, nearestPoint, 0.1f, Rgba8::RED);
            AddVertsForArrow3D(nearestPointVerts, nearestPoint, nearestPoint + nearestPoint - (aabb3.m_mins + aabb3.m_maxs) / 2.f, 0.5f, 0.15f, 0.3f, Rgba8::GREEN);
        }

        if (m_testShapes[i].m_type == TestShapeType::SPHERE3)
        {
            nearestPoint = sphere3.GetNearestPoint(m_worldCamera->GetPosition());
            AddVertsForSphere3D(nearestPointVerts, nearestPoint, 0.1f, Rgba8::RED);
            AddVertsForArrow3D(nearestPointVerts, nearestPoint, nearestPoint + nearestPoint - sphere3.m_centerPosition, 0.5f, 0.15f, 0.3f, Rgba8::GREEN);
        }
        if (m_testShapes[i].m_type == TestShapeType::CYLINDER3)
        {
            nearestPoint = cylinder3.GetNearestPoint(m_worldCamera->GetPosition());
            AddVertsForSphere3D(nearestPointVerts, nearestPoint, 0.1f, Rgba8::RED);
        }

        g_theRenderer->SetModelConstants();
        g_theRenderer->SetBlendMode(BlendMode::ALPHA);
        g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
        g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
        g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
        g_theRenderer->BindTexture(nullptr);
        g_theRenderer->DrawVertexArray(static_cast<int>(nearestPointVerts.size()), nearestPointVerts.data());
    }
}

void GameShapes3D::RenderTest() const
{
    VertexList verts;
    Vec3       forwardNormal = m_worldCamera->GetOrientation().GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D().GetNormalized();
    Ray3 const ray           = Ray3(m_worldCamera->GetPosition(), m_worldCamera->GetPosition() + forwardNormal * 100.f);


    Sphere3   sphere3   = Sphere3(m_testShapes[2].m_startPosition, m_testShapes[2].m_radius);
    AABB3     box       = AABB3(m_testShapes[1].m_startPosition - Vec3::ONE, m_testShapes[1].m_startPosition + Vec3::ONE);
    Cylinder3 cylinder3 = Cylinder3(m_testShapes[0].m_startPosition, m_testShapes[0].m_endPosition, m_testShapes[0].m_radius);
    AddVertsForSphere3D(verts, sphere3.m_centerPosition, sphere3.m_radius, Rgba8::WHITE, AABB2(Vec2::ZERO, Vec2::ONE));
    AddVertsForAABB3D(verts, box);
    AddVertsForCylinder3D(verts, cylinder3.m_startPosition, cylinder3.m_endPosition, cylinder3.m_radius);
    RaycastResult3D raycastResult  = RaycastVsSphere3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, sphere3.m_centerPosition, sphere3.m_radius);
    RaycastResult3D raycastResult3 = RaycastVsAABB3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, box);
    Vec3            center         = (cylinder3.m_startPosition + cylinder3.m_endPosition) / 2.f;
    FloatRange cylinderRange = FloatRange( cylinder3.m_startPosition.z, cylinder3.m_endPosition.z);
    RaycastResult3D raycastResult2 = RaycastVsCylinderZ3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, Vec2(center.x, center.y),cylinderRange, cylinder3.m_radius);

    if (raycastResult.m_didImpact == true)
    {
        AddVertsForArrow3D(verts, raycastResult.m_impactPosition, raycastResult.m_impactPosition + raycastResult.m_impactNormal, 0.8f, 0.03f, 0.06f, Rgba8::YELLOW);
    }

    if (raycastResult2.m_didImpact == true)
    {
        AddVertsForArrow3D(verts, raycastResult2.m_impactPosition, raycastResult2.m_impactPosition + raycastResult2.m_impactNormal, 0.8f, 0.03f, 0.06f, Rgba8::YELLOW);
    }

    if (raycastResult3.m_didImpact == true)
    {
        AddVertsForArrow3D(verts, raycastResult3.m_impactPosition, raycastResult3.m_impactPosition + raycastResult3.m_impactNormal, 0.8f, 0.03f, 0.06f, Rgba8::YELLOW);
    }


    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
    g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
    g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
    g_theRenderer->BindTexture(m_texture);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::RenderPlayerBasis() const
{
    VertexList verts;

    Vec3 const worldCameraPosition = m_worldCamera->GetPosition();
    Vec3 const forwardNormal       = m_worldCamera->GetOrientation().GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D().GetNormalized();

    // Add vertices in world space.
    AddVertsForArrow3D(verts, worldCameraPosition + forwardNormal, worldCameraPosition + forwardNormal + Vec3::X_BASIS * 0.1f, 0.8f, 0.001f, 0.003f, Rgba8::RED);
    AddVertsForArrow3D(verts, worldCameraPosition + forwardNormal, worldCameraPosition + forwardNormal + Vec3::Y_BASIS * 0.1f, 0.8f, 0.001f, 0.003f, Rgba8::GREEN);
    AddVertsForArrow3D(verts, worldCameraPosition + forwardNormal, worldCameraPosition + forwardNormal + Vec3::Z_BASIS * 0.1f, 0.8f, 0.001f, 0.003f, Rgba8::BLUE);

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(BlendMode::OPAQUE);
    g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
    g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

void GameShapes3D::GenerateRandomShapes()
{
    for (int i = 0; i < 15; i++)
    {
        float const randomX      = g_theRNG->RollRandomFloatInRange(-5.f, 5.f);
        float const randomY      = g_theRNG->RollRandomFloatInRange(-5.f, 5.f);
        float const randomZ      = g_theRNG->RollRandomFloatInRange(-5.f, 5.f);
        float const randomYaw    = g_theRNG->RollRandomFloatInRange(0.f, 360.f);
        int const   randomNum    = g_theRNG->RollRandomIntInRange(0, 2);
        float const randomRadius = g_theRNG->RollRandomFloatInRange(1.f, 3.f);

        m_testShapes[i].m_startPosition = Vec3(randomX, randomY, randomZ);
        m_testShapes[i].m_orientation   = EulerAngles(randomYaw, 0.f, 0.f);
        m_testShapes[i].m_radius        = randomRadius;

        switch (randomNum)
        {
        case 0:
            {
                m_testShapes[i].m_type = TestShapeType::AABB3;
                break;
            }

        case 1:
            {
                m_testShapes[i].m_type = TestShapeType::SPHERE3;
                break;
            }

        case 2:
            {
                m_testShapes[i].m_type        = TestShapeType::CYLINDER3;
                m_testShapes[i].m_endPosition = m_testShapes[i].m_startPosition + Vec3::Z_BASIS * g_theRNG->RollRandomFloatInRange(1.f, 3.f);
                break;
            }
        }
    }
}

void GameShapes3D::GenerateTest()
{
    m_test                 = TestShape3D();
    m_test.m_startPosition = Vec3::ZERO;
    m_test.m_endPosition   = Vec3::Z_BASIS;
    m_test.m_orientation   = EulerAngles::ZERO;
    m_test.m_radius        = 3.f;
}
