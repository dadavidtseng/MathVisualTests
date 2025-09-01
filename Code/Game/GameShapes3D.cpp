//----------------------------------------------------------------------------------------------------
// GameShapes3D.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameShapes3D.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Cylinder3.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/Plane3.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Math/Sphere3.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
GameShapes3D::GameShapes3D()
{
    m_screenCamera = new Camera();
    m_worldCamera  = new Camera();

    // m_player.m_startPosition = Vec3(0, 0.f, 0);
    // m_player.m_orientation   = EulerAngles::ZERO;

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 0.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 0.f);
    m_space                 = AABB2(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));

    m_screenCamera->SetOrthoGraphicView(m_space.m_mins, m_space.m_maxs);
    m_worldCamera->SetPerspectiveGraphicView(m_space.GetWidthOverHeightRatios(), 60.f, 0.1f, 100.f);
    m_screenCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);
    m_worldCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);
    m_worldCamera->SetPosition(Vec3(-2.f, 0.f, 1.f));


    Mat44 c2r;
    c2r.SetIJK3D(Vec3::Z_BASIS, -Vec3::X_BASIS, Vec3::Y_BASIS);
    m_worldCamera->SetCameraToRenderTransform(c2r);

    m_gameClock = new Clock(Clock::GetSystemClock());

    m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");

    // Initialize DebugRenderSystem for world basis and world text.
    DebugAddWorldBasis(Mat44(), -1.f);

    Mat44 transform;

    transform.SetIJKT3D(-Vec3::Y_BASIS, Vec3::X_BASIS, Vec3::Z_BASIS, Vec3(0.25f, 0.f, 0.25f));
    DebugAddWorldText("X-Forward", transform, 0.25f, Vec2::ONE, -1.f, Rgba8::RED);

    transform.SetIJKT3D(-Vec3::X_BASIS, -Vec3::Y_BASIS, Vec3::Z_BASIS, Vec3(0.f, 0.25f, 0.5f));
    DebugAddWorldText("Y-Left", transform, 0.25f, Vec2::ZERO, -1.f, Rgba8::GREEN);

    transform.SetIJKT3D(-Vec3::X_BASIS, Vec3::Z_BASIS, Vec3::Y_BASIS, Vec3(0.f, -0.25f, 0.25f));
    DebugAddWorldText("Z-Up", transform, 0.25f, Vec2(1.f, 0.f), -1.f, Rgba8::BLUE);

    GenerateRandomShapes();
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::Update()
{
    g_input->SetCursorMode(eCursorMode::FPS);

    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    DebugAddScreenText(Stringf("Time: %.2f\nFPS: %.2f\nScale: %.1f", m_gameClock->GetTotalSeconds(), 1.f / m_gameClock->GetDeltaSeconds(), m_gameClock->GetTimeScale()), m_screenCamera->GetOrthographicTopRight() - Vec2(250.f, 60.f), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);
    UpdateFromKeyboard(deltaSeconds);
    UpdateFromController(deltaSeconds);

    if (g_input->WasKeyJustPressed(NUMCODE_6))
    {
        DebugAddWorldCylinder(m_worldCamera->GetPosition(), m_worldCamera->GetPosition() + Vec3::Z_BASIS * 2, 1.f, 10.f, true, Rgba8::WHITE, Rgba8::RED);
    }

    UpdateShapes();
    // m_worldCamera->SetPositionAndOrientation(m_player.m_startPosition, m_player.m_orientation);
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::Render() const
{
    //-Start-of-World-Camera--------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_worldCamera);

    RenderShapes();
    RenderPlayerBasis();

    // VertexList_PCU verts1;
    //     // AddVertsForSphere3D(verts1, m_worldCamera->PerspectiveScreenPosToWorld(Vec2(0.5f, 0.5f)), 0.05f);
    // //     Ray3 ray = m_worldCamera->ScreenPosToWorldRay(Vec2(0.5f, 0.5f));
    // // AddVertsForArrow3D(verts1, ray.m_startPosition, ray.m_startPosition+ray.m_forwardNormal*ray.m_maxLength,0.5f, 0.005f, 0.005f);
    //
    //     g_theRenderer->SetBlendMode(eBlendMode::OPAQUE);
    //     g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    //     g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    //     g_theRenderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
    //     g_theRenderer->BindTexture(nullptr);
    //     g_theRenderer->DrawVertexArray(static_cast<int>(verts1.size()), verts1.data());
    //     g_theRenderer->EndCamera(*m_worldCamera);

    //-End-of-World-Camera----------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    DebugRenderWorld(*m_worldCamera);
    //------------------------------------------------------------------------------------------------
    //-Start-of-Screen-Camera-------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_screenCamera);

    RenderCurrentModeText("CurrentMode: 3D Shapes");


    VertexList_PCU verts;


    float const currentControlTextBoxMinX = g_gameConfigBlackboard.GetValue("currentControlTextBoxMinX", 0.f);
    float const currentControlTextBoxMinY = g_gameConfigBlackboard.GetValue("currentControlTextBoxMinY", 760.f);
    float const currentControlTextBoxMaxX = g_gameConfigBlackboard.GetValue("currentControlTextBoxMaxX", 1600.f);
    float const currentControlTextBoxMaxY = g_gameConfigBlackboard.GetValue("currentControlTextBoxMaxY", 780.f);
    AABB2 const currentModeTextBox(Vec2(currentControlTextBoxMinX, currentControlTextBoxMinY), Vec2(currentControlTextBoxMaxX, currentControlTextBoxMaxY));

    String const currentControlText = "F8 to randomize; WASD:fly, ZC:fly vertical, hold T=slow";
    g_theBitmapFont->AddVertsForTextInBox2D(verts, currentControlText, currentModeTextBox, 20.f, Rgba8::GREEN);
    g_theBitmapFont->AddVertsForTextInBox2D(verts, m_raycastResultText + m_grabbedShapeText, AABB2(Vec2(currentModeTextBox.m_mins.x, currentModeTextBox.m_mins.y - 20.f), Vec2(currentModeTextBox.m_maxs.x, currentModeTextBox.m_maxs.y - 20.f)), 20.f, Rgba8::GREEN);
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
void GameShapes3D::UpdateFromKeyboard(float deltaSeconds)
{
    if (g_input->WasKeyJustPressed(KEYCODE_O)) m_gameClock->StepSingleFrame();
    if (g_input->WasKeyJustPressed(KEYCODE_T)) m_gameClock->SetTimeScale(0.1f);
    if (g_input->WasKeyJustReleased(KEYCODE_T)) m_gameClock->SetTimeScale(1.f);
    if (g_input->WasKeyJustPressed(KEYCODE_P)) m_gameClock->TogglePause();
    if (g_input->WasKeyJustPressed(KEYCODE_ESC)) App::RequestQuit();
    if (g_input->WasKeyJustPressed(KEYCODE_F8)) GenerateRandomShapes();

    XboxController const& controller = g_input->GetController(0);

    if (g_input->WasKeyJustPressed(KEYCODE_H) || controller.WasButtonJustPressed(XBOX_BUTTON_START))
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

    if (g_input->IsKeyDown(KEYCODE_SHIFT) || controller.IsButtonDown(XBOX_BUTTON_A)) deltaSeconds *= 10.f;
    targetPosition += Vec3(leftStickInput.y, -leftStickInput.x, 0.f) * moveSpeed * deltaSeconds;

    if (g_input->IsKeyDown(KEYCODE_W)) targetPosition += forward * moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_S)) targetPosition -= forward * moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_A)) targetPosition += left * moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_D)) targetPosition -= left * moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_Z) || controller.IsButtonDown(XBOX_BUTTON_LSHOULDER)) targetPosition -= Vec3::Z_BASIS * moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_C) || controller.IsButtonDown(XBOX_BUTTON_RSHOULDER)) targetPosition += Vec3::Z_BASIS * moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_Z)) targetPosition -= Vec3::Z_BASIS * moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_C)) targetPosition += Vec3::Z_BASIS * moveSpeed * deltaSeconds;


    Vec2 const rightStickInput = controller.GetRightStick().GetPosition();

    EulerAngles targetEulerAngles = m_worldCamera->GetOrientation();

    targetEulerAngles.m_yawDegrees -= rightStickInput.x * 0.125f;
    targetEulerAngles.m_pitchDegrees -= rightStickInput.y * 0.125f;

    targetEulerAngles.m_yawDegrees -= g_input->GetCursorClientDelta().x * 0.125f;
    targetEulerAngles.m_pitchDegrees += g_input->GetCursorClientDelta().y * 0.125f;
    targetEulerAngles.m_pitchDegrees = GetClamped(targetEulerAngles.m_pitchDegrees, -89.9f, 89.9f);

    float const leftTriggerInput  = controller.GetLeftTrigger();
    float const rightTriggerInput = controller.GetRightTrigger();

    if (leftTriggerInput != 0.f || g_input->IsKeyDown(KEYCODE_E))
    {
        targetEulerAngles.m_rollDegrees -= 90.f * deltaSeconds;
    }

    if (rightTriggerInput != 0.f || g_input->IsKeyDown(KEYCODE_Q))
    {
        targetEulerAngles.m_rollDegrees += 90.f * deltaSeconds;
    }

    targetEulerAngles.m_rollDegrees = GetClamped(targetEulerAngles.m_rollDegrees, -45.f, 45.f);

    m_worldCamera->SetPositionAndOrientation(targetPosition, targetEulerAngles);

    //----------------------------------------------------------------------------------------------------
    Vec3 forwardNormal = m_worldCamera->GetOrientation().GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D().GetNormalized();

    float closestDistance = FLOAT_MAX;
    int   closestIndex    = -1;

    if (g_input->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
    {
        for (int i = 0; i < 25; i++)
        {
            TestShape3D testShape         = m_testShapes[i];
            AABB3       aabb3             = AABB3(testShape.m_centerPosition - Vec3::ONE, testShape.m_centerPosition + Vec3::ONE);
            Sphere3     sphere3           = Sphere3(testShape.m_centerPosition, testShape.m_radius);
            Cylinder3   cylinder3         = Cylinder3(testShape.m_centerPosition - Vec3::Z_BASIS, testShape.m_centerPosition + Vec3::Z_BASIS, testShape.m_radius);
            Mat44       orientationMatrix = testShape.m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
            OBB3        obb3              = OBB3(testShape.m_centerPosition, testShape.m_halfDimensions, orientationMatrix.GetIBasis3D(), orientationMatrix.GetJBasis3D(), orientationMatrix.GetKBasis3D());
            Plane3      plane3            = Plane3(testShape.m_centerPosition.GetNormalized(), testShape.m_distanceFromOrigin);

            Ray3 ray = Ray3(m_worldCamera->GetPosition(), m_worldCamera->GetPosition() + forwardNormal * 20.f);

            RaycastResult3D result;

            if (testShape.m_type == eTestShapeType::AABB3)
            {
                result = RaycastVsAABB3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, aabb3.m_mins, aabb3.m_maxs);
            }
            else if (testShape.m_type == eTestShapeType::SPHERE3)
            {
                result = RaycastVsSphere3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, sphere3.m_centerPosition, sphere3.m_radius);
            }
            else if (testShape.m_type == eTestShapeType::CYLINDER3)
            {
                result = RaycastVsCylinderZ3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, cylinder3.GetCenterPositionXY(), cylinder3.GetFloatRange(), cylinder3.m_radius);
            }
            else if (testShape.m_type == eTestShapeType::OBB3)
            {
                result = RaycastVsOBB3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, obb3);
            }
            // else if (testShape.m_type == eTestShapeType::PLANE3)
            // {
            //     result = RaycastVsPlane3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, plane3);
            // }

            if (result.m_didImpact && result.m_impactLength < closestDistance)
            {
                closestDistance = result.m_impactLength;
                closestIndex    = i;
            }
        }

        if (closestIndex == -1) return;

        if (m_grabbedShapeIndex == closestIndex ||
            m_grabbedShapeIndex == -1)
        {
            if (m_testShapes[closestIndex].m_state == eTestShapeState::IDLE)
            {
                m_testShapes[closestIndex].m_state       = eTestShapeState::GRABBED;
                m_testShapes[closestIndex].m_targetColor = Rgba8::RED;
                m_grabbedShapeIndex                      = closestIndex;
                m_grabbedShapeText                       = "LMB=release object";
            }
            else if (m_testShapes[closestIndex].m_state == eTestShapeState::GRABBED)
            {
                m_testShapes[closestIndex].m_state       = eTestShapeState::IDLE;
                m_testShapes[closestIndex].m_targetColor = Rgba8::WHITE;
                m_grabbedShapeIndex                      = -1;
                m_grabbedShapeText                       = "LMB=grab object";
            }
        }

        if (m_grabbedShapeIndex != -1 &&
            m_grabbedShapeIndex != closestIndex)
        {
            if (m_testShapes[m_grabbedShapeIndex].m_state == eTestShapeState::IDLE)
            {
                m_testShapes[m_grabbedShapeIndex].m_state       = eTestShapeState::GRABBED;
                m_testShapes[m_grabbedShapeIndex].m_targetColor = Rgba8::RED;
                // m_grabbedShapeIndex     = closestIndex;
                m_grabbedShapeText = "LMB=release object";
            }
            else if (m_testShapes[m_grabbedShapeIndex].m_state == eTestShapeState::GRABBED)
            {
                m_testShapes[m_grabbedShapeIndex].m_state       = eTestShapeState::IDLE;
                m_testShapes[m_grabbedShapeIndex].m_targetColor = Rgba8::WHITE;
                m_grabbedShapeIndex                             = -1;
                m_grabbedShapeText                              = "LMB=grab object";
            }
        }

        m_grabbedShapeCameraSpaceStartPosition = m_worldCamera->GetWorldToCameraTransform().TransformPosition3D(m_testShapes[m_grabbedShapeIndex].m_centerPosition);
    }


    if (g_input->WasKeyJustPressed(KEYCODE_SPACE))
    {
        if (m_storedRay == nullptr)
        {
            m_storedRay         = new Ray3(m_worldCamera->GetPosition(), m_worldCamera->GetPosition() + forwardNormal * 20.f);
            m_raycastResultText = "space=unlock raycast; ";
        }
        else
        {
            GAME_SAFE_RELEASE(m_storedRay);
            m_raycastResultText = "space=lock raycast; ";
        }
    }
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::UpdateFromController(float const deltaSeconds)
{
    UNUSED(deltaSeconds)
}

void GameShapes3D::UpdateShapes()
{
    for (int i = 0; i < 25; i++)
    {
        if (m_testShapes[i].m_state == eTestShapeState::GRABBED)
        {
            Mat44 cameraToWorld = m_worldCamera->GetCameraToWorldTransform();

            m_testShapes[i].m_centerPosition = cameraToWorld.TransformPosition3D(m_grabbedShapeCameraSpaceStartPosition);
        }
    }

    std::vector isOverlappingArray(25, false);

    for (int i = 0; i < 25; i++)
    {
        for (int j = 0; j < 25; j++)
        {
            if (j == i)
            {
                continue;
            }

            bool         isOverlapping = false;
            TestShape3D& shapeA        = m_testShapes[i];
            TestShape3D& shapeB        = m_testShapes[j];

            AABB3 aabb3A             = AABB3(shapeA.m_centerPosition - Vec3::ONE, shapeA.m_centerPosition + Vec3::ONE);
            AABB3 aabb3B             = AABB3(shapeB.m_centerPosition - Vec3::ONE, shapeB.m_centerPosition + Vec3::ONE);
            Mat44 orientationMatrixA = shapeA.m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
            Mat44 orientationMatrixB = shapeB.m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
            OBB3  obb3A              = OBB3(shapeA.m_centerPosition, shapeA.m_halfDimensions, orientationMatrixA.GetIBasis3D(), orientationMatrixA.GetJBasis3D(), orientationMatrixA.GetKBasis3D());
            OBB3  obb3B              = OBB3(shapeB.m_centerPosition, shapeB.m_halfDimensions, orientationMatrixB.GetIBasis3D(), orientationMatrixB.GetJBasis3D(), orientationMatrixB.GetKBasis3D());

            // AABB3 vs. AABB3
            if (shapeA.m_type == eTestShapeType::AABB3 &&
                shapeB.m_type == eTestShapeType::AABB3)
            {
                isOverlapping = DoAABB3sOverlap3D(aabb3A, aabb3B);
            }

            // Sphere3 vs. Sphere3
            else if (shapeA.m_type == eTestShapeType::SPHERE3 &&
                shapeB.m_type == eTestShapeType::SPHERE3)
            {
                isOverlapping = DoSpheresOverlap3D(shapeA.m_centerPosition, shapeA.m_radius, shapeB.m_centerPosition, shapeB.m_radius);
            }

            // Cylinder3 vs. Cylinder3
            else if (shapeA.m_type == eTestShapeType::CYLINDER3 &&
                shapeB.m_type == eTestShapeType::CYLINDER3)
            {
                Vec2 const       cylinderACenterXY = Vec2(shapeA.m_centerPosition.x, shapeA.m_centerPosition.y);
                Vec2 const       cylinderBCenterXY = Vec2(shapeB.m_centerPosition.x, shapeB.m_centerPosition.y);
                FloatRange const cylinderAMinMaxZ  = FloatRange(shapeA.m_centerPosition.z - 1.0f, shapeA.m_centerPosition.z + 1.f);
                FloatRange const cylinderBMinMaxZ  = FloatRange(shapeB.m_centerPosition.z - 1.0f, shapeB.m_centerPosition.z + 1.f);

                isOverlapping = DoZCylindersOverlap3D(cylinderACenterXY, shapeA.m_radius, cylinderAMinMaxZ, cylinderBCenterXY, shapeB.m_radius, cylinderBMinMaxZ);
            }

            // Sphere3 vs. AABB3
            else if (shapeA.m_type == eTestShapeType::SPHERE3 &&
                shapeB.m_type == eTestShapeType::AABB3)
            {
                isOverlapping = DoSphereAndAABB3Overlap3D(shapeA.m_centerPosition, shapeA.m_radius, aabb3B);
            }

            // Sphere3 vs. Cylinder3
            else if (shapeA.m_type == eTestShapeType::SPHERE3 &&
                shapeB.m_type == eTestShapeType::CYLINDER3)
            {
                Vec2 const       cylinderCenterXY = Vec2(shapeB.m_centerPosition.x, shapeB.m_centerPosition.y);
                FloatRange const cylinderMinMaxZ  = FloatRange(shapeB.m_centerPosition.z - 1.0f, shapeB.m_centerPosition.z + 1.f);

                isOverlapping = DoSphereAndZCylinderOverlap3D(shapeA.m_centerPosition, shapeA.m_radius, cylinderCenterXY, shapeB.m_radius, cylinderMinMaxZ);
            }

            // AABB3 vs. Cylinder3
            else if (shapeA.m_type == eTestShapeType::AABB3 &&
                shapeB.m_type == eTestShapeType::CYLINDER3)
            {
                Vec2 const       cylinderCenterXY = Vec2(shapeB.m_centerPosition.x, shapeB.m_centerPosition.y);
                FloatRange const cylinderMinMaxZ  = FloatRange(shapeB.m_centerPosition.z - 1.0f, shapeB.m_centerPosition.z + 1.f);

                isOverlapping = DoAABB3AndZCylinderOverlap3D(aabb3A, cylinderCenterXY, shapeB.m_radius, cylinderMinMaxZ);
            }

            // Sphere3 vs. OBB3
            else if (shapeA.m_type == eTestShapeType::SPHERE3 &&
                shapeB.m_type == eTestShapeType::OBB3)
            {
                isOverlapping = DoSphereAndOBB3Overlap3D(shapeA.m_centerPosition, shapeA.m_radius, obb3B);
            }
            // Sphere3 vs. Plane3
            else if (shapeA.m_type == eTestShapeType::SPHERE3 &&
                shapeB.m_type == eTestShapeType::PLANE3)
            {
                Plane3 plane  = Plane3(shapeB.m_centerPosition.GetNormalized(), shapeB.m_distanceFromOrigin);
                isOverlapping = DoSphereAndPlaneOverlap3D(shapeA.m_centerPosition, shapeA.m_radius, plane);
            }
            // AABB3 vs. Plane3
            else if (shapeA.m_type == eTestShapeType::AABB3 &&
                shapeB.m_type == eTestShapeType::PLANE3)
            {
                Plane3 plane  = Plane3(shapeB.m_centerPosition.GetNormalized(), shapeB.m_distanceFromOrigin);
                isOverlapping = DoAABB3AndPlane3Overlap3D(aabb3A, plane);
            }
            // OBB3 vs. Plane3
            else if (shapeA.m_type == eTestShapeType::OBB3 &&
                shapeB.m_type == eTestShapeType::PLANE3)
            {
                Plane3 plane  = Plane3(shapeB.m_centerPosition.GetNormalized(), shapeB.m_distanceFromOrigin);
                isOverlapping = DoOBB3AndPlane3Overlap3D(obb3A, plane);
            }

            if (isOverlapping)
            {
                isOverlappingArray[i] = true;
                isOverlappingArray[j] = true;
            }
        }
    }

    for (int i = 0; i < 25; i++)
    {
        TestShape3D& shape = m_testShapes[i];

        if (isOverlappingArray[i])
        {
            float const time         = static_cast<float>(m_gameClock->GetTotalSeconds());
            float const colorValue   = (sinf(time * 10.f) + 1.0f) * 0.5f;
            Rgba8 const currentColor = Interpolate(shape.m_targetColor, Rgba8::BLACK, colorValue);
            shape.m_currentColor.r   = static_cast<unsigned char>(GetClamped(currentColor.r + 50, 0, 255));
            shape.m_currentColor.g   = static_cast<unsigned char>(GetClamped(currentColor.g + 50, 0, 255));
            shape.m_currentColor.b   = static_cast<unsigned char>(GetClamped(currentColor.b + 50, 0, 255));
            shape.m_currentColor.a   = static_cast<unsigned char>(GetClamped(currentColor.a - 100, 0, 255));
        }
        else
        {
            shape.m_currentColor = shape.m_targetColor;
        }
    }

    Vec3       forwardNormal = m_worldCamera->GetOrientation().GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D().GetNormalized();
    Ray3 const ray           = Ray3(m_worldCamera->GetPosition(), m_worldCamera->GetPosition() + forwardNormal * 20.f);

    float closestDistance = FLOAT_MAX;
    int   closestIndex    = -1;

    std::vector isHoveringArray(25, false);

    for (int i = 0; i < 25; i++)
    {
        TestShape3D testShape         = m_testShapes[i];
        AABB3       aabb3             = AABB3(testShape.m_centerPosition - Vec3::ONE, testShape.m_centerPosition + Vec3::ONE);
        Sphere3     sphere3           = Sphere3(testShape.m_centerPosition, testShape.m_radius);
        Cylinder3   cylinder3         = Cylinder3(testShape.m_centerPosition - Vec3::Z_BASIS, testShape.m_centerPosition + Vec3::Z_BASIS, testShape.m_radius);
        Mat44       orientationMatrix = testShape.m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
        OBB3        obb3              = OBB3(testShape.m_centerPosition, testShape.m_halfDimensions, orientationMatrix.GetIBasis3D(), orientationMatrix.GetJBasis3D(), orientationMatrix.GetKBasis3D());
        Plane3      plane3            = Plane3(testShape.m_centerPosition.GetNormalized(), testShape.m_distanceFromOrigin);

        if (m_storedRay != nullptr)
        {
            continue;
        }

        RaycastResult3D result;

        if (testShape.m_type == eTestShapeType::AABB3)
        {
            result = RaycastVsAABB3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, aabb3.m_mins, aabb3.m_maxs);
        }
        else if (testShape.m_type == eTestShapeType::SPHERE3)
        {
            result = RaycastVsSphere3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, sphere3.m_centerPosition, sphere3.m_radius);
        }
        else if (testShape.m_type == eTestShapeType::CYLINDER3)
        {
            result = RaycastVsCylinderZ3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, cylinder3.GetCenterPositionXY(), cylinder3.GetFloatRange(), cylinder3.m_radius);
        }
        else if (testShape.m_type == eTestShapeType::OBB3)
        {
            result = RaycastVsOBB3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, obb3);
        }
        else if (testShape.m_type == eTestShapeType::PLANE3)
        {
            result = RaycastVsPlane3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, plane3);
        }

        if (result.m_didImpact == true && result.m_impactLength < closestDistance)
        {
            closestDistance = result.m_impactLength;
            closestIndex    = i;
        }
    }

    if (closestIndex != -1)
    {
        isHoveringArray[closestIndex] = true;
    }

    if (m_grabbedShapeIndex == -1 &&
        closestIndex == -1)
    {
        m_grabbedShapeText = "";
    }

    for (int i = 0; i < 25; i++)
    {
        if (isHoveringArray[i] == false)
        {
            if (m_testShapes[i].m_state == eTestShapeState::GRABBED)
            {
                m_testShapes[i].m_targetColor = Rgba8::RED;
            }
            else
            {
                m_testShapes[i].m_targetColor = Rgba8::WHITE;
            }
        }
        else
        {
            if (m_testShapes[i].m_state == eTestShapeState::GRABBED)
            {
                m_testShapes[i].m_targetColor = Rgba8::RED;
            }
            else if (m_grabbedShapeIndex == -1)
            {
                m_testShapes[i].m_targetColor = Rgba8::BLUE;
                m_grabbedShapeText            = "LMB=grab object";
            }
        }
    }
}

void GameShapes3D::RenderRaycastResult() const
{
    VertexList_PCU  raycastResultVerts;
    Vec3 const      forwardNormal = m_worldCamera->GetOrientation().GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D().GetNormalized();
    Ray3 const      ray           = Ray3(m_worldCamera->GetPosition(), m_worldCamera->GetPosition() + forwardNormal * 20.f);
    RaycastResult3D closestResult;
    float           minLengthSquared = FLOAT_MAX;
    bool            hasValidImpact   = false;

    for (int i = 0; i < 25; i++)
    {
        TestShape3D testShape         = m_testShapes[i];
        AABB3       aabb3             = AABB3(testShape.m_centerPosition - Vec3::ONE, testShape.m_centerPosition + Vec3::ONE);
        Sphere3     sphere3           = Sphere3(testShape.m_centerPosition, testShape.m_radius);
        Cylinder3   cylinder3         = Cylinder3(testShape.m_centerPosition - Vec3::Z_BASIS, testShape.m_centerPosition + Vec3::Z_BASIS, testShape.m_radius);
        Mat44       orientationMatrix = testShape.m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
        OBB3        obb3              = OBB3(testShape.m_centerPosition, testShape.m_halfDimensions, orientationMatrix.GetIBasis3D(), orientationMatrix.GetJBasis3D(), orientationMatrix.GetKBasis3D());
        Plane3      plane3            = Plane3(testShape.m_centerPosition.GetNormalized(), testShape.m_distanceFromOrigin);

        if (m_storedRay != nullptr)
        {
            continue;
        }

        RaycastResult3D result;

        if (testShape.m_type == eTestShapeType::AABB3)
        {
            result = RaycastVsAABB3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, aabb3.m_mins, aabb3.m_maxs);
        }
        else if (testShape.m_type == eTestShapeType::SPHERE3)
        {
            result = RaycastVsSphere3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, sphere3.m_centerPosition, sphere3.m_radius);
        }
        else if (testShape.m_type == eTestShapeType::CYLINDER3)
        {
            result = RaycastVsCylinderZ3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, cylinder3.GetCenterPositionXY(), cylinder3.GetFloatRange(), cylinder3.m_radius);
        }
        else if (testShape.m_type == eTestShapeType::OBB3)
        {
            result = RaycastVsOBB3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, obb3);
        }
        else if (testShape.m_type == eTestShapeType::PLANE3)
        {
            result = RaycastVsPlane3D(ray.m_startPosition, ray.m_forwardNormal, ray.m_maxLength, plane3);
        }

        float const lengthSquared = (result.m_impactPosition - m_worldCamera->GetPosition()).GetLengthSquared();

        if (result.m_didImpact)
        {
            if (lengthSquared < minLengthSquared)
            {
                minLengthSquared = lengthSquared;
                closestResult    = result;
                hasValidImpact   = true;
            }
        }
    }

    if (hasValidImpact)
    {
        AddVertsForArrow3D(raycastResultVerts, closestResult.m_impactPosition, closestResult.m_impactPosition + closestResult.m_impactNormal, 0.8f, 0.03f, 0.06f, Rgba8::YELLOW);
        AddVertsForSphere3D(raycastResultVerts, closestResult.m_impactPosition, 0.1f);
    }

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(raycastResultVerts.size()), raycastResultVerts.data());
}

void GameShapes3D::RenderNearestPoint() const
{
    VertexList_PCU nearestPointVerts;
    Vec3           nearestPoint;
    Vec3           closestNearestPoint;
    float          minLengthSquared = FLOAT_MAX;

    for (int i = 0; i < 25; i++)
    {
        TestShape3D testShape         = m_testShapes[i];
        AABB3       aabb3             = AABB3(testShape.m_centerPosition - Vec3::ONE, testShape.m_centerPosition + Vec3::ONE);
        Sphere3     sphere3           = Sphere3(testShape.m_centerPosition, testShape.m_radius);
        Cylinder3   cylinder3         = Cylinder3(testShape.m_centerPosition - Vec3::Z_BASIS, testShape.m_centerPosition + Vec3::Z_BASIS, testShape.m_radius);
        Mat44       orientationMatrix = testShape.m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
        OBB3        obb3              = OBB3(testShape.m_centerPosition, testShape.m_halfDimensions, orientationMatrix.GetIBasis3D(), orientationMatrix.GetJBasis3D(), orientationMatrix.GetKBasis3D());
        Plane3      plane3            = Plane3(testShape.m_centerPosition.GetNormalized(), testShape.m_distanceFromOrigin);

        if (testShape.m_type == eTestShapeType::AABB3)
        {
            nearestPoint = aabb3.GetNearestPoint(m_worldCamera->GetPosition());
        }
        else if (testShape.m_type == eTestShapeType::SPHERE3)
        {
            nearestPoint = sphere3.GetNearestPoint(m_worldCamera->GetPosition());
        }
        else if (testShape.m_type == eTestShapeType::CYLINDER3)
        {
            nearestPoint = cylinder3.GetNearestPoint(m_worldCamera->GetPosition());
        }
        else if (testShape.m_type == eTestShapeType::OBB3)
        {
            nearestPoint = obb3.GetNearestPoint(m_worldCamera->GetPosition());
        }
        else if (testShape.m_type == eTestShapeType::PLANE3)
        {
            nearestPoint = plane3.GetNearestPoint(m_worldCamera->GetPosition());
        }

        float const lengthSquared = (nearestPoint - m_worldCamera->GetPosition()).GetLengthSquared();

        if (lengthSquared < minLengthSquared)
        {
            minLengthSquared    = lengthSquared;
            closestNearestPoint = nearestPoint;
        }

        // Nearest point on every test shapes.
        AddVertsForSphere3D(nearestPointVerts, nearestPoint, 0.1f, Rgba8::ORANGE);
    }

    // Closest nearest point.
    AddVertsForSphere3D(nearestPointVerts, closestNearestPoint, 0.1f, Rgba8::GREEN);

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(nearestPointVerts.size()), nearestPointVerts.data());
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::RenderStoredRaycastResult() const
{
    if (m_storedRay == nullptr) return;

    VertexList_PCU  storedRaycastResultVerts;
    RaycastResult3D closestResult;
    float           minLengthSquared = FLOAT_MAX;
    bool            hasValidImpact   = false;

    for (int i = 0; i < 25; i++)
    {
        TestShape3D testShape         = m_testShapes[i];
        AABB3       aabb3             = AABB3(testShape.m_centerPosition - Vec3::ONE, testShape.m_centerPosition + Vec3::ONE);
        Sphere3     sphere3           = Sphere3(testShape.m_centerPosition, testShape.m_radius);
        Cylinder3   cylinder3         = Cylinder3(testShape.m_centerPosition - Vec3::Z_BASIS, testShape.m_centerPosition + Vec3::Z_BASIS, testShape.m_radius);
        Mat44       orientationMatrix = testShape.m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
        OBB3        obb3              = OBB3(testShape.m_centerPosition, testShape.m_halfDimensions, orientationMatrix.GetIBasis3D(), orientationMatrix.GetJBasis3D(), orientationMatrix.GetKBasis3D());
        Plane3      plane3            = Plane3(testShape.m_centerPosition.GetNormalized(), testShape.m_distanceFromOrigin);

        RaycastResult3D result;

        if (testShape.m_type == eTestShapeType::AABB3)
        {
            result = RaycastVsAABB3D(m_storedRay->m_startPosition, m_storedRay->m_forwardNormal, m_storedRay->m_maxLength, aabb3.m_mins, aabb3.m_maxs);
        }
        else if (testShape.m_type == eTestShapeType::SPHERE3)
        {
            result = RaycastVsSphere3D(m_storedRay->m_startPosition, m_storedRay->m_forwardNormal, m_storedRay->m_maxLength, sphere3.m_centerPosition, sphere3.m_radius);
        }
        else if (testShape.m_type == eTestShapeType::CYLINDER3)
        {
            result = RaycastVsCylinderZ3D(m_storedRay->m_startPosition, m_storedRay->m_forwardNormal, m_storedRay->m_maxLength, cylinder3.GetCenterPositionXY(), cylinder3.GetFloatRange(), cylinder3.m_radius);
        }
        else if (testShape.m_type == eTestShapeType::OBB3)
        {
            result = RaycastVsOBB3D(m_storedRay->m_startPosition, m_storedRay->m_forwardNormal, m_storedRay->m_maxLength, obb3);
        }
        else if (testShape.m_type == eTestShapeType::PLANE3)
        {
            result = RaycastVsPlane3D(m_storedRay->m_startPosition, m_storedRay->m_forwardNormal, m_storedRay->m_maxLength, plane3);
        }

        if (result.m_didImpact)
        {
            float lengthSquared = (result.m_impactPosition - m_storedRay->m_startPosition).GetLengthSquared();
            if (lengthSquared < minLengthSquared)
            {
                minLengthSquared = lengthSquared;
                closestResult    = result;
                hasValidImpact   = true;
            }
        }
    }

    if (hasValidImpact)
    {
        AddVertsForArrow3D(storedRaycastResultVerts, m_storedRay->m_startPosition, closestResult.m_impactPosition, 0.8f, 0.03f, 0.06f, Rgba8::RED);
        AddVertsForArrow3D(storedRaycastResultVerts, closestResult.m_impactPosition, closestResult.m_impactPosition + closestResult.m_impactNormal, 0.8f, 0.03f, 0.06f, Rgba8::YELLOW);
        AddVertsForArrow3D(storedRaycastResultVerts, closestResult.m_impactPosition, m_storedRay->m_startPosition + m_storedRay->m_forwardNormal * m_storedRay->m_maxLength, 0.8f, 0.03f, 0.06f, Rgba8::GREY);
        AddVertsForSphere3D(storedRaycastResultVerts, closestResult.m_impactPosition, 0.1f);
    }
    else
    {
        AddVertsForArrow3D(storedRaycastResultVerts, m_storedRay->m_startPosition, m_storedRay->m_startPosition + m_storedRay->m_forwardNormal * m_storedRay->m_maxLength, 0.8f, 0.03f, 0.06f, Rgba8::GREEN);
    }

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(storedRaycastResultVerts.size()), storedRaycastResultVerts.data());
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::RenderShapes() const
{
    RenderRaycastResult();
    RenderNearestPoint();
    RenderStoredRaycastResult();
    VertexList_PCU outsideVerts;
    VertexList_PCU insideVerts;

    for (int i = 0; i < 25; i++)
    {
        TestShape3D testShape         = m_testShapes[i];
        AABB3       aabb3             = AABB3(testShape.m_centerPosition - Vec3::ONE, testShape.m_centerPosition + Vec3::ONE);
        Sphere3     sphere3           = Sphere3(testShape.m_centerPosition, testShape.m_radius);
        Cylinder3   cylinder3         = Cylinder3(testShape.m_centerPosition - Vec3::Z_BASIS, testShape.m_centerPosition + Vec3::Z_BASIS, testShape.m_radius);
        Mat44       orientationMatrix = testShape.m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
        OBB3        obb3              = OBB3(testShape.m_centerPosition, testShape.m_halfDimensions, orientationMatrix.GetIBasis3D(), orientationMatrix.GetJBasis3D(), orientationMatrix.GetKBasis3D());
        Plane3      plane3            = Plane3(testShape.m_centerPosition.GetNormalized(), testShape.m_distanceFromOrigin);

        if (testShape.m_type == eTestShapeType::AABB3)
        {
            if (IsPointInsideAABB3D(m_worldCamera->GetPosition(), aabb3.m_mins, aabb3.m_maxs))
            {
                AddVertsForWireframeAABB3D(insideVerts, aabb3, 0.05f, testShape.m_currentColor);
            }
            else
            {
                AddVertsForAABB3D(outsideVerts, aabb3, testShape.m_currentColor);
            }
        }

        if (testShape.m_type == eTestShapeType::SPHERE3)
        {
            if (IsPointInsideSphere3D(m_worldCamera->GetPosition(), sphere3.m_centerPosition, sphere3.m_radius))
            {
                Rgba8 color = Rgba8(0, 0, testShape.m_currentColor.b, testShape.m_currentColor.a);
                AddVertsForWireframeSphere3D(insideVerts, sphere3.m_centerPosition, sphere3.m_radius, 0.05f, color);
            }
            else
            {
                AddVertsForSphere3D(outsideVerts, sphere3.m_centerPosition, sphere3.m_radius, testShape.m_currentColor);
            }
        }

        if (testShape.m_type == eTestShapeType::CYLINDER3)
        {
            if (IsPointInsideZCylinder3D(m_worldCamera->GetPosition(), cylinder3.m_startPosition, cylinder3.m_endPosition, cylinder3.m_radius))
            {
                AddVertsForWireframeCylinder3D(insideVerts, cylinder3.m_startPosition, cylinder3.m_endPosition, cylinder3.m_radius, 0.05f, testShape.m_currentColor, AABB2(Vec2::ZERO, Vec2::ONE));
            }
            else
            {
                AddVertsForCylinder3D(outsideVerts, cylinder3.m_startPosition, cylinder3.m_endPosition, cylinder3.m_radius, testShape.m_currentColor, AABB2(Vec2::ZERO, Vec2::ONE));
            }
        }

        if (testShape.m_type == eTestShapeType::OBB3)
        {
            if (IsPointInsideOBB3D(m_worldCamera->GetPosition(), obb3))
            {
                AddVertsForWireframeOBB3D(insideVerts, obb3, testShape.m_currentColor);
            }
            {
                AddVertsForOBB3D(outsideVerts, obb3, testShape.m_currentColor);
            }
        }
        if (testShape.m_type == eTestShapeType::PLANE3)
        {
            Vec3 planeCenterPosition = plane3.GetOriginPoint();
            Vec3 left, up;
            plane3.m_normal.GetOrthonormalBasis(plane3.m_normal, &left, &up);

            float gridLineLength = 20.f;

            for (int j = -(int)gridLineLength / 2; j < (int)gridLineLength / 2; j++)
            {
                float lineWidth = 0.01f;
                if (j == 0) lineWidth = 0.05f;

                Vec3 obb3PositionX = planeCenterPosition + (float)j * left;
                Vec3 obb3PositionY = planeCenterPosition + (float)j * up;
                OBB3 boundsX       = OBB3(obb3PositionX, Vec3(lineWidth, lineWidth, gridLineLength / 2.f), plane3.m_normal, left, up);
                OBB3 boundsY       = OBB3(obb3PositionY, Vec3(lineWidth, gridLineLength / 2.f, lineWidth), plane3.m_normal, left, up);


                AddVertsForOBB3D(insideVerts, boundsX, Rgba8::RED);
                AddVertsForOBB3D(insideVerts, boundsY, Rgba8::GREEN);
            }
        }

        g_theRenderer->SetBlendMode(eBlendMode::OPAQUE);
        g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);
        g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
        g_theRenderer->SetDepthMode(eDepthMode::READ_WRITE_LESS_EQUAL);
        g_theRenderer->BindTexture(m_texture);
        g_theRenderer->DrawVertexArray(static_cast<int>(outsideVerts.size()), outsideVerts.data());

        g_theRenderer->BindTexture(nullptr);
        g_theRenderer->DrawVertexArray(static_cast<int>(insideVerts.size()), insideVerts.data());
    }
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::RenderPlayerBasis() const
{
    VertexList_PCU verts;

    Vec3 const worldCameraPosition = m_worldCamera->GetPosition();
    Vec3 const forwardNormal       = m_worldCamera->GetOrientation().GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D().GetNormalized();

    // Add vertices in world space.
    AddVertsForArrow3D(verts, worldCameraPosition + forwardNormal, worldCameraPosition + forwardNormal + Vec3::X_BASIS * 0.1f, 0.8f, 0.001f, 0.003f, Rgba8::RED);
    AddVertsForArrow3D(verts, worldCameraPosition + forwardNormal, worldCameraPosition + forwardNormal + Vec3::Y_BASIS * 0.1f, 0.8f, 0.001f, 0.003f, Rgba8::GREEN);
    AddVertsForArrow3D(verts, worldCameraPosition + forwardNormal, worldCameraPosition + forwardNormal + Vec3::Z_BASIS * 0.1f, 0.8f, 0.001f, 0.003f, Rgba8::BLUE);

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::OPAQUE);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_BACK);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void GameShapes3D::GenerateRandomShapes()
{
    for (int i = 0; i < 25; i++)
    {
        int const   randomNum                = g_theRNG->RollRandomIntInRange(0, 4);
        float const randomRadius             = g_theRNG->RollRandomFloatInRange(1.f, 3.f);
        float const randomDistanceFromOrigin = g_theRNG->RollRandomFloatInRange(10.f, 30.f);

        m_testShapes[i].m_centerPosition     = RollVec3InRange(FloatRange(-10.f, 10.f), FloatRange(-10.f, 10.f), FloatRange(-10.f, 10.f));
        m_testShapes[i].m_orientation        = RollEulerAngleInRange(FloatRange(0.f, 360.f), FloatRange(0.f, 360.f), FloatRange(0.f, 360.f));
        m_testShapes[i].m_radius             = randomRadius;
        m_testShapes[i].m_distanceFromOrigin = randomDistanceFromOrigin;
        m_testShapes[i].m_halfDimensions     = RollVec3InRange(FloatRange(1.f, 3.f), FloatRange(1.f, 3.f), FloatRange(1.f, 3.f));

        switch (randomNum)
        {
        case 0: m_testShapes[i].m_type = eTestShapeType::AABB3;
            break;
        case 1: m_testShapes[i].m_type = eTestShapeType::SPHERE3;
            break;
        case 2: m_testShapes[i].m_type = eTestShapeType::CYLINDER3;
            break;
        case 3: m_testShapes[i].m_type = eTestShapeType::OBB3;
            break;
        case 4: m_testShapes[i].m_type = eTestShapeType::PLANE3;
            break;
        default: ;
        }
    }
}

//----------------------------------------------------------------------------------------------------
Vec3 GameShapes3D::RollVec3InRange(FloatRange const& rangeX,
                                   FloatRange const& rangeY,
                                   FloatRange const& rangeZ) const
{
    float const x = g_theRNG->RollRandomFloatInRange(rangeX.m_min, rangeX.m_max);
    float const y = g_theRNG->RollRandomFloatInRange(rangeY.m_min, rangeY.m_max);
    float const z = g_theRNG->RollRandomFloatInRange(rangeZ.m_min, rangeZ.m_max);

    return Vec3(x, y, z);
}

//----------------------------------------------------------------------------------------------------
EulerAngles GameShapes3D::RollEulerAngleInRange(FloatRange const& rangeX,
                                                FloatRange const& rangeY,
                                                FloatRange const& rangeZ) const
{
    float const x = g_theRNG->RollRandomFloatInRange(rangeX.m_min, rangeX.m_max);
    float const y = g_theRNG->RollRandomFloatInRange(rangeY.m_min, rangeY.m_max);
    float const z = g_theRNG->RollRandomFloatInRange(rangeZ.m_min, rangeZ.m_max);

    return EulerAngles(x, y, z);
}
