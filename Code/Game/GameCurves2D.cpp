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

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);

    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));
    m_worldCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));

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
    startPos             = Vec2(50.f, 50.f);
    guide1               = Vec2(100.f, 200.f);
    guide2               = Vec2(100.f, 400.f);
    endPos               = Vec2(1000.f, 500.f);
    m_cubicBezierCurve2D = CubicBezierCurve2D(startPos, guide1, guide2, endPos);
    // m_cubicHermiteCurve2D = CubicHermiteCurve2D(startPos, guide1, endPos, guide2);
    m_cubicHermiteCurve2D = CubicHermiteCurve2D(m_cubicBezierCurve2D);

    std::vector<Vec2> controlPoints = {
        Vec2(0.f, 0.f),
        Vec2(100.f, 200.f),
        Vec2(200.f, 100.f),
        Vec2(300.f, 300.f),
        Vec2(400.f, 250.f)
    };

    m_catmullRomSpline2D = CatmullRomSpline2D(controlPoints);
}

void GameCurves2D::UpdateShapes()
{
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::Update()
{
    g_theInput->SetCursorMode(CursorMode::POINTER);

    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    DebugAddScreenText(Stringf("Time: %.2f\nFPS: %.2f\nScale: %.1f", m_gameClock->GetTotalSeconds(), 1.f / m_gameClock->GetDeltaSeconds(), m_gameClock->GetTimeScale()), m_screenCamera->GetOrthographicTopRight() - Vec2(250.f, 60.f), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);
    UpdateFromKeyboard(deltaSeconds);
    UpdateFromController(deltaSeconds);

    if (g_theInput->WasKeyJustPressed(NUMCODE_6))
    {
        DebugAddWorldWireCylinder(m_worldCamera->GetPosition(), m_worldCamera->GetPosition() + Vec3::Z_BASIS * 2, 1.f, 10.f, Rgba8::WHITE, Rgba8::RED);
    }

    UpdateShapes();
    // m_worldCamera->SetPositionAndOrientation(m_player.m_startPosition, m_player.m_orientation);
}

void GameCurves2D::RenderShapes() const
{
    VertexList_PCU verts;

    std::vector<Vec2> bezierPoints;
    std::vector<Vec2> hermitePoints;
    int               numSteps = 32;
    for (int i = 0; i <= numSteps; ++i)
    {
        float t = (float)i / (float)numSteps;
        bezierPoints.push_back(m_cubicBezierCurve2D.EvaluateAtParametric(t));
        hermitePoints.push_back((m_cubicHermiteCurve2D.EvaluateAtParametric(t)));
    }

    // 設定參數
    int   numSubdivisions = 64;
    float thickness       = 3.f;
    Rgba8 color           = Rgba8::CYAN;

    // m_catmullRomSpline2D.AddVertsForCurve2D(verts, thickness, color, numSubdivisions);

    for (int i = 0; i < numSteps; ++i)
    {
        AddVertsForLineSegment2D(verts, bezierPoints[i], bezierPoints[i + 1], 3.f, false, Rgba8::WHITE);
    }

    // AddVertsForDisc2D(verts, m_cubicBezierCurve2D.m_startPosition, 5.f, Rgba8::RED);
    // AddVertsForDisc2D(verts, m_cubicBezierCurve2D.m_guidePosition1, 5.f, Rgba8::YELLOW);
    // AddVertsForDisc2D(verts, m_cubicBezierCurve2D.m_guidePosition2, 5.f, Rgba8::GREEN);
    // AddVertsForDisc2D(verts, m_cubicBezierCurve2D.m_endPosition, 5.f, Rgba8::BLUE);

    // for (int i = 0; i < numSteps; ++i)
    // {
    //     AddVertsForLineSegment2D(verts, hermitePoints[i], hermitePoints[i + 1], 3.f, false, Rgba8::WHITE);
    // }
    //
    // AddVertsForDisc2D(verts, m_cubicHermiteCurve2D.m_startPos, 5.f, Rgba8::RED);
    // AddVertsForArrow2D(verts, m_cubicHermiteCurve2D.m_startPos, m_cubicHermiteCurve2D.m_startPos+ m_cubicHermiteCurve2D.m_startVel, 5.f, 3.f, Rgba8::YELLOW);
    // AddVertsForArrow2D(verts, m_cubicHermiteCurve2D.m_endPos, m_cubicHermiteCurve2D.m_endPos + m_cubicHermiteCurve2D.m_endVel, 5.f, 3.f, Rgba8::GREEN);
    // AddVertsForDisc2D(verts, m_cubicHermiteCurve2D.m_endPos, 5.f, Rgba8::BLUE);
    // int   numSubdivisions = 64;
    float curveLength      = m_cubicBezierCurve2D.GetApproximateLength(numSubdivisions);
    float time             = fmod((float)m_gameClock->GetTotalSeconds(), 2.f);  // 2秒loop
    float normalizedTime   = time / 2.f;

    // ------ 白色點（parametric） ------
    Vec2 whiteDot = m_cubicBezierCurve2D.EvaluateAtParametric(normalizedTime);
    AddVertsForDisc2D(verts, whiteDot, 8.f, Rgba8::WHITE);

    // ------ 綠色點（distance-based） ------
    float speed             = curveLength / 2.f; // 要2秒走完整條曲線
    float distanceAlongPath = speed * time;
    Vec2 greenDot           = m_cubicBezierCurve2D.EvaluateAtApproximateDistance(distanceAlongPath, numSubdivisions);
    AddVertsForDisc2D(verts, greenDot, 8.f, Rgba8::GREEN);


    // AddVertsForDisc2D(verts, m_startPos, 10.f, Rgba8::WHITE);
    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::Render() const
{
    //-Start-of-World-Camera--------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_worldCamera);

    RenderShapes();
    // RenderPlayerBasis();

    g_theRenderer->EndCamera(*m_worldCamera);

    //-End-of-World-Camera----------------------------------------------------------------------------
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

    if (g_theInput->IsKeyDown(KEYCODE_W)) m_cubicBezierCurve2D.m_guidePosition1 += Vec2(0.1f, 0.1f);
    if (g_theInput->IsKeyDown(KEYCODE_S)) m_cubicBezierCurve2D.m_guidePosition2 += Vec2(0.1f, 0.1f);
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::UpdateFromController(float const deltaSeconds)
{
    UNUSED(deltaSeconds)
}
