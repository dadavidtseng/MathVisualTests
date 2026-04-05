//----------------------------------------------------------------------------------------------------
// GameConvexScene.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/GameConvexScene.hpp"
//----------------------------------------------------------------------------------------------------
#include "Game/App.hpp"
#include "Game/Convex.hpp"
#include "Game/GameCommon.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Core/BufferParser.hpp"
#include "Engine/Core/BufferWriter.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Math/RaycastUtils.hpp"
#include "Engine/Platform/Window.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexUtils.hpp"
#include "Engine/Resource/ResourceSubsystem.hpp"
//----------------------------------------------------------------------------------------------------
#include <algorithm>
#include <cfloat>
#include <filesystem>
#include <unordered_map>

// Windows.h redefines ERROR after DevConsole.hpp #undef'd it
#if defined(ERROR)
#undef ERROR
#endif

//----------------------------------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------------------------------
constexpr float CONVEX_WORLD_SIZE_X = 200.f;
constexpr float CONVEX_WORLD_SIZE_Y = 100.f;
constexpr float MIN_CONVEX_RADIUS   = 2.f;
constexpr float MAX_CONVEX_RADIUS   = 8.f;
constexpr int   INITIAL_CONVEX_COUNT = 8;

//----------------------------------------------------------------------------------------------------
GameConvexScene::GameConvexScene()
{
    // Screen camera matches other game modes (config-based 1600x800)
    m_screenCamera = new Camera();
    m_worldCamera  = new Camera();

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float const screenSizeY = g_gameConfigBlackboard.GetValue("screenSizeY", 800.f);

    m_screenCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(screenSizeX, screenSizeY));
    m_screenCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);
    m_worldCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(CONVEX_WORLD_SIZE_X, CONVEX_WORLD_SIZE_Y));
    m_worldCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);

    m_gameClock = new Clock(Clock::GetSystemClock());

    // Spawn initial random convexes
    for (int i = 0; i < INITIAL_CONVEX_COUNT; ++i)
    {
        Vec2 randomPos = Vec2(
            g_rng->RollRandomFloatInRange(0.f, CONVEX_WORLD_SIZE_X),
            g_rng->RollRandomFloatInRange(0.f, CONVEX_WORLD_SIZE_Y)
        );
        Convex2* convex = CreateRandomConvex(randomPos, MIN_CONVEX_RADIUS, MAX_CONVEX_RADIUS);
        m_convexes.push_back(convex);
    }

    // Initialize ray with random start/end points
    m_rayStart = Vec2(
        g_rng->RollRandomFloatInRange(0.f, CONVEX_WORLD_SIZE_X),
        g_rng->RollRandomFloatInRange(0.f, CONVEX_WORLD_SIZE_Y)
    );
    m_rayEnd = Vec2(
        g_rng->RollRandomFloatInRange(0.f, CONVEX_WORLD_SIZE_X),
        g_rng->RollRandomFloatInRange(0.f, CONVEX_WORLD_SIZE_Y)
    );

    RebuildAllTrees();

    // Register dev console commands
    g_eventSystem->SubscribeEventCallbackFunction("SaveConvexScene", SaveConvexSceneCommand);
    g_eventSystem->SubscribeEventCallbackFunction("LoadConvexScene", LoadConvexSceneCommand);
}

//----------------------------------------------------------------------------------------------------
GameConvexScene::~GameConvexScene()
{
    g_eventSystem->UnsubscribeEventCallbackFunction("SaveConvexScene", SaveConvexSceneCommand);
    g_eventSystem->UnsubscribeEventCallbackFunction("LoadConvexScene", LoadConvexSceneCommand);

    for (Convex2* convex : m_convexes)
    {
        delete convex;
    }
    m_convexes.clear();
}

//----------------------------------------------------------------------------------------------------
void GameConvexScene::Update()
{
    float const deltaSeconds = static_cast<float>(m_gameClock->GetDeltaSeconds());

    UpdateFromKeyboard(deltaSeconds);
    UpdateFromController(deltaSeconds);
}

//----------------------------------------------------------------------------------------------------
void GameConvexScene::Render() const
{
    //-Start-of-World-Camera--------------------------------------------------------------------------
    g_renderer->BeginCamera(*m_worldCamera);

    RenderConvexScene();

    g_renderer->EndCamera(*m_worldCamera);
    //-End-of-World-Camera----------------------------------------------------------------------------

    //-Start-of-Screen-Camera-------------------------------------------------------------------------
    g_renderer->BeginCamera(*m_screenCamera);

    RenderCurrentModeText("CurrentMode: ConvexScene (Raycast vs Convex Hulls)");
    RenderConvexSceneControlText();

    g_renderer->EndCamera(*m_screenCamera);
    //-End-of-Screen-Camera---------------------------------------------------------------------------
}

//----------------------------------------------------------------------------------------------------
void GameConvexScene::UpdateFromKeyboard(float const deltaSeconds)
{
    UNUSED(deltaSeconds)

    // Get cursor position for rotation/scaling
    Vec2 cursorUV  = g_window->GetNormalizedMouseUV();
    Vec2 cursorPos = m_worldCamera->GetCursorWorldPosition(cursorUV);

    // Scale interaction speeds relative to scene size
    float interactScale = GetSceneScale();

    // Handle object scaling
    if (m_hoveringConvex && g_input->IsKeyDown('L'))
    {
        m_hoveringConvex->Scale(1.f * interactScale * deltaSeconds, cursorPos);
        m_sceneModified = true;
        RebuildAllTrees();
    }
    if (m_hoveringConvex && g_input->IsKeyDown('K'))
    {
        m_hoveringConvex->Scale(-1.f * interactScale * deltaSeconds, cursorPos);
        m_sceneModified = true;
        RebuildAllTrees();
    }

    // Handle object rotation
    if (m_hoveringConvex && g_input->IsKeyDown('W'))
    {
        m_hoveringConvex->Rotate(90.f * deltaSeconds, cursorPos);
        m_sceneModified = true;
        RebuildAllTrees();
    }
    if (m_hoveringConvex && g_input->IsKeyDown('R'))
    {
        m_hoveringConvex->Rotate(-90.f * deltaSeconds, cursorPos);
        m_sceneModified = true;
        RebuildAllTrees();
    }

    // Handle object dragging
    if (m_hoveringConvex && g_input->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
    {
        m_isDragging = true;
    }

    if (m_isDragging && m_hoveringConvex && g_input->IsKeyDown(KEYCODE_LEFT_MOUSE))
    {
        Vec2 delta = cursorPos - m_cursorPrevPos;
        m_hoveringConvex->Translate(delta);
        m_cursorPrevPos = cursorPos;
        m_sceneModified = true;
        RebuildAllTrees();
    }

    if (g_input->WasKeyJustReleased(KEYCODE_LEFT_MOUSE))
    {
        m_isDragging = false;
    }

    // Update hover detection (skipped during drag for sticky focus)
    UpdateHoverDetection();

    // Update ray endpoints via mouse buttons (only when not hovering a convex)
    if (!m_hoveringConvex)
    {
        if (g_input->IsKeyDown(KEYCODE_LEFT_MOUSE))
        {
            m_rayStart = cursorPos;
        }
        if (g_input->IsKeyDown(KEYCODE_RIGHT_MOUSE))
        {
            m_rayEnd = cursorPos;
        }
    }

    if (g_input->WasKeyJustPressed(KEYCODE_F8))
    {
        // Reset to default view if a scene was loaded
        if (m_hasLoadedScene)
        {
            m_hasLoadedScene = false;
            m_worldCamera->SetOrthoGraphicView(Vec2::ZERO, Vec2(CONVEX_WORLD_SIZE_X, CONVEX_WORLD_SIZE_Y));
        }
        // Re-randomize all shapes, keeping current count
        int numShapes = static_cast<int>(m_convexes.size());
        ClearScene();
        m_sceneModified = true;
        for (int i = 0; i < numShapes; ++i)
        {
            AABB2 bounds = GetWorldBounds();
            Vec2 randomPos = Vec2(
                g_rng->RollRandomFloatInRange(bounds.m_mins.x, bounds.m_maxs.x),
                g_rng->RollRandomFloatInRange(bounds.m_mins.y, bounds.m_maxs.y)
            );
            Convex2* convex = CreateRandomConvex(randomPos, MIN_CONVEX_RADIUS, MAX_CONVEX_RADIUS);
            m_convexes.push_back(convex);
        }
        RebuildAllTrees();
    }
    else if (g_input->WasKeyJustPressed(KEYCODE_F1))
    {
        m_showBoundingDiscs = !m_showBoundingDiscs;
    }
    else if (g_input->WasKeyJustPressed(KEYCODE_F2))
    {
        m_drawEdgesMode = !m_drawEdgesMode;
    }
    else if (g_input->WasKeyJustPressed(KEYCODE_F3))
    {
        m_debugDrawBVHMode = !m_debugDrawBVHMode;
    }
    else if (g_input->WasKeyJustPressed(KEYCODE_F4))
    {
        m_showSpatialStructure = !m_showSpatialStructure;
    }
    else if (g_input->WasKeyJustPressed(KEYCODE_F9))
    {
        m_rayOptimizationMode = (m_rayOptimizationMode + 1) % 3;
    }
    else if (g_input->WasKeyJustPressed('C'))
    {
        // Spawn convex at mouse position
        Vec2 mouseUV  = g_window->GetNormalizedMouseUV();
        Vec2 worldPos = m_worldCamera->GetCursorWorldPosition(mouseUV);
        Convex2* convex = CreateRandomConvex(worldPos, MIN_CONVEX_RADIUS, MAX_CONVEX_RADIUS);
        m_convexes.push_back(convex);
    }
    else if (g_input->WasKeyJustPressed('Y'))
    {
        // Double object count (max 2048)
        int numOfShapesToAdd = static_cast<int>(m_convexes.size());
        if (numOfShapesToAdd == 0) numOfShapesToAdd = 1;
        if (static_cast<int>(m_convexes.size()) < 2048)
        {
            for (int i = 0; i < numOfShapesToAdd && static_cast<int>(m_convexes.size()) < 2048; ++i)
            {
                AABB2 bounds = GetWorldBounds();
                Vec2 randomPos = Vec2(
                    g_rng->RollRandomFloatInRange(bounds.m_mins.x, bounds.m_maxs.x),
                    g_rng->RollRandomFloatInRange(bounds.m_mins.y, bounds.m_maxs.y)
                );
                Convex2* convex = CreateRandomConvex(randomPos, MIN_CONVEX_RADIUS, MAX_CONVEX_RADIUS);
                m_convexes.push_back(convex);
            }
            RebuildAllTrees();
        }
    }
    else if (g_input->WasKeyJustPressed('U'))
    {
        // Halve object count (min 1)
        int numOfShapesToRemove = static_cast<int>(m_convexes.size()) / 2;
        if (static_cast<int>(m_convexes.size()) == 1) numOfShapesToRemove = 1;
        for (int i = 0; i < numOfShapesToRemove; ++i)
        {
            if (m_convexes.back() == m_hoveringConvex) m_hoveringConvex = nullptr;
            delete m_convexes.back();
            m_convexes.pop_back();
        }
        RebuildAllTrees();
    }
    else if (g_input->WasKeyJustPressed('M'))
    {
        if (m_numOfRandomRays < 134217728)
        {
            m_numOfRandomRays *= 2;
            if (m_numOfRandomRays > 134217728) m_numOfRandomRays = 134217728;
        }
    }
    else if (g_input->WasKeyJustPressed('N'))
    {
        m_numOfRandomRays /= 2;
        if (m_numOfRandomRays < 1) m_numOfRandomRays = 1;
    }
    else if (g_input->WasKeyJustPressed('T'))
    {
        TestRays();
    }

    // Time controls
    if (g_input->WasKeyJustPressed(KEYCODE_P)) m_gameClock->TogglePause();
    if (g_input->WasKeyJustPressed(KEYCODE_O)) m_gameClock->StepSingleFrame();
    if (g_input->WasKeyJustPressed(KEYCODE_ESC)) App::RequestQuit();
}

//----------------------------------------------------------------------------------------------------
void GameConvexScene::UpdateFromController(float const deltaSeconds)
{
    UNUSED(deltaSeconds)
}

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
float GameConvexScene::GetSceneScale() const
{
    if (!m_hasLoadedScene) return 1.f;
    float w = m_loadedSceneBounds.m_maxs.x - m_loadedSceneBounds.m_mins.x;
    float h = m_loadedSceneBounds.m_maxs.y - m_loadedSceneBounds.m_mins.y;
    float diagonal = sqrtf(w * w + h * h);
    constexpr float DEFAULT_DIAGONAL = 223.6f; // sqrt(200^2 + 100^2)
    float scale = diagonal / DEFAULT_DIAGONAL;
    return (scale < 0.01f) ? 0.01f : scale;
}

//----------------------------------------------------------------------------------------------------
AABB2 GameConvexScene::GetWorldBounds() const
{
    if (m_hasLoadedScene) return m_loadedSceneBounds;
    return AABB2(Vec2(0.f, 0.f), Vec2(CONVEX_WORLD_SIZE_X, CONVEX_WORLD_SIZE_Y));
}

//----------------------------------------------------------------------------------------------------
void GameConvexScene::RenderConvexScene() const
{
    VertexList_PCU verts;

    float const sceneScale  = GetSceneScale();
    float const edgeThick   = 0.8f * sceneScale;
    float const edgeThin    = 0.5f * sceneScale;
    float const debugStroke = 0.3f * sceneScale;

    if (m_drawEdgesMode)
    {
        // Mode B: Thick edges first, then opaque fill
        for (Convex2 const* convex : m_convexes)
        {
            if (convex == m_hoveringConvex) continue;
            AddVertsForConvexPolyEdges(verts, convex->m_convexPoly, edgeThick, Rgba8(0, 0, 153));
        }
        for (Convex2 const* convex : m_convexes)
        {
            if (convex == m_hoveringConvex) continue;
            AddVertsForConvexPoly2D(verts, convex->m_convexPoly, Rgba8(153, 204, 255));
        }
        if (m_hoveringConvex)
        {
            AddVertsForConvexPoly2D(verts, m_hoveringConvex->m_convexPoly, Rgba8(255, 255, 153));
            AddVertsForConvexPolyEdges(verts, m_hoveringConvex->m_convexPoly, edgeThick, Rgba8(255, 153, 0));
        }
    }
    else
    {
        // Mode A: Translucent fill first, then opaque edges
        for (Convex2 const* convex : m_convexes)
        {
            if (convex == m_hoveringConvex) continue;
            AddVertsForConvexPoly2D(verts, convex->m_convexPoly, Rgba8(204, 229, 255, 128));
        }
        for (Convex2 const* convex : m_convexes)
        {
            if (convex == m_hoveringConvex) continue;
            AddVertsForConvexPolyEdges(verts, convex->m_convexPoly, edgeThin, Rgba8(0, 0, 153));
        }
        if (m_hoveringConvex)
        {
            AddVertsForConvexPoly2D(verts, m_hoveringConvex->m_convexPoly, Rgba8(255, 255, 153, 128));
            AddVertsForConvexPolyEdges(verts, m_hoveringConvex->m_convexPoly, edgeThin, Rgba8(255, 153, 0));
        }
    }

    // Debug visualization: bounding discs (F1)
    if (m_showBoundingDiscs)
    {
        for (Convex2 const* convex : m_convexes)
        {
            AddVertsForDisc2D(verts, convex->m_boundingDiscCenter, convex->m_boundingRadius, debugStroke, Rgba8(0, 255, 0, 128));
        }
    }

    // Debug visualization: per-object bounding volumes (F4)
    if (m_showSpatialStructure)
    {
        for (Convex2 const* convex : m_convexes)
        {
            DebugDrawRing(convex->m_boundingDiscCenter, convex->m_boundingRadius, debugStroke, Rgba8(100, 100, 100, 160));
            AABB2 const& box = convex->m_boundingAABB;
            DebugDrawLine(box.m_mins, Vec2(box.m_mins.x, box.m_maxs.y), debugStroke, Rgba8(100, 100, 100, 160));
            DebugDrawLine(Vec2(box.m_mins.x, box.m_maxs.y), box.m_maxs, debugStroke, Rgba8(100, 100, 100, 160));
            DebugDrawLine(Vec2(box.m_maxs.x, box.m_mins.y), box.m_maxs, debugStroke, Rgba8(100, 100, 100, 160));
            DebugDrawLine(Vec2(box.m_maxs.x, box.m_mins.y), box.m_mins, debugStroke, Rgba8(100, 100, 100, 160));
        }
    }

    // Debug visualization: BVH tree node bounds (F3)
    if (m_debugDrawBVHMode)
    {
        for (auto const& node : m_AABB2Tree.m_nodes)
        {
            AABB2 const& box = node.m_bounds;
            DebugDrawLine(box.m_mins, Vec2(box.m_mins.x, box.m_maxs.y), debugStroke, Rgba8(100, 100, 100, 160));
            DebugDrawLine(Vec2(box.m_mins.x, box.m_maxs.y), box.m_maxs, debugStroke, Rgba8(100, 100, 100, 160));
            DebugDrawLine(Vec2(box.m_maxs.x, box.m_mins.y), box.m_maxs, debugStroke, Rgba8(100, 100, 100, 160));
            DebugDrawLine(Vec2(box.m_maxs.x, box.m_mins.y), box.m_mins, debugStroke, Rgba8(100, 100, 100, 160));
        }
    }

    // Loaded scene bounds outline (white rectangle to delineate scene area)
    if (m_hasLoadedScene)
    {
        AABB2 const& sb = m_loadedSceneBounds;
        Rgba8 const borderColor = Rgba8::WHITE;
        AddVertsForLineSegment2D(verts, sb.m_mins, Vec2(sb.m_maxs.x, sb.m_mins.y), debugStroke, false, borderColor);
        AddVertsForLineSegment2D(verts, Vec2(sb.m_maxs.x, sb.m_mins.y), sb.m_maxs, debugStroke, false, borderColor);
        AddVertsForLineSegment2D(verts, sb.m_maxs, Vec2(sb.m_mins.x, sb.m_maxs.y), debugStroke, false, borderColor);
        AddVertsForLineSegment2D(verts, Vec2(sb.m_mins.x, sb.m_maxs.y), sb.m_mins, debugStroke, false, borderColor);
    }

    // Raycast visualization
    RenderRaycast(verts);

    g_renderer->SetModelConstants();
    g_renderer->SetBlendMode(eBlendMode::ALPHA);
    g_renderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_renderer->SetSamplerMode(eSamplerMode::BILINEAR_CLAMP);
    g_renderer->SetDepthMode(eDepthMode::DISABLED);
    g_renderer->BindTexture(nullptr);
    g_renderer->BindShader(nullptr);
    g_renderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void GameConvexScene::RenderConvexSceneControlText() const
{
    VertexList_PCU verts;
    BitmapFont* bitmapFont = g_resourceSubsystem->CreateOrGetBitmapFontFromFile("Data/Fonts/SquirrelFixedFont");

    float const screenSizeX = g_gameConfigBlackboard.GetValue("screenSizeX", 1600.f);
    float constexpr lineHeight = 20.f;
    float yTop = 760.f;

    // Line 1: Controls
    char const* optModeNames[] = {"None", "Disc", "AABB"};
    std::string controlLine = Stringf("F8=Randomize, LMB/RMB=RayStart/End, W/R=Rotate, L/K=Scale, C=Spawn, ESC=Quit, F9=Opt(%s)", optModeNames[m_rayOptimizationMode]);
    AABB2 controlBox(Vec2(0.f, yTop - lineHeight), Vec2(screenSizeX, yTop));
    bitmapFont->AddVertsForTextInBox2D(verts, controlLine.c_str(), controlBox, lineHeight, Rgba8::GREEN);
    yTop -= lineHeight;

    // Line 2: Debug toggles + shape/ray counts
    std::string infoLine = Stringf("F1=Discs, F2=DrawMode, F3=BVH, F4=AABB | %d shapes (Y/U), %d rays (M/N), T=Test", static_cast<int>(m_convexes.size()), m_numOfRandomRays);
    AABB2 infoBox(Vec2(0.f, yTop - lineHeight), Vec2(screenSizeX, yTop));
    bitmapFont->AddVertsForTextInBox2D(verts, infoLine.c_str(), infoBox, lineHeight, Rgba8::GREEN);
    yTop -= lineHeight;

    // Line 3+: Performance results (if available)
    if (m_avgDist != 0.f)
    {
        std::string resultLine = Stringf("%d Rays Vs %d objects: avg dist %.3f", m_numOfRandomRays, static_cast<int>(m_convexes.size()), m_avgDist);
        AABB2 resultBox(Vec2(0.f, yTop - lineHeight), Vec2(screenSizeX, yTop));
        bitmapFont->AddVertsForTextInBox2D(verts, resultLine.c_str(), resultBox, lineHeight, Rgba8::YELLOW);
        yTop -= lineHeight;

        std::string timingLine1 = Stringf("No Opt: %.2fms  Disc: %.2fms  AABB: %.2fms", m_lastRayTestNormalTime, m_lastRayTestDiscRejectionTime, m_lastRayTestAABBRejectionTime);
        AABB2 timingBox1(Vec2(0.f, yTop - lineHeight), Vec2(screenSizeX, yTop));
        bitmapFont->AddVertsForTextInBox2D(verts, timingLine1.c_str(), timingBox1, lineHeight, Rgba8::YELLOW);
        yTop -= lineHeight;

        std::string timingLine2 = Stringf("QuadTree: %.2fms  BVH: %.2fms", m_lastRayTestSymmetricTreeTime, m_lastRayTestAABBTreeTime);
        AABB2 timingBox2(Vec2(0.f, yTop - lineHeight), Vec2(screenSizeX, yTop));
        bitmapFont->AddVertsForTextInBox2D(verts, timingLine2.c_str(), timingBox2, lineHeight, Rgba8::YELLOW);
    }

    g_renderer->SetModelConstants();
    g_renderer->SetBlendMode(eBlendMode::ALPHA);
    g_renderer->SetRasterizerMode(eRasterizerMode::SOLID_CULL_NONE);
    g_renderer->SetSamplerMode(eSamplerMode::POINT_CLAMP);
    g_renderer->SetDepthMode(eDepthMode::DISABLED);
    g_renderer->BindTexture(&bitmapFont->GetTexture());
    g_renderer->DrawVertexArray(static_cast<int>(verts.size()), verts.data());
}

//----------------------------------------------------------------------------------------------------
void GameConvexScene::AddVertsForConvexPolyEdges(std::vector<Vertex_PCU>& verts, ConvexPoly2 const& convexPoly2, float thickness, Rgba8 const& color) const
{
    std::vector<Vec2> const& points = convexPoly2.GetVertexArray();
    int numPoints = static_cast<int>(points.size());

    for (int i = 0; i < numPoints; ++i)
    {
        Vec2 const& start = points[i];
        Vec2 const& end   = points[(i + 1) % numPoints];
        AddVertsForLineSegment2D(verts, start, end, thickness, false, color);
    }
}

//----------------------------------------------------------------------------------------------------
void GameConvexScene::RenderRaycast(std::vector<Vertex_PCU>& verts) const
{
    float const sceneScale     = GetSceneScale();
    float const rayThickness    = 0.3f * sceneScale;
    float const normalLength    = 3.f  * sceneScale;
    float const normalThickness = 0.3f * sceneScale;
    float const normalArrowSize = 1.f  * sceneScale;

    float rayMaxLength = (m_rayEnd - m_rayStart).GetLength();
    if (rayMaxLength < 0.001f) return;
    Vec2 rayNormal = (m_rayEnd - m_rayStart) / rayMaxLength;

    // Find closest raycast hit across all convexes
    RaycastResult2D closestResult;
    closestResult.m_didImpact = false;

    for (Convex2* convex : m_convexes)
    {
        RaycastResult2D result;
        bool discRejection = (m_rayOptimizationMode == 1);
        bool boxRejection  = (m_rayOptimizationMode == 2);
        bool didHit = convex->RayCastVsConvex2D(result, m_rayStart, rayNormal, rayMaxLength, discRejection, boxRejection);
        if (didHit && (!closestResult.m_didImpact || result.m_impactLength < closestResult.m_impactLength))
        {
            closestResult = result;
        }
    }

    // Full ray arrow (white)
    AddVertsForArrow2D(verts, m_rayStart, m_rayEnd, normalArrowSize, rayThickness, Rgba8(200, 200, 200));

    if (closestResult.m_didImpact)
    {
        Vec2 const& impactPos    = closestResult.m_impactPosition;
        Vec2 const& impactNormal = closestResult.m_impactNormal;

        // Green segment from start to impact
        AddVertsForLineSegment2D(verts, m_rayStart, impactPos, rayThickness, false, Rgba8(0, 255, 0));

        // Red impact normal arrow
        Vec2 normalEnd = impactPos + impactNormal * normalLength;
        AddVertsForArrow2D(verts, impactPos, normalEnd, normalArrowSize, normalThickness, Rgba8(255, 0, 0));
    }

    // Single object mode: draw infinite plane lines
    if (m_convexes.size() == 1)
    {
        for (auto const& plane : m_convexes[0]->m_convexHull.m_boundingPlanes)
        {
            float altitude = plane.GetAltitudeOfPoint(m_rayStart);
            float NdotF    = DotProduct2D(rayNormal, plane.m_normal);

            float planeLineHalfLen = 1000.f * sceneScale;
            float planeLineWidth   = 0.2f * sceneScale;
            float discRadius       = 0.5f * sceneScale;
            Vec2 vert1 = plane.GetOriginPoint() + planeLineHalfLen * plane.m_normal.GetRotated90Degrees();
            Vec2 vert2 = plane.GetOriginPoint() - planeLineHalfLen * plane.m_normal.GetRotated90Degrees();

            if (altitude > 0.f && NdotF < 0.f)
            {
                AddVertsForLineSegment2D(verts, vert1, vert2, planeLineWidth, false, Rgba8(255, 0, 255));
                float SdotN = DotProduct2D(m_rayStart, plane.m_normal);
                float dist  = (plane.m_distanceFromOrigin - SdotN) / NdotF;
                AddVertsForDisc2D(verts, m_rayStart + dist * rayNormal, discRadius, Rgba8(255, 0, 255));
            }
            else if (altitude > 0.f && NdotF >= 0.f)
            {
                AddVertsForLineSegment2D(verts, vert1, vert2, planeLineWidth, false, Rgba8(255, 0, 0));
            }
            else if (altitude <= 0.f && NdotF < 0.f)
            {
                AddVertsForLineSegment2D(verts, vert1, vert2, planeLineWidth, false, Rgba8(0, 255, 0));
            }
            else
            {
                AddVertsForLineSegment2D(verts, vert1, vert2, planeLineWidth, false, Rgba8(0, 255, 255));
                float SdotN = DotProduct2D(m_rayStart, plane.m_normal);
                float dist  = (plane.m_distanceFromOrigin - SdotN) / NdotF;
                AddVertsForDisc2D(verts, m_rayStart + dist * rayNormal, discRadius, Rgba8(0, 255, 255));
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------
Convex2* GameConvexScene::CreateRandomConvex(Vec2 const& center, float minRadius, float maxRadius)
{
    int numSides = g_rng->RollRandomIntInRange(3, 8);
    float radius = g_rng->RollRandomFloatInRange(minRadius, maxRadius);

    float angleStep = 360.f / static_cast<float>(numSides);
    std::vector<float> angles;
    for (int i = 0; i < numSides; ++i)
    {
        float baseAngle      = angleStep * static_cast<float>(i);
        float angleVariation = g_rng->RollRandomFloatInRange(-angleStep * 0.3f, angleStep * 0.3f);
        angles.push_back(baseAngle + angleVariation);
    }
    std::sort(angles.begin(), angles.end());

    std::vector<Vec2> vertices;
    for (int i = 0; i < numSides; ++i)
    {
        Vec2 vertex = center + Vec2::MakeFromPolarDegrees(angles[i], radius);
        vertices.push_back(vertex);
    }

    ConvexPoly2 poly(vertices);
    return new Convex2(poly);
}

//----------------------------------------------------------------------------------------------------
void GameConvexScene::TestRays()
{
    RebuildAllTrees();

    int numRays = m_numOfRandomRays;

    std::vector<Vec2>  rayStartPos(numRays);
    std::vector<Vec2>  rayForwardNormal(numRays);
    std::vector<float> rayMaxDist(numRays);

    AABB2 worldBounds = GetWorldBounds();
    for (int j = 0; j < numRays; ++j)
    {
        Vec2 p1(g_rng->RollRandomFloatInRange(worldBounds.m_mins.x, worldBounds.m_maxs.x),
                g_rng->RollRandomFloatInRange(worldBounds.m_mins.y, worldBounds.m_maxs.y));
        Vec2 p2(g_rng->RollRandomFloatInRange(worldBounds.m_mins.x, worldBounds.m_maxs.x),
                g_rng->RollRandomFloatInRange(worldBounds.m_mins.y, worldBounds.m_maxs.y));
        rayStartPos[j]      = p1;
        Vec2 disp            = p2 - p1;
        rayMaxDist[j]        = disp.GetLength();
        rayForwardNormal[j]  = disp.GetNormalized();
    }

    RaycastResult2D rayRes;
    double startTime, endTime;
    float sumDist;
    int numOfRayHit, correctNumOfRayHit;

    // Mode 1: No optimization (baseline)
    sumDist = 0.f; numOfRayHit = 0;
    startTime = GetCurrentTimeSeconds();
    for (int j = 0; j < numRays; ++j)
    {
        float minDist = FLT_MAX;
        for (int i = 0; i < static_cast<int>(m_convexes.size()); ++i)
        {
            if (m_convexes[i]->RayCastVsConvex2D(rayRes, rayStartPos[j], rayForwardNormal[j], rayMaxDist[j], false, false))
            {
                if (rayRes.m_impactLength < minDist) minDist = rayRes.m_impactLength;
            }
        }
        if (minDist != FLT_MAX) { sumDist += minDist; ++numOfRayHit; }
    }
    endTime = GetCurrentTimeSeconds();
    m_avgDist = sumDist / static_cast<float>(numOfRayHit);
    correctNumOfRayHit = numOfRayHit;
    m_lastRayTestNormalTime = static_cast<float>((endTime - startTime) * 1000.0);

    // Mode 2: Disc rejection
    sumDist = 0.f; numOfRayHit = 0;
    startTime = GetCurrentTimeSeconds();
    for (int j = 0; j < numRays; ++j)
    {
        float minDist = FLT_MAX;
        for (int i = 0; i < static_cast<int>(m_convexes.size()); ++i)
        {
            if (m_convexes[i]->RayCastVsConvex2D(rayRes, rayStartPos[j], rayForwardNormal[j], rayMaxDist[j], true, false))
            {
                if (rayRes.m_impactLength < minDist) minDist = rayRes.m_impactLength;
            }
        }
        if (minDist != FLT_MAX) { sumDist += minDist; ++numOfRayHit; }
    }
    endTime = GetCurrentTimeSeconds();
    GUARANTEE_OR_DIE(numOfRayHit == correctNumOfRayHit, "Disc rejection mismatch");
    m_lastRayTestDiscRejectionTime = static_cast<float>((endTime - startTime) * 1000.0);

    // Mode 3: AABB rejection
    sumDist = 0.f; numOfRayHit = 0;
    startTime = GetCurrentTimeSeconds();
    for (int j = 0; j < numRays; ++j)
    {
        float minDist = FLT_MAX;
        for (int i = 0; i < static_cast<int>(m_convexes.size()); ++i)
        {
            if (m_convexes[i]->RayCastVsConvex2D(rayRes, rayStartPos[j], rayForwardNormal[j], rayMaxDist[j], true, true))
            {
                if (rayRes.m_impactLength < minDist) minDist = rayRes.m_impactLength;
            }
        }
        if (minDist != FLT_MAX) { sumDist += minDist; ++numOfRayHit; }
    }
    endTime = GetCurrentTimeSeconds();
    GUARANTEE_OR_DIE(numOfRayHit == correctNumOfRayHit, "AABB rejection mismatch");
    m_lastRayTestAABBRejectionTime = static_cast<float>((endTime - startTime) * 1000.0);

    // Mode 4: QuadTree
    sumDist = 0.f; numOfRayHit = 0;
    startTime = GetCurrentTimeSeconds();
    for (int j = 0; j < numRays; ++j)
    {
        float minDist = FLT_MAX;
        std::vector<Convex2*> candidates;
        m_symQuadTree.SolveRayResult(rayStartPos[j], rayForwardNormal[j], rayMaxDist[j], m_convexes, candidates);
        for (int i = 0; i < static_cast<int>(candidates.size()); ++i)
        {
            if (candidates[i]->RayCastVsConvex2D(rayRes, rayStartPos[j], rayForwardNormal[j], rayMaxDist[j], true, true))
            {
                if (rayRes.m_impactLength < minDist) minDist = rayRes.m_impactLength;
            }
        }
        if (minDist != FLT_MAX) { sumDist += minDist; ++numOfRayHit; }
    }
    endTime = GetCurrentTimeSeconds();
    GUARANTEE_OR_DIE(numOfRayHit == correctNumOfRayHit, "QuadTree mismatch");
    m_lastRayTestSymmetricTreeTime = static_cast<float>((endTime - startTime) * 1000.0);

    // Mode 5: BVH (AABB2Tree)
    sumDist = 0.f; numOfRayHit = 0;
    startTime = GetCurrentTimeSeconds();
    for (int j = 0; j < numRays; ++j)
    {
        float minDist = FLT_MAX;
        std::vector<Convex2*> candidates;
        m_AABB2Tree.SolveRayResult(rayStartPos[j], rayForwardNormal[j], rayMaxDist[j], candidates);
        for (int i = 0; i < static_cast<int>(candidates.size()); ++i)
        {
            if (candidates[i]->RayCastVsConvex2D(rayRes, rayStartPos[j], rayForwardNormal[j], rayMaxDist[j], true, true))
            {
                if (rayRes.m_impactLength < minDist) minDist = rayRes.m_impactLength;
            }
        }
        if (minDist != FLT_MAX) { sumDist += minDist; ++numOfRayHit; }
    }
    endTime = GetCurrentTimeSeconds();
    GUARANTEE_OR_DIE(numOfRayHit == correctNumOfRayHit, "BVH mismatch");
    m_lastRayTestAABBTreeTime = static_cast<float>((endTime - startTime) * 1000.0);
}

//----------------------------------------------------------------------------------------------------
void GameConvexScene::RebuildAllTrees()
{
    AABB2 totalBounds = GetWorldBounds();

    int numConvexes = static_cast<int>(m_convexes.size());
    int bvhDepth = 0;
    if (numConvexes > 0)
    {
        bvhDepth = static_cast<int>(log2(static_cast<double>(numConvexes))) - 3;
        if (bvhDepth < 3) bvhDepth = 3;
    }

    m_AABB2Tree.BuildTree(m_convexes, bvhDepth, totalBounds);
    m_symQuadTree.BuildTree(m_convexes, 4, totalBounds);
}

//----------------------------------------------------------------------------------------------------
void GameConvexScene::ClearScene()
{
    for (Convex2* convex : m_convexes)
    {
        delete convex;
    }
    m_convexes.clear();
    m_hoveringConvex = nullptr;
    m_isDragging = false;
}

//----------------------------------------------------------------------------------------------------
void GameConvexScene::UpdateHoverDetection()
{
    if (m_isDragging) return;

    Vec2 cursorUV  = g_window->GetNormalizedMouseUV();
    Vec2 cursorPos = m_worldCamera->GetCursorWorldPosition(cursorUV);

    m_hoveringConvex = nullptr;
    for (int i = static_cast<int>(m_convexes.size()) - 1; i >= 0; --i)
    {
        if (m_convexes[i]->IsPointInside(cursorPos))
        {
            m_hoveringConvex = m_convexes[i];
            break;
        }
    }

    m_cursorPrevPos = cursorPos;
}

//----------------------------------------------------------------------------------------------------
STATIC bool GameConvexScene::SaveConvexSceneCommand(EventArgs& args)
{
    std::string name = args.GetValue("name", "default");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, Stringf("> SaveConvexScene name=%s", name.c_str()));
    GameConvexScene* scene = static_cast<GameConvexScene*>(g_game);
    if (scene->SaveSceneToFile("Data/Scenes/" + name + ".ghcs"))
    {
        g_devConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("Saved scene to Data/Scenes/%s.ghcs", name.c_str()));
    }
    return true;
}

//----------------------------------------------------------------------------------------------------
STATIC bool GameConvexScene::LoadConvexSceneCommand(EventArgs& args)
{
    std::string name = args.GetValue("name", "default");
    g_devConsole->AddLine(DevConsole::INFO_MINOR, Stringf("> LoadConvexScene name=%s", name.c_str()));
    GameConvexScene* scene = static_cast<GameConvexScene*>(g_game);
    if (scene->LoadSceneFromFile("Data/Scenes/" + name + ".ghcs"))
    {
        g_devConsole->AddLine(DevConsole::INFO_MAJOR, Stringf("Loaded scene from Data/Scenes/%s.ghcs", name.c_str()));
    }
    return true;
}

//----------------------------------------------------------------------------------------------------
bool GameConvexScene::SaveSceneToFile(std::string const& filePath)
{
    constexpr unsigned int CHUNK_HEADER_SIZE = 10;
    constexpr unsigned int CHUNK_FOOTER_SIZE = 4;
    constexpr unsigned int CHUNK_OVERHEAD    = CHUNK_HEADER_SIZE + CHUNK_FOOTER_SIZE;

    struct ChunkInfo
    {
        uint8_t type;
        size_t  startPos;
        size_t  dataStart;
        size_t  dataEnd;
    };

    std::vector<ChunkInfo> chunkInfos;
    std::vector<uint8_t> buffer;
    buffer.reserve(4096);
    BufferWriter bufWrite(buffer);
    bufWrite.SetEndianMode(eEndianMode::LITTLE);

    // --- File Header (28 bytes) ---
    bufWrite.AppendChar('G');
    bufWrite.AppendChar('H');
    bufWrite.AppendChar('C');
    bufWrite.AppendChar('S');
    bufWrite.AppendByte(34);  // cohort
    bufWrite.AppendByte(2);   // major version
    bufWrite.AppendByte(1);   // minor version
    bufWrite.AppendByte(1);   // endianness: 1=LE
    bufWrite.AppendUint32(0); // placeholder for total file size
    bufWrite.AppendByte(1);   // hash type: 1=simple rolling hash
    bufWrite.AppendByte(0);
    bufWrite.AppendByte(0);
    bufWrite.AppendByte(0);
    bufWrite.AppendUint32(0); // placeholder for data hash
    bufWrite.AppendUint32(0); // placeholder for ToC offset
    bufWrite.AppendChar('E');
    bufWrite.AppendChar('N');
    bufWrite.AppendChar('D');
    bufWrite.AppendChar('H');

    auto BeginChunk = [&](uint8_t chunkType) -> size_t
    {
        ChunkInfo info;
        info.type = chunkType;
        info.startPos = bufWrite.GetTotalSize();
        bufWrite.AppendChar('G');
        bufWrite.AppendChar('H');
        bufWrite.AppendChar('C');
        bufWrite.AppendChar('K');
        bufWrite.AppendByte(chunkType);
        bufWrite.AppendByte(1);
        bufWrite.AppendUint32(0);
        info.dataStart = bufWrite.GetTotalSize();
        info.dataEnd = 0;
        chunkInfos.push_back(info);
        return chunkInfos.size() - 1;
    };

    auto EndChunk = [&](size_t chunkIndex)
    {
        ChunkInfo& info = chunkInfos[chunkIndex];
        info.dataEnd = bufWrite.GetTotalSize();
        unsigned int dataSize = static_cast<unsigned int>(info.dataEnd - info.dataStart);
        bufWrite.OverwriteUint32(info.dataStart - sizeof(unsigned int), dataSize);
        bufWrite.AppendChar('E');
        bufWrite.AppendChar('N');
        bufWrite.AppendChar('D');
        bufWrite.AppendChar('C');
    };

    // --- Chunk 0x01: SceneInfo ---
    {
        size_t idx = BeginChunk(0x01);
        AABB2 cameraBounds(m_worldCamera->GetOrthographicBottomLeft(), m_worldCamera->GetOrthographicTopRight());
        bufWrite.AppendAABB2(cameraBounds);
        bufWrite.AppendUshort(static_cast<unsigned short>(m_convexes.size()));
        EndChunk(idx);
    }

    // --- Chunk 0x02: ConvexPolys ---
    {
        size_t idx = BeginChunk(0x02);
        bufWrite.AppendUshort(static_cast<unsigned short>(m_convexes.size()));
        for (Convex2 const* convex : m_convexes)
        {
            std::vector<Vec2> const& verts = convex->m_convexPoly.GetVertexArray();
            bufWrite.AppendByte(static_cast<uint8_t>(verts.size()));
            for (Vec2 const& v : verts)
            {
                bufWrite.AppendVec2(v);
            }
        }
        EndChunk(idx);
    }

    // --- Chunk 0x81: BoundingDiscs ---
    {
        size_t idx = BeginChunk(0x81);
        bufWrite.AppendUshort(static_cast<unsigned short>(m_convexes.size()));
        for (Convex2 const* convex : m_convexes)
        {
            bufWrite.AppendVec2(convex->m_boundingDiscCenter);
            bufWrite.AppendFloat(convex->m_boundingRadius);
        }
        EndChunk(idx);
    }

    // --- Chunk 0x80: ConvexHulls ---
    {
        size_t idx = BeginChunk(0x80);
        bufWrite.AppendUshort(static_cast<unsigned short>(m_convexes.size()));
        for (Convex2 const* convex : m_convexes)
        {
            std::vector<Plane2> const& planes = convex->m_convexHull.m_boundingPlanes;
            bufWrite.AppendByte(static_cast<uint8_t>(planes.size()));
            for (Plane2 const& p : planes)
            {
                bufWrite.AppendPlane2(p);
            }
        }
        EndChunk(idx);
    }

    // --- Chunk 0x82: BoundingAABBs ---
    {
        size_t idx = BeginChunk(0x82);
        bufWrite.AppendUshort(static_cast<unsigned short>(m_convexes.size()));
        for (Convex2 const* convex : m_convexes)
        {
            bufWrite.AppendAABB2(convex->m_boundingAABB);
        }
        EndChunk(idx);
    }

    // --- Build pointer-to-index map for tree serialization ---
    std::unordered_map<Convex2*, uint16_t> convexIndexMap;
    for (uint16_t i = 0; i < static_cast<uint16_t>(m_convexes.size()); ++i)
    {
        convexIndexMap[m_convexes[i]] = i;
    }

    // --- Chunk 0x83: AABB2 Tree (BVH) ---
    if (!m_AABB2Tree.m_nodes.empty())
    {
        size_t idx = BeginChunk(0x83);
        bufWrite.AppendByte(static_cast<uint8_t>(m_AABB2Tree.m_nodes.size() > 0 ? 1 : 0));
        unsigned int numNodes = static_cast<unsigned int>(m_AABB2Tree.m_nodes.size());
        bufWrite.AppendUint32(numNodes);
        for (unsigned int n = 0; n < numNodes; ++n)
        {
            AABB2TreeNode const& node = m_AABB2Tree.m_nodes[n];
            bufWrite.AppendAABB2(node.m_bounds);
            int leftChild  = static_cast<int>(n) * 2 + 1;
            int rightChild = static_cast<int>(n) * 2 + 2;
            bufWrite.AppendInt32(leftChild < static_cast<int>(numNodes) ? leftChild : -1);
            bufWrite.AppendInt32(rightChild < static_cast<int>(numNodes) ? rightChild : -1);
            bufWrite.AppendUint32(static_cast<unsigned int>(node.m_containingConvex.size()));
            for (Convex2* convex : node.m_containingConvex)
            {
                auto it = convexIndexMap.find(convex);
                bufWrite.AppendUshort(it != convexIndexMap.end() ? it->second : static_cast<unsigned short>(0xFFFF));
            }
        }
        EndChunk(idx);
    }

    // --- Chunk 0x87: Symmetric Quadtree (QuadKey format) ---
    if (!m_symQuadTree.m_nodes.empty())
    {
        struct QuadKeyEntry
        {
            uint8_t  depth;
            uint16_t x;
            uint16_t y;
            std::vector<uint16_t> objectIndices;
        };
        std::vector<QuadKeyEntry> entries;

        int levelStart = 0;
        int levelSize  = 1;
        int depth      = 0;
        int totalNodes = static_cast<int>(m_symQuadTree.m_nodes.size());

        while (levelStart < totalNodes)
        {
            int gridDim = 1 << depth;
            for (int j = 0; j < levelSize && (levelStart + j) < totalNodes; ++j)
            {
                int nodeIdx = levelStart + j;
                SymmetricQuadTreeNode const& node = m_symQuadTree.m_nodes[nodeIdx];
                if (!node.m_containingConvex.empty())
                {
                    QuadKeyEntry entry;
                    entry.depth = static_cast<uint8_t>(depth);
                    entry.x = static_cast<uint16_t>(j % gridDim);
                    entry.y = static_cast<uint16_t>(j / gridDim);
                    for (Convex2* convex : node.m_containingConvex)
                    {
                        auto it = convexIndexMap.find(convex);
                        entry.objectIndices.push_back(it != convexIndexMap.end() ? it->second : static_cast<uint16_t>(0xFFFF));
                    }
                    entries.push_back(entry);
                }
            }
            levelStart += levelSize;
            levelSize *= 4;
            ++depth;
        }

        size_t idx = BeginChunk(0x87);
        bufWrite.AppendUint32(static_cast<unsigned int>(entries.size()));
        for (QuadKeyEntry const& entry : entries)
        {
            bufWrite.AppendByte(entry.depth);
            bufWrite.AppendUshort(entry.x);
            bufWrite.AppendUshort(entry.y);
            bufWrite.AppendUshort(static_cast<unsigned short>(entry.objectIndices.size()));
            for (uint16_t objIdx : entry.objectIndices)
            {
                bufWrite.AppendUshort(objIdx);
            }
        }
        EndChunk(idx);
    }

    // --- Write preserved unrecognized chunks ---
    if (!m_sceneModified)
    {
        for (UnrecognizedChunk const& preserved : m_preservedChunks)
        {
            size_t preservedStart = bufWrite.GetTotalSize();
            for (uint8_t byte : preserved.rawData)
            {
                bufWrite.AppendByte(byte);
            }
            ChunkInfo info;
            info.type      = preserved.chunkType;
            info.startPos  = preservedStart;
            info.dataStart = preservedStart + CHUNK_HEADER_SIZE;
            info.dataEnd   = bufWrite.GetTotalSize() - CHUNK_FOOTER_SIZE;
            chunkInfos.push_back(info);
        }
    }

    // --- Backpatch ToC offset ---
    bufWrite.OverwriteUint32(20, static_cast<unsigned int>(bufWrite.GetTotalSize()));

    // --- Table of Contents ---
    bufWrite.AppendChar('G');
    bufWrite.AppendChar('H');
    bufWrite.AppendChar('T');
    bufWrite.AppendChar('C');
    bufWrite.AppendByte(static_cast<uint8_t>(chunkInfos.size()));
    for (ChunkInfo const& info : chunkInfos)
    {
        unsigned int chunkTotalSize = static_cast<unsigned int>(info.dataEnd - info.dataStart) + CHUNK_OVERHEAD;
        bufWrite.AppendByte(info.type);
        bufWrite.AppendUint32(static_cast<unsigned int>(info.startPos));
        bufWrite.AppendUint32(chunkTotalSize);
    }
    bufWrite.AppendChar('E');
    bufWrite.AppendChar('N');
    bufWrite.AppendChar('D');
    bufWrite.AppendChar('T');

    // --- Backpatch total file size ---
    bufWrite.OverwriteUint32(8, static_cast<unsigned int>(buffer.size()));

    // --- Backpatch data hash ---
    {
        constexpr size_t HEADER_SIZE = 28;
        unsigned int hash = 0;
        for (size_t i = HEADER_SIZE; i < buffer.size(); ++i)
        {
            hash *= 31;
            hash += buffer[i];
        }
        bufWrite.OverwriteUint32(16, hash);
    }

    // --- Write to file ---
    size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        EnsureDirectoryExists(filePath.substr(0, lastSlash));
    }
    FileWriteFromBuffer(buffer, filePath);
    return true;
}

//----------------------------------------------------------------------------------------------------
bool GameConvexScene::LoadSceneFromFile(std::string const& filePath)
{
    if (!std::filesystem::exists(filePath))
    {
        g_devConsole->AddLine(DevConsole::ERROR, Stringf("Error: File not found: %s", filePath.c_str()));
        return false;
    }
    if (std::filesystem::file_size(filePath) == 0)
    {
        g_devConsole->AddLine(DevConsole::ERROR, Stringf("Error: File is empty: %s", filePath.c_str()));
        return false;
    }

    std::vector<uint8_t> buffer;
    if (!FileReadToBuffer(buffer, filePath))
    {
        g_devConsole->AddLine(DevConsole::ERROR, Stringf("Error: Could not read file %s", filePath.c_str()));
        return false;
    }

    BufferParser bufParse(buffer);

    if (buffer.size() < 37)
    {
        g_devConsole->AddLine(DevConsole::ERROR, Stringf("Error: File too small (%zu bytes)", buffer.size()));
        return false;
    }

    // --- Parse file header ---
    char magic0 = bufParse.ParseChar();
    char magic1 = bufParse.ParseChar();
    char magic2 = bufParse.ParseChar();
    char magic3 = bufParse.ParseChar();
    if (magic0 != 'G' || magic1 != 'H' || magic2 != 'C' || magic3 != 'S')
    {
        g_devConsole->AddLine(DevConsole::ERROR, "Error: Invalid GHCS file header");
        return false;
    }

    uint8_t cohort       = bufParse.ParseByte();
    uint8_t majorVersion = bufParse.ParseByte();
    uint8_t minorVersion = bufParse.ParseByte();
    uint8_t endianByte   = bufParse.ParseByte();
    UNUSED(cohort); UNUSED(majorVersion); UNUSED(minorVersion);

    if (endianByte == 1)
        bufParse.SetEndianMode(eEndianMode::LITTLE);
    else if (endianByte == 2)
        bufParse.SetEndianMode(eEndianMode::BIG);
    else
    {
        g_devConsole->AddLine(DevConsole::ERROR, Stringf("Error: Invalid endianness byte %d", endianByte));
        return false;
    }

    unsigned int totalFileSize = bufParse.ParseUint32();
    uint8_t hashType = bufParse.ParseByte();
    bufParse.ParseByte(); bufParse.ParseByte(); bufParse.ParseByte(); // padding
    unsigned int storedHash = bufParse.ParseUint32();
    unsigned int tocOffset  = bufParse.ParseUint32();

    if (totalFileSize != static_cast<unsigned int>(buffer.size()))
    {
        g_devConsole->AddLine(DevConsole::WARNING, Stringf("Warning: totalFileSize mismatch"));
    }

    // Validate hash
    {
        constexpr size_t HEADER_SIZE = 28;
        if (hashType == 1)
        {
            unsigned int computedHash = 0;
            for (size_t i = HEADER_SIZE; i < buffer.size(); ++i)
            {
                computedHash *= 31;
                computedHash += buffer[i];
            }
            if (storedHash != computedHash)
                g_devConsole->AddLine(DevConsole::WARNING, "Warning: data hash mismatch");
        }
        else if (hashType == 2)
        {
            unsigned int computedHash = 2166136261u;
            for (size_t i = HEADER_SIZE; i < buffer.size(); ++i)
            {
                computedHash ^= buffer[i];
                computedHash *= 16777619u;
            }
            if (storedHash != computedHash)
                g_devConsole->AddLine(DevConsole::WARNING, "Warning: FNV-1a hash mismatch");
        }
    }

    if (bufParse.ParseChar() != 'E' || bufParse.ParseChar() != 'N' ||
        bufParse.ParseChar() != 'D' || bufParse.ParseChar() != 'H')
    {
        g_devConsole->AddLine(DevConsole::ERROR, "Error: Missing ENDH footer");
        return false;
    }

    // --- Jump to Table of Contents ---
    if (static_cast<size_t>(tocOffset) + 9 > buffer.size())
    {
        g_devConsole->AddLine(DevConsole::ERROR, "Error: ToC offset exceeds buffer");
        return false;
    }
    bufParse.SetCurrentPosition(static_cast<size_t>(tocOffset));

    if (bufParse.ParseChar() != 'G' || bufParse.ParseChar() != 'H' ||
        bufParse.ParseChar() != 'T' || bufParse.ParseChar() != 'C')
    {
        g_devConsole->AddLine(DevConsole::ERROR, "Error: Invalid ToC magic");
        return false;
    }

    struct ToCEntry { uint8_t type; unsigned int startPos; unsigned int totalSize; };
    uint8_t numChunks = bufParse.ParseByte();
    std::vector<ToCEntry> tocEntries;
    for (int i = 0; i < static_cast<int>(numChunks); ++i)
    {
        ToCEntry entry;
        entry.type      = bufParse.ParseByte();
        entry.startPos  = bufParse.ParseUint32();
        entry.totalSize = bufParse.ParseUint32();
        tocEntries.push_back(entry);
    }

    if (bufParse.ParseChar() != 'E' || bufParse.ParseChar() != 'N' ||
        bufParse.ParseChar() != 'D' || bufParse.ParseChar() != 'T')
    {
        g_devConsole->AddLine(DevConsole::ERROR, "Error: Missing ENDT footer");
        return false;
    }

    // --- Process each chunk ---
    std::vector<Convex2*> tempConvexes;
    std::vector<UnrecognizedChunk> tempPreservedChunks;
    AABB2    sceneBounds;
    uint16_t recordedNumObjects = static_cast<uint16_t>(-1);
    bool     hasSceneInfo     = false;
    bool     hasConvexPolys   = false;
    bool     hasConvexHulls   = false;
    bool     hasBoundingDiscs = false;
    bool     hasBoundingAABBs = false;
    bool     hasAABB2Tree     = false;
    bool     hasSymQuadTree   = false;
    AABB2Tree         tempAABB2Tree;
    SymmetricQuadTree tempSymQuadTree;

    for (ToCEntry const& entry : tocEntries)
    {
        if (static_cast<size_t>(entry.startPos) + 14 > buffer.size())
        {
            g_devConsole->AddLine(DevConsole::ERROR, "Error: Chunk startPos exceeds buffer");
            for (Convex2* c : tempConvexes) delete c;
            return false;
        }
        bufParse.SetCurrentPosition(static_cast<size_t>(entry.startPos));
        size_t chunkStartPos = bufParse.GetCurrentPosition();

        if (bufParse.ParseChar() != 'G' || bufParse.ParseChar() != 'H' ||
            bufParse.ParseChar() != 'C' || bufParse.ParseChar() != 'K')
        {
            g_devConsole->AddLine(DevConsole::ERROR, "Error: Invalid chunk header");
            for (Convex2* c : tempConvexes) delete c;
            return false;
        }

        uint8_t chunkType   = bufParse.ParseByte();
        uint8_t chunkEndian = bufParse.ParseByte();
        if (chunkEndian == 1) bufParse.SetEndianMode(eEndianMode::LITTLE);
        else if (chunkEndian == 2) bufParse.SetEndianMode(eEndianMode::BIG);

        unsigned int dataSize    = bufParse.ParseUint32();
        size_t       dataStartPos = bufParse.GetCurrentPosition();

        if (dataStartPos + static_cast<size_t>(dataSize) + 4 > buffer.size())
        {
            g_devConsole->AddLine(DevConsole::ERROR, "Error: Chunk data exceeds buffer");
            for (Convex2* c : tempConvexes) delete c;
            return false;
        }

        if (chunkType == 0x01) // SceneInfo
        {
            hasSceneInfo = true;
            sceneBounds = bufParse.ParseAABB2();
            recordedNumObjects = bufParse.ParseUshort();
        }
        else if (chunkType == 0x02) // ConvexPolys
        {
            hasConvexPolys = true;
            uint16_t numObjects = bufParse.ParseUshort();
            for (int i = 0; i < static_cast<int>(numObjects); ++i)
            {
                uint8_t numVerts = bufParse.ParseByte();
                std::vector<Vec2> verts;
                for (int j = 0; j < static_cast<int>(numVerts); ++j)
                {
                    verts.push_back(bufParse.ParseVec2());
                }
                Convex2* newConvex = new Convex2();
                newConvex->m_convexPoly = ConvexPoly2(verts);
                tempConvexes.push_back(newConvex);
            }
        }
        else if (chunkType == 0x80) // ConvexHulls
        {
            hasConvexHulls = true;
            uint16_t numObjects = bufParse.ParseUshort();
            for (int i = 0; i < static_cast<int>(numObjects) && i < static_cast<int>(tempConvexes.size()); ++i)
            {
                uint8_t numPlanes = bufParse.ParseByte();
                std::vector<Plane2> planes;
                for (int j = 0; j < static_cast<int>(numPlanes); ++j)
                {
                    planes.push_back(bufParse.ParsePlane2());
                }
                tempConvexes[i]->m_convexHull = ConvexHull2(planes);
            }
        }
        else if (chunkType == 0x81) // BoundingDiscs
        {
            hasBoundingDiscs = true;
            uint16_t numObjects = bufParse.ParseUshort();
            for (int i = 0; i < static_cast<int>(numObjects) && i < static_cast<int>(tempConvexes.size()); ++i)
            {
                tempConvexes[i]->m_boundingDiscCenter = bufParse.ParseVec2();
                tempConvexes[i]->m_boundingRadius     = bufParse.ParseFloat();
            }
        }
        else if (chunkType == 0x82) // BoundingAABBs
        {
            hasBoundingAABBs = true;
            uint16_t numObjects = bufParse.ParseUshort();
            for (int i = 0; i < static_cast<int>(numObjects) && i < static_cast<int>(tempConvexes.size()); ++i)
            {
                tempConvexes[i]->m_boundingAABB = bufParse.ParseAABB2();
            }
        }
        else if (chunkType == 0x83) // AABB2 Tree (BVH)
        {
            hasAABB2Tree = true;
            uint8_t depthFlag = bufParse.ParseByte();
            UNUSED(depthFlag);
            unsigned int numNodes = bufParse.ParseUint32();
            tempAABB2Tree.m_nodes.resize(numNodes);
            for (unsigned int n = 0; n < numNodes; ++n)
            {
                tempAABB2Tree.m_nodes[n].m_bounds = bufParse.ParseAABB2();
                int leftChildIdx  = bufParse.ParseInt32();
                int rightChildIdx = bufParse.ParseInt32();
                UNUSED(leftChildIdx); UNUSED(rightChildIdx);
                unsigned int numConvex = bufParse.ParseUint32();
                for (unsigned int c = 0; c < numConvex; ++c)
                {
                    uint16_t objIdx = bufParse.ParseUshort();
                    if (objIdx < static_cast<uint16_t>(tempConvexes.size()))
                    {
                        tempAABB2Tree.m_nodes[n].m_containingConvex.push_back(tempConvexes[objIdx]);
                    }
                }
            }
        }
        else if (chunkType == 0x87) // Symmetric Quadtree (QuadKey format)
        {
            hasSymQuadTree = true;
            unsigned int numEntries = bufParse.ParseUint32();

            size_t savedPos = bufParse.GetCurrentPosition();
            int maxDepth = 0;
            for (unsigned int e = 0; e < numEntries; ++e)
            {
                uint8_t entryDepth = bufParse.ParseByte();
                if (static_cast<int>(entryDepth) > maxDepth) maxDepth = static_cast<int>(entryDepth);
                bufParse.ParseUshort();
                bufParse.ParseUshort();
                uint16_t numConvex = bufParse.ParseUshort();
                for (int c = 0; c < static_cast<int>(numConvex); ++c) bufParse.ParseUshort();
            }

            int totalNodes = 0;
            { int ls = 1; for (int d = 0; d <= maxDepth; ++d) { totalNodes += ls; ls *= 4; } }
            tempSymQuadTree.m_nodes.resize(totalNodes);

            // Reconstruct bounds
            {
                Vec2 worldMins = sceneBounds.m_mins;
                Vec2 worldSize = Vec2(sceneBounds.m_maxs.x - sceneBounds.m_mins.x, sceneBounds.m_maxs.y - sceneBounds.m_mins.y);
                int levelStart2 = 0;
                int levelSize2  = 1;
                for (int d = 0; d <= maxDepth; ++d)
                {
                    int gridDim = 1 << d;
                    Vec2 cellSize = Vec2(worldSize.x / static_cast<float>(gridDim), worldSize.y / static_cast<float>(gridDim));
                    for (int j = 0; j < levelSize2 && (levelStart2 + j) < totalNodes; ++j)
                    {
                        int gx = j % gridDim;
                        int gy = j / gridDim;
                        Vec2 mins = worldMins + Vec2(static_cast<float>(gx) * cellSize.x, static_cast<float>(gy) * cellSize.y);
                        tempSymQuadTree.m_nodes[levelStart2 + j].m_bounds = AABB2(mins, mins + cellSize);
                    }
                    levelStart2 += levelSize2;
                    levelSize2 *= 4;
                }
            }

            // Populate convex references
            bufParse.SetCurrentPosition(savedPos);
            for (unsigned int e = 0; e < numEntries; ++e)
            {
                uint8_t entryDepth = bufParse.ParseByte();
                uint16_t qx = bufParse.ParseUshort();
                uint16_t qy = bufParse.ParseUshort();
                int levelStart2 = 0;
                { int ls = 1; for (int d = 0; d < static_cast<int>(entryDepth); ++d) { levelStart2 += ls; ls *= 4; } }
                int gridDim = 1 << entryDepth;
                int nodeIdx = levelStart2 + static_cast<int>(qy) * gridDim + static_cast<int>(qx);
                uint16_t numConvex = bufParse.ParseUshort();
                if (nodeIdx < totalNodes)
                {
                    for (int c = 0; c < static_cast<int>(numConvex); ++c)
                    {
                        uint16_t objIdx = bufParse.ParseUshort();
                        if (objIdx < static_cast<uint16_t>(tempConvexes.size()))
                            tempSymQuadTree.m_nodes[nodeIdx].m_containingConvex.push_back(tempConvexes[objIdx]);
                    }
                }
                else
                {
                    for (int c = 0; c < static_cast<int>(numConvex); ++c) bufParse.ParseUshort();
                }
            }
        }
        else
        {
            // Preserve unrecognized chunks for round-trip
            UnrecognizedChunk preserved;
            preserved.chunkType  = chunkType;
            preserved.endianness = chunkEndian;
            preserved.rawData.assign(buffer.begin() + chunkStartPos, buffer.begin() + chunkStartPos + entry.totalSize);
            tempPreservedChunks.push_back(preserved);
            bufParse.SetCurrentPosition(dataStartPos + static_cast<size_t>(dataSize));
        }

        // Validate chunk footer
        if (bufParse.ParseChar() != 'E' || bufParse.ParseChar() != 'N' ||
            bufParse.ParseChar() != 'D' || bufParse.ParseChar() != 'C')
        {
            g_devConsole->AddLine(DevConsole::ERROR, Stringf("Error: Missing ENDC footer for chunk type 0x%02X", chunkType));
            for (Convex2* c : tempConvexes) delete c;
            return false;
        }
    }

    // --- Validation ---
    if (!hasConvexPolys)
    {
        g_devConsole->AddLine(DevConsole::ERROR, "Error: No ConvexPolys chunk found");
        for (Convex2* c : tempConvexes) delete c;
        return false;
    }

    // Rebuild missing data
    if (!hasConvexHulls)
    {
        for (Convex2* convex : tempConvexes)
            convex->m_convexHull = ConvexHull2(convex->m_convexPoly);
    }
    if (!hasBoundingDiscs || !hasBoundingAABBs)
    {
        for (Convex2* convex : tempConvexes)
            convex->RebuildBoundingVolumes();
    }

    // --- Replace current scene ---
    ClearScene();
    m_convexes = std::move(tempConvexes);
    m_preservedChunks = std::move(tempPreservedChunks);
    m_sceneModified = false;

    // Adjust camera to scene bounds
    if (hasSceneInfo)
    {
        m_hasLoadedScene = true;
        m_loadedSceneBounds = sceneBounds;

        float sceneWidth  = sceneBounds.m_maxs.x - sceneBounds.m_mins.x;
        float sceneHeight = sceneBounds.m_maxs.y - sceneBounds.m_mins.y;
        float sceneAspect = sceneWidth / sceneHeight;

        float windowAspect = CONVEX_WORLD_SIZE_X / CONVEX_WORLD_SIZE_Y; // default 2:1

        if (sceneAspect > windowAspect)
        {
            float viewHeight = sceneWidth / windowAspect;
            float offsetY    = (viewHeight - sceneHeight) * 0.5f;
            m_worldCamera->SetOrthoGraphicView(
                Vec2(sceneBounds.m_mins.x, sceneBounds.m_mins.y - offsetY),
                Vec2(sceneBounds.m_maxs.x, sceneBounds.m_maxs.y + offsetY));
        }
        else
        {
            float viewWidth = sceneHeight * windowAspect;
            float offsetX   = (viewWidth - sceneWidth) * 0.5f;
            m_worldCamera->SetOrthoGraphicView(
                Vec2(sceneBounds.m_mins.x - offsetX, sceneBounds.m_mins.y),
                Vec2(sceneBounds.m_maxs.x + offsetX, sceneBounds.m_maxs.y));
        }
    }

    // Restore or rebuild spatial acceleration structures
    if (hasAABB2Tree)
    {
        int numNodes = static_cast<int>(tempAABB2Tree.m_nodes.size());
        int lastLevelStart = 0;
        int levelSize = 1;
        while (lastLevelStart + levelSize < numNodes)
        {
            lastLevelStart += levelSize;
            levelSize *= 2;
        }
        tempAABB2Tree.SetStartOfLastLevel(lastLevelStart);
        m_AABB2Tree = std::move(tempAABB2Tree);
    }
    if (hasSymQuadTree)
    {
        m_symQuadTree = std::move(tempSymQuadTree);
    }
    if (!hasAABB2Tree || !hasSymQuadTree)
    {
        AABB2 totalBounds = GetWorldBounds();
        int numConvexes = static_cast<int>(m_convexes.size());
        int bvhDepth = 0;
        if (numConvexes > 0)
        {
            bvhDepth = static_cast<int>(log2(static_cast<double>(numConvexes))) - 3;
            if (bvhDepth < 3) bvhDepth = 3;
        }
        if (!hasAABB2Tree) m_AABB2Tree.BuildTree(m_convexes, bvhDepth, totalBounds);
        if (!hasSymQuadTree) m_symQuadTree.BuildTree(m_convexes, 4, totalBounds);
    }

    return true;
}
