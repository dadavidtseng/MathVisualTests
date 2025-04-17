//----------------------------------------------------------------------------------------------------
// GamePachinkoMachine2D.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GamePachinkoMachine2D.hpp"

#include "App.hpp"
#include "GameCommon.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"

//----------------------------------------------------------------------------------------------------
GamePachinkoMachine2D::GamePachinkoMachine2D()
{
    m_screenCamera = new Camera();
    m_worldCamera  = new Camera();

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);

    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));
    m_worldCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));

    m_gameClock = new Clock(Clock::GetSystemClock());
    GenerateRandomLineSegmentInScreen();
    GenerateRandomShapes();
    m_ballElasticity = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Ball.DefaultElasticity", -1.f);
    m_fixedTimeStep  = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Misc.InitialTimeStep", -1.f);
}

void GamePachinkoMachine2D::Update()
{
    g_theInput->SetCursorMode(CursorMode::POINTER);

    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    UpdateFromKeyboard(deltaSeconds);
    UpdateFromController(deltaSeconds);

    m_physicsTimeOwed += deltaSeconds;

    while (m_physicsTimeOwed >= deltaSeconds)
    {
        UpdateBall(m_fixedTimeStep);
        m_physicsTimeOwed -= m_fixedTimeStep;
    }
}

void GamePachinkoMachine2D::RenderShapes() const
{
    VertexList_PCU verts;
    // Ray direction and starting position
    Vec2 const  forwardNormal = (m_lineSegment.m_endPosition - m_lineSegment.m_startPosition).GetNormalized();
    Vec2 const  tailPosition  = m_lineSegment.m_startPosition;
    float const maxDistance   = m_lineSegment.GetLength();

    // Draw the ray as a white arrow
    DrawArrow2D(m_lineSegment.m_startPosition,
                tailPosition + forwardNormal * maxDistance,
                50.f,
                m_lineSegment.m_thickness,
                Rgba8::WHITE);

    // AddVertsForDisc2D(verts, m_ball.m_startPosition, 5.f, Rgba8::WHITE);

    for (int i = 0; i < (int)m_ballList.size(); ++i)
    {
        AddVertsForDisc2D(verts, m_ballList[i].m_position, m_ballList[i].m_radius, m_ballList[i].m_color);
    }

    for (int i = 0; i < (int)m_bumperList.size(); ++i)
    {
        if (m_bumperList[i].m_type == eBumperType::DISC2)
        {
            AddVertsForDisc2D(verts, m_bumperList[i].m_startPosition, m_bumperList[i].m_radius, m_bumperList[i].m_color);
        }

        if (m_bumperList[i].m_type == eBumperType::CAPSULE2)
        {
            AddVertsForCapsule2D(verts, m_bumperList[i].m_startPosition, m_bumperList[i].m_endPosition, m_bumperList[i].m_radius, m_bumperList[i].m_color);
        }

        if (m_bumperList[i].m_type == eBumperType::OBB2)
        {
            Vec2 iBasis = (m_bumperList[i].m_endPosition - m_bumperList[i].m_startPosition).GetNormalized();
            AddVertsForOBB2D(verts, m_bumperList[i].m_startPosition, iBasis, Vec2(m_bumperList[i].m_radius, m_bumperList[i].m_radius), m_bumperList[i].m_color);
        }
    }

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

void GamePachinkoMachine2D::Render() const
{
    //-Start-of-World-Camera--------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_worldCamera);

    RenderShapes();

    g_theRenderer->EndCamera(*m_worldCamera);

    //-End-of-World-Camera----------------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //-Start-of-Screen-Camera-------------------------------------------------------------------------

    g_theRenderer->BeginCamera(*m_screenCamera);

    RenderCurrentModeText("CurrentMode: GamePachinkoMachine2D");

    VertexList_PCU verts;

    float const currentControlTextBoxMinX = g_gameConfigBlackboard.GetValue("currentControlTextBoxMinX", 0.f);
    float const currentControlTextBoxMinY = g_gameConfigBlackboard.GetValue("currentControlTextBoxMinY", 760.f);
    float const currentControlTextBoxMaxX = g_gameConfigBlackboard.GetValue("currentControlTextBoxMaxX", 1600.f);
    float const currentControlTextBoxMaxY = g_gameConfigBlackboard.GetValue("currentControlTextBoxMaxY", 780.f);
    AABB2 const currentModeTextBox(Vec2(currentControlTextBoxMinX, currentControlTextBoxMinY - 20.f), Vec2(currentControlTextBoxMaxX, currentControlTextBoxMaxY - 20.f));

    String const currentControlText = Stringf("F8 to randomize; W/E=pre/next easing function,\nN/M=curve subdivisions(), hold T=slow");
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
void GamePachinkoMachine2D::GenerateRandomLineSegmentInScreen()
{
    m_lineSegment = LineSegment2(GenerateRandomPointInScreen(), GenerateRandomPointInScreen(), 2.f, false);
}

void GamePachinkoMachine2D::UpdateFromKeyboard(float deltaSeconds)
{
    if (g_theInput->WasKeyJustPressed(KEYCODE_O)) m_gameClock->StepSingleFrame();
    if (g_theInput->WasKeyJustPressed(KEYCODE_T)) m_gameClock->SetTimeScale(0.1f);
    if (g_theInput->WasKeyJustReleased(KEYCODE_T)) m_gameClock->SetTimeScale(1.f);
    if (g_theInput->WasKeyJustPressed(KEYCODE_P)) m_gameClock->TogglePause();
    if (g_theInput->WasKeyJustPressed(KEYCODE_ESC)) App::RequestQuit();
    if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
    {
        GenerateRandomShapes();
    }

    if (g_theInput->WasKeyJustPressed(KEYCODE_F8)) GenerateRandomLineSegmentInScreen();
    if (g_theInput->IsKeyDown(KEYCODE_W)) m_lineSegment.m_startPosition.y += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_S)) m_lineSegment.m_startPosition.y -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_A)) m_lineSegment.m_startPosition.x -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_D)) m_lineSegment.m_startPosition.x += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_I)) m_lineSegment.m_endPosition.y += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_K)) m_lineSegment.m_endPosition.y -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_J)) m_lineSegment.m_endPosition.x -= m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_L)) m_lineSegment.m_endPosition.x += m_moveSpeed * deltaSeconds;
    if (g_theInput->IsKeyDown(KEYCODE_LEFT_MOUSE)) m_lineSegment.m_startPosition = GetMouseWorldPos();
    if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE)) m_lineSegment.m_endPosition = GetMouseWorldPos();

    if (g_theInput->IsKeyDown(KEYCODE_N) ||
        g_theInput->WasKeyJustPressed(KEYCODE_SPACE))
    {
        Ball ball                    = Ball();
        ball.m_position              = Vec2(m_lineSegment.m_endPosition.x, m_lineSegment.m_endPosition.y);
        ball.m_velocity              = Vec2(m_lineSegment.m_endPosition - m_lineSegment.m_startPosition) * 3.f;
        FloatRange const radiusRange = FloatRange(g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Ball.Radius", FloatRange::ZERO));
        ball.m_color                 = Interpolate(Rgba8::BLUE, Rgba8::WHITE, g_theRNG->RollRandomFloatZeroToOne());
        ball.m_radius                = g_theRNG->RollRandomFloatInRange(radiusRange.m_min, radiusRange.m_max);
        ball.m_elasticity            = m_ballElasticity;

        m_ballList.push_back(ball);
    }
}

void GamePachinkoMachine2D::UpdateFromController(float deltaSeconds)
{
}

void GamePachinkoMachine2D::UpdateBall(float const timeSteps)
{
    float gravityY = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Misc.Gravity", -1.f);

    for (int i = 0; i < (int)m_ballList.size(); i++)
    {
        m_ballList[i].m_velocity -= Vec2(0.f, gravityY) * timeSteps;
        m_ballList[i].m_position += m_ballList[i].m_velocity * timeSteps;
    }

    for (int i = 0; i < (int)m_ballList.size(); i++)
    {
        for (int j = 0; j < (int)m_ballList.size(); j++)
        {
            BounceDiscOutOfEachOther2D(m_ballList[i].m_position, m_ballList[i].m_radius, m_ballList[i].m_velocity, m_ballList[i].m_elasticity, m_ballList[j].m_position, m_ballList[j].m_radius, m_ballList[j].m_velocity, m_ballList[j].m_elasticity);
        }
    }

    for (int i = 0; i < (int)m_ballList.size(); i++)
    {
        for (int j = 0; j < (int)m_bumperList.size(); j++)
        {
            if (m_bumperList[j].m_type == eBumperType::DISC2)
            {
                BounceDiscOutOfFixedDisc2D(m_ballList[i].m_position, m_ballList[i].m_radius, m_ballList[i].m_velocity, m_ballList[i].m_elasticity, m_bumperList[j].m_startPosition, m_bumperList[j].m_radius, m_bumperList[j].m_elasticity);
            }
            if (m_bumperList[j].m_type == eBumperType::CAPSULE2)
            {
                BounceDiscOutOfFixedCapsule2D(m_ballList[i].m_position, m_ballList[i].m_radius, m_ballList[i].m_velocity, m_ballElasticity, m_bumperList[j].m_startPosition, m_bumperList[j].m_endPosition, m_bumperList[j].m_radius, m_bumperList[j].m_elasticity);
            }
            if (m_bumperList[j].m_type == eBumperType::OBB2)
            {
                Vec2 iBasis = (m_bumperList[j].m_endPosition - m_bumperList[j].m_startPosition).GetNormalized();
                BounceDiscOutOfFixedOBB2D(m_ballList[i].m_position, m_ballList[i].m_radius, m_ballList[i].m_velocity, m_ballElasticity, m_bumperList[j].m_startPosition, iBasis,Vec2(m_bumperList[j].m_radius, m_bumperList[j].m_radius), m_bumperList[i].m_elasticity);
            }
        }
    }
}

void GamePachinkoMachine2D::GenerateRandomShapes()
{
    int        discNum            = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Disc.Num", -1);
    FloatRange discRadiusRange    = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Disc.Radius", FloatRange::ZERO);
    int        capsuleNum         = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Capsule.Num", -1);
    FloatRange capsuleLengthRange = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Capsule.Length", FloatRange::ZERO);
    FloatRange capsuleRadiusRange = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Capsule.Radius", FloatRange::ZERO);
    int        obb2Num            = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Obb2.Num", -1);
    FloatRange obb2WidthRange     = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Obb2.Width", FloatRange::ZERO);
    FloatRange elasticityRange    = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Elasticity", FloatRange::ZERO);

    for (int i = 0; i < discNum; i++)
    {
        Bumper bumper          = Bumper();
        bumper.m_type          = eBumperType::DISC2;
        bumper.m_startPosition = GenerateRandomPointInScreen();
        bumper.m_radius        = g_theRNG->RollRandomFloatInRange(discRadiusRange.m_min, discRadiusRange.m_max);
        bumper.m_elasticity    = g_theRNG->RollRandomFloatInRange(elasticityRange.m_min, elasticityRange.m_max);;
        bumper.m_color         = Interpolate(Rgba8::RED, Rgba8::GREEN, bumper.m_elasticity);
        m_bumperList.push_back(bumper);
    }

    for (int i = 0; i < capsuleNum; i++)
    {
        Bumper bumper                   = Bumper();
        bumper.m_type                   = eBumperType::CAPSULE2;
        Vec2 tempStartPosition          = GenerateRandomPointInScreen();
        Vec2 tempEndPosition            = GenerateRandomPointInScreen();
        Vec2 tempVelocity               = tempEndPosition - tempStartPosition;
        bumper.m_startPosition          = tempStartPosition;
        float const randomCapsuleLength = g_theRNG->RollRandomFloatInRange(capsuleLengthRange.m_min, capsuleLengthRange.m_max);
        bumper.m_endPosition            = tempStartPosition + tempVelocity.GetNormalized() * randomCapsuleLength;
        bumper.m_radius                 = g_theRNG->RollRandomFloatInRange(capsuleRadiusRange.m_min, capsuleRadiusRange.m_max);
        bumper.m_elasticity             = g_theRNG->RollRandomFloatInRange(elasticityRange.m_min, elasticityRange.m_max);
        bumper.m_color                  = Interpolate(Rgba8::RED, Rgba8::GREEN, bumper.m_elasticity);
        m_bumperList.push_back(bumper);
    }

    for (int i = 0; i < obb2Num; i++)
    {
        Bumper bumper               = Bumper();
        bumper.m_type               = eBumperType::OBB2;
        Vec2 tempStartPosition      = GenerateRandomPointInScreen();
        Vec2 tempEndPosition        = GenerateRandomPointInScreen();
        Vec2 tempVelocity           = tempEndPosition - tempStartPosition;
        bumper.m_startPosition      = tempStartPosition;
        float const randomObb2Width = g_theRNG->RollRandomFloatInRange(obb2WidthRange.m_min, obb2WidthRange.m_max);
        bumper.m_endPosition        = tempStartPosition + tempVelocity.GetNormalized() * randomObb2Width;
        bumper.m_radius             = g_theRNG->RollRandomFloatInRange(obb2WidthRange.m_min, obb2WidthRange.m_max);
        bumper.m_elasticity         = g_theRNG->RollRandomFloatInRange(elasticityRange.m_min, elasticityRange.m_max);
        bumper.m_color              = Interpolate(Rgba8::RED, Rgba8::GREEN, bumper.m_elasticity);
        m_bumperList.push_back(bumper);
    }
}
