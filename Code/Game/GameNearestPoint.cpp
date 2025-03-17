//----------------------------------------------------------------------------------------------------
// GameNearestPoint.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameNearestPoint.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"

//----------------------------------------------------------------------------------------------------
GameNearestPoint::GameNearestPoint()
{
    m_screenCamera = new Camera();

    float const screenSizeX   = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY   = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    float const screenCenterX = g_gameConfigBlackboard.GetValue("screenCenterX", 800.f);
    float const screenCenterY = g_gameConfigBlackboard.GetValue("screenCenterY", 400.f);

    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));
    m_referencePoint = Vec2(screenCenterX, screenCenterY);
    m_gameClock      = new Clock(Clock::GetSystemClock());

    GenerateRandomShapes();
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::Update()
{
    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    UpdateFromKeyboard(deltaSeconds);
    UpdateFromController(deltaSeconds);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::Render() const
{
    g_theRenderer->BeginCamera(*m_screenCamera);

    RenderShapes();
    RenderNearestPoints();
    RenderReferencePoint();
    RenderCurrentModeText("CurrentMode: NearestPoint");
    RenderControlText();

    g_theRenderer->EndCamera(*m_screenCamera);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::UpdateFromKeyboard(float const deltaSeconds)
{
    if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
    {
        GenerateRandomShapes();

        float const screenCenterX = g_gameConfigBlackboard.GetValue("screenCenterX", 800.f);
        float const screenCenterY = g_gameConfigBlackboard.GetValue("screenCenterY", 400.f);
        m_referencePoint          = Vec2(screenCenterX, screenCenterY);
    }

    if (g_theInput->WasKeyJustPressed(KEYCODE_O)) m_gameClock->StepSingleFrame();
    if (g_theInput->WasKeyJustPressed(KEYCODE_T)) m_gameClock->SetTimeScale(0.1f);
    if (g_theInput->WasKeyJustReleased(KEYCODE_T)) m_gameClock->SetTimeScale(1.f);
    if (g_theInput->WasKeyJustPressed(KEYCODE_P)) m_gameClock->TogglePause();
    if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) App::RequestQuit();

    if (g_theInput->IsKeyDown(KEYCODE_W)) m_referencePoint.y += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_S)) m_referencePoint.y -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_A)) m_referencePoint.x -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_D)) m_referencePoint.x += m_moveSpeed * deltaSeconds;

    if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE)) m_referencePoint = GetMouseWorldPos();
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::UpdateFromController(float const deltaSeconds)
{
    XboxController const controller = g_theInput->GetController(0);

    m_referencePoint += controller.GetLeftStick().GetPosition() * m_moveSpeed * deltaSeconds;
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderShapes() const
{
    VertexList verts;

    for (int i = 0; i < static_cast<int>(eTestShape2DType::COUNT); ++i)
    {
        if (m_testShapes[i].m_type == eTestShape2DType::DISC2)
        {
            bool const isPointInside = IsPointInsideDisc2D(m_referencePoint, m_testShapes[i].m_startPosition, m_testShapes[i].m_radius);
            AddVertsForDisc2D(verts, m_testShapes[i].m_startPosition, m_testShapes[i].m_radius, isPointInside ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
        }

        if (m_testShapes[i].m_type == eTestShape2DType::LINESEGMENT2 ||
            m_testShapes[i].m_type == eTestShape2DType::INFINITE_LINESEGMENT2)
        {
            AddVertsForLineSegment2D(verts, m_testShapes[i].m_startPosition, m_testShapes[i].m_endPosition, m_testShapes[i].m_thickness, m_testShapes[i].m_isInfinite, Rgba8::BLUE);
        }

        if (m_testShapes[i].m_type == eTestShape2DType::TRIANGLE2)
        {
            bool const isPointInside = IsPointInsideTriangle(m_referencePoint, m_testShapes[i].m_startPosition, m_testShapes[i].m_endPosition, m_testShapes[i].m_thirdPosition);
            AddVertsForTriangle2D(verts, m_testShapes[i].m_startPosition, m_testShapes[i].m_endPosition, m_testShapes[i].m_thirdPosition, isPointInside ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
        }

        if (m_testShapes[i].m_type == eTestShape2DType::AABB2)
        {
            bool const isPointInside = IsPointInsideAABB2D(m_referencePoint, m_testShapes[i].m_startPosition, m_testShapes[i].m_endPosition);
            AddVertsForAABB2D(verts, m_testShapes[i].m_startPosition, m_testShapes[i].m_endPosition, isPointInside ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
        }

        if (m_testShapes[i].m_type == eTestShape2DType::OBB2)
        {
            bool const isPointInside = IsPointInsideOBB2D(m_referencePoint, m_testShapes[i].m_startPosition, m_testShapes[i].m_iBasisNormal, m_testShapes[i].m_halfDimensions);
            AddVertsForOBB2D(verts, m_testShapes[i].m_startPosition, m_testShapes[i].m_iBasisNormal, m_testShapes[i].m_halfDimensions, isPointInside ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
        }

        if (m_testShapes[i].m_type == eTestShape2DType::CAPSULE2)
        {
            bool const isPointInside = IsPointInsideCapsule(m_referencePoint, m_testShapes[i].m_startPosition, m_testShapes[i].m_endPosition, m_testShapes[i].m_radius);
            AddVertsForCapsule2D(verts, m_testShapes[i].m_startPosition, m_testShapes[i].m_endPosition, m_testShapes[i].m_radius, isPointInside ? Rgba8::LIGHT_BLUE : Rgba8::BLUE);
        }
    }

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(BlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(DepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomShapes()
{
    GenerateRandomDisc2D();
    GenerateRandomLineSegment2D();
    GenerateRandomInfiniteLine2D();
    GenerateRandomTriangle2D();
    GenerateRandomAABB2D();
    GenerateRandomOBB2D();
    GenerateRandomCapsule2D();
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderNearestPoints() const
{
    VertexList verts;
    Vec2       nearestPoint;
    Vec2       closestNearestPoint;
    float      minLengthSquared = FLOAT_MAX;

    for (int i = 0; i < static_cast<int>(eTestShape2DType::COUNT); ++i)
    {
        if (m_testShapes[i].m_type == eTestShape2DType::DISC2)
        {
            nearestPoint = GetNearestPointOnDisc2D(m_referencePoint, m_testShapes[i].m_startPosition, m_testShapes[i].m_radius);
        }

        if (m_testShapes[i].m_type == eTestShape2DType::LINESEGMENT2 ||
            m_testShapes[i].m_type == eTestShape2DType::INFINITE_LINESEGMENT2)
        {
            nearestPoint = GetNearestPointOnLineSegment2D(m_referencePoint, m_testShapes[i].m_startPosition, m_testShapes[i].m_endPosition, m_testShapes[i].m_isInfinite);
        }

        if (m_testShapes[i].m_type == eTestShape2DType::TRIANGLE2)
        {
            Vec2 const points[3] = {m_testShapes[i].m_startPosition, m_testShapes[i].m_endPosition, m_testShapes[i].m_thirdPosition};
            nearestPoint         = GetNearestPointOnTriangle2D(m_referencePoint, points);
        }

        if (m_testShapes[i].m_type == eTestShape2DType::AABB2)
        {
            nearestPoint = GetNearestPointOnAABB2D(m_referencePoint, m_testShapes[i].m_startPosition, m_testShapes[i].m_endPosition);
        }

        if (m_testShapes[i].m_type == eTestShape2DType::OBB2)
        {
            nearestPoint = GetNearestPointOnOBB2D(m_referencePoint, m_testShapes[i].m_startPosition, m_testShapes[i].m_iBasisNormal, m_testShapes[i].m_halfDimensions);
        }

        if (m_testShapes[i].m_type == eTestShape2DType::CAPSULE2)
        {
            nearestPoint = GetNearestPointOnCapsule2D(m_referencePoint, m_testShapes[i].m_startPosition, m_testShapes[i].m_endPosition, m_testShapes[i].m_radius);
        }

        float const lengthSquared = (nearestPoint - m_referencePoint).GetLengthSquared();

        if (lengthSquared < minLengthSquared)
        {
            minLengthSquared    = lengthSquared;
            closestNearestPoint = nearestPoint;
        }

        AddVertsForLineSegment2D(verts, m_referencePoint, nearestPoint, 3.f, false, Rgba8::TRANSLUCENT_WHITE);
        AddVertsForDisc2D(verts, nearestPoint, 5.f, Rgba8::ORANGE);
    }

    AddVertsForDisc2D(verts, closestNearestPoint, 5.f, Rgba8::GREEN);

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(BlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(DepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::RenderReferencePoint() const
{
    DrawDisc2D(m_referencePoint, 3.f, Rgba8::WHITE);
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomDisc2D()
{
    Vec2        centerPosition = GenerateRandomPointInScreen();
    float const randomRadius   = g_theRNG->RollRandomFloatInRange(10.f, 100.f);

    centerPosition = ClampPointToScreen(centerPosition, randomRadius);

    m_testShapes[static_cast<int>(eTestShape2DType::DISC2)].m_type          = eTestShape2DType::DISC2;
    m_testShapes[static_cast<int>(eTestShape2DType::DISC2)].m_startPosition = centerPosition;
    m_testShapes[static_cast<int>(eTestShape2DType::DISC2)].m_radius        = randomRadius;
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomLineSegment2D()
{
    Vec2        startPosition   = GenerateRandomPointInScreen();
    Vec2        endPosition     = GenerateRandomPointInScreen();
    float const randomThickness = g_theRNG->RollRandomFloatInRange(1.f, 5.f);

    startPosition = ClampPointToScreen(startPosition, randomThickness);
    endPosition   = ClampPointToScreen(endPosition, randomThickness);

    m_testShapes[static_cast<int>(eTestShape2DType::LINESEGMENT2)].m_type          = eTestShape2DType::LINESEGMENT2;
    m_testShapes[static_cast<int>(eTestShape2DType::LINESEGMENT2)].m_startPosition = startPosition;
    m_testShapes[static_cast<int>(eTestShape2DType::LINESEGMENT2)].m_endPosition   = endPosition;
    m_testShapes[static_cast<int>(eTestShape2DType::LINESEGMENT2)].m_thickness     = randomThickness;
    m_testShapes[static_cast<int>(eTestShape2DType::LINESEGMENT2)].m_isInfinite    = false;
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomInfiniteLine2D()
{
    Vec2        startPosition   = GenerateRandomPointInScreen();
    Vec2        endPosition     = GenerateRandomPointInScreen();
    float const randomThickness = g_theRNG->RollRandomFloatInRange(1.f, 5.f);

    startPosition = ClampPointToScreen(startPosition, randomThickness);
    endPosition   = ClampPointToScreen(endPosition, randomThickness);

    m_testShapes[static_cast<int>(eTestShape2DType::INFINITE_LINESEGMENT2)].m_type          = eTestShape2DType::INFINITE_LINESEGMENT2;
    m_testShapes[static_cast<int>(eTestShape2DType::INFINITE_LINESEGMENT2)].m_startPosition = startPosition;
    m_testShapes[static_cast<int>(eTestShape2DType::INFINITE_LINESEGMENT2)].m_endPosition   = endPosition;
    m_testShapes[static_cast<int>(eTestShape2DType::INFINITE_LINESEGMENT2)].m_thickness     = randomThickness;
    m_testShapes[static_cast<int>(eTestShape2DType::INFINITE_LINESEGMENT2)].m_isInfinite    = true;
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomTriangle2D()
{
    while (true)
    {
        Vec2 const randomA = GenerateRandomPointInScreen();
        Vec2       randomB = GenerateRandomPointInScreen();
        Vec2       randomC = GenerateRandomPointInScreen();

        Vec2 const edgeAB = randomB - randomA;
        Vec2 const edgeAC = randomC - randomA;

        if (CrossProduct2D(edgeAB, edgeAC) == 0.f ||
            edgeAB.GetLengthSquared() > 100000.f ||
            edgeAC.GetLengthSquared() > 100000.f)
        {
            continue;
        }

        if (CrossProduct2D(edgeAB, edgeAC) < 0.f)
        {
            std::swap(randomB, randomC);
        }

        m_testShapes[static_cast<int>(eTestShape2DType::TRIANGLE2)].m_type          = eTestShape2DType::TRIANGLE2;
        m_testShapes[static_cast<int>(eTestShape2DType::TRIANGLE2)].m_startPosition = randomA;
        m_testShapes[static_cast<int>(eTestShape2DType::TRIANGLE2)].m_endPosition   = randomB;
        m_testShapes[static_cast<int>(eTestShape2DType::TRIANGLE2)].m_thirdPosition = randomC;

        break;
    }
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomAABB2D()
{
    float const screenSizeX  = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY  = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    float const randomWidth  = g_theRNG->RollRandomFloatInRange(10.f, screenSizeX / 5.f);
    float const randomHeight = g_theRNG->RollRandomFloatInRange(10.f, screenSizeY / 5.f);

    Vec2 const centerPosition = GenerateRandomPointInScreen();
    Vec2 const randomMins     = ClampPointToScreen(centerPosition - Vec2(randomWidth / 2.f, randomHeight / 2.f), randomWidth / 2.f, randomHeight / 2.f);
    Vec2 const randomMaxs     = ClampPointToScreen(centerPosition + Vec2(randomWidth / 2.f, randomHeight / 2.f), randomWidth / 2.f, randomHeight / 2.f);

    m_testShapes[static_cast<int>(eTestShape2DType::AABB2)].m_type          = eTestShape2DType::AABB2;
    m_testShapes[static_cast<int>(eTestShape2DType::AABB2)].m_startPosition = randomMins;
    m_testShapes[static_cast<int>(eTestShape2DType::AABB2)].m_endPosition   = randomMaxs;
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomOBB2D()
{
    float const screenSizeX    = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY    = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);
    float const randomIBasisX  = g_theRNG->RollRandomFloatZeroToOne();
    float const randomIBasisY  = g_theRNG->RollRandomFloatZeroToOne();
    Vec2 const  centerPosition = GenerateRandomPointInScreen();
    Vec2 const  halfDimensions = Vec2(g_theRNG->RollRandomFloatInRange(10.f, screenSizeX / 5.f),
                                     g_theRNG->RollRandomFloatInRange(10.f, screenSizeY / 5.f));

    m_testShapes[static_cast<int>(eTestShape2DType::OBB2)].m_type           = eTestShape2DType::OBB2;
    m_testShapes[static_cast<int>(eTestShape2DType::OBB2)].m_startPosition  = centerPosition - halfDimensions;
    m_testShapes[static_cast<int>(eTestShape2DType::OBB2)].m_endPosition    = centerPosition + halfDimensions;
    m_testShapes[static_cast<int>(eTestShape2DType::OBB2)].m_iBasisNormal   = Vec2(randomIBasisX, randomIBasisY).GetNormalized();
    m_testShapes[static_cast<int>(eTestShape2DType::OBB2)].m_halfDimensions = halfDimensions;
}

//----------------------------------------------------------------------------------------------------
void GameNearestPoint::GenerateRandomCapsule2D()
{
    Vec2        startPosition = GenerateRandomPointInScreen();
    Vec2        endPosition   = GenerateRandomPointInScreen();
    float const randomRadius  = g_theRNG->RollRandomFloatInRange(10.f, 100.f);

    startPosition = ClampPointToScreen(startPosition, randomRadius);
    endPosition   = ClampPointToScreen(endPosition, randomRadius);

    m_testShapes[static_cast<int>(eTestShape2DType::CAPSULE2)].m_type          = eTestShape2DType::CAPSULE2;
    m_testShapes[static_cast<int>(eTestShape2DType::CAPSULE2)].m_startPosition = startPosition;
    m_testShapes[static_cast<int>(eTestShape2DType::CAPSULE2)].m_endPosition   = endPosition;
    m_testShapes[static_cast<int>(eTestShape2DType::CAPSULE2)].m_radius        = randomRadius;
}


