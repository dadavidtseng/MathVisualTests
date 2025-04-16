//----------------------------------------------------------------------------------------------------
// GameCurves2D.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameCurves2D.hpp"

// #include <algorithm>

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"

STATIC std::vector<EasingFunctionInfo> GameCurves2D::s_easingFunctions =
{
    {"SmoothStart2", [](float const t) { return SmoothStart2(t); }},
    {"SmoothStart3", [](float const t) { return SmoothStart3(t); }},
    {"SmoothStart4", [](float const t) { return SmoothStart4(t); }},
    {"SmoothStart5", [](float const t) { return SmoothStart5(t); }},
    {"SmoothStart6", [](float const t) { return SmoothStart6(t); }},
    {"SmoothStop2", [](float const t) { return SmoothStop2(t); }},
    {"SmoothStop3", [](float const t) { return SmoothStop3(t); }},
    {"SmoothStop4", [](float const t) { return SmoothStop4(t); }},
    {"SmoothStop5", [](float const t) { return SmoothStop5(t); }},
    {"SmoothStop6", [](float const t) { return SmoothStop6(t); }},
    {"SmoothStep3", [](float const t) { return SmoothStep3(t); }},
    {"SmoothStep5", [](float const t) { return SmoothStep5(t); }},
    {"Hesitate3", [](float const t) { return Hesitate3(t); }},
    {"Hesitate5", [](float const t) { return Hesitate5(t); }},
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

    GenerateAABB2s();
    GenerateRandomShapes();
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::Update()
{
    g_theInput->SetCursorMode(CursorMode::POINTER);

    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    DebugAddScreenText(Stringf("Time: %.2f\nFPS: %.2f\nScale: %.1f", m_gameClock->GetTotalSeconds(), 1.f / m_gameClock->GetDeltaSeconds(), m_gameClock->GetTimeScale()), m_screenCamera->GetOrthographicTopRight() - Vec2(250.f, 60.f), 20.f, Vec2::ZERO, 0.f, Rgba8::WHITE, Rgba8::WHITE);
    UpdateFromKeyboard(deltaSeconds);
    UpdateFromController(deltaSeconds);
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::Render() const
{
    //-Start-of-World-Camera--------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_worldCamera);

    RenderShapes();

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
    AABB2 const currentModeTextBox(Vec2(currentControlTextBoxMinX, currentControlTextBoxMinY - 20.f), Vec2(currentControlTextBoxMaxX, currentControlTextBoxMaxY - 20.f));

    String const currentControlText = Stringf("F8 to randomize; W/E=pre/next easing function,\nN/M=curve subdivisions(%d), hold T=slow", m_numSubDivisions);
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

//----------------------------------------------------------------------------------------------------
void GameCurves2D::UpdateFromKeyboard(float deltaSeconds)
{
    if (g_theInput->WasKeyJustPressed(KEYCODE_O)) m_gameClock->StepSingleFrame();
    if (g_theInput->WasKeyJustPressed(KEYCODE_T)) m_gameClock->SetTimeScale(0.1f);
    if (g_theInput->WasKeyJustReleased(KEYCODE_T)) m_gameClock->SetTimeScale(1.f);
    if (g_theInput->WasKeyJustPressed(KEYCODE_P)) m_gameClock->TogglePause();
    if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) App::RequestQuit();
    if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
    {
        m_catmullRomSpline2D.ResetAllPoints(m_points);
        m_points.clear();
        GenerateRandomShapes();
    }

    XboxController const& controller = g_theInput->GetController(0);

    if (g_theInput->WasKeyJustPressed(KEYCODE_H) || controller.WasButtonJustPressed(XBOX_BUTTON_START))
    {
        m_worldCamera->SetPosition(Vec3::ZERO);
        m_worldCamera->SetOrientation(EulerAngles::ZERO);
    }

    if (g_theInput->IsKeyDown(KEYCODE_W)) m_cubicBezierCurve2D.m_guidePosition1 += Vec2(0.1f, 0.1f);
    if (g_theInput->IsKeyDown(KEYCODE_S)) m_cubicBezierCurve2D.m_guidePosition2 += Vec2(0.1f, 0.1f);

    if (g_theInput->WasKeyJustPressed(KEYCODE_W)) m_easeIndex++;
    if (g_theInput->WasKeyJustPressed(KEYCODE_E)) m_easeIndex--;

    int const size = static_cast<int>(s_easingFunctions.size());
    m_easeIndex    = (m_easeIndex + size) % size;

    if (g_theInput->WasKeyJustPressed(KEYCODE_M)) m_numSubDivisions *= 2;
    if (g_theInput->WasKeyJustPressed(KEYCODE_N)) m_numSubDivisions /= 2;
    m_numSubDivisions = m_numSubDivisions < 1 ? 1 : m_numSubDivisions;
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::UpdateFromController(float const deltaSeconds)
{
    UNUSED(deltaSeconds)
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::GenerateRandomShapes()
{
    GenerateEaseFunction();
    GenerateCubicBezierCurves();
    GenerateCubicHermiteCurves();
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::GenerateAABB2s()
{
    m_boundA      = AABB2(Vec2(50.f, 425.f), Vec2(775.f, 750.f));
    m_boundAChild = AABB2(Vec2(275.f, 450.f), Vec2(550.f, 750.f));
    m_boundB      = AABB2(Vec2(825.f, 425.f), Vec2(1550.f, 750.f));
    m_boundC      = AABB2(Vec2(50.f, 50.f), Vec2(1550.f, 375.f));
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::GenerateEaseFunction()
{
    m_easeFunctionStartPosition = Vec2(275.f, 450.f);
    m_easeFunctionEndPosition   = Vec2(550.f, 750.f);
    m_easeFunctionBound         = AABB2(m_easeFunctionStartPosition, m_easeFunctionEndPosition);
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::GenerateCubicBezierCurves()
{
    m_cubicBezierCurve2D                  = CubicBezierCurve2D();
    m_cubicBezierCurve2D.m_startPosition  = m_boundB.GetRandomPointInBounds();
    m_cubicBezierCurve2D.m_guidePosition1 = m_boundB.GetRandomPointInBounds();
    m_cubicBezierCurve2D.m_guidePosition2 = m_boundB.GetRandomPointInBounds();
    m_cubicBezierCurve2D.m_endPosition    = m_boundB.GetRandomPointInBounds();
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::GenerateCubicHermiteCurves()
{
    m_cubicHermiteCurve2D = CubicHermiteCurve2D();

    // for (int i = 0; i < g_theRNG->RollRandomIntInRange(0, 5); i++)
    for (int i = 0; i < 2; i++)
    {
        m_cubicHermiteCurve2D.m_startPosition = m_boundC.GetRandomPointInBounds();
        m_cubicHermiteCurve2D.m_endPosition   = m_boundC.GetRandomPointInBounds();
        m_cubicHermiteCurve2D.m_startVelocity = m_boundC.GetRandomPointInBounds() * 0.5f;
        m_cubicHermiteCurve2D.m_endVelocity   = m_boundC.GetRandomPointInBounds() * 0.5f;

        m_points.push_back(m_cubicHermiteCurve2D.m_startPosition);
        m_points.push_back(m_cubicHermiteCurve2D.m_endPosition);
    }

    m_catmullRomSpline2D = CatmullRomSpline2D(m_points);
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::RenderShapes() const
{
    RenderAABB2s();
    RenderEaseFunctions();
    RenderCubicBezierCurves();
    RenderCubicHermiteCurves();
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::RenderAABB2s() const
{
    VertexList_PCU verts;

    Rgba8 const boundsColor       = Rgba8(255, 0, 0, 127);
    Rgba8 const boundsAChildColor = Rgba8(30, 30, 60);

    AddVertsForAABB2D(verts, m_boundA, boundsColor);
    AddVertsForAABB2D(verts, m_boundAChild, boundsAChildColor);
    AddVertsForAABB2D(verts, m_boundB, boundsColor);
    AddVertsForAABB2D(verts, m_boundC, boundsColor);

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::RenderEaseFunctions() const
{
    VertexList_PCU verts;
    Vec2 const     dimension = m_boundAChild.GetDimensions();

    for (int i = 0; i < m_fixedNumSubDivisions; ++i)
    {
        float const t0 = static_cast<float>(i) / static_cast<float>(m_fixedNumSubDivisions);
        float const t1 = static_cast<float>(i + 1) / static_cast<float>(m_fixedNumSubDivisions);

        float const easedT0 = s_easingFunctions[m_easeIndex].easeFunction(t0);
        float const easedT1 = s_easingFunctions[m_easeIndex].easeFunction(t1);

        Vec2 p0 = m_boundAChild.m_mins + Vec2(t0 * dimension.x, easedT0 * dimension.y);
        Vec2 p1 = m_boundAChild.m_mins + Vec2(t1 * dimension.x, easedT1 * dimension.y);

        AddVertsForLineSegment2D(verts, p0, p1, m_lineThickness, false, Rgba8::DARK_GREY);
    }

    std::vector<Vec2> curvePoints;
    curvePoints.reserve(m_fixedNumSubDivisions);

    for (int i = 0; i < m_fixedNumSubDivisions; ++i)
    {
        float const t      = static_cast<float>(i) / static_cast<float>(m_fixedNumSubDivisions - 1);
        float const easedT = s_easingFunctions[m_easeIndex].easeFunction(t);
        Vec2        point  = m_boundAChild.m_mins + Vec2(t * dimension.x, easedT * dimension.y);
        curvePoints.push_back(point);
    }

    float const t            = fmodf(static_cast<float>(m_gameClock->GetTotalSeconds()), 2.f) / 2.f;
    int const   currentIndex = static_cast<int>(t * (m_fixedNumSubDivisions - 1));
    Vec2 const  position     = curvePoints[currentIndex];

    Vec2 const horizontalPosition = Vec2(position.x, m_boundAChild.m_mins.y);
    Vec2 const verticalPosition   = Vec2(m_boundAChild.m_mins.x, position.y);

    AddVertsForLineSegment2D(verts, position, horizontalPosition, m_lineThickness, false, Rgba8::TRANSLUCENT_BLACK);
    AddVertsForLineSegment2D(verts, position, verticalPosition, m_lineThickness, false, Rgba8::TRANSLUCENT_BLACK);

    for (int i = 0; i < m_numSubDivisions; ++i)
    {
        float const t0 = static_cast<float>(i) / static_cast<float>(m_numSubDivisions);
        float const t1 = static_cast<float>(i + 1) / static_cast<float>(m_numSubDivisions);

        float const easedT0 = s_easingFunctions[m_easeIndex].easeFunction(t0);
        float const easedT1 = s_easingFunctions[m_easeIndex].easeFunction(t1);

        Vec2 p0 = m_boundAChild.m_mins + Vec2(t0 * dimension.x, easedT0 * dimension.y);
        Vec2 p1 = m_boundAChild.m_mins + Vec2(t1 * dimension.x, easedT1 * dimension.y);

        AddVertsForLineSegment2D(verts, p0, p1, m_lineThickness, false, Rgba8::GREEN);
    }

    AddVertsForDisc2D(verts, position, m_discRadius, Rgba8::WHITE);

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());

    // Render EaseFunction text
    VertexList_PCU  textVerts;
    float constexpr textHeight        = 25.f;
    float const     textWidth         = g_theBitmapFont->GetTextWidth(textHeight, s_easingFunctions[m_easeIndex].easeFunctionName);
    Vec2 const      textStartPosition = Vec2((m_boundAChild.m_mins.x + m_boundAChild.m_maxs.x - textWidth) * 0.5f, m_boundAChild.m_mins.y - textHeight);

    g_theBitmapFont->AddVertsForText2D(textVerts, s_easingFunctions[m_easeIndex].easeFunctionName, textStartPosition, textHeight);
    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(&g_theBitmapFont->GetTexture());
    g_theRenderer->DrawVertexArray(static_cast<int>(textVerts.size()), textVerts.data());
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::RenderCubicBezierCurves() const
{
    VertexList_PCU verts;

    // Lines connecting A to B, and from B to C, and C to D (faint / thin).
    AddVertsForLineSegment2D(verts, m_cubicBezierCurve2D.m_startPosition, m_cubicBezierCurve2D.m_guidePosition1, m_lineThickness, false, Rgba8::TRANSLUCENT_BLACK);
    AddVertsForLineSegment2D(verts, m_cubicBezierCurve2D.m_guidePosition1, m_cubicBezierCurve2D.m_guidePosition2, m_lineThickness, false, Rgba8::TRANSLUCENT_BLACK);
    AddVertsForLineSegment2D(verts, m_cubicBezierCurve2D.m_guidePosition2, m_cubicBezierCurve2D.m_endPosition, m_lineThickness, false, Rgba8::TRANSLUCENT_BLACK);

    // The curve itself, in dark grey; which must use a fixed, high number of curve subdivisions for its drawing (e.g. 64).
    std::vector<Vec2> fixedBezierPoints;

    for (int i = 0; i <= m_fixedNumSubDivisions; ++i)
    {
        float const t = static_cast<float>(i) / static_cast<float>(m_fixedNumSubDivisions);
        fixedBezierPoints.push_back(m_cubicBezierCurve2D.EvaluateAtParametric(t));
    }

    for (int i = 0; i < m_fixedNumSubDivisions; ++i)
    {
        AddVertsForLineSegment2D(verts, fixedBezierPoints[i], fixedBezierPoints[i + 1], m_lineThickness, false, Rgba8::DARK_GREY);
    }

    // The curve itself, in medium green, which must use the current (global) number of curve subdivisions for its drawing.
    std::vector<Vec2> bezierPoints;

    for (int i = 0; i <= m_numSubDivisions; ++i)
    {
        float const t = static_cast<float>(i) / static_cast<float>(m_numSubDivisions);
        bezierPoints.push_back(m_cubicBezierCurve2D.EvaluateAtParametric(t));
    }

    for (int i = 0; i < m_numSubDivisions; ++i)
    {
        AddVertsForLineSegment2D(verts, bezierPoints[i], bezierPoints[i + 1], m_lineThickness, false, Rgba8::GREEN);
    }

    // The four Bezier control points A, B, C, D (where A and D are the start and end positions, and B and C are the guide points).
    AddVertsForDisc2D(verts, m_cubicBezierCurve2D.m_startPosition, m_discRadius, Rgba8::LIGHT_BLUE);
    AddVertsForDisc2D(verts, m_cubicBezierCurve2D.m_guidePosition1, m_discRadius, Rgba8::LIGHT_BLUE);
    AddVertsForDisc2D(verts, m_cubicBezierCurve2D.m_guidePosition2, m_discRadius, Rgba8::LIGHT_BLUE);
    AddVertsForDisc2D(verts, m_cubicBezierCurve2D.m_endPosition, m_discRadius, Rgba8::LIGHT_BLUE);

    // A point (white, per the Demo) moving parametrically with time (at 50% timescale, so it repeats every 2.0 seconds).
    float const curveLength    = m_cubicBezierCurve2D.GetApproximateLength(m_numSubDivisions);
    float const time           = fmod(static_cast<float>(m_gameClock->GetTotalSeconds()), m_loopDuration);
    float const normalizedTime = time / m_loopDuration;
    Vec2 const  whitePoint     = m_cubicBezierCurve2D.EvaluateAtParametric(normalizedTime);
    AddVertsForDisc2D(verts, whitePoint, m_discRadius, Rgba8::WHITE);

    // A point (bright green, per the Demo) moving at a fixed speed (distance/second) onscreen;
    // must follow the drawn subdivision lines exactly,
    // and finish/reset perfectly in sync with the parametrically-moving point.
    float const speed             = curveLength / m_loopDuration;
    float const distanceAlongPath = speed * time;
    Vec2 const  greenPoint        = m_cubicBezierCurve2D.EvaluateAtApproximateDistance(distanceAlongPath, m_numSubDivisions);
    AddVertsForDisc2D(verts, greenPoint, m_discRadius, Rgba8::GREEN);

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void GameCurves2D::RenderCubicHermiteCurves() const
{
    VertexList_PCU verts;

    // Lines connecting position 0 to 1, 1 to 2, etc. (faint / thin).
    m_catmullRomSpline2D.AddVertsForCurve2D(verts, m_lineThickness, Rgba8::TRANSLUCENT_BLACK, 1);

    // The spline, dark grey; each curve section of which must use a fixed, high number of curve subdivisions for its drawing (e.g. 64).
    m_catmullRomSpline2D.AddVertsForCurve2D(verts, m_lineThickness, Rgba8::DARK_GREY, m_fixedNumSubDivisions);

    // The spline, medium green; each curve section must use the current (global, variable) number of subdivisions for its drawing.
    m_catmullRomSpline2D.AddVertsForCurve2D(verts, m_lineThickness, Rgba8::GREEN, m_numSubDivisions);

    for (int i = 0; i < m_catmullRomSpline2D.GetNumOfCurves(); i++)
    {
        CubicHermiteCurve2D currentHermiteCurve = m_catmullRomSpline2D.GetCubicHermiteCurveAtIndex(i);

        // The velocity vector (as a red arrow) at each position; positions with zero velocity (including start and end) do not draw this.
        AddVertsForArrow2D(verts, currentHermiteCurve.m_startPosition, currentHermiteCurve.m_startPosition + currentHermiteCurve.m_startVelocity, 5.f, m_lineThickness, Rgba8::RED);

        // Each control position point in the spline.
        AddVertsForDisc2D(verts, currentHermiteCurve.m_startPosition, m_discRadius, Rgba8::LIGHT_BLUE);
        AddVertsForDisc2D(verts, currentHermiteCurve.m_endPosition, m_discRadius, Rgba8::LIGHT_BLUE);
    }

    // A white point (see Demo) moving parametrically with time (at 50% timescale), traversing each spline curve section in exactly 2.0 seconds.
    // Therefore, a spline with 7 curve sections (8 construction points) will finish and repeat every 14 seconds.
    float const curveLength       = m_catmullRomSpline2D.GetApproximateLength(m_numSubDivisions);
    float const totalLoopDuration = m_loopDuration * static_cast<float>(m_catmullRomSpline2D.GetNumOfCurves());
    float const time              = fmod(static_cast<float>(m_gameClock->GetTotalSeconds()), totalLoopDuration);
    float const normalizedTime    = time / m_loopDuration;
    Vec2 const  whitePoint        = m_catmullRomSpline2D.EvaluateAtParametric(normalizedTime);
    AddVertsForDisc2D(verts, whitePoint, m_discRadius, Rgba8::WHITE);

    // A bright green point (see Demo) moving at a constant speed; must follow the drawn subdivision lines exactly,
    // and arrive at the end of the spline (but NOT necessarily at any given curve weld point) in sync with the parametrically-moving white point.
    float const speed             = curveLength / totalLoopDuration; // 要2秒走完整條曲線
    float const distanceAlongPath = speed * time;
    Vec2 const  greenPoint        = m_catmullRomSpline2D.EvaluateAtApproximateDistance(distanceAlongPath, m_numSubDivisions);
    AddVertsForDisc2D(verts, greenPoint, m_discRadius, Rgba8::GREEN);

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}
