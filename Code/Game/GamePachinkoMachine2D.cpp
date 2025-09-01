//----------------------------------------------------------------------------------------------------
// GamePachinkoMachine2D.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GamePachinkoMachine2D.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/DebugRenderSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"


//----------------------------------------------------------------------------------------------------
GamePachinkoMachine2D::GamePachinkoMachine2D()
{
    m_screenCamera = new Camera();
    m_worldCamera  = new Camera();

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);

    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));
    m_worldCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));
    m_screenCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);
    m_worldCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);
    m_gameClock = new Clock(Clock::GetSystemClock());
    GenerateRandomLineSegmentInScreen();
    GenerateRandomShapes();
    m_ballElasticity      = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Ball.DefaultElasticity", -1.f);
    m_ballElasticityDelta = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Ball.ElasticityDelta", -1.f);
    m_fixedTimeStep       = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Misc.InitialTimeStep", -1.f);
}

void GamePachinkoMachine2D::Update()
{
    g_input->SetCursorMode(eCursorMode::POINTER);

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
    AABB2 const currentModeTextBox(Vec2(currentControlTextBoxMinX, currentControlTextBoxMinY - 40.f), Vec2(currentControlTextBoxMaxX, currentControlTextBoxMaxY - 40.f));

    char const*  isWallWarpEnabledText = m_isWallWarpEnabled ? "On" : "Off";
    String const currentControlText    = Stringf("F8 to randomize; LMB/RMB/WASD/IJKL=move\nhold T=slow, space/N=ball(%d)\ne=%.2f(G/H), B=bottom warp (%s), timestep=%.2fms, (P,[,]), dt=%.2fms", m_ballList.size(), m_ballElasticity, isWallWarpEnabledText, m_fixedTimeStep * 1000.f, m_physicsTimeOwed);
    g_theBitmapFont->AddVertsForTextInBox2D(verts, currentControlText, currentModeTextBox, 20.f, Rgba8::GREEN, 1.f, Vec2::ZERO, OVERRUN);
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


void GamePachinkoMachine2D::UpdateFromKeyboard(float const deltaSeconds)
{
    if (g_input->WasKeyJustPressed(KEYCODE_O)) m_gameClock->StepSingleFrame();
    if (g_input->WasKeyJustPressed(KEYCODE_T)) m_gameClock->SetTimeScale(0.05f);
    if (g_input->WasKeyJustReleased(KEYCODE_T)) m_gameClock->SetTimeScale(1.f);
    if (g_input->WasKeyJustPressed(KEYCODE_P)) m_gameClock->TogglePause();
    if (g_input->WasKeyJustPressed(KEYCODE_ESC)) App::RequestQuit();
    if (g_input->WasKeyJustPressed(KEYCODE_F8))
    {
        GenerateRandomShapes();
    }

    if (g_input->WasKeyJustPressed(KEYCODE_F8)) GenerateRandomLineSegmentInScreen();
    if (g_input->IsKeyDown(KEYCODE_W)) m_lineSegment.m_startPosition.y += m_moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_S)) m_lineSegment.m_startPosition.y -= m_moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_A)) m_lineSegment.m_startPosition.x -= m_moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_D)) m_lineSegment.m_startPosition.x += m_moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_I)) m_lineSegment.m_endPosition.y += m_moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_K)) m_lineSegment.m_endPosition.y -= m_moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_J)) m_lineSegment.m_endPosition.x -= m_moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_L)) m_lineSegment.m_endPosition.x += m_moveSpeed * deltaSeconds;
    if (g_input->IsKeyDown(KEYCODE_LEFT_MOUSE)) m_lineSegment.m_startPosition = GetMouseWorldPos();
    if (g_input->IsKeyDown(KEYCODE_RIGHT_MOUSE)) m_lineSegment.m_endPosition = GetMouseWorldPos();

    if (g_input->IsKeyDown(KEYCODE_N) ||
        g_input->WasKeyJustPressed(KEYCODE_SPACE))
    {
        Ball ball;
        ball.m_position              = Vec2(m_lineSegment.m_startPosition.x, m_lineSegment.m_startPosition.y);
        ball.m_velocity              = Vec2(m_lineSegment.m_endPosition - m_lineSegment.m_startPosition) * 3.f;
        FloatRange const radiusRange = FloatRange(g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Ball.Radius", FloatRange::ZERO));
        ball.m_color                 = Interpolate(Rgba8::BLUE, Rgba8::WHITE, g_theRNG->RollRandomFloatZeroToOne());
        ball.m_radius                = g_theRNG->RollRandomFloatInRange(radiusRange.m_min, radiusRange.m_max);
        ball.m_elasticity            = m_ballElasticity;

        m_ballList.push_back(ball);
    }

    if (g_input->WasKeyJustPressed(KEYCODE_B))
    {
        m_wallList[0].m_isWarped = !m_wallList[0].m_isWarped;
        m_isWallWarpEnabled      = !m_isWallWarpEnabled;
    }

    if (g_input->WasKeyJustPressed(KEYCODE_G)) m_ballElasticity -= m_ballElasticityDelta;
    if (g_input->WasKeyJustPressed(KEYCODE_H)) m_ballElasticity += m_ballElasticityDelta;
    m_ballElasticity = GetClampedZeroToOne(m_ballElasticity);

    if (g_input->WasKeyJustPressed(KEYCODE_LEFT_BRACKET)) m_fixedTimeStep *= 0.9f;
    if (g_input->WasKeyJustPressed(KEYCODE_RIGHT_BRACKET)) m_fixedTimeStep *= 1.1f;
}

void GamePachinkoMachine2D::UpdateFromController(float deltaSeconds)
{
    UNUSED(deltaSeconds)
}

void GamePachinkoMachine2D::UpdateBall(float const timeSteps)
{
    float const gravityY = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Misc.Gravity", -1.f);

    for (int i = 0; i < (int)m_ballList.size(); i++)
    {
        m_ballList[i].m_velocity -= Vec2(0.f, gravityY) * timeSteps;

        if (m_isWallWarpEnabled)
        {
            if (m_ballList[i].m_position.y < -100.f) m_ballList[i].m_position.y = 900.f;
        }

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
                BounceDiscOutOfFixedOBB2D(m_ballList[i].m_position, m_ballList[i].m_radius, m_ballList[i].m_velocity, m_ballElasticity, m_bumperList[j].m_startPosition, iBasis, m_bumperList[j].m_halfDimension, m_bumperList[j].m_elasticity);
            }
        }
    }

    for (int i = 0; i < (int)m_ballList.size(); i++)
    {
        for (int j = 0; j < (int)m_wallList.size(); j++)
        {
            if (m_wallList[j].m_isWarped) continue;

            Vec2 iBasis = (m_wallList[j].m_endPosition - m_wallList[j].m_startPosition).GetNormalized();
            BounceDiscOutOfFixedOBB2D(m_ballList[i].m_position, m_ballList[i].m_radius, m_ballList[i].m_velocity, m_ballElasticity, m_wallList[j].m_startPosition, iBasis, m_wallList[j].m_halfDimension, m_wallList[j].m_elasticity);
        }
    }
}

void GamePachinkoMachine2D::GenerateRandomShapes()
{
    m_wallList.clear();
    m_bumperList.clear();
    int const        discNum            = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Disc.Num", -1);
    FloatRange const discRadiusRange    = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Disc.Radius", FloatRange::ZERO);
    int const        capsuleNum         = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Capsule.Num", -1);
    FloatRange const capsuleLengthRange = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Capsule.Length", FloatRange::ZERO);
    FloatRange const capsuleRadiusRange = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Capsule.Radius", FloatRange::ZERO);
    int const        obb2Num            = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Obb2.Num", -1);
    FloatRange const obb2WidthRange     = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Obb2.Width", FloatRange::ZERO);
    FloatRange const elasticityRange    = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Bumper.Elasticity", FloatRange::ZERO);
    float const      wallElasticity     = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Wall.Elasticity", -1.f);

    for (int i = 0; i < discNum; i++)
    {
        Bumper bumper          = Bumper();
        bumper.m_type          = eBumperType::DISC2;
        bumper.m_startPosition = GenerateRandomPointInScreen();
        bumper.m_radius        = g_theRNG->RollRandomFloatInRange(discRadiusRange.m_min, discRadiusRange.m_max);
        bumper.m_elasticity    = g_theRNG->RollRandomFloatInRange(elasticityRange.m_min, elasticityRange.m_max);
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
        bumper.m_halfDimension      = Vec2(g_theRNG->RollRandomFloatInRange(obb2WidthRange.m_min, obb2WidthRange.m_max), g_theRNG->RollRandomFloatInRange(obb2WidthRange.m_min, obb2WidthRange.m_max));
        bumper.m_elasticity         = g_theRNG->RollRandomFloatInRange(elasticityRange.m_min, elasticityRange.m_max);
        bumper.m_color              = Interpolate(Rgba8::RED, Rgba8::GREEN, bumper.m_elasticity);
        m_bumperList.push_back(bumper);
    }

    Wall wallA, wallB, wallC;
    wallA.m_startPosition = Vec2(800.f, -100.f);
    wallB.m_startPosition = Vec2(-100.f, 0.f);
    wallC.m_startPosition = Vec2(1700.f, 0.f);
    wallA.m_endPosition   = Vec2(800.f, 0.f);
    wallB.m_endPosition   = Vec2(-100.f, 800.f);
    wallC.m_endPosition   = Vec2(1700.f, 800.f);
    wallA.m_halfDimension = Vec2(100.f, 800.f);
    wallB.m_halfDimension = Vec2(800.f, 100.f);
    wallC.m_halfDimension = Vec2(800.f, 100.f);
    wallA.m_elasticity    = wallElasticity;
    wallB.m_elasticity    = wallElasticity;
    wallC.m_elasticity    = wallElasticity;
    m_wallList.push_back(wallA);
    m_wallList.push_back(wallB);
    m_wallList.push_back(wallC);
}

//----------------------------------------------------------------------------------------------------
void GamePachinkoMachine2D::GenerateRandomLineSegmentInScreen()
{
    float const lineThickness = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Misc.LineThickness", -1.f);

    m_lineSegment = LineSegment2(GenerateRandomPointInScreen(), GenerateRandomPointInScreen(), lineThickness, false);
}

void GamePachinkoMachine2D::RenderShapes() const
{
    VertexList_PCU   verts;
    FloatRange const discRadiusRange = g_gameConfigBlackboard.GetValue("GamePachinkoMachine2D.Ball.Radius", FloatRange::ZERO);

    AddVertsForArrow2D(verts, m_lineSegment.m_startPosition, m_lineSegment.m_endPosition, 50.f, m_lineSegment.m_thickness, Rgba8::WHITE);
    AddVertsForDisc2D(verts, m_lineSegment.m_startPosition, discRadiusRange.m_min,3.f, Rgba8::BLUE );
    AddVertsForDisc2D(verts, m_lineSegment.m_startPosition, discRadiusRange.m_max,3.f, Rgba8::BLUE );

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
            AddVertsForOBB2D(verts, m_bumperList[i].m_startPosition, iBasis, m_bumperList[i].m_halfDimension, m_bumperList[i].m_color);
        }
    }

    for (int i = 0; i < (int)m_wallList.size(); ++i)
    {
        Vec2 iBasis = (m_wallList[i].m_endPosition - m_wallList[i].m_startPosition).GetNormalized();

        AddVertsForOBB2D(verts, m_wallList[i].m_startPosition, iBasis, m_wallList[i].m_halfDimension, Rgba8::TRANSLUCENT_WHITE);
    }



    g_theRenderer->SetModelConstants();
    g_theRenderer->SetBlendMode(eBlendMode::ALPHA);
    g_theRenderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_theRenderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_theRenderer->SetDepthMode(eDepthMode::DISABLED);
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}
