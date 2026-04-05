//----------------------------------------------------------------------------------------------------
// GameConvexScene.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Game/Game.hpp"
#include "Game/BVH.hpp"
#include "Game/QuadTree.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Rgba8.hpp"
//----------------------------------------------------------------------------------------------------
#include <string>
#include <vector>

struct Convex2;
struct ConvexPoly2;
struct Vertex_PCU;

//----------------------------------------------------------------------------------------------------
struct UnrecognizedChunk
{
    uint8_t              chunkType;
    uint8_t              endianness;
    std::vector<uint8_t> rawData;
};

//----------------------------------------------------------------------------------------------------
class GameConvexScene final : public Game
{
public:
    GameConvexScene();
    ~GameConvexScene();

    void Update() override;
    void Render() const override;

    //------------------------------------------------------------------------------------------------
    // Save / Load (dev console commands)
    //------------------------------------------------------------------------------------------------
    bool SaveSceneToFile(std::string const& filePath);
    bool LoadSceneFromFile(std::string const& filePath);

    static bool SaveConvexSceneCommand(EventArgs& args);
    static bool LoadConvexSceneCommand(EventArgs& args);

private:
    void UpdateFromKeyboard(float deltaSeconds) override;
    void UpdateFromController(float deltaSeconds) override;

    //------------------------------------------------------------------------------------------------
    // Convex generation
    //------------------------------------------------------------------------------------------------
    static Convex2* CreateRandomConvex(Vec2 const& center, float minRadius, float maxRadius);

    //------------------------------------------------------------------------------------------------
    // Scene management
    //------------------------------------------------------------------------------------------------
    void RebuildAllTrees();
    void ClearScene();

    //------------------------------------------------------------------------------------------------
    // Interaction
    //------------------------------------------------------------------------------------------------
    void UpdateHoverDetection();

    //------------------------------------------------------------------------------------------------
    // Scene-relative scaling (adapts rendering/interaction to loaded scene size)
    //------------------------------------------------------------------------------------------------
    float GetSceneScale() const;
    AABB2 GetWorldBounds() const;

    //------------------------------------------------------------------------------------------------
    // Rendering helpers
    //------------------------------------------------------------------------------------------------
    void AddVertsForConvexPolyEdges(std::vector<Vertex_PCU>& verts, ConvexPoly2 const& convexPoly2, float thickness, Rgba8 const& color) const;
    void RenderConvexScene() const;
    void RenderConvexSceneControlText() const;
    void RenderRaycast(std::vector<Vertex_PCU>& verts) const;
    void TestRays();

    //------------------------------------------------------------------------------------------------
    // Member variables
    //------------------------------------------------------------------------------------------------
    // Convex objects
    std::vector<Convex2*> m_convexes;

    // Interaction state
    Convex2* m_hoveringConvex = nullptr;
    Vec2     m_cursorPrevPos;
    bool     m_isDragging    = false;
    bool     m_drawEdgesMode = false;
    bool     m_showBoundingDiscs = false;
    bool     m_showSpatialStructure = false;
    bool     m_debugDrawBVHMode     = false;
    int      m_rayOptimizationMode = 0; // 0=None, 1=Disc, 2=AABB

    // Raycast testing
    Vec2 m_rayStart;
    Vec2 m_rayEnd;
    int  m_numOfRandomRays = 1024;

    // Performance metrics
    float m_avgDist                      = 0.f;
    float m_lastRayTestNormalTime        = 0.f;
    float m_lastRayTestDiscRejectionTime = 0.f;
    float m_lastRayTestAABBRejectionTime = 0.f;
    float m_lastRayTestSymmetricTreeTime = 0.f;
    float m_lastRayTestAABBTreeTime      = 0.f;

    // Spatial structures
    SymmetricQuadTree m_symQuadTree;
    AABB2Tree         m_AABB2Tree;

    // Scene persistence
    AABB2 m_loadedSceneBounds;
    bool  m_hasLoadedScene = false;
    std::vector<UnrecognizedChunk> m_preservedChunks;
    bool m_sceneModified = false;
};
