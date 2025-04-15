//----------------------------------------------------------------------------------------------------
// GameCurves2D.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameCurves2D.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
// #include "Engine/Core/ErrorWarningAssert.hpp"
// #include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
// #include "Engine/Math/AABB3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"

std::vector<EasingFunctionInfo> GameCurves2D::s_easingFunctions =
{
    {"SmoothStart2", [](float t) { return SmoothStart2(t); }},
    {"SmoothStart3", [](float t) { return SmoothStart3(t); }},
    {"SmoothStop2", [](float t) { return SmoothStop2(t); }},
    {"SmoothStop3", [](float t) { return SmoothStop3(t); }},
    {"SmoothStep3", [](float t) { return SmoothStep3(t); }},
    {"SmoothStep5", [](float t) { return SmoothStep5(t); }},
};

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

    GenerateAABB2s();
    GenerateEaseFunction();
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::Update()
{
    g_theInput->SetCursorMode(CursorMode::POINTER);

    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    DebugAddScreenText(Stringf("Time: %.2f\nFPS: %.2f\nScale: %.1f", m_gameClock->GetTotalSeconds(), 1.f / m_gameClock->GetDeltaSeconds(), m_gameClock->GetTimeScale()), m_screenCamera->GetOrthographicTopRight() - Vec2(250.f, 60.f), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);
    UpdateFromKeyboard(deltaSeconds);
    UpdateFromController(deltaSeconds);

    UpdateEaseFunction();
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
    float curveLength    = m_cubicBezierCurve2D.GetApproximateLength(numSubdivisions);
    float time           = fmod((float)m_gameClock->GetTotalSeconds(), 2.f);  // 2秒loop
    float normalizedTime = time / 2.f;

    // ------ 白色點（parametric） ------
    Vec2 whiteDot = m_cubicBezierCurve2D.EvaluateAtParametric(normalizedTime);
    AddVertsForDisc2D(verts, whiteDot, 8.f, Rgba8::WHITE);

    // ------ 綠色點（distance-based） ------
    float speed             = curveLength / 2.f; // 要2秒走完整條曲線
    float distanceAlongPath = speed * time;
    Vec2  greenDot          = m_cubicBezierCurve2D.EvaluateAtApproximateDistance(distanceAlongPath, numSubdivisions);
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

    // RenderShapes();
    RenderAABB2s();
    RenderEaseFunctions();

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

    if (g_theInput->WasKeyJustPressed(KEYCODE_N)) m_easeIndex += 1;
    if (g_theInput->WasKeyJustPressed(KEYCODE_M)) m_easeIndex -= 1;
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::UpdateFromController(float const deltaSeconds)
{
    UNUSED(deltaSeconds)
}

void GameCurves2D::UpdateEaseFunction()
{
    // float t = fmod((float)m_gameClock->GetTotalSeconds(), 2.f)/2.f;
    // float easedT = SmoothStart2(t);
    // easeFunctionStartPosition = Interpolate(easeFunctionStartPosition,easeFunctionEndPosition, easedT);
}

void GameCurves2D::RenderEaseFunctions() const
{
    Vec2 graphSize = boundAChild.GetDimensions();

    const int numSamples = 64;
    VertexList_PCU curveVerts;
    Rgba8 curveColor = Rgba8::GREEN;

    for (int i = 0; i < numSamples - 1; ++i)
    {
        float t0 = (float)i / (float)(numSamples - 1);
        float t1 = (float)(i + 1) / (float)(numSamples - 1);

        float easedT0 = s_easingFunctions[m_easeIndex].easeFunction(t0);
        float easedT1 = s_easingFunctions[m_easeIndex].easeFunction(t1);

        // 將 (t, easedT) 映射到 graphBounds 區域
        Vec2 p0 = boundAChild.m_mins + Vec2(t0 * graphSize.x, easedT0 * graphSize.y);
        Vec2 p1 = boundAChild.m_mins + Vec2(t1 * graphSize.x, easedT1 * graphSize.y);

        AddVertsForLineSegment2D(curveVerts, p0, p1,3.f,false,Rgba8::WHITE);
    }

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(curveVerts.size()), curveVerts.data());

    std::vector<Vec2> curvePoints;
    curvePoints.reserve(numSamples);


    // 預先計算曲線點
    for (int i = 0; i < numSamples; ++i)
    {
        float tSample = (float)i / (float)(numSamples - 1);
        float easedT = s_easingFunctions[m_easeIndex].easeFunction(tSample);
        Vec2 p = boundAChild.m_mins + Vec2(tSample * graphSize.x, easedT * graphSize.y);
        curvePoints.push_back(p);
    }

    // 找到目前時間對應的曲線點
    float t = fmodf((float)m_gameClock->GetTotalSeconds(), 2.f) / 2.f;
    int currentIndex = static_cast<int>(t * (numSamples - 1));
    Vec2 position = curvePoints[currentIndex];

    // float const t = fmodf((float)m_gameClock->GetTotalSeconds(), 2.f) / 2.f;
    //
    // float const easedT = s_easingFunctions[m_easeIndex].easeFunction(t);
    //
    // Vec2 const     position = Interpolate(m_easeFunctionStartPosition, m_easeFunctionEndPosition, easedT);
    // VertexList_PCU verts;
    //
    // AABB2 bounds = AABB2(Vec2(275.f, 425.f), Vec2(550.f, 450.f));
    //
    AddVertsForDisc2D(curveVerts, position, 5.f, Rgba8::WHITE);
    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(curveVerts.size()), curveVerts.data());

    VertexList_PCU textVerts;
    g_theBitmapFont->AddVertsForTextInBox2D(textVerts, s_easingFunctions[m_easeIndex].easeFunctionName, boundAChild, 25.f);
    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(&g_theBitmapFont->GetTexture());
    g_theRenderer->DrawVertexArray(static_cast<int>(textVerts.size()), textVerts.data());
}

void GameCurves2D::GenerateAABB2s()
{
    boundA      = AABB2(Vec2(50.f, 425.f), Vec2(775.f, 750.f));
    boundAChild = AABB2(Vec2(275.f, 450.f), Vec2(550.f, 750.f));
    boundB      = AABB2(Vec2(825.f, 425.f), Vec2(1550.f, 750.f));
    boundC      = AABB2(Vec2(50.f, 50.f), Vec2(1550.f, 375.f));
}

void GameCurves2D::GenerateEaseFunction()
{
    m_easeFunctionStartPosition = Vec2(275.f, 450.f);
    m_easeFunctionEndPosition   = Vec2(550.f, 750.f);
    m_easeFunctionBound         = AABB2(m_easeFunctionStartPosition, m_easeFunctionEndPosition);
}

void GameCurves2D::RenderAABB2s() const
{
    VertexList_PCU verts;

    Rgba8 const boundsColor = Rgba8(255, 0, 0, 127);

    AddVertsForAABB2D(verts, boundA, boundsColor);
    AddVertsForAABB2D(verts, boundB, Rgba8::BLUE);
    AddVertsForAABB2D(verts, boundC, boundsColor);

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}
